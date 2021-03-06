/*
 * Copyright (C) 2014 LNLS (www.lnls.br)
 * Author: Lucas Russo <lucas.russo@lnls.br>
 *
 * Released according to the GNU LGPL, version 3 or any later version.
 */

#include "sm_io_dsp_exports.h"
#include "sm_io_dsp_codes.h"

/* Description SMIO DSP functions */

disp_op_t dsp_set_get_kx_exp = {
    .name = DSP_NAME_SET_GET_KX,
    .opcode = DSP_OPCODE_SET_GET_KX,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_ky_exp = {
    .name = DSP_NAME_SET_GET_KY,
    .opcode = DSP_OPCODE_SET_GET_KY,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_ksum_exp = {
    .name = DSP_NAME_SET_GET_KSUM,
    .opcode = DSP_OPCODE_SET_GET_KSUM,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_ds_tbt_thres_exp = {
    .name = DSP_NAME_SET_GET_DS_TBT_THRES,
    .opcode = DSP_OPCODE_SET_GET_DS_TBT_THRES,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_ds_fofb_thres_exp = {
    .name = DSP_NAME_SET_GET_DS_FOFB_THRES,
    .opcode = DSP_OPCODE_SET_GET_DS_FOFB_THRES,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_ds_monit_thres_exp = {
    .name = DSP_NAME_SET_GET_DS_MONIT_THRES,
    .opcode = DSP_OPCODE_SET_GET_DS_MONIT_THRES,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_monit_amp_ch0_exp = {
    .name = DSP_NAME_SET_GET_MONIT_AMP_CH0,
    .opcode = DSP_OPCODE_SET_GET_MONIT_AMP_CH0,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_monit_amp_ch1_exp = {
    .name = DSP_NAME_SET_GET_MONIT_AMP_CH1,
    .opcode = DSP_OPCODE_SET_GET_MONIT_AMP_CH1,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_monit_amp_ch2_exp = {
    .name = DSP_NAME_SET_GET_MONIT_AMP_CH2,
    .opcode = DSP_OPCODE_SET_GET_MONIT_AMP_CH2,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_monit_amp_ch3_exp = {
    .name = DSP_NAME_SET_GET_MONIT_AMP_CH3,
    .opcode = DSP_OPCODE_SET_GET_MONIT_AMP_CH3,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_monit_pos_x_exp = {
    .name = DSP_NAME_SET_GET_MONIT_POS_X,
    .opcode = DSP_OPCODE_SET_GET_MONIT_POS_X,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_monit_pos_y_exp = {
    .name = DSP_NAME_SET_GET_MONIT_POS_Y,
    .opcode = DSP_OPCODE_SET_GET_MONIT_POS_Y,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_monit_pos_q_exp = {
    .name = DSP_NAME_SET_GET_MONIT_POS_Q,
    .opcode = DSP_OPCODE_SET_GET_MONIT_POS_Q,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

disp_op_t dsp_set_get_monit_pos_sum_exp = {
    .name = DSP_NAME_SET_GET_MONIT_POS_SUM,
    .opcode = DSP_OPCODE_SET_GET_MONIT_POS_SUM,
    .retval = DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
    .retval_owner = DISP_OWNER_OTHER,
    .args = {
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_ENCODE(DISP_ATYPE_UINT32, uint32_t),
        DISP_ARG_END
    }
};

/* Exported function description */
const disp_op_t *dsp_exp_ops [] = {
    &dsp_set_get_kx_exp,
    &dsp_set_get_ky_exp,
    &dsp_set_get_ksum_exp,
    &dsp_set_get_ds_tbt_thres_exp,
    &dsp_set_get_ds_fofb_thres_exp,
    &dsp_set_get_ds_monit_thres_exp,
    &dsp_set_get_monit_amp_ch0_exp,
    &dsp_set_get_monit_amp_ch1_exp,
    &dsp_set_get_monit_amp_ch2_exp,
    &dsp_set_get_monit_amp_ch3_exp,
    &dsp_set_get_monit_pos_x_exp,
    &dsp_set_get_monit_pos_y_exp,
    &dsp_set_get_monit_pos_q_exp,
    &dsp_set_get_monit_pos_sum_exp,
    NULL
};

