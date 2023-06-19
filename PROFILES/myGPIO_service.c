/*
 * myGPIO_service.c
 *
 *  Created on: May 18, 2023
 *      Author: Krystene
 */

/*
 * Includes
 */
#include <string.h>

#include <xdc/runtime/Diags.h>
#include <uartlog/UartLog.h>

#include <icall.h>
#include "util.h"

#include "icall_ble_api.h"

#include "myGPIO_service.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */

// myGPIO_Service UUID
CONST uint8_t MyGPIOServiceUUID[ATT_UUID_SIZE] =
{
  MYGPIO_SERVICE_SERV_UUID_BASE128(MYGPIO_SERVICE_SERV_UUID)
};

// ON/OFF UUID
CONST uint8_t ps_PINCHARAUUID[ATT_UUID_SIZE] =
{
  PS_PIN_UUID_BASE128(PS_PIN_UUID)
};


/*********************************************************************
 * LOCAL VARIABLES
 */

static MyGPIOServiceCBs_t *pAppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Service declaration
static CONST gattAttrType_t GPIOServiceDec1 = { ATT_UUID_SIZE, MyGPIOServiceUUID };

// Characteristic "PIN" Properties (for declaration)
static uint8_t ps_PINProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;

// Characteristic "PIN" Value variable
static uint8_t ps_PINVal[PS_PIN_LEN] = {0};

// Length of data in characteristic "PIN" Value variable, initialized to minimal size.

static uint16_t ps_PINValLen = PS_PIN_LEN_MIN;


