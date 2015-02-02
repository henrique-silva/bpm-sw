/*
 * Copyright (C) 2014 LNLS (www.lnls.br)
 * Author: Lucas Russo <lucas.russo@lnls.br>
 *
 * Released according to the GNU LGPL, version 3 or any later version.
 */

#include "bpm_client.h"
#include "hal_assert.h"

#include "sm_io_acq_codes.h"
#include "sm_io_dsp_codes.h"
#include "sm_io_swap_codes.h"
#include "sm_io_swap_useful_macros.h"
#include "sm_io_codes.h"
#include "rw_param_codes.h"
#include "rw_param_client.h"

/* Undef ASSERT_ALLOC to avoid conflicting with other ASSERT_ALLOC */
#ifdef ASSERT_TEST
#undef ASSERT_TEST
#endif
#define ASSERT_TEST(test_boolean, err_str, err_goto_label, /* err_core */ ...) \
    ASSERT_HAL_TEST(test_boolean, LIB_CLIENT, "[libclient]",\
            err_str, err_goto_label, /* err_core */ __VA_ARGS__)

#ifdef ASSERT_ALLOC
#undef ASSERT_ALLOC
#endif
#define ASSERT_ALLOC(ptr, err_goto_label, /* err_core */ ...) \
    ASSERT_HAL_ALLOC(ptr, LIB_CLIENT, "[libclient]",        \
            bpm_client_err_str(BPM_CLIENT_ERR_ALLOC),       \
            err_goto_label, /* err_core */ __VA_ARGS__)

#ifdef CHECK_ERR
#undef CHECK_ERR
#endif
#define CHECK_ERR(err, err_type)                            \
    CHECK_HAL_ERR(err, LIB_CLIENT, "[libclient]",           \
            bpm_client_err_str (err_type))

#define BPMCLIENT_DFLT_LOG_MODE             "w"

static bpm_client_t *_bpm_client_new (char *broker_endp, int verbose,
        const char *log_file_name, const char *log_mode);

/* Acquisition channel definitions for user's application */
#if defined(__BOARD_ML605__)
/* Global structure merging all of the channel's sample sizes */
acq_chan_t acq_chan[END_CHAN_ID] =  {   [0] = {.chan = ADC0_CHAN_ID, .sample_size = ADC0_SAMPLE_SIZE},
                                        [1] = {.chan = TBTAMP0_CHAN_ID, .sample_size = TBTAMP0_SAMPLE_SIZE},
                                        [2] = {.chan = TBTPOS0_CHAN_ID, .sample_size = TBTPOS0_SAMPLE_SIZE},
                                        [3] = {.chan = FOFBAMP0_CHAN_ID, .sample_size = FOFBAMP0_SAMPLE_SIZE},
                                        [4] = {.chan = FOFBPOS0_CHAN_ID, .sample_size = FOFBPOS0_SAMPLE_SIZE},
                                        [5] = {.chan = MONITAMP0_CHAN_ID, .sample_size = MONITAMP0_SAMPLE_SIZE},
                                        [6] = {.chan = MONITPOS0_CHAN_ID, .sample_size = MONITPOS0_SAMPLE_SIZE},
                                        [7] = {.chan = MONIT1POS0_CHAN_ID, .sample_size = MONIT1POS0_SAMPLE_SIZE}
                                    };
#elif defined(__BOARD_AFCV3__)
acq_chan_t acq_chan[END_CHAN_ID] =  {   [0]  =  {.chan = ADC0_CHAN_ID, .sample_size = ADC0_SAMPLE_SIZE},
                                        [1]  =  {.chan = MIX0_CHAN_ID, .sample_size = MIX0_SAMPLE_SIZE},
                                        [2]  =  {.chan = TBTAMP0_CHAN_ID, .sample_size = TBTAMP0_SAMPLE_SIZE},
                                        [3]  =  {.chan = TBTPOS0_CHAN_ID, .sample_size = TBTPOS0_SAMPLE_SIZE},
                                        [4]  =  {.chan = FOFBAMP0_CHAN_ID, .sample_size = FOFBAMP0_SAMPLE_SIZE},
                                        [5]  =  {.chan = FOFBPOS0_CHAN_ID, .sample_size = FOFBPOS0_SAMPLE_SIZE},
                                        [6]  =  {.chan = MONITAMP0_CHAN_ID, .sample_size = MONITAMP0_SAMPLE_SIZE},
                                        [7]  =  {.chan = MONITPOS0_CHAN_ID, .sample_size = MONITPOS0_SAMPLE_SIZE},
                                        [8]  =  {.chan = MONIT1POS0_CHAN_ID, .sample_size = MONIT1POS0_SAMPLE_SIZE}
                                    };
#else
#error "Unsupported board!"
#endif


/********************************************************/
/************************ Our API ***********************/
/********************************************************/
bpm_client_t *bpm_client_new (char *broker_endp, int verbose,
        const char *log_file_name)
{
    return _bpm_client_new (broker_endp, verbose, log_file_name,
            BPMCLIENT_DFLT_LOG_MODE);
}

bpm_client_t *bpm_client_new_log_mode (char *broker_endp, int verbose,
        const char *log_file_name, const char *log_mode)
{
    return _bpm_client_new (broker_endp, verbose, log_file_name,
            log_mode);
}

void bpm_client_destroy (bpm_client_t **self_p)
{
    assert (self_p);

    if (*self_p) {
        bpm_client_t *self = *self_p;

        self->acq_chan = NULL;
        mdp_client_destroy (&self->mdp_client);
        free (self);
        *self_p = NULL;
    }
}

/**************** Static LIB Client Functions ****************/
static bpm_client_t *_bpm_client_new (char *broker_endp, int verbose,
        const char *log_file_name, const char *log_mode)
{
    assert (broker_endp);

    /* Set logfile available for all dev_mngr and dev_io instances.
     * We accept NULL as a parameter, meaning to suppress all messages */
    debug_set_log (log_file_name, log_mode);

    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_INFO, "[libclient] Spawing LIBCLIENT"
            " with broker address %s, with logfile on %s ...\n", broker_endp,
            (log_file_name == NULL) ? "NULL" : log_file_name);

    bpm_client_t *self = zmalloc (sizeof *self);
    ASSERT_ALLOC(self, err_self_alloc);
    self->mdp_client = mdp_client_new (broker_endp, verbose);
    ASSERT_TEST(self->mdp_client!=NULL, "Could not create MDP client",
            err_mdp_client);

    /* Initialize acquisition table */
    self->acq_chan = acq_chan;

    return self;

err_mdp_client:
    free (self);
err_self_alloc:
    return NULL;
}

/**************** General Function to call the others *********/

