/*
 * Copyright (C) 2014 LNLS (www.lnls.br)
 * Author: Lucas Russo <lucas.russo@lnls.br>
 *
 * Released according to the GNU LGPL, version 3 or any later version.
 */

#include "ll_io_pcie.h"
#include "hal_assert.h"

/* Undef ASSERT_ALLOC to avoid conflicting with other ASSERT_ALLOC */
#ifdef ASSERT_TEST
#undef ASSERT_TEST
#endif
#define ASSERT_TEST(test_boolean, err_str, err_goto_label, /* err_core */ ...) \
    ASSERT_HAL_TEST(test_boolean, LL_IO, "[ll_io:pcie]",    \
            err_str, err_goto_label, /* err_core */ __VA_ARGS__)

#ifdef ASSERT_ALLOC
#undef ASSERT_ALLOC
#endif
#define ASSERT_ALLOC(ptr, err_goto_label, /* err_core */ ...) \
    ASSERT_HAL_ALLOC(ptr, LL_IO, "[ll_io:pcie]",            \
            llio_err_str(LLIO_ERR_ALLOC),                   \
            err_goto_label, /* err_core */ __VA_ARGS__)

#ifdef CHECK_ERR
#undef CHECK_ERR
#endif
#define CHECK_ERR(err, err_type)                            \
    CHECK_HAL_ERR(err, LL_IO, "[ll_io:pcie]",               \
            llio_err_str (err_type))

#define READ_FROM_BAR                           1
#define WRITE_TO_BAR                            0

#define PCIE_TIMEOUT_MAX_TRIES                  32
/* Wait between reads/writes, in usecs */
#define PCIE_TIMEOUT_WAIT                       100000

/* Timeout byte pattern */
#define PCIE_TIMEOUT_PATT_INIT                  0xFF
/* Number of timeout pattern bytes in a row to detect a timeout */
#define PCIE_TIMEOUT_PATT_SIZE                  32

static uint32_t pcie_timeout_patt [PCIE_TIMEOUT_PATT_SIZE];

static ssize_t _pcie_rw_32 (llio_t *self, loff_t offs, uint32_t *data, int rw);
static ssize_t _pcie_rw_bar2_block_raw (llio_t *self, uint32_t pg_start, loff_t pg_offs,
        uint32_t *data, uint32_t size, int rw);
static ssize_t _pcie_rw_bar4_block_raw (llio_t *self, uint32_t pg_start, loff_t pg_offs,
        uint32_t *data, uint32_t size, int rw);
static ssize_t _pcie_rw_block (llio_t *self, loff_t offs, size_t size,
        uint32_t *data, int rw);
static ssize_t _pcie_timeout_reset (llio_t *self);

/************ Our methods implementation **********/

/* Creates a new instance of the dev_pcie */
llio_dev_pcie_t * llio_dev_pcie_new (char *dev_entry)
{
    llio_dev_pcie_t *self = (llio_dev_pcie_t *) zmalloc (sizeof *self);
    ASSERT_ALLOC (self, err_llio_dev_pcie_alloc);

    self->dev = (pd_device_t *) zmalloc (sizeof *self->dev);
    ASSERT_ALLOC (self->dev, err_dev_pcie_alloc);

    /* FIXME: hardcoded dev number */
    /* TODO: should we use llio_endpoint_get to get the endpoint name? */
    int err = pd_open (0, self->dev, dev_entry);

    if (err != 0) {
        perror ("pd_open");
    }
    ASSERT_TEST(err==0, "Error opening device", err_dev_pcie_open);

    /* Map all available BARs */
    self->bar0 = (uint32_t *) pd_mapBAR (self->dev, BAR0NO);
    ASSERT_TEST(self->bar0!=NULL, "Could not allocate bar0", err_bar0_alloc);
    self->bar2 = (uint32_t *) pd_mapBAR (self->dev, BAR2NO);
    ASSERT_TEST(self->bar2!=NULL, "Could not allocate bar2", err_bar2_alloc);
    DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE, "[ll_io_pcie] BAR2 addr = %p\n",
            self->bar2);
    self->bar4 = (uint64_t *) pd_mapBAR (self->dev, BAR4NO);
    ASSERT_TEST(self->bar4!=NULL, "Could not allocate bar4", err_bar4_alloc);
    DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE, "[ll_io_pcie] BAR4 addr = %p\n",
            self->bar4);

    /* Initialize PCIE timeout pattern */
    memset (&pcie_timeout_patt, PCIE_TIMEOUT_PATT_INIT, sizeof (pcie_timeout_patt));

    DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE, "[ll_io_pcie] Created instance of llio_dev_pcie\n");

    return self;

