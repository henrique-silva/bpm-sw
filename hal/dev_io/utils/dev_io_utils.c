/*
 * Copyright (C) 2015  LNLS (www.lnls.br)
 * Author: Lucas Russo <lucas.russo@lnls.br>
 *
 * Released according to the GNU LGPL, version 3 or any later version.
 */

#include "dev_io_utils.h"
#include "ll_io_utils.h"
#include "hal_assert.h"

/* Undef ASSERT_ALLOC to avoid conflicting with other ASSERT_ALLOC */
#ifdef ASSERT_TEST
#undef ASSERT_TEST
#endif
#define ASSERT_TEST(test_boolean, err_str, err_goto_label, /* err_core */ ...) \
    ASSERT_HAL_TEST(test_boolean, DEV_IO, "[dev_io:utils]",         \
            err_str, err_goto_label, /* err_core */ __VA_ARGS__)

#ifdef ASSERT_ALLOC
#undef ASSERT_ALLOC
#endif
#define ASSERT_ALLOC(ptr, err_goto_label, /* err_core */ ...)       \
    ASSERT_HAL_ALLOC(ptr, DEV_IO, "[dev_io:utils]",                 \
            devio_err_str(DEVIO_ERR_ALLOC),                         \
            err_goto_label, /* err_core */ __VA_ARGS__)

#ifdef CHECK_ERR
#undef CHECK_ERR
#endif
#define CHECK_ERR(err, err_type)                                    \
    CHECK_HAL_ERR(err, DEV_IO, "[dev_io:utils]",                    \
            devio_err_str (err_type))

/************** Utility functions ****************/

static const llio_types_t devio_types_map [] = {
    {.name = BE_DEVIO_STR,          .type = BE_DEVIO},
    {.name = FE_DEVIO_STR,          .type = FE_DEVIO},
    {.name = INVALID_DEVIO_STR,     .type = INVALID_DEVIO},
    {.name = LLIO_TYPE_NAME_END,    .type = LLIO_TYPE_END}        /* End marker */
};

devio_type_e devio_str_to_type (const char *type_str)
{
    devio_type_e ret = llio_str_to_gen_type (type_str, devio_types_map);

    return (ret == LLIO_TYPE_END)? INVALID_DEVIO: ret;
}

char *devio_type_to_str (devio_type_e type)
{
    return llio_gen_type_to_str (type, devio_types_map);
}