bpm_client_err_e bpm_func_exec (bpm_client_t *self, const disp_op_t *func, char *service, uint8_t *input, uint8_t *output)
{
    bpm_client_err_e err = BPM_CLIENT_SUCCESS;
    
    /* Check input arguments */
    ASSERT_TEST(self != NULL, "Bpm_client is NULL", err_null_exp, BPM_CLIENT_ERR_INV_FUNCTION);
    ASSERT_TEST(func != NULL, "Function structure is NULL", err_null_exp, BPM_CLIENT_ERR_INV_FUNCTION);
    ASSERT_TEST(!(func->args[0] != DISP_ARG_END && input == NULL), "Invalid input arguments!", err_inv_param, BPM_CLIENT_ERR_INV_PARAM);
    ASSERT_TEST(!(func->retval != DISP_ARG_END && output == NULL), "Invalid output arguments!", err_inv_param, BPM_CLIENT_ERR_INV_PARAM);

    /* Create the message */
    zmsg_t *msg = zmsg_new ();
    ASSERT_ALLOC(msg, err_msg_alloc, BPM_CLIENT_ERR_ALLOC);

    /* Add the frame containing the opcode for the function desired (always first) */
    zmsg_addmem (msg, &func->opcode, sizeof (func->opcode));


    /* Add the arguments in their respective frames in the message */
    for (int i = 0; func->args[i] != DISP_ARG_END; ++i) {
        /* Get the size of the message being sent */
        uint32_t in_size = DISP_GET_ASIZE(func->args[i]);
        /* Create a frame to compose the message */
        zmsg_addmem (msg, input, in_size);
        /* Moves along the pointer */
        input += in_size;
    }

    mdp_client_send (self->mdp_client, service, &msg);

    /* Receive report */
    zmsg_t *report = mdp_client_recv (self->mdp_client, NULL, NULL);
    ASSERT_TEST(report != NULL, "Report received is NULL", err_msg);

    /* Message is:
     * frame 0: Error code
     * frame 1: Number of bytes received
     * frame 2+: Data received      */

    /* Handling malformed messages */
    size_t msg_size = zmsg_size (report);
    ASSERT_TEST(msg_size == MSG_ERR_CODE_SIZE || msg_size == MSG_FULL_SIZE,
            "Unexpected message received", err_msg);

    /* Get message contents */
    zframe_t *err_code = zmsg_pop(report);
    ASSERT_TEST(err_code != NULL, "Could not receive error code", err_msg);

    zframe_t *data_size_frm = NULL;
    zframe_t *data_frm = NULL;
    if (msg_size == MSG_FULL_SIZE)
    {
        data_size_frm = zmsg_pop (report);
        ASSERT_TEST(data_size_frm != NULL, "Could not receive data size", err_null_data_size);
        data_frm = zmsg_pop (report);
        ASSERT_TEST(data_frm != NULL, "Could not receive data", err_null_data);

        ASSERT_TEST(zframe_size (data_size_frm) == RW_REPLY_SIZE,
                "Wrong <number of payload bytes> parameter size", err_msg_fmt);

        /* Size in the second frame must match the frame size of the third */
        RW_REPLY_TYPE data_size = *(RW_REPLY_TYPE *) zframe_data(data_size_frm);
        ASSERT_TEST(data_size == zframe_size (data_frm),
                "<payload> parameter size does not match size in <number of payload bytes> parameter",
                err_msg_fmt);

        uint32_t *data_out = (uint32_t *)zframe_data(data_frm);
        /* Copy message contents to user */
        memcpy (output, data_out, data_size);
    }

err_msg_fmt:
    zframe_destroy (&data_frm);
err_null_data:
    zframe_destroy (&data_size_frm);
err_null_data_size:
    zframe_destroy (&err_code);
err_msg:
    zmsg_destroy (&report);
err_msg_alloc:
    zmsg_destroy (&msg);
err_null_exp:
err_inv_param:
    return err;
}

const disp_op_t *bpm_func_translate (char *name)
{
    if (name != NULL)
    {
        /* Search the function table for a match in the 'name' field */
        for (int i=0; smio_exp_ops[i] != NULL; i++)
        {
            for (int j=0; smio_exp_ops[i][j] != NULL; j++)
            {
                if (streq(name, smio_exp_ops[i][j]->name))
                {
                    return smio_exp_ops[i][j];
                }
            }
        }
    }
    return 0;
}

bpm_client_err_e bpm_func_trans_exec (bpm_client_t *self, char *name, char *service, uint8_t *input, uint8_t *output)
{
    const disp_op_t *func = bpm_func_translate(name);
    bpm_client_err_e err = bpm_func_exec (self, func, service, input, output);
    return err;
}
/**************** FMC130M SMIO Functions ****************/
bpm_client_err_e bpm_blink_leds (bpm_client_t *self, char *service, uint32_t leds)
{
    assert (self);
    assert (service);

    bpm_client_err_e err = BPM_CLIENT_SUCCESS;
    FMC130M_4CH_REPLY_TYPE operation = FMC130M_4CH_OPCODE_LEDS;

    zmsg_t *request = zmsg_new ();
    ASSERT_ALLOC(request, err_send_msg_alloc, BPM_CLIENT_ERR_ALLOC);
    zmsg_addmem (request, &operation, sizeof (operation));
    zmsg_addmem (request, &leds, sizeof (leds));
    mdp_client_send (self->mdp_client, service, &request);

err_send_msg_alloc:
    return err;
}

/* FUNCTION pin functions */
PARAM_FUNC_CLIENT_WRITE(fmc_pll_function)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_PLL_FUNCTION,
            fmc_pll_function);
}

PARAM_FUNC_CLIENT_READ(fmc_pll_function)
{
    return param_client_read (self, service, FMC130M_4CH_OPCODE_PLL_FUNCTION,
            fmc_pll_function);
}

bpm_client_err_e bpm_ad9510_cfg_defaults (bpm_client_t *self, char *service)
{
    assert (self);
    assert (service);

    bpm_client_err_e err = BPM_CLIENT_SUCCESS;
    FMC130M_4CH_REPLY_TYPE operation = FMC130M_4CH_OPCODE_AD9510_CFG_DEFAULTS;

    zmsg_t *request = zmsg_new ();
    ASSERT_ALLOC(request, err_send_msg_alloc, BPM_CLIENT_ERR_ALLOC);
    zmsg_addmem (request, &operation, sizeof (operation));
    mdp_client_send (self->mdp_client, service, &request);

err_send_msg_alloc:
    return err;
}

/* RAW ADC data 0 value */
PARAM_FUNC_CLIENT_WRITE(adc_data0)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DATA0, adc_data0);
}

PARAM_FUNC_CLIENT_READ(adc_data0)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DATA0, adc_data0);
}

/* RAW ADC data 1 value */
PARAM_FUNC_CLIENT_WRITE(adc_data1)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DATA1, adc_data1);
}

PARAM_FUNC_CLIENT_READ(adc_data1)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DATA1, adc_data1);
}

/* RAW ADC data 2 value */
PARAM_FUNC_CLIENT_WRITE(adc_data2)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DATA2, adc_data2);
}

PARAM_FUNC_CLIENT_READ(adc_data2)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DATA2, adc_data2);
}

/* RAW ADC data 3 value */
PARAM_FUNC_CLIENT_WRITE(adc_data3)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DATA3, adc_data3);
}

PARAM_FUNC_CLIENT_READ(adc_data3)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DATA3, adc_data3);
}

/****************** FMC130M Delay Value Functions ****************/

/* ADC delay value 0 */
PARAM_FUNC_CLIENT_WRITE(adc_dly_val0)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DLY_VAL0, adc_dly_val0);
}

PARAM_FUNC_CLIENT_READ(adc_dly_val0)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DLY_VAL0, adc_dly_val0);
}

/* ADC delay value 1 */
PARAM_FUNC_CLIENT_WRITE(adc_dly_val1)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DLY_VAL1, adc_dly_val1);
}

PARAM_FUNC_CLIENT_READ(adc_dly_val1)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DLY_VAL1, adc_dly_val1);
}

/* ADC delay value 2 */
PARAM_FUNC_CLIENT_WRITE(adc_dly_val2)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DLY_VAL2, adc_dly_val2);
}

PARAM_FUNC_CLIENT_READ(adc_dly_val2)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DLY_VAL2, adc_dly_val2);
}

/* ADC delay value 3 */
PARAM_FUNC_CLIENT_WRITE(adc_dly_val3)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DLY_VAL3, adc_dly_val3);
}

PARAM_FUNC_CLIENT_READ(adc_dly_val3)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DLY_VAL3, adc_dly_val3);
}

/****************** FMC130M Delay Line selection Functions ****************/

/* ADC line value 0 */
PARAM_FUNC_CLIENT_WRITE(adc_dly_line0)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DLY_LINE0, adc_dly_line0);
}

PARAM_FUNC_CLIENT_READ(adc_dly_line0)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DLY_LINE0, adc_dly_line0);
}

/* ADC line value 1 */
PARAM_FUNC_CLIENT_WRITE(adc_dly_line1)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DLY_LINE1, adc_dly_line1);
}

PARAM_FUNC_CLIENT_READ(adc_dly_line1)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DLY_LINE1, adc_dly_line1);
}

/* ADC line value 2 */
PARAM_FUNC_CLIENT_WRITE(adc_dly_line2)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DLY_LINE2, adc_dly_line2);
}

PARAM_FUNC_CLIENT_READ(adc_dly_line2)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DLY_LINE2, adc_dly_line2);
}

/* ADC line value 3 */
PARAM_FUNC_CLIENT_WRITE(adc_dly_line3)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DLY_LINE3, adc_dly_line3);
}