err_bar4_alloc:
    pd_unmapBAR (self->dev, BAR2NO, self->bar2);
err_bar2_alloc:
    pd_unmapBAR (self->dev, BAR0NO, self->bar0);
err_bar0_alloc:
    pd_close (self->dev);
err_dev_pcie_open:
    free (self->dev);
err_dev_pcie_alloc:
    free (self);
err_llio_dev_pcie_alloc:
    return NULL;
}

/* Destroy an instance of the Endpoint */
llio_err_e llio_dev_pcie_destroy (llio_dev_pcie_t **self_p)
{
    if (*self_p) {
        llio_dev_pcie_t *self = *self_p;

        /* Unmap all bars first and then destroy the remaining structures */
        pd_unmapBAR (self->dev, BAR4NO, self->bar4);
        pd_unmapBAR (self->dev, BAR2NO, self->bar2);
        pd_unmapBAR (self->dev, BAR0NO, self->bar0);
        pd_close (self->dev);

        free (self->dev);
        free (self);

        self_p = NULL;
    }

    return LLIO_SUCCESS;
}

/************ llio_ops_pcie Implementation **********/

/* Open PCIe device */
int pcie_open (llio_t *self, llio_endpoint_t *endpoint)
{
    (void) endpoint;
    if (self->endpoint->opened) {
        return 0;
    }

    /* Create new private PCIe handler */
    self->dev_handler = llio_dev_pcie_new (self->endpoint->name);
    ASSERT_TEST(self->dev_handler!=NULL, "Could not allocate dev_handler", err_dev_handler_alloc);

    /* Initialize Wishbone and SDRAM pages to 0 */
    SET_SDRAM_PG(0);
    SET_WB_PG(0);

    /* Signal that the endpoint is opened and ready to work */
    self->endpoint->opened = true;

    DBE_DEBUG (DBG_LL_IO | DBG_LVL_INFO,
            "[ll_io_pcie] Opened PCIe device located at %s\n",
            self->endpoint->name);

    return 0;

err_dev_handler_alloc:
    return -1;
}

/* Release PCIe device */
int pcie_release (llio_t *self, llio_endpoint_t *endpoint)
{
    (void) endpoint;

    /* Nothing to close */
    if (!self->endpoint->opened) {
        return 0;
    }

    /* Deattach specific device handler to generic one */
    llio_err_e err = llio_dev_pcie_destroy ((llio_dev_pcie_t **) &self->dev_handler);
    ASSERT_TEST (err==LLIO_SUCCESS, "Could not close device appropriately", err_dealloc);

    self->dev_handler = NULL;
    /* TODO: should we use llio_endpoint_set_opened() to close the endpoint? */
    self->endpoint->opened = false;

    DBE_DEBUG (DBG_LL_IO | DBG_LVL_INFO,
            "[ll_io_pcie] Closed PCIe device located at %s\n", self->endpoint->name);

    return 0;

err_dealloc:
    return -1;
}

/* NOTE: For the PCIe device, we only have up to 28 address bits, as
 * the 4 MSB are reserved for selecting the BAR to operate on */

/* Read data from PCIe device */
ssize_t pcie_read_32 (llio_t *self, loff_t offs, uint32_t *data)
{
    return _pcie_rw_32 (self, offs, data, READ_FROM_BAR);
}

