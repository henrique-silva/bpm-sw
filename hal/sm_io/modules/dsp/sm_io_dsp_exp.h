/*
 * Copyright (C) 2014 LNLS (www.lnls.br)
 * Author: Lucas Russo <lucas.russo@lnls.br>
 *
 * Released according to the GNU LGPL, version 3 or any later version.
 */

#ifndef _DSP_H_
#define _DSP_H_

#include "sm_io_bootstrap.h"
#include "smio_thsafe_zmq_client.h"
#include "exp_ops_codes.h"
#include "sm_io_dsp_core.h"

/* Known modules IDs (from SDB records defined in FPGA) */
#define DSP_SDB_DEVID       0x1bafbf1e
#define DSP_SDB_NAME        "DSP"

extern const smio_bootstrap_ops_t dsp_bootstrap_ops;

#endif

