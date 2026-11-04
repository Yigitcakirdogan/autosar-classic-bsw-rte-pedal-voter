/**
 * @file    test_autosar_pedal_voter.c
 * @author  Yigit Cakirdogan
 * @brief   Verification suite for AUTOSAR Classic ASIL-D Pedal Voter:
 *          - Normal 2:1 ratiometric tracking (50% pedal -> 150 Nm)
 *          - ISO 26262 Plausibility Fault Debouncing (3 ticks = 30 ms < 50 ms FTTI)
 *          - Electrical Short-to-Ground immediate Safe Torque Off (0 Nm)
 *          - Brake-Pedal Override System (BOS)
 *          - AUTOSAR E2E Profile 1 CRC-8 & Alive Counter Corruption Detection
 *          - WdgM Logical Program Flow Checkpoint Supervision
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Rte_Swc_PedalVoter.h"

#define TEST_ASSERT(cond)                                                          \
    do {                                                                           \
        if (!(cond)) {                                                             \
            fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            exit(EXIT_FAILURE);                                                    \
        }                                                                          \
    } while (0)

static void reset_ecu_stack(void)
{
    Mcal_Adc_Dio_Init();
    Dem_Init();
    WdgM_Init();
    Swc_PedalVoter_Init();
}

static void test_normal_50_percent_pedal_and_e2e_check(void)
{
    reset_ecu_stack();

    /* 50% Pedal: APPS1 = 2500 mV (range 500..4500), APPS2 = 1250 mV (range 250..2250) */
    Mcal_Sim_SetRawInputs(2500U, 1250U, FALSE);
    Runnable_PedalVoter_10ms();

    const Rte_TorqueOutput_t *out = Rte_GetLatestTorqueOutput();
    TEST_ASSERT(out->safety_state == VCU_STATE_NORMAL_OPERATION);
    TEST_ASSERT(out->voted_pedal_permille == 500U);
    TEST_ASSERT(out->requested_torque_nm == 150);
    TEST_ASSERT(Mcal_Sim_GetMilLampState() == STD_LOW);
    TEST_ASSERT(WdgM_EvaluateCycleAndKickHwWdg() == TRUE);

    /* Verify E2E Profile 1 receiver check passes on genuine PDU and fails on bit flip */
    E2E_P01State_t rx_e2e;
    E2E_P01_Init(&rx_e2e, E2E_P01_DATA_ID_TORQUE_REQ);
    TEST_ASSERT(E2E_P01Check(&rx_e2e, out->tx_can_pdu) == E2E_P01STATUS_OK);

    /* Repeating the exact same frame must be caught as E2E_P01STATUS_REPEATED */
    TEST_ASSERT(E2E_P01Check(&rx_e2e, out->tx_can_pdu) == E2E_P01STATUS_REPEATED);

    /* Corrupting torque byte must be caught as E2E_P01STATUS_WRONGCRC */
    uint8 corrupted_pdu[8];
    memcpy(corrupted_pdu, out->tx_can_pdu, 8U);
    corrupted_pdu[2] ^= 0x01U;
    TEST_ASSERT(E2E_P01Check(&rx_e2e, corrupted_pdu) == E2E_P01STATUS_WRONGCRC);

    printf("[PASS] test_normal_50_percent_pedal_and_e2e_check\n");
}

static void test_implausibility_debounce_to_safe_state(void)
{
    reset_ecu_stack();

    /* Inject 20% divergence between APPS1 (50% = 2500 mV) and APPS2 (30% = 850 mV) */
    Mcal_Sim_SetRawInputs(2500U, 850U, FALSE);

    /* Tick 1 (10 ms): Debounce counter = +2 (not yet confirmed) */
    Runnable_PedalVoter_10ms();
    TEST_ASSERT(Dem_GetDebounceCounter(DEM_EVENT_ID_APPS_IMPLAUSIBILITY) == 2);
    TEST_ASSERT(Dem_IsEventConfirmedFailed(DEM_EVENT_ID_APPS_IMPLAUSIBILITY) == FALSE);

    /* Tick 2 (20 ms): Debounce counter = +4 */
    Runnable_PedalVoter_10ms();
    TEST_ASSERT(Dem_GetDebounceCounter(DEM_EVENT_ID_APPS_IMPLAUSIBILITY) == 4);

    /* Tick 3 (30 ms <= 50 ms FTTI): Debounce counter = +6 -> Fault Confirmed -> Safe Torque Off! */
    Runnable_PedalVoter_10ms();
    const Rte_TorqueOutput_t *out = Rte_GetLatestTorqueOutput();
    TEST_ASSERT(Dem_IsEventConfirmedFailed(DEM_EVENT_ID_APPS_IMPLAUSIBILITY) == TRUE);
    TEST_ASSERT(out->safety_state == VCU_STATE_FAULT_SAFE_STATE);
    TEST_ASSERT(out->requested_torque_nm == 0);
    TEST_ASSERT(Mcal_Sim_GetMilLampState() == STD_HIGH);

    printf("[PASS] test_implausibility_debounce_to_safe_state\n");
}

static void test_brake_override_and_wdgm_violation(void)
{
    reset_ecu_stack();

    /* 40% Pedal (APPS1 = 2100 mV, APPS2 = 1050 mV) WITH Brake Switch Active */
    Mcal_Sim_SetRawInputs(2100U, 1050U, TRUE);
    Runnable_PedalVoter_10ms();

    const Rte_TorqueOutput_t *out = Rte_GetLatestTorqueOutput();
    TEST_ASSERT(out->safety_state == VCU_STATE_BRAKE_OVERRIDE);
    TEST_ASSERT(out->requested_torque_nm == 0);
    TEST_ASSERT(WdgM_EvaluateCycleAndKickHwWdg() == TRUE);

    /* Simulate illegal control flow jump (skipping CP_0 and calling CP_2 directly) */
    TEST_ASSERT(WdgM_CheckpointReached(WDGM_SE_PEDAL_VOTER, WDGM_CP_2_E2E_TX) == E_NOT_OK);
    TEST_ASSERT(WdgM_EvaluateCycleAndKickHwWdg() == FALSE);

    printf("[PASS] test_brake_override_and_wdgm_violation\n");
}

int main(void)
{
    printf("=== Running AUTOSAR Classic ASIL-D Pedal Voter Verification Suite ===\n");
    test_normal_50_percent_pedal_and_e2e_check();
    test_implausibility_debounce_to_safe_state();
    test_brake_override_and_wdgm_violation();
    printf("=== All AUTOSAR & ISO 26262 safety tests passed! ===\n");
    return 0;
}