ssize_t pcie_read_64 (llio_t *self, loff_t offs, uint64_t *data)
{
    return _pcie_rw_32 (self, offs,
            (uint32_t *) data, READ_FROM_BAR)
            +
            _pcie_rw_32 (self, offs,
            (uint32_t *)((uint8_t *) data + sizeof (uint32_t)), READ_FROM_BAR);
}

/* Write data to PCIe device */
ssize_t pcie_write_32 (llio_t *self, loff_t offs, const uint32_t *data)
{
    uint32_t _data = *data;
    return _pcie_rw_32 (self, offs, &_data, WRITE_TO_BAR);
}

ssize_t pcie_write_64 (llio_t *self, loff_t offs, const uint64_t *data)
{
    uint64_t _data = *data;
    return _pcie_rw_32 (self, offs,
            (uint32_t *) &_data, WRITE_TO_BAR)
            +
            _pcie_rw_32 (self, offs,
            (uint32_t *)((uint8_t *) &_data + sizeof (uint32_t)), WRITE_TO_BAR);
}

/* Read data block from PCIe device, size in bytes */
ssize_t pcie_read_block (llio_t *self, loff_t offs, size_t size, uint32_t *data)
{
    return _pcie_rw_block (self, offs, size, data, READ_FROM_BAR);
}

/* Write data block from PCIe device, size in bytes */
ssize_t pcie_write_block (llio_t *self, loff_t offs, size_t size, uint32_t *data)
{
    /* _pcie_rw_block with WRITE_TO_BAR does not modify "data" */
    return _pcie_rw_block (self, offs, size, data, WRITE_TO_BAR);
}

/* Read data block from PCIe device, size in bytes */
ssize_t pcie_read_dma (llio_t *self, loff_t offs, size_t size, uint32_t *data)
{
    (void) self;
    (void) offs;
    (void) size;
    (void) data;
    return -1;
}

/* Write data block from PCIe device, size in bytes */
ssize_t pcie_write_dma (llio_t *self, loff_t offs, size_t size, uint32_t *data)
{
    (void) self;
    (void) offs;
    (void) size;
    (void) data;
    return -1;
}

/* Read PCIe device information */
/*int pcie_read_info (llio_t *self, llio_dev_info_t *dev_info)
{
    (void) self;
    (void) dev_info;
    return -1;
}
*/

/************ Helper functions **********/
static ssize_t _pcie_rw_32 (llio_t *self, loff_t offs, uint32_t *data, int rw)
{
    if (!self->endpoint->opened) {
        return -1;
    }

    /* Determine which bar to operate on */
    int bar_no = PCIE_ADDR_BAR (offs);
    loff_t full_offs = PCIE_ADDR_GEN (offs);
    int pg_num;
    loff_t pg_offs;

    /* FIXME: This switch is in the critical path! Remove it */
    switch (bar_no) {
        /* PCIe config registers */
        case BAR0NO:
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "----------------------------------------------------------\n"
                    "[ll_io_pcie:_pcie_rw_32] Going to read/write in BAR0\n");
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "[ll_io_pcie:_pcie_rw_32] bar_no = %d, full_offs = %lX\n"
                    "-------------------------------------------------------------------------------------\n",
                    bar_no, full_offs);
            BAR0_RW(BAR0, full_offs, data, rw);
            break;

        /* FPGA SDRAM */
        case BAR2NO:
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "----------------------------------------------------------\n"
                    "[ll_io_pcie:_pcie_rw_32] Going to read/write in BAR2\n");
            pg_num = PCIE_ADDR_SDRAM_PG (full_offs);
            pg_offs = PCIE_ADDR_SDRAM_PG_OFFS (full_offs);
            SET_SDRAM_PG (pg_num);
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "[ll_io_pcie:_pcie_rw_32] bar_no = %d, pg_num  = %d,\n\tfull_offs = 0x%lx, pg_offs = 0x%lx\n",
                    bar_no, pg_num, full_offs, pg_offs);
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "[ll_io_pcie:_pcie_rw_32] full_addr = 0x%p\n"
                    "-------------------------------------------------------------------------------------\n",
                    ((llio_dev_pcie_t *) self->dev_handler)->bar2 + pg_offs);
            BAR2_RW(BAR2, pg_offs, data, rw);
            break;

        /* FPGA Wishbone */
        case BAR4NO:
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "----------------------------------------------------------\n"
                    "[ll_io_pcie:_pcie_rw_32] Going to read/write in BAR4\n");
            pg_num = PCIE_ADDR_WB_PG (full_offs);
            pg_offs = PCIE_ADDR_WB_PG_OFFS (full_offs);
            SET_WB_PG (pg_num);
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "[ll_io_pcie:_pcie_rw_32] bar_no = %d, pg_num  = %d,\n\tfull_offs = 0x%lx, pg_offs = 0x%lx\n",
                    bar_no, pg_num, full_offs, pg_offs);
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "[ll_io_pcie:_pcie_rw_32] full_addr = %p\n"
                    "-------------------------------------------------------------------------------------\n",
                    ((llio_dev_pcie_t *) self->dev_handler)->bar4 + pg_offs);
            BAR4_RW(BAR4, pg_offs, data, rw);
            break;

        /* Invalid BAR */
        default:
            return -1;
    }

    return sizeof (*data);
}

