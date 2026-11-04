/**
 * @file    Swc_PedalVoter.c
 * @author  Yigit Cakirdogan
 * @brief   ISO 26262 ASIL-D Application Software Component (SWC) implementing
 *          1oo2D Dual-Sensor Plausibility Voting, Brake Override (BOS), and Safe State.
 */

#include "Rte_Swc_PedalVoter.h"
#include <string.h>

static Rte_TorqueOutput_t s_rte_out;
static E2E_P01State_t     s_e2e_tx_state;

void Swc_PedalVoter_Init(void)
{
    (void)memset(&s_rte_out, 0, sizeof(s_rte_out));
    s_rte_out.safety_state = VCU_STATE_NORMAL_OPERATION;
    E2E_P01_Init(&s_e2e_tx_state, E2E_P01_DATA_ID_TORQUE_REQ);
}

const Rte_TorqueOutput_t *Rte_GetLatestTorqueOutput(void)
{
    return &s_rte_out;
}

void Runnable_PedalVoter_10ms(void)
{
    IoHwAb_PedalSensorData_t sensors;

    /* 1. Checkpoint 0: Input Acquisition */
    (void)WdgM_CheckpointReached(WDGM_SE_PEDAL_VOTER, WDGM_CP_0_INPUT_ACQUIRE);

    if (IoHwAb_ReadPedalSensors(&sensors) != E_OK) {
        (void)Dem_SetEventStatus(DEM_EVENT_ID_APPS_RANGE_FAULT, DEM_EVENT_STATUS_FAILED);
    } else if ((sensors.apps1_range_fault != FALSE) || (sensors.apps2_range_fault != FALSE)) {
        /* Immediate electrical out-of-range fault (Short-to-GND or Short-to-Battery) */
        (void)Dem_SetEventStatus(DEM_EVENT_ID_APPS_RANGE_FAULT, DEM_EVENT_STATUS_FAILED);
    } else {
        (void)Dem_SetEventStatus(DEM_EVENT_ID_APPS_RANGE_FAULT, DEM_EVENT_STATUS_PASSED);
    }

    /* 2. Checkpoint 1: ASIL-D 1oo2D Plausibility Voting & Debouncing */
    (void)WdgM_CheckpointReached(WDGM_SE_PEDAL_VOTER, WDGM_CP_1_SAFETY_VOTE);

    uint16 diff = (sensors.apps1_permille >= sensors.apps2_permille)
                ? (uint16)(sensors.apps1_permille - sensors.apps2_permille)
                : (uint16)(sensors.apps2_permille - sensors.apps1_permille);

    if (diff > APPS_MAX_PLAUSIBILITY_DIFF_PERMILLE) {
        (void)Dem_SetEventStatus(DEM_EVENT_ID_APPS_IMPLAUSIBILITY, DEM_EVENT_STATUS_PREFAILED);
    } else {
        (void)Dem_SetEventStatus(DEM_EVENT_ID_APPS_IMPLAUSIBILITY, DEM_EVENT_STATUS_PREPASSED);
    }

    boolean fault_active =
        ((Dem_IsEventConfirmedFailed(DEM_EVENT_ID_APPS_RANGE_FAULT) != FALSE) ||
         (Dem_IsEventConfirmedFailed(DEM_EVENT_ID_APPS_IMPLAUSIBILITY) != FALSE))
        ? TRUE : FALSE;

    if (fault_active != FALSE) {
        /* ISO 26262 Safe State: Safe Torque Off (0 Nm) + MIL Lamp HIGH */
        s_rte_out.safety_state         = VCU_STATE_FAULT_SAFE_STATE;
        s_rte_out.voted_pedal_permille = 0U;
        s_rte_out.requested_torque_nm  = 0;
        Dio_WriteChannel(DIO_CH_SAFE_STATE_MIL_LAMP, STD_HIGH);
    } else {
        /* Conservative 1oo2D Voter: select the minimum of the two validated channels */
        uint16 min_pedal = (sensors.apps1_permille < sensors.apps2_permille)
                         ? sensors.apps1_permille
                         : sensors.apps2_permille;

        /* Brake-Pedal Override System (BOS) Check */
        if ((sensors.brake_pressed != FALSE) && (min_pedal > BOS_PEDAL_OVERRIDE_THRESHOLD)) {
            s_rte_out.safety_state         = VCU_STATE_BRAKE_OVERRIDE;
            s_rte_out.voted_pedal_permille = 0U;
            s_rte_out.requested_torque_nm  = 0;
        } else {
            s_rte_out.safety_state         = VCU_STATE_NORMAL_OPERATION;
            s_rte_out.voted_pedal_permille = min_pedal;
            s_rte_out.requested_torque_nm  =
                (sint16)(((sint32)min_pedal * (sint32)MAX_PROPULSION_TORQUE_NM) / 1000L);
        }
        Dio_WriteChannel(DIO_CH_SAFE_STATE_MIL_LAMP, STD_LOW);
    }

    /* 3. Checkpoint 2: Pack Torque Request PDU & Apply AUTOSAR E2E Profile 1 Protection */
    (void)WdgM_CheckpointReached(WDGM_SE_PEDAL_VOTER, WDGM_CP_2_E2E_TX);

    uint16 raw_torque = (uint16)s_rte_out.requested_torque_nm;
    s_rte_out.tx_can_pdu[1] = (uint8)(((uint8)s_rte_out.safety_state << 4U) & 0xF0U);
    s_rte_out.tx_can_pdu[2] = (uint8)(raw_torque & 0xFFU);
    s_rte_out.tx_can_pdu[3] = (uint8)((raw_torque >> 8U) & 0xFFU);
    s_rte_out.tx_can_pdu[4] = (uint8)(s_rte_out.voted_pedal_permille & 0xFFU);
    s_rte_out.tx_can_pdu[5] = (uint8)((s_rte_out.voted_pedal_permille >> 8U) & 0xFFU);
    s_rte_out.tx_can_pdu[6] = (fault_active != FALSE) ? 0xAAU : 0x55U;
    s_rte_out.tx_can_pdu[7] = 0x00U;

    (void)E2E_P01Protect(&s_e2e_tx_state, s_rte_out.tx_can_pdu);
}