/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t MyGPIO_ServiceAttrTbl[] =
{
  // PIN_Service Service Declaration
  {
    { ATT_BT_UUID_SIZE, primaryServiceUUID },
    GATT_PERMIT_READ,
    0,
    (uint8_t *)&GPIOServiceDec1
  },
  // PIN Characteristic Declaration
  {
    { ATT_BT_UUID_SIZE, characterUUID },
    GATT_PERMIT_READ,
    0,
    &ps_PINProps
  },
    // PIN Characteristic Value
    {
      { ATT_UUID_SIZE, ps_PINCHARAUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE | GATT_PERMIT_WRITE,
      0,
      ps_PINVal
    }
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t MyGPIO_Service_ReadAttrCB( uint16_t connHandle, gattAttribute_t *pAttr,
                                              uint8_t *pValue, uint16_t *pLen, uint16_t offset,
                                              uint16_t maxLen, uint8_t method );
static bStatus_t MyGPIO_Service_WriteAttrCB( uint16_t connHandle, gattAttribute_t *pAttr,
                                               uint8_t *pValue, uint16_t len, uint16_t offset,
                                               uint8_t method );

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Simple Profile SErvice Callbacks
CONST gattServiceCBs_t MyGPIO_ServiceCBs =
{
  MyGPIO_Service_ReadAttrCB,
  MyGPIO_Service_WriteAttrCB,
  NULL
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*
 * MyGPIOService_AddService - Initializes the GPIOService service by registering
 *          GATT attributes with the GATT server.
 *
 *      rspTaskID - The ICall Task Id that should receive responses for Indications.
 */
extern bStatus_t MyGPIOService_AddService( uint8_t rspTaskId )
{
    uint8_t status;

    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( MyGPIO_ServiceAttrTbl,
                                          GATT_NUM_ATTRS( MyGPIO_ServiceAttrTbl ),
                                          GATT_MAX_ENCRYPT_KEY_SIZE,
                                          &MyGPIO_ServiceCBs );
    Log_info1("Registered service, %d attributes", (IArg)GATT_NUM_ATTRS( MyGPIO_ServiceAttrTbl ));
    return ( status );
}

bStatus_t MyGPIOService_RegisterAppCBs( MyGPIOServiceCBs_t *appCallbacks )
{
    if ( appCallbacks )
    {
        pAppCBs = appCallbacks;
        Log_info1("Registered callbacks to application. Struct %p", (IArg)appCallbacks);
        return ( SUCCESS );
    }
    else
    {
        Log_warning0("Null pointer given for app callbacks.");
        return ( FAILURE );
    }
}

/*********************************************************************
 * MyGPIO_SetParameter = set a MyGPIOService parameter.
 */
bStatus_t MyGPIOService_SetParameter( uint8_t param, uint16_t len, void *value )
{
    bStatus_t ret = SUCCESS;
    uint8_t *pAttrVal;
    uint16_t *pValLen;
    uint16_t valMinLen;
    uint16_t valMaxLen;

    switch ( param )
    {
        case PS_PIN_ID:
            pAttrVal    =   ps_PINVal;
            pValLen     =  &ps_PINValLen;
            valMinLen   =   PS_PIN_LEN_MIN;
            valMaxLen   =   PS_PIN_LEN;
            Log_info2("SetParameter : %s len: %d", (IArg)"PIN", (IArg)len);
            break;

        default:
            Log_error1("SetParameter: Parameter #%d not valid.", (IArg)param);
            return INVALIDPARAMETER;
    }

    // Check bounds, update value and send notification or indication if possible.
    if ( len <= valMaxLen && len >= valMinLen )
    {
        memcpy(pAttrVal, value, len);
        *pValLen = len;
    }
    else
    {
        Log_error3("Length outside bounds: Len: %d MinLen: %d MaxLen: %d.",
                   (IArg)len,
                   (IArg)valMinLen,
                   (IArg)valMaxLen);
        ret = bleInvalidRange;
    }

    return ret;
}

/*********************************************************************
 * MyGPIOService_GetParameter - Get a GPIOService parameter.
 */
bStatus_t MyGPIOService_GetParameter( uint8_t param, uint16_t *len, void *value )
{
    bStatus_t ret = SUCCESS;
    switch ( param )
    {
        case PS_PIN_ID:
            *len = MIN(*len, ps_PINValLen);
            memcpy(value, ps_PINVal, *len);
            Log_info2("GetParameter : %s returning %d bytes",
                      (IArg)"PIN",
                      (IArg)*len);
        default:
            Log_error1("GetParameter: Parameter #%d not valid.",
                       (IArg)param);
            ret = INVALIDPARAMETER;
            break;
    }
    return ret;
}

/*********************************************************************
 * MYGPIO_Service_findCharParamId
 */
static uint8_t MyGPIO_Service_findCharParamId(gattAttribute_t *pAttr)
{
    // Is this a Client Characteristic Configuration Descriptor?
    if (ATT_BT_UUID_SIZE == pAttr->type.len && GATT_CLIENT_CHAR_CFG_UUID == *(uint16_t *)pAttr->type.uuid)
        return MyGPIO_Service_findCharParamId(pAttr - 1);
    // Is this attribute in MYGPIO PIN?
    else if (ATT_UUID_SIZE == pAttr->type.len && !memcmp(pAttr->type.uuid, ps_PINCHARAUUID, pAttr->type.len))
        return PS_PIN_ID;

    else
        return 0xFF; // Not found. Return invalid.
}

/*********************************************************************
 * MYGPIO_Service_ReadAttrCB
 */
static bStatus_t MyGPIO_Service_ReadAttrCB( uint16_t connHandle, gattAttribute_t *pAttr,
                                         uint8_t *pValue, uint16_t *pLen, uint16_t offset,
                                         uint16_t maxLen, uint8_t method )
{
    bStatus_t status = SUCCESS;
    uint16_t valueLen;
    uint8_t paramID = 0xFF;

    // Find settings for the characteristic to be read.
    paramID = MyGPIO_Service_findCharParamId( pAttr );
    switch ( paramID )
    {
        case PS_PIN_ID:
            valueLen = ps_PINValLen;

            Log_info4("ReadAttrCB : %s connHandle: %d offset: %d method: 0x%02x",
                      (IArg)"PIN",
                      (IArg)connHandle,
                      (IArg)offset,
                      (IArg)method);
            break;
        default:
            Log_error0("Attribute was not found.");
            return ATT_ERR_ATTR_NOT_FOUND;
    }
    // Check bounds and return the value
    if ( offset > valueLen )
    {
        Log_error0("An in valid offset was requested.");
        status = ATT_ERR_INVALID_OFFSET;
    }
    else
    {
        *pLen = MIN(maxLen, valueLen - offset);
        memcpy(pValue, pAttr->pValue + offset, *pLen);
    }

    return status;
}

/*********************************************************************
 * MYGPIO_Service_WriteAttrCB
 */
static bStatus_t MyGPIO_Service_WriteAttrCB( uint16_t connHandle, gattAttribute_t *pAttr,
                                             uint8_t *pValue, uint16_t len, uint16_t offset,
                                             uint8_t method )
{
    bStatus_t   status   = SUCCESS;
    uint8_t     paramID  = 0xFF;
    uint8_t     changeParamID = 0xFF;
    uint16_t    writeLenMin;
    uint16_t    writeLenMax;
    uint16_t    *pValueLenVar;

    // Find settings for the characteristic to be written.
    paramID = MyGPIO_Service_findCharParamId( pAttr );
    switch ( paramID )
    {
        case PS_PIN_ID:
            writeLenMin = PS_PIN_LEN_MIN;
            writeLenMax = PS_PIN_LEN;
            pValueLenVar = &ps_PINValLen;

            Log_info5("WriteAttrCB : %s connHandle(%d) len(%d) offset(%d) method(0x%02x",
                      (IArg)"PIN",
                      (IArg)connHandle,
                      (IArg)len,
                      (IArg)offset,
                      (IArg)method);
            break;
        default:
            Log_error0("Attribute was not found.");
            return ATT_ERR_ATTR_NOT_FOUND;
    }

    // Check whether the length is within bounds.
    if ( offset >= writeLenMax )
    {
        Log_error0("An invalid offset was requested.");
        status = ATT_ERR_INVALID_OFFSET;
    }
    else if ( offset + len > writeLenMax )
    {
        Log_error0("Invalid value length was received.");
        status = ATT_ERR_INVALID_VALUE_SIZE;
    }
    else if ( offset + len < writeLenMin && ( method == ATT_EXECUTE_WRITE_REQ || method == ATT_WRITE_REQ ) )
    {
        Log_error0("Invalid value length was received.");
        status = ATT_ERR_INVALID_VALUE_SIZE;
    }
    else
    {
        memcpy(pAttr->pValue + offset, pValue, len);
        if ( offset + len >= writeLenMin )
        {
            changeParamID = paramID;
            *pValueLenVar = offset + len;
        }
    }

    if ( changeParamID != 0xFF )
        if ( pAppCBs && pAppCBs->pfnChangeCb )
            pAppCBs->pfnChangeCb( connHandle, MYGPIO_SERVICE_SERV_UUID, paramID, pValue, len+offset );
    return status;
}











