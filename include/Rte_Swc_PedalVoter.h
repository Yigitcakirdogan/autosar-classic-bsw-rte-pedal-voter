/**
 * @file    Rte_Swc_PedalVoter.h
 * @author  Yigit Cakirdogan
 * @brief   AUTOSAR RTE contract and ASIL-D 1oo2D Pedal Plausibility Voter SWC interface.
 */

#ifndef RTE_SWC_PEDALVOTER_H
#define RTE_SWC_PEDALVOTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"
#include "Mcal_Adc_Dio.h"
#include "Bsw_Dem_WdgM_E2E.h"

/* ISO 26262 Safety Thresholds */
#define APPS_MAX_PLAUSIBILITY_DIFF_PERMILLE  ((uint16)50U)  /**< 5.0% max channel divergence */
#define BOS_PEDAL_OVERRIDE_THRESHOLD         ((uint16)150U) /**< 15.0% brake override limit  */
#define MAX_PROPULSION_TORQUE_NM             ((sint16)300)  /**< 300 Nm @ 1000 ‰ pedal       */

typedef enum {
    VCU_STATE_NORMAL_OPERATION = 0U,
    VCU_STATE_BRAKE_OVERRIDE   = 1U,
    VCU_STATE_FAULT_SAFE_STATE = 2U
} Vcu_SafetyState_t;

typedef struct {
    sint16            requested_torque_nm;
    uint16            voted_pedal_permille;
    Vcu_SafetyState_t safety_state;
    uint8             tx_can_pdu[8];
} Rte_TorqueOutput_t;

void                   Swc_PedalVoter_Init(void);
void                   Runnable_PedalVoter_10ms(void);
const Rte_TorqueOutput_t *Rte_GetLatestTorqueOutput(void);

#ifdef __cplusplus
}
#endif

#endif /* RTE_SWC_PEDALVOTER_H */
