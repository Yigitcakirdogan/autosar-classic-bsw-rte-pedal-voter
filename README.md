# AUTOSAR Classic ASIL-D Redundant Pedal Voter & E2E Stack (`autosar-classic-bsw-rte-pedal-voter`)

[![CI - Build & Unit Tests](https://github.com/Yigitcakirdogan/autosar-classic-bsw-rte-pedal-voter/actions/workflows/ci.yml/badge.svg)](https://github.com/Yigitcakirdogan/autosar-classic-bsw-rte-pedal-voter/actions/workflows/ci.yml)
![Standard](https://img.shields.io/badge/Architecture-AUTOSAR%20Classic%20R4.4-00599C?style=flat-square&logo=c)
![Safety](https://img.shields.io/badge/ISO%2026262-ASIL--D%20(1oo2D%20Voter)-critical?style=flat-square)
![Compliance](https://img.shields.io/badge/MISRA%20C%3A2012-Compliant%20Subset-2EA44F?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-green?style=flat-square)

A turnkey **AUTOSAR Classic layered architecture** and **ISO 26262 ASIL-D Functional Safety** reference implementation for an automotive **Vehicle Control Unit (VCU) Dual-Channel Accelerator Pedal Position Sensor (APPS) Plausibility Voter & E2E-Protected Torque Request Transmitter**, written in strict **MISRA C:2012** compliant C.

---

## ISO 26262 Safety Concept & ASIL-D Architecture

- **Hazard Identification (HARA):** Unintended vehicle acceleration caused by sensor drift, short-to-battery, open-circuit ground loss, or corrupted CAN torque request frames.
- **Safety Goal (`SG-01`):** Prevent positive propulsion torque request when pedal sensors disagree by $> 5.0\%$ ($50\text{ ‰}$) or when any sensor electrical fault occurs (**ASIL-D**, **FTTI = 50 ms**).
- **Safe State:** Immediate transition to **Safe Torque Off (`0 Nm`)**, assertion of Fault Indicator Lamp (`DIO_CH_MIL_LAMP = STD_HIGH`), and storage of confirmed Diagnostic Trouble Code (`DTC 0xP2138`) in `Dem`.

```mermaid
flowchart TB
    subgraph ASW["Application Software Layer (ASW - ASIL-D)"]
        SWC["Swc_PedalVoter\nRunnable_PedalVoter_10ms()\n• 1oo2D Dual-Slope Plausibility\n• Brake-Pedal Override (BOS)\n• Safe Torque Limiter"]
    end

    subgraph RTE["AUTOSAR Run-Time Environment (RTE)"]
        RTE_API["Rte_Read_* / Rte_Write_* / Rte_Call_*"]
    end

    subgraph BSW["Basic Software (BSW Services & ECUAL)"]
        IOHWAB["IoHwAb\nRatiometric Dual-Slope Scaling\n(APPS1: 0.5-4.5V, APPS2: 0.25-2.25V)"]
        DEM["Dem (Diagnostic Event Manager)\nCounter-Based Debouncer\n(+2 Prefailed / -1 Prepassed)"]
        WDGM["WdgM (Watchdog Manager)\nLogical Program Flow Supervision\n(CP0 -> CP1 -> CP2 Graph)"]
        E2E["E2E Profile 1 (SAE J1850 CRC-8)\n4-Bit Alive Counter + DataID"]
    end

    subgraph MCAL["Microcontroller Abstraction Layer (MCAL)"]
        ADC["Mcal_Adc (Adc_ReadGroup)"]
        DIO["Mcal_Dio (Dio_WriteChannel)"]
    end

    SWC <--> RTE_API
    RTE_API --> IOHWAB & DEM & WDGM & E2E
    IOHWAB --> ADC
    SWC --> DIO
```

---

## Technical Highlights

1. **Dual-Slope Redundant Sensor Conditioning (`IoHwAb`):**
   - Automotive pedals use two physically independent Hall-effect sensors with a **2:1 voltage ratio** (`APPS1: 500–4500 mV`, `APPS2: 250–2250 mV`) so a common-mode power supply drop or cross-wire short between channels is immediately detected as a slope divergence.
   - Out-of-bounds electrical checks detect **Short-to-Ground (`< 400 mV`)** and **Short-to-Supply (`> 4600 mV`)** within a single `10 ms` tick.
2. **ISO 26262 Counter-Based Fault Debouncing (`Dem`):**
   - Implements AUTOSAR `Dem_SetEventStatus()` with asymmetric counter debouncing (`+2` on `DEM_EVENT_STATUS_PREFAILED`, `-1` on `DEM_EVENT_STATUS_PREPASSED`, confirmation threshold = `+6` / `30 ms` < `50 ms FTTI`).
3. **AUTOSAR End-to-End Protection (`E2E_P01`):**
   - Protects the outgoing 8-byte CAN Torque Request PDU using **SAE J1850 CRC-8 (`0x1D` polynomial)**, a **4-bit monotonic Alive Counter**, and a **16-bit implicit Data ID** to detect frame repetition, loss, insertion, or bit corruption.
4. **Watchdog Manager Logical Supervision (`WdgM`):**
   - Verifies that `Runnable_PedalVoter_10ms()` traverses checkpoints `CP_0_INPUT_ACQUIRE -> CP_1_SAFETY_VOTE -> CP_2_E2E_TX` in strict deterministic order before permitting hardware watchdog refresh.
5. **MISRA C:2012 Adherence:**
   - Fixed-width AUTOSAR types (`uint8`, `uint16`, `uint32`, `sint16`, `boolean`), zero dynamic memory allocation (Rule 21.3), no recursion (Rule 17.2), explicit boolean comparisons (Rule 14.4), and defensive `NULL_PTR` checks on every BSW/RTE entry point.

---

## Build & Run Verification Suite

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## License

Released under the [MIT License](LICENSE).
