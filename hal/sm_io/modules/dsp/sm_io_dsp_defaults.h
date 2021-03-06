/*
 * Copyright (C) 2014 LNLS (www.lnls.br)
 * Author: Lucas Russo <lucas.russo@lnls.br>
 *
 * Released according to the GNU LGPL, version 3 or any later version.
 */

#ifndef _DSP_DEFAULTS_H_
#define _DSP_DEFAULTS_H_

/********************** K Default Values ***************************/
#define DSP_DFLT_KX_VAL                 1000000    /* nm */
#define DSP_DFLT_KY_VAL                 1000000    /* nm */
#define DSP_DFLT_KSUM_VAL               0x0FFFFFF   /* 1.0 in FIX25_24 notation */


/************ Delta Sigma Minimum Calculation Threshold ***********/
#define DSP_DFLT_DS_TBT_THRES           0           /* No minimum threslhold */
#define DSP_DFLT_DS_FOFB_THRES          0           /* No minimum threslhold */
#define DSP_DFLT_DS_MONIT_THRES         0           /* No minimum threslhold */

smio_err_e dsp_config_defaults (char *broker_endp, char *service,
        const char *log_file_name);

#endif