static ssize_t _pcie_rw_bar2_block_raw (llio_t *self, uint32_t pg_start, loff_t pg_offs,
        uint32_t *data, uint32_t size, int rw)
{
    uint32_t offs = pg_offs;
    uint32_t num_bytes_rem = size;
    /* Number of bytes read/written, for faster comparison */
    uint32_t num_bytes_rw = 0;
    /* PCIe timeout correlation counter */
    ////uint32_t timeout_count = 0;
    DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
            "----------------------------------------------------------\n"
            "[ll_io_pcie:_pcie_rw_bar2_block_raw] pg_start = %u, pg_end = %lu...\n",
            pg_start, pg_start + (pg_offs+size)/PCIE_SDRAM_PG_SIZE + 1);
    for (unsigned int pg = pg_start;
            pg < (pg_start + (pg_offs+size)/PCIE_SDRAM_PG_SIZE + 1);
            ++pg) {
        SET_SDRAM_PG (pg);
        uint32_t num_bytes_page = (num_bytes_rem > PCIE_SDRAM_PG_SIZE) ?
            (PCIE_SDRAM_PG_SIZE-offs) : (num_bytes_rem);
        num_bytes_rem -= num_bytes_page;
        num_bytes_rw += num_bytes_page;

        DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                "[ll_io_pcie:_pcie_rw_bar2_block_raw] Reading %u bytes from addr: %p\n"
                "-------------------------------------------------------------------------------------\n",
                num_bytes_page, BAR2);
        BAR2_RW_BLOCK(BAR2, offs, num_bytes_page,
                (uint32_t *)((uint8_t *)data + (pg-pg_start)*PCIE_SDRAM_PG_SIZE), rw);

        /* Always 0 after the first page */
        offs = 0;
    }

    return size /*- num_bytes_rem*/;
}

/* Read/Write BAR2 block with timeout detection */
static ssize_t _pcie_rw_bar2_block_td (llio_t *self, uint32_t pg_start, loff_t pg_offs,
        uint32_t *data, uint32_t size, int rw)
{
    size_t num_bytes_rw = 0;

    uint32_t i;
    for (i = 0; i < PCIE_TIMEOUT_MAX_TRIES; ++i) {
        num_bytes_rw = _pcie_rw_bar2_block_raw (self, pg_start, pg_offs, data,
                size, rw);

        /* If sufficient number of bytes read, try to detect a PCIe core
         * timeout by reading specified number of words and comparing to
         * the timeout pattern */
        if (num_bytes_rw >= PCIE_TIMEOUT_PATT_SIZE && !memcmp (data,
                    pcie_timeout_patt, PCIE_TIMEOUT_PATT_SIZE)) {
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "[ll_io_pcie:_pcie_rw_bar2_td] Timeout detected. Retrying\n");
            /////////////////////
            _pcie_timeout_reset (self);
            usleep (PCIE_TIMEOUT_WAIT);
        }
        else {
            break;
        }
    }

    if (i >= PCIE_TIMEOUT_MAX_TRIES) {
        DBE_DEBUG (DBG_LL_IO | DBG_LVL_ERR,
                "[ll_io_pcie:_pcie_rw_bar2_td] Unrecoverable timeout detected. Exceeded "
                "maximum number of tries\n");
        return -1;
    }

    return num_bytes_rw;
}