PARAM_FUNC_CLIENT_READ(adc_dly_line3)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DLY_LINE3, adc_dly_line3);
}

/****************** FMC130M Delay update Functions ****************/

/* ADC Update channel 0 */
PARAM_FUNC_CLIENT_WRITE(adc_dly_updt0)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DLY_UPDT0, adc_dly_updt0);
}

PARAM_FUNC_CLIENT_READ(adc_dly_updt0)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DLY_UPDT0, adc_dly_updt0);
}

/* ADC Update channel 1 */
PARAM_FUNC_CLIENT_WRITE(adc_dly_updt1)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DLY_UPDT1, adc_dly_updt1);
}

PARAM_FUNC_CLIENT_READ(adc_dly_updt1)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DLY_UPDT1, adc_dly_updt1);
}

/* ADC Update channel 2 */
PARAM_FUNC_CLIENT_WRITE(adc_dly_updt2)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DLY_UPDT2, adc_dly_updt2);
}

PARAM_FUNC_CLIENT_READ(adc_dly_updt2)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DLY_UPDT2, adc_dly_updt2);
}

/* ADC Update channel 3 */
PARAM_FUNC_CLIENT_WRITE(adc_dly_updt3)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_ADC_DLY_UPDT3, adc_dly_updt3);
}

PARAM_FUNC_CLIENT_READ(adc_dly_updt3)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_ADC_DLY_UPDT3, adc_dly_updt3);
}

/****************** FMC130M set delay functions ****************/

/* ADC set delay channel 0 */
PARAM_FUNC_CLIENT_WRITE2(adc_dly0, type, val)
{
    return param_client_write_raw (self, service, FMC130M_4CH_OPCODE_ADC_DLY0,
            type, val);
}

/* ADC set delay channel 1 */
PARAM_FUNC_CLIENT_WRITE2(adc_dly1, type, val)
{
    return param_client_write_raw (self, service, FMC130M_4CH_OPCODE_ADC_DLY1,
            type, val);
}

/* ADC set delay channel 2 */
PARAM_FUNC_CLIENT_WRITE2(adc_dly2, type, val)
{
    return param_client_write_raw (self, service, FMC130M_4CH_OPCODE_ADC_DLY2,
            type, val);
}
/* ADC set delay channel 3 */
PARAM_FUNC_CLIENT_WRITE2(adc_dly3, type, val)
{
    return param_client_write_raw (self, service, FMC130M_4CH_OPCODE_ADC_DLY3,
            type, val);
}

/****************** FMC130M Control functions ****************/

PARAM_FUNC_CLIENT_WRITE(adc_test_data_en)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_TEST_DATA_EN, adc_test_data_en);
}

PARAM_FUNC_CLIENT_READ(adc_test_data_en)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_TEST_DATA_EN, adc_test_data_en);
}

PARAM_FUNC_CLIENT_WRITE(si571_oe)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_SI571_OE, si571_oe);
}

PARAM_FUNC_CLIENT_READ(si571_oe)
{
     return param_client_read (self, service, FMC130M_4CH_OPCODE_SI571_OE, si571_oe);
}

PARAM_FUNC_CLIENT_WRITE(trig_dir)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_TRIG_DIR, trig_dir);
}

PARAM_FUNC_CLIENT_READ(trig_dir)
{
    return param_client_read (self, service, FMC130M_4CH_OPCODE_TRIG_DIR, trig_dir);
}

PARAM_FUNC_CLIENT_WRITE(trig_term)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_TRIG_TERM, trig_term);
}

PARAM_FUNC_CLIENT_READ(trig_term)
{
    return param_client_read (self, service, FMC130M_4CH_OPCODE_TRIG_TERM, trig_term);
}

PARAM_FUNC_CLIENT_WRITE(trig_val)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_TRIG_VAL, trig_val);
}

PARAM_FUNC_CLIENT_READ(trig_val)
{
    return param_client_read (self, service, FMC130M_4CH_OPCODE_TRIG_VAL, trig_val);
}

/*************************** FMC130M Chips Functions *************************/

/* AD9510 PLL A divider */
PARAM_FUNC_CLIENT_WRITE(ad9510_pll_a_div)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_AD9510_PLL_A_DIV,
            ad9510_pll_a_div);
}

/* AD9510 PLL B divider */
PARAM_FUNC_CLIENT_WRITE(ad9510_pll_b_div)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_AD9510_PLL_B_DIV,
            ad9510_pll_b_div);
}

/* AD9510 PLL Prescaler */
PARAM_FUNC_CLIENT_WRITE(ad9510_pll_prescaler)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_AD9510_PLL_PRESCALER,
            ad9510_pll_prescaler);
}

/* AD9510 R divider */
PARAM_FUNC_CLIENT_WRITE(ad9510_r_div)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_AD9510_R_DIV,
            ad9510_r_div);
}

/* AD9510 PLL Power Down */
PARAM_FUNC_CLIENT_WRITE(ad9510_pll_pdown)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_AD9510_PLL_PDOWN,
            ad9510_pll_pdown);
}

/* AD9510 Mux Status */
PARAM_FUNC_CLIENT_WRITE(ad9510_mux_status)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_AD9510_MUX_STATUS,
            ad9510_mux_status);
}

/* AD9510 CP current */
PARAM_FUNC_CLIENT_WRITE(ad9510_cp_current)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_AD9510_CP_CURRENT,
            ad9510_cp_current);
}

/* AD9510 Outputs */
PARAM_FUNC_CLIENT_WRITE(ad9510_outputs)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_AD9510_OUTPUTS,
            ad9510_outputs);
}

/* AD9510 PLL CLK Selection */
PARAM_FUNC_CLIENT_WRITE(ad9510_pll_clk_sel)
{
    return param_client_write (self, service, FMC130M_4CH_OPCODE_AD9510_PLL_CLK_SEL,
            ad9510_pll_clk_sel);
}

/* SI571 Set frequency */
PARAM_FUNC_CLIENT_WRITE_DOUBLE(si571_set_freq)
{
    return param_client_write_double (self, service, FMC130M_4CH_OPCODE_SI571_SET_FREQ,
            si571_set_freq);
}

/* SI571 Get defaults */
PARAM_FUNC_CLIENT_WRITE_DOUBLE(si571_defaults)
{
    return param_client_write_double (self, service, FMC130M_4CH_OPCODE_SI571_GET_DEFAULTS,
            si571_defaults);
}

/****************** ACQ SMIO Functions ****************/
#define MIN_WAIT_TIME           1                           /* in ms */
#define MSECS                   1000                        /* in seconds */

static bpm_client_err_e _bpm_data_acquire (bpm_client_t *self, char *service,
        acq_req_t *acq_req);
static bpm_client_err_e _bpm_check_data_acquire (bpm_client_t *self, char *service);
static bpm_client_err_e _bpm_wait_data_acquire_timed (bpm_client_t *self, char *service,
        int timeout);
static bpm_client_err_e _bpm_get_data_block (bpm_client_t *self, char *service,
        acq_trans_t *acq_trans);
static bpm_client_err_e _bpm_acq_start (bpm_client_t *self, char *service, acq_req_t *acq_req);
static bpm_client_err_e _bpm_acq_check (bpm_client_t *self, char *service);
static bpm_client_err_e _bpm_acq_get_data_block (bpm_client_t *self, char *service, acq_trans_t *acq_trans);
static bpm_client_err_e _bpm_acq_get_curve (bpm_client_t *self, char *service, acq_trans_t *acq_trans);
static bpm_client_err_e _bpm_full_acq (bpm_client_t *self, char *service, acq_trans_t *acq_trans, int timeout);

bpm_client_err_e bpm_data_acquire (bpm_client_t *self, char *service, acq_req_t *acq_req)
{
    return _bpm_data_acquire (self, service, acq_req);
}

bpm_client_err_e bpm_check_data_acquire (bpm_client_t *self, char *service)
{
    return _bpm_check_data_acquire (self, service);
}

bpm_client_err_e bpm_wait_data_acquire_timed (bpm_client_t *self, char *service,
        int timeout)
{
    return _bpm_wait_data_acquire_timed (self, service, timeout);
}

