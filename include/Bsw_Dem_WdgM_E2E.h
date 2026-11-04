/**
 * @file    Bsw_Dem_WdgM_E2E.h
 * @author  Yigit Cakirdogan
 * @brief   AUTOSAR Basic Software Services:
 *          - Dem  (Diagnostic Event Manager with ISO 26262 Counter Debouncing)
 *          - WdgM (Watchdog Manager with Logical Checkpoint Graph Supervision)
 *          - E2E  (End-to-End Protection Profile 1, SAE J1850 CRC-8)
 */

#ifndef BSW_DEM_WDGM_E2E_H
#define BSW_DEM_WDGM_E2E_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"

/* ==========================================================================
 * 1. Diagnostic Event Manager (Dem)
 * ========================================================================== */
typedef uint16 Dem_EventIdType;
typedef uint8  Dem_EventStatusType;

#define DEM_EVENT_ID_APPS_RANGE_FAULT        ((Dem_EventIdType)1U)
#define DEM_EVENT_ID_APPS_IMPLAUSIBILITY     ((Dem_EventIdType)2U)
#define DEM_EVENT_ID_MAX                     ((Dem_EventIdType)4U)

#define DEM_EVENT_STATUS_PASSED              ((Dem_EventStatusType)0x00U)
#define DEM_EVENT_STATUS_FAILED              ((Dem_EventStatusType)0x01U)
#define DEM_EVENT_STATUS_PREPASSED           ((Dem_EventStatusType)0x02U)
#define DEM_EVENT_STATUS_PREFAILED           ((Dem_EventStatusType)0x03U)

#define DEM_CONFIRM_THRESHOLD                ((sint16)6)
#define DEM_CLEAR_THRESHOLD                  ((sint16)-6)

void           Dem_Init(void);
Std_ReturnType Dem_SetEventStatus(Dem_EventIdType event_id, Dem_EventStatusType status);
boolean        Dem_IsEventConfirmedFailed(Dem_EventIdType event_id);
sint16         Dem_GetDebounceCounter(Dem_EventIdType event_id);

/* ==========================================================================
 * 2. Watchdog Manager (WdgM) Logical Program Flow Supervision
 * ========================================================================== */
#define WDGM_SE_PEDAL_VOTER                  ((uint8)0U)
#define WDGM_CP_0_INPUT_ACQUIRE              ((uint8)0U)
#define WDGM_CP_1_SAFETY_VOTE                ((uint8)1U)
#define WDGM_CP_2_E2E_TX                     ((uint8)2U)

void           WdgM_Init(void);
Std_ReturnType WdgM_CheckpointReached(uint8 se_id, uint8 checkpoint_id);
boolean        WdgM_EvaluateCycleAndKickHwWdg(void);
uint32         WdgM_GetRefreshCount(void);

/* ==========================================================================
 * 3. AUTOSAR E2E Profile 1 (SAE J1850 CRC-8 0x1D + 4-bit Alive Counter)
 * ========================================================================== */
#define E2E_P01_DATA_ID_TORQUE_REQ           ((uint16)0x01A5U)

typedef enum {
    E2E_P01STATUS_OK            = 0x00U,
    E2E_P01STATUS_WRONGCRC      = 0x01U,
    E2E_P01STATUS_REPEATED      = 0x02U,
    E2E_P01STATUS_WRONGSEQUENCE = 0x03U
} E2E_P01CheckStatusType;

typedef struct {
    uint16 data_id;
    uint8  alive_counter;
    uint8  last_rx_counter;
    boolean rx_initialized;
} E2E_P01State_t;

void                   E2E_P01_Init(E2E_P01State_t *state, uint16 data_id);
uint8                  E2E_Crc8_SaeJ1850(const uint8 *data, uint8 length, uint8 start_val);
Std_ReturnType         E2E_P01Protect(E2E_P01State_t *state, uint8 *pdu_8bytes);
E2E_P01CheckStatusType E2E_P01Check(E2E_P01State_t *state, const uint8 *pdu_8bytes);

#ifdef __cplusplus
}
#endif

#endif /* BSW_DEM_WDGM_E2E_H */
