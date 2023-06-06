/*
 * myGPIO_service.h
 *
 *  Created on: May 18, 2023
 *      Author: Krystene
 */

#ifndef _MYGPIO_SERVICE_H_
#define _MYGPIO_SERVICE_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include <bcomdef.h>

/*********************************************************************
 * CONSTANTS
 */
// Service UUID
#define MYGPIO_SERVICE_SERV_UUID 0X1140
#define MYGPIO_SERVICE_SERV_UUID_BASE128(uuid) 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0xB0, 0x00, 0x40, 0x51, 0x04, LO_UINT16(uuid), HI_UINT16(uuid), 0x00, \
        0xF0

// PIN Characteristic defines
#define PS_PIN_ID                   0
#define PS_PIN_UUID                 0x1141
#define PS_PIN_UUID_BASE128(uuid) 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0xB0, 0x00, 0x40, 0x51, 0x04, LO_UINT16(uuid), HI_UINT16(uuid), 0x00, 0xF0
#define PS_PIN_LEN                  1
#define PS_PIN_LEN_MIN              1

/*********************************************************************
 * Profile Callbacks
 */

// Callback when a characteristic value has changed
typedef void (*PinServiceChange_t)( uint16_t connHandle, uint16_t svcUuid, uint8_t paramID, uint8_t *pValue, uint16_t len );

typedef struct
{
    PinServiceChange_t pfnChangeCb;         // Called when characteristic value changes
    PinServiceChange_t pfnCfgChangeCb;      // Called when characteristic CCCD changes
} MyGPIOServiceCBs_t;

/*********************************************************************
 * API FUNCTIONS
 */

extern bStatus_t MyGPIOService_AddService(uint8_t rspTaskId);
extern bStatus_t MyGPIOService_RegisterAppCBs(MyGPIOServiceCBs_t *appCallbacks);
extern bStatus_t MyGPIOService_SetParameter(uint8_t param, uint16_t len, void *value);
extern bStatus_t MyGPIOService_GetParameter(uint8_t param, uint16_t *len, void *value);

#ifdef __cplusplus
}
#endif

#endif /* _MYGPIO_SERVICE_H_ */