bpm_client_err_e bpm_get_data_block (bpm_client_t *self, char *service,
        acq_trans_t *acq_trans)
{
    return _bpm_get_data_block (self, service, acq_trans);
}

bpm_client_err_e bpm_acq_start (bpm_client_t *self, char *service, acq_req_t *acq_req)
{
    return _bpm_acq_start (self, service, acq_req);
}

bpm_client_err_e bpm_acq_check (bpm_client_t *self, char *service)
{
    return _bpm_acq_check (self, service);
}

bpm_client_err_e bpm_acq_get_data_block (bpm_client_t *self, char *service, acq_trans_t *acq_trans)
{
    return _bpm_acq_get_data_block (self, service, acq_trans);
}

bpm_client_err_e bpm_acq_get_curve (bpm_client_t *self, char *service, acq_trans_t *acq_trans)
{
    return _bpm_acq_get_curve (self, service, acq_trans);
}

bpm_client_err_e bpm_full_acq (bpm_client_t *self, char *service, acq_trans_t *acq_trans, int timeout)
{
    return _bpm_full_acq (self, service, acq_trans, timeout);
}

static bpm_client_err_e _bpm_data_acquire (bpm_client_t *self, char *service,
        acq_req_t *acq_req)
{
    assert (self);
    assert (service);
    assert (acq_req);

    bpm_client_err_e err = BPM_CLIENT_SUCCESS;
    ACQ_OPCODE_TYPE operation = ACQ_OPCODE_DATA_ACQUIRE;

    /* Message is:
     * frame 0: operation code
     * frame 1: number of samples
     * frame 2: channel             */
    zmsg_t *request = zmsg_new ();
    zmsg_addmem (request, &operation, sizeof (operation));
    zmsg_addmem (request, &acq_req->num_samples, sizeof (acq_req->num_samples));
    zmsg_addmem (request, &acq_req->chan, sizeof (acq_req->chan));
    mdp_client_send (self->mdp_client, service, &request);

    /* Receive report */
    zmsg_t *report = mdp_client_recv (self->mdp_client, NULL, NULL);
    ASSERT_TEST(report != NULL, "Report received is NULL", err_null_report);

    /* Message is:
     * frame 0: error code      */

    /* Handling malformed messages */
    size_t msg_size = zmsg_size (report);
    ASSERT_TEST(msg_size == MSG_ERR_CODE_SIZE, "Unexpected message received", err_msg);

    /* Get message contents */
    zframe_t *err_code = zmsg_pop(report);
    ASSERT_TEST(err_code != NULL, "Could not receive error code", err_null_code);

    /* Check for return code from server */
    ASSERT_TEST(*(ACQ_REPLY_TYPE *) zframe_data(err_code) == ACQ_OK,
            "bpm_data_acquire: Data acquire was not required correctly",
            err_data_acquire, BPM_CLIENT_ERR_AGAIN);
    /* If we are here, then the request was successfully acquired*/
    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_data_acquire: "
            "Data acquire was successfully required\n");

err_data_acquire:
    zframe_destroy (&err_code);
err_null_code:
err_msg:
    zmsg_destroy (&report);
err_null_report:
    return err;
}

static bpm_client_err_e _bpm_check_data_acquire (bpm_client_t *self, char *service)
{
    assert (self);
    assert (service);

    int err = BPM_CLIENT_SUCCESS;
    ACQ_OPCODE_TYPE operation = ACQ_OPCODE_CHECK_DATA_ACQUIRE;

    /* Message is:
     * frame 0: operation code      */
    zmsg_t *request = zmsg_new ();
    zmsg_addmem (request, &operation, sizeof (operation));
    mdp_client_send (self->mdp_client, service, &request);

    /* Receive report */
    zmsg_t *report = mdp_client_recv (self->mdp_client, NULL, NULL);
    ASSERT_TEST(report != NULL, "Report received is NULL", err_null_report);

    /* Message is:
     * frame 0: error code      */

    /* Handling malformed messages */
    size_t msg_size = zmsg_size (report);
    ASSERT_TEST(msg_size == MSG_ERR_CODE_SIZE, "Unexpected message received", err_msg);

    /* Get message contents */
    zframe_t *err_code = zmsg_pop (report);
    ASSERT_TEST(err_code != NULL, "Could not receive error code", err_null_code);

    /* Check for return code from server */
    if (*(ACQ_REPLY_TYPE *) zframe_data (err_code) != ACQ_OK) {
        DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_check_data_acquire: "
                "Check fail: data acquire was not completed\n");
        err = BPM_CLIENT_ERR_AGAIN;
        goto err_check_data_acquire;
    }

    /* If we are here, then the request was successfully acquired*/
    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_check_data_acquire: "
            "Check ok: data acquire was successfully completed\n");

err_check_data_acquire:
    zframe_destroy (&err_code);
err_null_code:
err_msg:
    zmsg_destroy (&report);
err_null_report:
    return err;
}

static bpm_client_err_e _bpm_wait_data_acquire_timed (bpm_client_t *self, char *service,
        int timeout)
{
    assert (self);
    assert (service);

    /* timeout < 0 means "infinite" wait */
    if (timeout < 0) {
        /* FIXME! Very unportable way! We assume that time_t is at least of
         * size INT */
        timeout = INT_MAX;
    }

    bpm_client_err_e err = BPM_CLIENT_SUCCESS;
    time_t start = time (NULL);
    while ((time(NULL) - start)*1000 < timeout) {
        if (zctx_interrupted) {
            err = BPM_CLIENT_INT;
            goto bpm_zctx_interrupted;
        }

        err = _bpm_check_data_acquire (self, service);
        if (err == BPM_CLIENT_SUCCESS) {
            DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] "
                    "bpm_wait_data_acquire_timed: finished waiting\n");
            goto exit;
        }

        usleep (MSECS*MIN_WAIT_TIME);
    }

    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] "
            "bpm_wait_data_acquire_timed: number of tries was exceeded\n");
    /* timeout occured */
    err = BPM_CLIENT_ERR_TIMEOUT;

bpm_zctx_interrupted:
exit:
    return err;
}

static bpm_client_err_e _bpm_get_data_block (bpm_client_t *self, char *service,
        acq_trans_t *acq_trans)
{
    assert (self);
    assert (service);
    assert (acq_trans);
    assert (acq_trans->block.data);

    bpm_client_err_e err = BPM_CLIENT_SUCCESS;
    ACQ_OPCODE_TYPE operation = ACQ_OPCODE_GET_DATA_BLOCK;

    /* Message is:
     * frame 0: operation code
     * frame 1: channel
     * frame 2: block required          */
    zmsg_t *request = zmsg_new ();
    zmsg_addmem (request, &operation, sizeof (operation));
    zmsg_addmem (request, &acq_trans->req.chan, sizeof (acq_trans->req.chan));
    zmsg_addmem (request, &acq_trans->block.idx, sizeof (acq_trans->block.idx));
    mdp_client_send (self->mdp_client, service, &request);

    /* Receive report */
    zmsg_t *report = mdp_client_recv (self->mdp_client, NULL, NULL);
    ASSERT_TEST(report != NULL, "Report received is NULL", err_null_report);

    /* Message is:
     * frame 0: error code
     * frame 1: number of bytes read (optional)
     * frame 2: data read (optional) */

    /* Handling malformed messages */
    size_t msg_size = zmsg_size (report);
    ASSERT_TEST(msg_size == MSG_ERR_CODE_SIZE || msg_size == MSG_FULL_SIZE,
            "Unexpected message received", err_msg);

    /* Get message contents */
    zframe_t *err_code = zmsg_pop (report);
    ASSERT_TEST(err_code != NULL, "Could not receive error code", err_null_code);

    /* Check for return code from server */
    ASSERT_TEST(*(RW_REPLY_TYPE *) zframe_data (err_code) == ACQ_OK,
            "bpm_get_data_block: Data block was not acquired",
            err_error_code, BPM_CLIENT_ERR_SERVER);

    zframe_t *data_size_frm = NULL;
    zframe_t *data_frm = NULL;
    if (msg_size == MSG_FULL_SIZE) {
        data_size_frm = zmsg_pop (report);
        ASSERT_TEST(data_size_frm != NULL, "Could not receive data size", err_null_data_size);
        data_frm = zmsg_pop (report);
        ASSERT_TEST(data_frm != NULL, "Could not receive data", err_null_data);

        ASSERT_TEST(zframe_size (data_size_frm) == RW_REPLY_SIZE,
                "Wrong <number of payload bytes> parameter size", err_msg_fmt);

        /* Size in the second frame must match the frame size of the third */
        RW_REPLY_TYPE data_size = *(RW_REPLY_TYPE *) zframe_data(data_size_frm);
        ASSERT_TEST(data_size == zframe_size (data_frm),
                "<payload> parameter size does not match size in <number of payload bytes> parameter",
                err_msg_fmt);

        /* frame 2: data read (optional)
         *          data read = data block
         *                      valid bytes */
        smio_acq_data_block_t *data_block = (smio_acq_data_block_t *) zframe_data (data_frm);

        /* Data size effectively returned */
        uint32_t read_size = (acq_trans->block.data_size < data_block->valid_bytes) ?
            acq_trans->block.data_size : data_block->valid_bytes;

        /* Copy message contents to user */
        memcpy (acq_trans->block.data, data_block->data, read_size);
        /* Inform user about the number of bytes effectively copied*/
        acq_trans->block.bytes_read = read_size;

        /* Print some debug messages */
        DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_data_block: "
                "read_size: %u\n", read_size);
        DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_data_block: "
                "acq_trans->block.data: %p\n", acq_trans->block.data);
    }

