/**
 * @file    Mcal_Adc_Dio.h
 * @author  Yigit Cakirdogan
 * @brief   AUTOSAR MCAL (Adc, Dio) and ECUAL (IoHwAb) interfaces for
 *          dual-channel redundant accelerator pedal & brake switch acquisition.
 */

#ifndef MCAL_ADC_DIO_H
#define MCAL_ADC_DIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"

/* MCAL ADC Group & Channel Identifiers */
#define ADC_GROUP_PEDAL_SENSORS      ((uint8)0U)
#define DIO_CH_BRAKE_SWITCH          ((uint8)0U)
#define DIO_CH_SAFE_STATE_MIL_LAMP   ((uint8)1U)

/* Dual-Slope Electrical Limits (in millivolts) */
#define APPS1_MIN_VALID_MV           ((uint16)400U)
#define APPS1_ZERO_PEDAL_MV          ((uint16)500U)
#define APPS1_FULL_PEDAL_MV          ((uint16)4500U)
#define APPS1_MAX_VALID_MV           ((uint16)4600U)

#define APPS2_MIN_VALID_MV           ((uint16)200U)
#define APPS2_ZERO_PEDAL_MV          ((uint16)250U)
#define APPS2_FULL_PEDAL_MV          ((uint16)2250U)
#define APPS2_MAX_VALID_MV           ((uint16)2350U)

typedef struct {
    uint16  apps1_permille;       /**< Normalized APPS1 position [0..1000 ‰] */
    uint16  apps2_permille;       /**< Normalized APPS2 position [0..1000 ‰] */
    boolean apps1_range_fault;    /**< TRUE if Short-to-GND or Short-to-Batt */
    boolean apps2_range_fault;    /**< TRUE if Short-to-GND or Short-to-Batt */
    boolean brake_pressed;        /**< TRUE if driver brake switch is active */
} IoHwAb_PedalSensorData_t;

void           Mcal_Adc_Dio_Init(void);
void           Mcal_Sim_SetRawInputs(uint16 apps1_mv, uint16 apps2_mv, boolean brake_active);
Std_ReturnType Adc_ReadGroup(uint8 group, uint16 *out_mv_pair);
uint8          Dio_ReadChannel(uint8 channel_id);
void           Dio_WriteChannel(uint8 channel_id, uint8 level);
uint8          Mcal_Sim_GetMilLampState(void);

/**
 * @brief ECUAL I/O Hardware Abstraction: Convert dual-slope ratiometric mV to 0..1000 ‰.
 */
Std_ReturnType IoHwAb_ReadPedalSensors(IoHwAb_PedalSensorData_t *out_data);

#ifdef __cplusplus
}
#endif

#endif /* MCAL_ADC_DIO_H */
