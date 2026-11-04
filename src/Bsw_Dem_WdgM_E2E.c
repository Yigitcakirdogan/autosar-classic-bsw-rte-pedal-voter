/**
 * @file    Bsw_Dem_WdgM_E2E.c
 * @author  Yigit Cakirdogan
 * @brief   Implementation of AUTOSAR Dem, WdgM, and E2E Profile 1 modules.
 */

#include "Bsw_Dem_WdgM_E2E.h"

/* ==========================================================================
 * 1. Dem Implementation
 * ========================================================================== */
static sint16  s_dem_counter[DEM_EVENT_ID_MAX];
static boolean s_dem_confirmed_failed[DEM_EVENT_ID_MAX];

void Dem_Init(void)
{
    for (uint16 i = 0U; i < DEM_EVENT_ID_MAX; i++) {
        s_dem_counter[i]          = 0;
        s_dem_confirmed_failed[i] = FALSE;
    }
}

Std_ReturnType Dem_SetEventStatus(Dem_EventIdType event_id, Dem_EventStatusType status)
{
    if ((event_id == 0U) || (event_id >= DEM_EVENT_ID_MAX)) {
        return E_NOT_OK;
    }

    switch (status) {
    case DEM_EVENT_STATUS_FAILED:
        s_dem_counter[event_id]          = DEM_CONFIRM_THRESHOLD;
        s_dem_confirmed_failed[event_id] = TRUE;
        break;

    case DEM_EVENT_STATUS_PASSED:
        s_dem_counter[event_id] = DEM_CLEAR_THRESHOLD;
        break;

    case DEM_EVENT_STATUS_PREFAILED:
        s_dem_counter[event_id] = (sint16)(s_dem_counter[event_id] + 2);
        if (s_dem_counter[event_id] >= DEM_CONFIRM_THRESHOLD) {
            s_dem_counter[event_id]          = DEM_CONFIRM_THRESHOLD;
            s_dem_confirmed_failed[event_id] = TRUE;
        }
        break;

    case DEM_EVENT_STATUS_PREPASSED:
        s_dem_counter[event_id] = (sint16)(s_dem_counter[event_id] - 1);
        if (s_dem_counter[event_id] < DEM_CLEAR_THRESHOLD) {
            s_dem_counter[event_id] = DEM_CLEAR_THRESHOLD;
        }
        break;

    default:
        return E_NOT_OK;
    }

    return E_OK;
}

boolean Dem_IsEventConfirmedFailed(Dem_EventIdType event_id)
{
    if ((event_id == 0U) || (event_id >= DEM_EVENT_ID_MAX)) {
        return TRUE;
    }
    return s_dem_confirmed_failed[event_id];
}

sint16 Dem_GetDebounceCounter(Dem_EventIdType event_id)
{
    if ((event_id == 0U) || (event_id >= DEM_EVENT_ID_MAX)) {
        return 0;
    }
    return s_dem_counter[event_id];
}

/* ==========================================================================
 * 2. WdgM Logical Supervision Implementation
 * ========================================================================== */
static uint8   s_wdgm_expected_cp = WDGM_CP_0_INPUT_ACQUIRE;
static boolean s_wdgm_seq_error   = FALSE;
static boolean s_wdgm_cycle_done  = FALSE;
static uint32  s_wdgm_refreshes   = 0UL;

void WdgM_Init(void)
{
    s_wdgm_expected_cp = WDGM_CP_0_INPUT_ACQUIRE;
    s_wdgm_seq_error   = FALSE;
    s_wdgm_cycle_done  = FALSE;
    s_wdgm_refreshes   = 0UL;
}

Std_ReturnType WdgM_CheckpointReached(uint8 se_id, uint8 checkpoint_id)
{
    if (se_id != WDGM_SE_PEDAL_VOTER) {
        s_wdgm_seq_error = TRUE;
        return E_NOT_OK;
    }

    if (checkpoint_id != s_wdgm_expected_cp) {
        s_wdgm_seq_error = TRUE;
        return E_NOT_OK;
    }

    if (checkpoint_id == WDGM_CP_2_E2E_TX) {
        s_wdgm_cycle_done  = TRUE;
        s_wdgm_expected_cp = WDGM_CP_0_INPUT_ACQUIRE;
    } else {
        s_wdgm_expected_cp = (uint8)(checkpoint_id + 1U);
    }

    return E_OK;
}