err_msg_fmt:
    zframe_destroy (&data_frm);
err_null_data:
    zframe_destroy (&data_size_frm);
err_null_data_size:
err_error_code:
    zframe_destroy (&err_code);
err_null_code:
err_msg:
    zmsg_destroy (&report);
err_null_report:
    return err;
}

bpm_client_err_e bpm_get_curve (bpm_client_t *self, char *service,
        acq_trans_t *acq_trans, int timeout)
{
    assert (self);
    assert (service);
    assert (acq_trans);
    assert (acq_trans->block.data);

    /* Client requisition: data acquire */
    bpm_client_err_e err = _bpm_data_acquire (self, service, &acq_trans->req);
    ASSERT_TEST(err == BPM_CLIENT_SUCCESS, "Could not request acqusition\n",
            err_bpm_data_acquire);

    /* Client requisition: wait data acquire indefinetly */
    err = _bpm_wait_data_acquire_timed (self, service, timeout);
    ASSERT_TEST(err == BPM_CLIENT_SUCCESS, "Request acquisition timed out\n",
            err_bpm_wait_data_acquire);

    /* FIXME: When the last block is full 'block_n_valid exceeds by one */
    uint32_t block_n_valid = acq_trans->req.num_samples /
        (BLOCK_SIZE/self->acq_chan[acq_trans->req.chan].sample_size);
    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
            "block_n_valid = %u\n", block_n_valid);

    uint32_t total_bread = 0;   /* Total bytes read */
    uint32_t data_size = acq_trans->block.data_size;  /* Save the original buffer size fopr later */
    /* Client requisition: get data block */
    for (uint32_t block_n = 0; block_n <= block_n_valid; block_n++) {
        if (zctx_interrupted) {
            err = BPM_CLIENT_INT;
            goto bpm_zctx_interrupted;
        }

        acq_trans->block.idx = block_n;
        err = _bpm_get_data_block (self, service, acq_trans);

        /* Check for return code */
        ASSERT_TEST(err == BPM_CLIENT_SUCCESS,
                "_bpm_get_data_block failed. block_n is probably out of range",
                err_bpm_get_data_block);

        total_bread += acq_trans->block.bytes_read;
        acq_trans->block.data = (uint32_t *)((uint8_t *)acq_trans->block.data + acq_trans->block.bytes_read);
        acq_trans->block.data_size -= acq_trans->block.bytes_read;

        /* Print some debug messages */
        DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
                "Total bytes read up to now: %u\n", total_bread);
        DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
                "Data pointer addr: %p\n", acq_trans->block.data);
        DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
                "Data buffer size: %u\n", acq_trans->block.data_size);
    }

    /* Return to client the total number of bytes read */
    acq_trans->block.bytes_read = total_bread;
    acq_trans->block.data_size = data_size;

    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
        "Data curve of %u bytes was successfully acquired\n", total_bread);

err_bpm_get_data_block:
bpm_zctx_interrupted:
err_bpm_wait_data_acquire:
err_bpm_data_acquire:
    return err;
}

static bpm_client_err_e _bpm_acq_start (bpm_client_t *self, char *service, acq_req_t *acq_req)
{
    uint8_t write_val[sizeof(uint32_t)*2] = {0};
    *write_val = acq_req->num_samples;
    *(write_val+4) = acq_req->chan;

    const disp_op_t* func = bpm_func_translate(ACQ_NAME_DATA_ACQUIRE);
    bpm_client_err_e err = bpm_func_exec(self, func, service, write_val, NULL);

    /* Check if any error ocurred */
    ASSERT_TEST(err == BPM_CLIENT_SUCCESS, "bpm_data_acquire: Data acquire was not required correctly",
            err_data_acquire, BPM_CLIENT_ERR_AGAIN);

    /* If we are here, then the request was successfully acquired*/
    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_data_acquire: "
            "Data acquire was successfully required\n");

err_data_acquire:
    return err;
}

static bpm_client_err_e _bpm_acq_check (bpm_client_t *self, char *service)
{
    const disp_op_t* func = bpm_func_translate(ACQ_NAME_CHECK_DATA_ACQUIRE);
    bpm_client_err_e err = bpm_func_exec(self, func, service, NULL, NULL);

    /* Check if any error ocurred */
    ASSERT_TEST(err == BPM_CLIENT_SUCCESS,
            "bpm_check_data_acquire: Check fail: data acquire was not completed",
            err_check_data_acquire, BPM_CLIENT_ERR_SERVER);

    /* If we are here, then the request was successfully acquired*/
    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_acq_check: "
            "Check ok: data acquire was successfully completed\n");

err_check_data_acquire:
    return err;
}

static bpm_client_err_e _bpm_acq_get_data_block (bpm_client_t *self, char *service, acq_trans_t *acq_trans)
{
    assert (self);
    assert (service);
    assert (acq_trans);
    assert (acq_trans->block.data);

    uint8_t write_val[sizeof(uint32_t)*2] = {0};
    *write_val = acq_trans->req.chan;
    *(write_val+4) = acq_trans->block.idx;

    smio_acq_data_block_t *read_val = zmalloc (sizeof (smio_acq_data_block_t));

    /* Sent Message is:
     * frame 0: operation code
     * frame 1: channel
     * frame 2: block required */

    const disp_op_t* func = bpm_func_translate(ACQ_NAME_GET_DATA_BLOCK);
    bpm_client_err_e err = bpm_func_exec(self, func, service, write_val, (uint8_t *)read_val);

    /* Received Message is:
     * frame 0: error code
     * frame 1: data size
     * frame 2: data block */

    /* Check if any error ocurred */
    ASSERT_TEST(err == BPM_CLIENT_SUCCESS,
            "bpm_get_data_block: Data block was not acquired",
            err_get_data_block, BPM_CLIENT_ERR_SERVER);

    /* Data size effectively returned */
    uint32_t read_size = (acq_trans->block.data_size < read_val->valid_bytes) ?
        acq_trans->block.data_size : read_val->valid_bytes;

    /* Copy message contents to user */
    memcpy (acq_trans->block.data, read_val->data, read_size);

    /* Inform user about the number of bytes effectively copied */
    acq_trans->block.bytes_read = read_size;

    /* Print some debug messages */
    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_data_block: "
            "read_size: %u\n", read_val->data_size);
    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_data_block: "
            "acq_trans->block.data: %p\n", acq_trans->block.data);

err_get_data_block:
    free(read_val);
    return err;
}

