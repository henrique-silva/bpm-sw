/*
 * Copyright (C) 2014 LNLS (www.lnls.br)
 * Author: Lucas Russo <lucas.russo@lnls.br>
 *
 * Released according to the GNU LGPL, version 3 or any later version.
 */

#ifndef _SM_IO_FMC130M_4CH_CODES_H_
#define _SM_IO_FMC130M_4CH_CODES_H_

#include <inttypes.h>

/* Messaging OPCODES */
#define FMC130M_4CH_OPCODE_SIZE                         (sizeof(uint32_t))
#define FMC130M_4CH_OPCODE_TYPE                         uint32_t

#define FMC130M_4CH_OPCODE_LEDS                         0
#define FMC130M_4CH_NAME_LEDS                           "fmc130m_4ch_leds"
#define FMC130M_4CH_OPCODE_PLL_FUNCTION                 1
#define FMC130M_4CH_NAME_PLL_FUNCTION                   "fmc130m_4ch_pll_function"
#define FMC130M_4CH_OPCODE_AD9510_CFG_DEFAULTS          2
#define FMC130M_4CH_NAME_AD9510_CFG_DEFAULTS            "fmc130m_4ch_ad9510_cfg_defaults"
#define FMC130M_4CH_OPCODE_ADC_DATA0                    3
#define FMC130M_4CH_NAME_ADC_DATA0                      "fmc130m_4ch_adc_data0"
#define FMC130M_4CH_OPCODE_ADC_DATA1                    4
#define FMC130M_4CH_NAME_ADC_DATA1                      "fmc130m_4ch_adc_data1"
#define FMC130M_4CH_OPCODE_ADC_DATA2                    5
#define FMC130M_4CH_NAME_ADC_DATA2                      "fmc130m_4ch_adc_data2"
#define FMC130M_4CH_OPCODE_ADC_DATA3                    6
#define FMC130M_4CH_NAME_ADC_DATA3                      "fmc130m_4ch_adc_data3"
#define FMC130M_4CH_OPCODE_ADC_DLY_VAL0                 7
#define FMC130M_4CH_NAME_ADC_DLY_VAL0                   "fmc130m_4ch_adc_dly_val0"
#define FMC130M_4CH_OPCODE_ADC_DLY_VAL1                 8
#define FMC130M_4CH_NAME_ADC_DLY_VAL1                   "fmc130m_4ch_adc_dly_val1"
#define FMC130M_4CH_OPCODE_ADC_DLY_VAL2                 9
#define FMC130M_4CH_NAME_ADC_DLY_VAL2                   "fmc130m_4ch_adc_dly_val2"
#define FMC130M_4CH_OPCODE_ADC_DLY_VAL3                 10
#define FMC130M_4CH_NAME_ADC_DLY_VAL3                   "fmc130m_4ch_adc_dly_val3"
#define FMC130M_4CH_OPCODE_ADC_DLY_LINE0                11
#define FMC130M_4CH_NAME_ADC_DLY_LINE0                  "fmc130m_4ch_adc_dly_line0"
#define FMC130M_4CH_OPCODE_ADC_DLY_LINE1                12
#define FMC130M_4CH_NAME_ADC_DLY_LINE1                  "fmc130m_4ch_adc_dly_line1"
#define FMC130M_4CH_OPCODE_ADC_DLY_LINE2                13
#define FMC130M_4CH_NAME_ADC_DLY_LINE2                  "fmc130m_4ch_adc_dly_line2"
#define FMC130M_4CH_OPCODE_ADC_DLY_LINE3                14
#define FMC130M_4CH_NAME_ADC_DLY_LINE3                  "fmc130m_4ch_adc_dly_line3"
#define FMC130M_4CH_OPCODE_ADC_DLY_UPDT0                15
#define FMC130M_4CH_NAME_ADC_DLY_UPDT0                  "fmc130m_4ch_adc_dly_updt0"
#define FMC130M_4CH_OPCODE_ADC_DLY_UPDT1                16
#define FMC130M_4CH_NAME_ADC_DLY_UPDT1                  "fmc130m_4ch_adc_dly_updt1"
#define FMC130M_4CH_OPCODE_ADC_DLY_UPDT2                17
#define FMC130M_4CH_NAME_ADC_DLY_UPDT2                  "fmc130m_4ch_adc_dly_updt2"
#define FMC130M_4CH_OPCODE_ADC_DLY_UPDT3                18
#define FMC130M_4CH_NAME_ADC_DLY_UPDT3                  "fmc130m_4ch_adc_dly_updt3"
#define FMC130M_4CH_OPCODE_ADC_DLY0                     19
#define FMC130M_4CH_NAME_ADC_DLY0                       "fmc130m_4ch_adc_dly0"
#define FMC130M_4CH_OPCODE_ADC_DLY1                     20
#define FMC130M_4CH_NAME_ADC_DLY1                       "fmc130m_4ch_adc_dly1"
#define FMC130M_4CH_OPCODE_ADC_DLY2                     21
#define FMC130M_4CH_NAME_ADC_DLY2                       "fmc130m_4ch_adc_dly2"
#define FMC130M_4CH_OPCODE_ADC_DLY3                     22
#define FMC130M_4CH_NAME_ADC_DLY3                       "fmc130m_4ch_adc_dly3"
#define FMC130M_4CH_OPCODE_TEST_DATA_EN                 23
#define FMC130M_4CH_NAME_TEST_DATA_EN                   "fmc130m_4ch_adc_test_data_en"
#define FMC130M_4CH_OPCODE_SI571_OE                     24
#define FMC130M_4CH_NAME_SI571_OE                       "fmc130m_4ch_si571_oe"
#define FMC130M_4CH_OPCODE_TRIG_DIR                     25
#define FMC130M_4CH_NAME_TRIG_DIR                       "fmc130m_4ch_trig_dir"
#define FMC130M_4CH_OPCODE_TRIG_TERM                    26
#define FMC130M_4CH_NAME_TRIG_TERM                      "fmc130m_4ch_trig_term"
#define FMC130M_4CH_OPCODE_TRIG_VAL                     27
#define FMC130M_4CH_NAME_TRIG_VAL                       "fmc130m_4ch_trig_val"
#define FMC130M_4CH_OPCODE_AD9510_PLL_A_DIV             28
#define FMC130M_4CH_NAME_AD9510_PLL_A_DIV               "fmc130m_4ch_ad9510_pll_a_div"
#define FMC130M_4CH_OPCODE_AD9510_PLL_B_DIV             29
#define FMC130M_4CH_NAME_AD9510_PLL_B_DIV               "fmc130m_4ch_ad9510_pll_b_div"
#define FMC130M_4CH_OPCODE_AD9510_PLL_PRESCALER         30
#define FMC130M_4CH_NAME_AD9510_PLL_PRESCALER           "fmc130m_4ch_ad9510_pll_prescaler"
#define FMC130M_4CH_OPCODE_AD9510_R_DIV                 31
#define FMC130M_4CH_NAME_AD9510_R_DIV                   "fmc130m_4ch_ad9510_r_div"
#define FMC130M_4CH_OPCODE_AD9510_PLL_PDOWN             32
#define FMC130M_4CH_NAME_AD9510_PLL_PDOWN               "fmc130m_4ch_ad9510_pll_pdown"
#define FMC130M_4CH_OPCODE_AD9510_MUX_STATUS            33
#define FMC130M_4CH_NAME_AD9510_MUX_STATUS              "fmc130m_4ch_ad9510_mux_status"
#define FMC130M_4CH_OPCODE_AD9510_CP_CURRENT            34
#define FMC130M_4CH_NAME_AD9510_CP_CURRENT              "fmc130m_4ch_ad9510_cp_current"
#define FMC130M_4CH_OPCODE_AD9510_OUTPUTS               35
#define FMC130M_4CH_NAME_AD9510_OUTPUTS                 "fmc130m_4ch_ad9510_outputs"
#define FMC130M_4CH_OPCODE_AD9510_PLL_CLK_SEL           36
#define FMC130M_4CH_NAME_AD9510_PLL_CLK_SEL             "fmc130m_4ch_ad9510_pll_clk_sel"
#define FMC130M_4CH_OPCODE_SI571_SET_FREQ               37
#define FMC130M_4CH_NAME_SI571_SET_FREQ                 "fmc130m_4ch_si571_set_freq"
#define FMC130M_4CH_OPCODE_SI571_GET_DEFAULTS           38
#define FMC130M_4CH_NAME_SI571_GET_DEFAULTS             "fmc130m_4ch_si571_get_defaults"
#define FMC130M_4CH_OPCODE_END                          39

/* Messaging Reply OPCODES */
#define FMC130M_4CH_REPLY_SIZE                          (sizeof(uint32_t))
#define FMC130M_4CH_REPLY_TYPE                          uint32_t

#define FMC130M_4CH_OK                                  0   /* Operation was successful */
#define FMC130M_4CH_ERR                                 1   /* Could not set/get value */
#define FMC130M_4CH_UNINPL                              2   /* Unimplemented function or operation */
#define FMC130M_4CH_REPLY_END                           3   /* End marker */

#endif
