/**
 * @file    Mcal_Adc_Dio.c
 * @author  Yigit Cakirdogan
 * @brief   AUTOSAR MCAL & IoHwAb implementation with ratiometric dual-slope scaling.
 */

#include "Mcal_Adc_Dio.h"

static uint16  s_sim_apps1_mv     = APPS1_ZERO_PEDAL_MV;
static uint16  s_sim_apps2_mv     = APPS2_ZERO_PEDAL_MV;
static boolean s_sim_brake_active = FALSE;
static uint8   s_sim_mil_lamp     = STD_LOW;

void Mcal_Adc_Dio_Init(void)
{
    s_sim_apps1_mv     = APPS1_ZERO_PEDAL_MV;
    s_sim_apps2_mv     = APPS2_ZERO_PEDAL_MV;
    s_sim_brake_active = FALSE;
    s_sim_mil_lamp     = STD_LOW;
}

void Mcal_Sim_SetRawInputs(uint16 apps1_mv, uint16 apps2_mv, boolean brake_active)
{
    s_sim_apps1_mv     = apps1_mv;
    s_sim_apps2_mv     = apps2_mv;
    s_sim_brake_active = brake_active;
}

Std_ReturnType Adc_ReadGroup(uint8 group, uint16 *out_mv_pair)
{
    if ((group != ADC_GROUP_PEDAL_SENSORS) || (out_mv_pair == NULL_PTR)) {
        return E_NOT_OK;
    }

    out_mv_pair[0] = s_sim_apps1_mv;
    out_mv_pair[1] = s_sim_apps2_mv;
    return E_OK;
}

uint8 Dio_ReadChannel(uint8 channel_id)
{
    if (channel_id == DIO_CH_BRAKE_SWITCH) {
        return (s_sim_brake_active != FALSE) ? STD_HIGH : STD_LOW;
    }
    return STD_LOW;
}

void Dio_WriteChannel(uint8 channel_id, uint8 level)
{
    if (channel_id == DIO_CH_SAFE_STATE_MIL_LAMP) {
        s_sim_mil_lamp = level;
    }
}

uint8 Mcal_Sim_GetMilLampState(void)
{
    return s_sim_mil_lamp;
}

static uint16 scale_mv_to_permille(uint16 mv, uint16 zero_mv, uint16 full_mv)
{
    if (mv <= zero_mv) {
        return 0U;
    }
    if (mv >= full_mv) {
        return 1000U;
    }
    uint32 span  = (uint32)(full_mv - zero_mv);
    uint32 delta = (uint32)(mv - zero_mv);
    return (uint16)((delta * 1000UL) / span);
}

Std_ReturnType IoHwAb_ReadPedalSensors(IoHwAb_PedalSensorData_t *out_data)
{
    uint16 raw_mv[2] = { 0U, 0U };

    if (out_data == NULL_PTR) {
        return E_NOT_OK;
    }

    if (Adc_ReadGroup(ADC_GROUP_PEDAL_SENSORS, raw_mv) != E_OK) {
        return E_NOT_OK;
    }

    /* Electrical Out-of-Range Diagnostics (Short-to-Ground / Short-to-Battery) */
    out_data->apps1_range_fault =
        ((raw_mv[0] < APPS1_MIN_VALID_MV) || (raw_mv[0] > APPS1_MAX_VALID_MV)) ? TRUE : FALSE;

    out_data->apps2_range_fault =
        ((raw_mv[1] < APPS2_MIN_VALID_MV) || (raw_mv[1] > APPS2_MAX_VALID_MV)) ? TRUE : FALSE;

    out_data->apps1_permille =
        scale_mv_to_permille(raw_mv[0], APPS1_ZERO_PEDAL_MV, APPS1_FULL_PEDAL_MV);

    out_data->apps2_permille =
        scale_mv_to_permille(raw_mv[1], APPS2_ZERO_PEDAL_MV, APPS2_FULL_PEDAL_MV);

    out_data->brake_pressed =
        (Dio_ReadChannel(DIO_CH_BRAKE_SWITCH) == STD_HIGH) ? TRUE : FALSE;

    return E_OK;
}