static bpm_client_err_e _bpm_acq_get_curve (bpm_client_t *self, char *service, acq_trans_t *acq_trans)
{
    assert (self);
    assert (service);
    assert (acq_trans);
    assert (acq_trans->block.data);

    bpm_client_err_e err = BPM_CLIENT_SUCCESS;
    
    uint32_t block_n_valid = acq_trans->req.num_samples /
        (BLOCK_SIZE/self->acq_chan[acq_trans->req.chan].sample_size);
    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_full_acq: "
            "block_n_valid = %u\n", block_n_valid);

    /* Total bytes read */
    uint32_t total_bread = 0;
    uint32_t data_size = acq_trans->block.data_size;
    /* Save the original buffer size for later */
    uint32_t *original_data_pt = acq_trans->block.data;

    /* Fill all blocks */
    for (uint32_t block_n = 0; block_n <= block_n_valid; block_n++) {
        if (zctx_interrupted) {
            err = BPM_CLIENT_INT;
            goto bpm_zctx_interrupted;
        }

        acq_trans->block.idx = block_n;
        err = bpm_acq_get_data_block(self, service, acq_trans);

        /* Check for return code */
        ASSERT_TEST(err == BPM_CLIENT_SUCCESS,
                "_bpm_get_data_block failed. block_n is probably out of range",
                err_bpm_get_data_block);

        total_bread += acq_trans->block.bytes_read;
        acq_trans->block.data = (uint32_t *)((uint8_t *)acq_trans->block.data + acq_trans->block.bytes_read);
        acq_trans->block.data_size -= acq_trans->block.bytes_read;

        /* Print some debug messages */
        DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
                "Total bytes read up to now: %u\n", total_bread);
        DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
                "Data pointer addr: %p\n", acq_trans->block.data);
        DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
                "Data buffer size: %u\n", acq_trans->block.data_size);
    }

    /* Return to client the total number of bytes read */
    acq_trans->block.bytes_read = total_bread;
    acq_trans->block.data_size = data_size;
    acq_trans->block.data = original_data_pt;

    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
            "Data curve of %u bytes was successfully acquired\n", total_bread);

bpm_zctx_interrupted:
err_bpm_get_data_block:
    return err;
}

static bpm_client_err_e _bpm_full_acq (bpm_client_t *self, char *service, acq_trans_t *acq_trans, int timeout)
{
    /* TODO: Use a error check like ASSERT_TEST instead assert() */
    assert (self);
    assert (service);
    assert (acq_trans);
    assert (acq_trans->block.data);

    /* Send Acquisition Request */
    bpm_acq_start (self, service, &acq_trans->req);

    /* Wait until the acquisition is finished */
    bpm_client_err_e err = func_polling (self, ACQ_NAME_CHECK_DATA_ACQUIRE, service, NULL, NULL, timeout);

    ASSERT_TEST(err == BPM_CLIENT_SUCCESS,
            "bpm_check_data_acquire: Check fail: data acquire was not completed",
            err_check_data_acquire, BPM_CLIENT_ERR_SERVER);

    /* If we are here, then the acquisition was successfully completed*/
    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_acq_check: "
            "Check ok: data acquire was successfully completed\n");

    /* FIXME: When the last block is full 'block_n_valid exceeds by one */
    uint32_t block_n_valid = acq_trans->req.num_samples /
        (BLOCK_SIZE/self->acq_chan[acq_trans->req.chan].sample_size);
    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_full_acq: "
            "block_n_valid = %u\n", block_n_valid);

    uint32_t total_bread = 0;   /* Total bytes read */
    uint32_t data_size = acq_trans->block.data_size;  /* Save the original buffer size for later */
    uint32_t *original_data_pt = acq_trans->block.data;

    /* Change this section of the code, responsible for receiving data, by the get_curve function, yet to be implemented */

    /* Client requisition: get data block */
    for (uint32_t block_n = 0; block_n <= block_n_valid; block_n++) {
        if (zctx_interrupted) {
            err = BPM_CLIENT_INT;
            goto bpm_zctx_interrupted;
        }

        acq_trans->block.idx = block_n;
        err = bpm_acq_get_data_block(self, service, acq_trans);

        /* Check for return code */
        ASSERT_TEST(err == BPM_CLIENT_SUCCESS,
                "_bpm_get_data_block failed. block_n is probably out of range",
                err_bpm_get_data_block);

        total_bread += acq_trans->block.bytes_read;
        acq_trans->block.data = (uint32_t *)((uint8_t *)acq_trans->block.data + acq_trans->block.bytes_read);
        acq_trans->block.data_size -= acq_trans->block.bytes_read;

        /* Print some debug messages */
        DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
                "Total bytes read up to now: %u\n", total_bread);
        DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
                "Data pointer addr: %p\n", acq_trans->block.data);
        DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
                "Data buffer size: %u\n", acq_trans->block.data_size);
    }

    /* Return to client the total number of bytes read */
    acq_trans->block.bytes_read = total_bread;
    acq_trans->block.data_size = data_size;
    acq_trans->block.data = original_data_pt;

    DBE_DEBUG (DBG_LIB_CLIENT | DBG_LVL_TRACE, "[libclient] bpm_get_curve: "
            "Data curve of %u bytes was successfully acquired\n", total_bread);

bpm_zctx_interrupted:
err_bpm_get_data_block:
err_check_data_acquire:
    return err;
}
/**************** DSP SMIO Functions ****************/

/* Kx functions */
PARAM_FUNC_CLIENT_WRITE(kx)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_KX, kx);
}

PARAM_FUNC_CLIENT_READ(kx)
{
    return param_client_read (self, service, DSP_OPCODE_SET_GET_KX, kx);
}

/* Ky functions */
PARAM_FUNC_CLIENT_WRITE(ky)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_KY, ky);
}

PARAM_FUNC_CLIENT_READ(ky)
{
    return param_client_read (self, service, DSP_OPCODE_SET_GET_KY, ky);
}

/* Ksum functions */
PARAM_FUNC_CLIENT_WRITE(ksum)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_KSUM, ksum);
}

PARAM_FUNC_CLIENT_READ(ksum)
{
    return param_client_read (self, service, DSP_OPCODE_SET_GET_KSUM, ksum);
}

/* Delta/Sigma TBT threshold calculation functions */
PARAM_FUNC_CLIENT_WRITE(ds_tbt_thres)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_DS_TBT_THRES, ds_tbt_thres);
}

PARAM_FUNC_CLIENT_READ(ds_tbt_thres)
{
    return param_client_read (self, service, DSP_OPCODE_SET_GET_DS_TBT_THRES, ds_tbt_thres);
}

/* Delta/Sigma FOFB threshold calculation functions */
PARAM_FUNC_CLIENT_WRITE(ds_fofb_thres)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_DS_FOFB_THRES, ds_fofb_thres);
}

PARAM_FUNC_CLIENT_READ(ds_fofb_thres)
{
     return param_client_read (self, service, DSP_OPCODE_SET_GET_DS_FOFB_THRES, ds_fofb_thres);
}

/* Delta/Sigma MONIT. threshold calculation functions */
PARAM_FUNC_CLIENT_WRITE(ds_monit_thres)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_DS_MONIT_THRES, ds_monit_thres);
}

PARAM_FUNC_CLIENT_READ(ds_monit_thres)
{
     return param_client_read (self, service, DSP_OPCODE_SET_GET_DS_MONIT_THRES, ds_monit_thres);
}

/* Monitoring Amplitude channel 0 value */
PARAM_FUNC_CLIENT_WRITE(monit_amp_ch0)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_MONIT_AMP_CH0, monit_amp_ch0);
}

PARAM_FUNC_CLIENT_READ(monit_amp_ch0)
{
     return param_client_read (self, service, DSP_OPCODE_SET_GET_MONIT_AMP_CH0, monit_amp_ch0);
}

/* Monitoring Amplitude channel 1 value */
PARAM_FUNC_CLIENT_WRITE(monit_amp_ch1)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_MONIT_AMP_CH1, monit_amp_ch1);
}

PARAM_FUNC_CLIENT_READ(monit_amp_ch1)
{
     return param_client_read (self, service, DSP_OPCODE_SET_GET_MONIT_AMP_CH1, monit_amp_ch1);
}