static ssize_t _pcie_rw_bar4_block_raw (llio_t *self, uint32_t pg_start, loff_t pg_offs,
        uint32_t *data, uint32_t size, int rw)
{
    uint32_t offs = pg_offs;
    uint32_t num_bytes_rem = size;
    DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
            "----------------------------------------------------------\n"
            "[ll_io_pcie:_pcie_rw_bar4_block_raw] pg_start = %u, pg_end = %lu...\n",
            pg_start, pg_start + (pg_offs+size)/PCIE_WB_PG_SIZE + 1);
    for (unsigned int pg = pg_start;
            pg < pg_start + (pg_offs+size)/PCIE_WB_PG_SIZE + 1;
            ++pg) {
        SET_WB_PG (pg);
        uint32_t num_bytes_page = (num_bytes_rem > PCIE_WB_PG_SIZE) ?
            (PCIE_WB_PG_SIZE-offs) : (num_bytes_rem);
        num_bytes_rem -= num_bytes_page;

        DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                "[ll_io_pcie:_pcie_rw_bar4_block_raw] Reading %u bytes from addr: %p\n"
                "-------------------------------------------------------------------------------------\n",
                num_bytes_page, BAR4);
        BAR4_RW_BLOCK(BAR4, offs, num_bytes_page,
                (uint32_t *)((uint8_t *)data + (pg-pg_start)*PCIE_WB_PG_SIZE), rw);

        /* Always 0 after the first page */
        offs = 0;
    }

    return size /*- num_bytes_rem*/;
}

/* Read/Write BAR4 block with timeout detection.
 * FIXME: Reduce code repetition. _pcie_rw_bar2_block_td () is almost the same */
static ssize_t _pcie_rw_bar4_block_td (llio_t *self, uint32_t pg_start, loff_t pg_offs,
        uint32_t *data, uint32_t size, int rw)
{
    size_t num_bytes_rw = 0;

    uint32_t i;
    for (i = 0; i < PCIE_TIMEOUT_MAX_TRIES; ++i) {
        num_bytes_rw = _pcie_rw_bar4_block_raw (self, pg_start, pg_offs, data,
                size, rw);

        /* If sufficient number of bytes read, try to detect a PCIe core
         * timeout by reading specified number of words and comparing to
         * the timeout pattern */
        if (num_bytes_rw >= PCIE_TIMEOUT_PATT_SIZE && !memcmp (data,
                    pcie_timeout_patt, PCIE_TIMEOUT_PATT_SIZE)) {
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "[ll_io_pcie:_pcie_rw_bar4_td] Timeout detected. Retrying\n");
            //////////////
            _pcie_timeout_reset (self);
            usleep (PCIE_TIMEOUT_WAIT);
        }
        else {
            break;
        }
    }

    if (i >= PCIE_TIMEOUT_MAX_TRIES) {
        DBE_DEBUG (DBG_LL_IO | DBG_LVL_ERR,
                "[ll_io_pcie:_pcie_rw_bar4_td] Unrecoverable timeout detected. Exceeded "
                "maximum number of tries\n");
        return -1;
    }

    return num_bytes_rw;
}