boolean WdgM_EvaluateCycleAndKickHwWdg(void)
{
    if ((s_wdgm_seq_error == FALSE) && (s_wdgm_cycle_done == TRUE)) {
        s_wdgm_cycle_done = FALSE;
        s_wdgm_refreshes++;
        return TRUE;
    }
    return FALSE;
}

uint32 WdgM_GetRefreshCount(void)
{
    return s_wdgm_refreshes;
}

/* ==========================================================================
 * 3. E2E Profile 1 (SAE J1850 CRC-8) Implementation
 * ========================================================================== */
uint8 E2E_Crc8_SaeJ1850(const uint8 *data, uint8 length, uint8 start_val)
{
    uint8 crc = start_val;

    if (data == NULL_PTR) {
        return 0U;
    }

    for (uint8 i = 0U; i < length; i++) {
        crc ^= data[i];
        for (uint8 bit = 0U; bit < 8U; bit++) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8)((uint8)(crc << 1U) ^ 0x1DU);
            } else {
                crc = (uint8)(crc << 1U);
            }
        }
    }
    return (uint8)(crc ^ 0xFFU);
}

static uint8 compute_p01_crc(uint16 data_id, const uint8 *pdu_8bytes)
{
    uint8 id_bytes[2];
    id_bytes[0] = (uint8)(data_id & 0xFFU);
    id_bytes[1] = (uint8)((data_id >> 8U) & 0xFFU);

    uint8 crc = E2E_Crc8_SaeJ1850(id_bytes, 2U, 0xFFU);
    /* Re-invert intermediate CRC to chain payload bytes [1..7] */
    crc = E2E_Crc8_SaeJ1850(&pdu_8bytes[1], 7U, (uint8)(crc ^ 0xFFU));
    return crc;
}

void E2E_P01_Init(E2E_P01State_t *state, uint16 data_id)
{
    if (state != NULL_PTR) {
        state->data_id         = data_id;
        state->alive_counter   = 0U;
        state->last_rx_counter = 0U;
        state->rx_initialized  = FALSE;
    }
}

Std_ReturnType E2E_P01Protect(E2E_P01State_t *state, uint8 *pdu_8bytes)
{
    if ((state == NULL_PTR) || (pdu_8bytes == NULL_PTR)) {
        return E_NOT_OK;
    }

    /* Byte 1 low nibble holds the 4-bit Alive Counter (0..14 per AUTOSAR E2E P01) */
    pdu_8bytes[1] = (uint8)((pdu_8bytes[1] & 0xF0U) | (state->alive_counter & 0x0FU));
    /* Byte 0 holds the SAE J1850 CRC-8 over DataID + Bytes[1..7] */
    pdu_8bytes[0] = compute_p01_crc(state->data_id, pdu_8bytes);

    state->alive_counter = (uint8)((state->alive_counter + 1U) % 15U);
    return E_OK;
}

E2E_P01CheckStatusType E2E_P01Check(E2E_P01State_t *state, const uint8 *pdu_8bytes)
{
    if ((state == NULL_PTR) || (pdu_8bytes == NULL_PTR)) {
        return E2E_P01STATUS_WRONGCRC;
    }

    uint8 expected_crc = compute_p01_crc(state->data_id, pdu_8bytes);
    if (expected_crc != pdu_8bytes[0]) {
        return E2E_P01STATUS_WRONGCRC;
    }

    uint8 rx_cnt = (uint8)(pdu_8bytes[1] & 0x0FU);
    if (state->rx_initialized == FALSE) {
        state->last_rx_counter = rx_cnt;
        state->rx_initialized  = TRUE;
        return E2E_P01STATUS_OK;
    }

    if (rx_cnt == state->last_rx_counter) {
        return E2E_P01STATUS_REPEATED;
    }

    uint8 delta = (rx_cnt >= state->last_rx_counter)
                ? (uint8)(rx_cnt - state->last_rx_counter)
                : (uint8)((15U + rx_cnt) - state->last_rx_counter);

    state->last_rx_counter = rx_cnt;

    if (delta > 1U) {
        return E2E_P01STATUS_WRONGSEQUENCE;
    }

    return E2E_P01STATUS_OK;
}