/* Monitoring Amplitude channel 2 value */
PARAM_FUNC_CLIENT_WRITE(monit_amp_ch2)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_MONIT_AMP_CH2, monit_amp_ch2);
}

PARAM_FUNC_CLIENT_READ(monit_amp_ch2)
{
     return param_client_read (self, service, DSP_OPCODE_SET_GET_MONIT_AMP_CH2, monit_amp_ch2);
}

/* Monitoring Amplitude channel 3 value */
PARAM_FUNC_CLIENT_WRITE(monit_amp_ch3)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_MONIT_AMP_CH3, monit_amp_ch3);
}

PARAM_FUNC_CLIENT_READ(monit_amp_ch3)
{
     return param_client_read (self, service, DSP_OPCODE_SET_GET_MONIT_AMP_CH3, monit_amp_ch3);
}

/* Monitoring Position X value */
PARAM_FUNC_CLIENT_WRITE(monit_pos_x)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_MONIT_POS_X, monit_pos_x);
}

PARAM_FUNC_CLIENT_READ(monit_pos_x)
{
     return param_client_read (self, service, DSP_OPCODE_SET_GET_MONIT_POS_X, monit_pos_x);
}

/* Monitoring Position Y value */
PARAM_FUNC_CLIENT_WRITE(monit_pos_y)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_MONIT_POS_Y, monit_pos_y);
}

PARAM_FUNC_CLIENT_READ(monit_pos_y)
{
     return param_client_read (self, service, DSP_OPCODE_SET_GET_MONIT_POS_Y, monit_pos_y);
}

/* Monitoring Position Q value */
PARAM_FUNC_CLIENT_WRITE(monit_pos_q)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_MONIT_POS_Q, monit_pos_q);
}

PARAM_FUNC_CLIENT_READ(monit_pos_q)
{
     return param_client_read (self, service, DSP_OPCODE_SET_GET_MONIT_POS_Q, monit_pos_q);
}

/* Monitoring Position Sum value */
PARAM_FUNC_CLIENT_WRITE(monit_pos_sum)
{
    return param_client_write (self, service, DSP_OPCODE_SET_GET_MONIT_POS_SUM, monit_pos_sum);
}

PARAM_FUNC_CLIENT_READ(monit_pos_sum)
{
     return param_client_read (self, service, DSP_OPCODE_SET_GET_MONIT_POS_SUM, monit_pos_sum);
}

/**************** Swap SMIO Functions ****************/

/* Switching functions */
PARAM_FUNC_CLIENT_WRITE(sw)
{
    return param_client_write (self, service, SWAP_OPCODE_SET_GET_SW, sw);
}

PARAM_FUNC_CLIENT_READ(sw)
{
    return param_client_read (self, service, SWAP_OPCODE_SET_GET_SW, sw);
}

/* Switching enabling functions */
PARAM_FUNC_CLIENT_WRITE(sw_en)
{
    return param_client_write (self, service, SWAP_OPCODE_SET_GET_SW_EN, sw_en);
}

PARAM_FUNC_CLIENT_READ(sw_en)
{
    return param_client_read (self, service, SWAP_OPCODE_SET_GET_SW_EN, sw_en);
}

/* Switching Clock division functions */
PARAM_FUNC_CLIENT_WRITE(div_clk)
{
    return param_client_write (self, service, SWAP_OPCODE_SET_GET_DIV_CLK, div_clk);
}

PARAM_FUNC_CLIENT_READ(div_clk)
{
    return param_client_read (self, service, SWAP_OPCODE_SET_GET_DIV_CLK, div_clk);
}

/* Switching delay functions */
PARAM_FUNC_CLIENT_WRITE(sw_dly)
{
    return param_client_write (self, service, SWAP_OPCODE_SET_GET_SW_DLY, sw_dly);
}

PARAM_FUNC_CLIENT_READ(sw_dly)
{
    return param_client_read (self, service, SWAP_OPCODE_SET_GET_SW_DLY, sw_dly);
}

/* Windowing functions */
/* FIXME: Change client function wdw () to wdw_en (), which is a more semantic name */
PARAM_FUNC_CLIENT_WRITE(wdw)
{
    return param_client_write (self, service, SWAP_OPCODE_SET_GET_WDW_EN, wdw);
}

/* FIXME: Change client function wdw () to wdw_en (), which is a more semantic name */
PARAM_FUNC_CLIENT_READ(wdw)
{
    return param_client_read (self, service, SWAP_OPCODE_SET_GET_WDW_EN, wdw);
}

PARAM_FUNC_CLIENT_WRITE(wdw_dly)
{
    return param_client_write (self, service, SWAP_OPCODE_SET_GET_WDW_DLY, wdw_dly);
}

PARAM_FUNC_CLIENT_READ(wdw_dly)
{
    return param_client_read (self, service, SWAP_OPCODE_SET_GET_WDW_DLY, wdw_dly);
}

/* Gain functions */
/* TODO: reduce code repetition by, possibilly, group the OPCODES in
 * structure and merge all functions in a single
 * generic one for all channels (A, B, C, D) */
PARAM_FUNC_CLIENT_WRITE2(gain_a, dir_gain, inv_gain)
{
    return param_client_write (self, service, SWAP_OPCODE_SET_GET_GAIN_A,
            RW_SWAP_GAIN_UPPER_W(inv_gain) | RW_SWAP_GAIN_LOWER_W(dir_gain));
}

PARAM_FUNC_CLIENT_READ2(gain_a, dir_gain, inv_gain)
{
    uint32_t gain;
    bpm_client_err_e err = param_client_read (self, service,
            SWAP_OPCODE_SET_GET_GAIN_A, &gain);

    *dir_gain = RW_SWAP_GAIN_LOWER_R(gain);
    *inv_gain = RW_SWAP_GAIN_UPPER_R(gain);

    return err;
}

PARAM_FUNC_CLIENT_WRITE2(gain_b, dir_gain, inv_gain)
{
    return param_client_write (self, service, SWAP_OPCODE_SET_GET_GAIN_B,
            RW_SWAP_GAIN_UPPER_W(inv_gain) | RW_SWAP_GAIN_LOWER_W(dir_gain));
}

PARAM_FUNC_CLIENT_READ2(gain_b, dir_gain, inv_gain)
{
    uint32_t gain;
    bpm_client_err_e err = param_client_read (self, service,
            SWAP_OPCODE_SET_GET_GAIN_B, &gain);

    *dir_gain = RW_SWAP_GAIN_LOWER_R(gain);
    *inv_gain = RW_SWAP_GAIN_UPPER_R(gain);

    return err;
}

PARAM_FUNC_CLIENT_WRITE2(gain_c, dir_gain, inv_gain)
{
    return param_client_write (self, service, SWAP_OPCODE_SET_GET_GAIN_C,
            RW_SWAP_GAIN_UPPER_W(inv_gain) | RW_SWAP_GAIN_LOWER_W(dir_gain));
}

PARAM_FUNC_CLIENT_READ2(gain_c, dir_gain, inv_gain)
{
    uint32_t gain;
    bpm_client_err_e err = param_client_read (self, service,
            SWAP_OPCODE_SET_GET_GAIN_C, &gain);

    *dir_gain = RW_SWAP_GAIN_LOWER_R(gain);
    *inv_gain = RW_SWAP_GAIN_UPPER_R(gain);

    return err;
}

PARAM_FUNC_CLIENT_WRITE2(gain_d, dir_gain, inv_gain)
{
    return param_client_write (self, service, SWAP_OPCODE_SET_GET_GAIN_D,
            RW_SWAP_GAIN_UPPER_W(inv_gain) | RW_SWAP_GAIN_LOWER_W(dir_gain));
}

PARAM_FUNC_CLIENT_READ2(gain_d, dir_gain, inv_gain)
{
    uint32_t gain;
    bpm_client_err_e err = param_client_read (self, service,
            SWAP_OPCODE_SET_GET_GAIN_D, &gain);

    *dir_gain = RW_SWAP_GAIN_LOWER_R(gain);
    *inv_gain = RW_SWAP_GAIN_UPPER_R(gain);

    return err;
}

/**************** RFFE SMIO Functions ****************/

