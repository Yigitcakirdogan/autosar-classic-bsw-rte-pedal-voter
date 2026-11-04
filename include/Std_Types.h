/**
 * @file    Std_Types.h
 * @author  Yigit Cakirdogan
 * @brief   AUTOSAR Classic R4.4 Standard & Platform Type Definitions
 *          compliant with MISRA C:2012 Directive 4.6.
 */

#ifndef STD_TYPES_H
#define STD_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/* AUTOSAR Platform Types */
typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;
typedef int8_t    sint8;
typedef int16_t   sint16;
typedef int32_t   sint32;
typedef uint8_t   boolean;

#ifndef TRUE
    #define TRUE  ((boolean)1U)
#endif

#ifndef FALSE
    #define FALSE ((boolean)0U)
#endif

#ifndef NULL_PTR
    #define NULL_PTR ((void *)0)
#endif

/* AUTOSAR Standard Return Type */
typedef uint8 Std_ReturnType;

#define E_OK      ((Std_ReturnType)0x00U)
#define E_NOT_OK  ((Std_ReturnType)0x01U)

/* Physical Pin States */
#define STD_LOW   ((uint8)0x00U)
#define STD_HIGH  ((uint8)0x01U)

#ifdef __cplusplus
}
#endif

#endif /* STD_TYPES_H */