static ssize_t _pcie_rw_block (llio_t *self, loff_t offs, size_t size, uint32_t *data, int rw)
{
    if (!self->endpoint->opened) {
        return -1;
    }

    /* Determine which bar to operate on */
    int bar_no = PCIE_ADDR_BAR (offs);
    loff_t full_offs = PCIE_ADDR_GEN (offs);
    int pg_start;
    loff_t pg_offs;
    ssize_t ret_size = -1;

    switch (bar_no) {
        /* PCIe config registers */
        case BAR0NO:
            /* Not available */
            break;

        /* FPGA SDRAM */
        case BAR2NO:
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "----------------------------------------------------------\n"
                    "[ll_io_pcie:_pcie_rw_block] Going to read/write in BAR2\n");
            pg_start = PCIE_ADDR_SDRAM_PG (full_offs);
            pg_offs = PCIE_ADDR_SDRAM_PG_OFFS (full_offs);
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "[ll_io_pcie:_pcie_rw_block] bar_no = %d, pg_start = %d,\n\tfull_offs = 0x%lx, pg_offs = 0x%lx\n",
                    bar_no, pg_start, full_offs, pg_offs);
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "[ll_io_pcie:_pcie_rw_block] full_addr = 0x%p\n"
                    "-------------------------------------------------------------------------------------\n",
                    ((llio_dev_pcie_t *) self->dev_handler)->bar2 + pg_offs);
            ret_size = _pcie_rw_bar2_block_td (self, pg_start, pg_offs, data, size, rw);
            break;

        /* FPGA Wishbone */
        case BAR4NO:
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "----------------------------------------------------------\n"
                    "[ll_io_pcie:_pcie_rw_block] Going to read/write in BAR4\n");
            pg_start = PCIE_ADDR_WB_PG (full_offs);
            pg_offs = PCIE_ADDR_WB_PG_OFFS (full_offs);
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "[ll_io_pcie:_pcie_rw_block] bar_no = %d, pg_start  = %d,\n\tfull_offs = 0x%lx, pg_offs = 0x%lx\n",
                    bar_no, pg_start, full_offs, pg_offs);
            DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
                    "[ll_io_pcie:_pcie_rw_block] full_addr = %p\n"
                    "-------------------------------------------------------------------------------------\n",
                    ((llio_dev_pcie_t *) self->dev_handler)->bar4 + pg_offs);
            ret_size =  _pcie_rw_bar4_block_td (self, pg_start, pg_offs, data, size, rw);
            break;

        /* Invalid BAR */
        default:
            break;
    }

    return ret_size;
}

static ssize_t _pcie_timeout_reset (llio_t *self)
{
    DBE_DEBUG (DBG_LL_IO | DBG_LVL_TRACE,
            "[ll_io_pcie:_pcie_timeout_reset] Reseting timeout\n");

    loff_t offs = BAR0_ADDR | PCIE_CFG_REG_TX_CTRL;
    uint32_t data = PCIE_CFG_TX_CTRL_CHANNEL_RST;
    return _pcie_rw_32 (self, offs, &data, WRITE_TO_BAR);
}

const llio_ops_t llio_ops_pcie = {
    .open           = pcie_open,        /* Open device */
    .release        = pcie_release,     /* Release device */
    .read_16        = NULL,             /* Read 16-bit data */
    .read_32        = pcie_read_32,     /* Read 32-bit data */
    .read_64        = pcie_read_64,     /* Read 64-bit data */
    .write_16       = NULL,             /* Write 16-bit data */
    .write_32       = pcie_write_32,    /* Write 32-bit data */
    .write_64       = pcie_write_64,    /* Write 64-bit data */
    .read_block     = pcie_read_block,  /* Read arbitrary block size data,
                                           parameter size in bytes */
    .write_block    = pcie_write_block, /* Write arbitrary block size data,
                                           parameter size in bytes */
    .read_dma       = pcie_read_dma,    /* Read arbitrary block size data via DMA,
                                            parameter size in bytes */
    .write_dma      = pcie_write_dma    /* Write arbitrary block size data via DMA,
                                            parameter size in bytes */
    /*.read_info      = pcie_read_info */   /* Read device information data */
};