/* RFFE get/set switching state */
PARAM_FUNC_CLIENT_WRITE(rffe_sw)
{
    return param_client_write (self, service, RFFE_OPCODE_SET_GET_SW,
            rffe_sw);
}

PARAM_FUNC_CLIENT_READ(rffe_sw)
{
    return param_client_read (self, service, RFFE_OPCODE_SET_GET_SW,
            rffe_sw);
}

/* RFFE get/set attenuator */
PARAM_FUNC_CLIENT_WRITE_DOUBLE(rffe_att1)
{
    return param_client_write_double (self, service, RFFE_OPCODE_SET_GET_ATT1,
            rffe_att1);
}

PARAM_FUNC_CLIENT_READ_DOUBLE(rffe_att1)
{
    return param_client_read_double (self, service, RFFE_OPCODE_SET_GET_ATT1,
            rffe_att1);
}

PARAM_FUNC_CLIENT_WRITE_DOUBLE(rffe_att2)
{
    return param_client_write_double (self, service, RFFE_OPCODE_SET_GET_ATT2,
            rffe_att2);
}

PARAM_FUNC_CLIENT_READ_DOUBLE(rffe_att2)
{
    return param_client_read_double (self, service, RFFE_OPCODE_SET_GET_ATT2,
            rffe_att2);
}

/* RFFE get temperatures */
PARAM_FUNC_CLIENT_READ_DOUBLE(rffe_temp1)
{
    return param_client_read_double (self, service, RFFE_OPCODE_SET_GET_TEMP1,
            rffe_temp1);
}

PARAM_FUNC_CLIENT_READ_DOUBLE(rffe_temp2)
{
    return param_client_read_double (self, service, RFFE_OPCODE_SET_GET_TEMP2,
            rffe_temp2);
}

PARAM_FUNC_CLIENT_READ_DOUBLE(rffe_temp3)
{
    return param_client_read_double (self, service, RFFE_OPCODE_SET_GET_TEMP3,
            rffe_temp3);
}

PARAM_FUNC_CLIENT_READ_DOUBLE(rffe_temp4)
{
    return param_client_read_double (self, service, RFFE_OPCODE_SET_GET_TEMP4,
            rffe_temp4);
}

/* RFFE get/set set points */
PARAM_FUNC_CLIENT_WRITE_DOUBLE(rffe_set_point1)
{
    return param_client_write_double (self, service, RFFE_OPCODE_SET_GET_SET_POINT1,
            rffe_set_point1);
}

PARAM_FUNC_CLIENT_READ_DOUBLE(rffe_set_point1)
{
    return param_client_read_double (self, service, RFFE_OPCODE_SET_GET_SET_POINT1,
            rffe_set_point1);
}

PARAM_FUNC_CLIENT_WRITE_DOUBLE(rffe_set_point2)
{
    return param_client_write_double (self, service, RFFE_OPCODE_SET_GET_SET_POINT2,
            rffe_set_point2);
}

PARAM_FUNC_CLIENT_READ_DOUBLE(rffe_set_point2)
{
    return param_client_read_double (self, service, RFFE_OPCODE_SET_GET_SET_POINT2,
            rffe_set_point2);
}

/* RFFE get/set temperature control */
PARAM_FUNC_CLIENT_WRITE(rffe_temp_control)
{
    return param_client_write (self, service, RFFE_OPCODE_SET_GET_TEMP_CONTROL,
            rffe_temp_control);
}

PARAM_FUNC_CLIENT_READ(rffe_temp_control)
{
    return param_client_read (self, service, RFFE_OPCODE_SET_GET_TEMP_CONTROL,
            rffe_temp_control);
}

/* RFFE outputs */
PARAM_FUNC_CLIENT_WRITE_DOUBLE(rffe_output1)
{
    return param_client_write_double (self, service, RFFE_OPCODE_SET_GET_OUTPUT1,
            rffe_output1);
}

PARAM_FUNC_CLIENT_READ_DOUBLE(rffe_output1)
{
    return param_client_read_double (self, service, RFFE_OPCODE_SET_GET_OUTPUT1,
            rffe_output1);
}

PARAM_FUNC_CLIENT_WRITE_DOUBLE(rffe_output2)
{
    return param_client_write_double (self, service, RFFE_OPCODE_SET_GET_OUTPUT2,
            rffe_output2);
}

PARAM_FUNC_CLIENT_READ_DOUBLE(rffe_output2)
{
    return param_client_read_double (self, service, RFFE_OPCODE_SET_GET_OUTPUT2,
            rffe_output2);
}

/* RFFE get/set reset */
PARAM_FUNC_CLIENT_WRITE(rffe_reset)
{
    return param_client_write (self, service, RFFE_OPCODE_SET_GET_RESET,
            rffe_reset);
}

PARAM_FUNC_CLIENT_READ(rffe_reset)
{
    return param_client_read (self, service, RFFE_OPCODE_SET_GET_RESET,
            rffe_reset);
}

/* RFFE get/set reprogram */
PARAM_FUNC_CLIENT_WRITE(rffe_reprog)
{
    return param_client_write (self, service, RFFE_OPCODE_SET_GET_REPROG,
            rffe_reprog);
}

PARAM_FUNC_CLIENT_READ(rffe_reprog)
{
    return param_client_read (self, service, RFFE_OPCODE_SET_GET_REPROG,
            rffe_reprog);
}

/* RFFE set/get data */
bpm_client_err_e bpm_set_rffe_data (bpm_client_t *self, char *service,
        smio_rffe_data_block_t *rffe_data_block)
{
    uint32_t rw = WRITE_MODE;
    return param_client_write_gen (self, service, RFFE_OPCODE_SET_GET_DATA,
            rw, rffe_data_block, sizeof (*rffe_data_block));
}

bpm_client_err_e bpm_get_rffe_data (bpm_client_t *self, char *service,
        smio_rffe_data_block_t *rffe_data_block)
{
    uint32_t rw = READ_MODE;
    return param_client_read_gen (self, service, RFFE_OPCODE_SET_GET_DATA,
            rw, rffe_data_block, sizeof (*rffe_data_block));
}

/* RFFE get version */
bpm_client_err_e bpm_get_rffe_version (bpm_client_t *self, char *service,
        smio_rffe_version_t *rffe_version)
{
    uint32_t rw = READ_MODE;
    return param_client_read_gen (self, service, RFFE_OPCODE_SET_GET_VERSION,
            rw, rffe_version, sizeof (*rffe_version));
}

/* RFFE get/set switching level */
PARAM_FUNC_CLIENT_WRITE(rffe_sw_lvl)
{
    return param_client_write (self, service, RFFE_OPCODE_SET_GET_SW_LVL,
            rffe_sw_lvl);
}

PARAM_FUNC_CLIENT_READ(rffe_sw_lvl)
{
    return param_client_read (self, service, RFFE_OPCODE_SET_GET_SW_LVL,
            rffe_sw_lvl);
}

/**************** Helper Function ****************/

static bpm_client_err_e _func_polling (bpm_client_t *self, char *name, char *service, uint8_t *input, uint8_t *output, int timeout);

bpm_client_err_e func_polling (bpm_client_t *self, char *name, char *service, uint8_t *input, uint8_t *output, int timeout)
{
    return _func_polling (self, name, service, input, output, timeout);
}

/* Polling Function */

static bpm_client_err_e _func_polling (bpm_client_t *self, char *name, char *service, uint8_t *input, uint8_t *output, int timeout)
{
    if (timeout < 0) {
        timeout = INT_MAX;
    }

    bpm_client_err_e err = BPM_CLIENT_SUCCESS;
    time_t start = time(NULL);
    while ((time(NULL) - start)*1000 < timeout) {
        if (zctx_interrupted) {
            err = BPM_CLIENT_INT;
            goto bpm_zctx_interrupted;
        }
        const disp_op_t* func = bpm_func_translate(name);
        err = bpm_func_exec(self, func, service, input, output);
        if (err == BPM_CLIENT_SUCCESS)
        {
            goto exit;
        }
        usleep (1000);
    }

    /* timeout occured */
    err = BPM_CLIENT_ERR_TIMEOUT;

bpm_zctx_interrupted:
exit:
    return err;
}
