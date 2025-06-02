/* ============================
 * File: usbd_cdc_if.c
 * Description: USB CDC Virtual COM interface for elevator control
 * ============================
 */

#include "usbd_cdc_if.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "elevator.h"

typedef struct {
    uint8_t currFloor;
    uint8_t targetFloor;
    uint8_t isMoving;
    uint16_t moveTimeMs;
    uint16_t tick;
    uint16_t totalWork;
} CarState;

extern CarState cReg[3];

static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len);

extern CarState cReg[3];
extern uint8_t userFloor;
extern uint8_t userReq;
extern void handleUserRequest(uint8_t from, uint8_t to);

uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

extern USBD_HandleTypeDef hUsbDeviceFS;

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = {
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS
};

static int8_t CDC_Init_FS(void) {
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
  return (USBD_OK);
}

static int8_t CDC_DeInit_FS(void) {
  return (USBD_OK);
}

static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length) {
  return (USBD_OK);
}

uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len) {
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
  if (hcdc->TxState != 0) return USBD_BUSY;
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, Buf, Len);
  return USBD_CDC_TransmitPacket(&hUsbDeviceFS);
}

static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len) {
  static char cmdBuffer[64];
  static uint8_t cmdIndex = 0;

  for (uint32_t i = 0; i < *Len; i++) {
    char ch = Buf[i];
    if (ch == '\r' || ch == '\n') {
      cmdBuffer[cmdIndex] = '\0';
      cmdIndex = 0;

      if (strncmp(cmdBuffer, "GETuserFloor", 13) == 0) {
        char msg[32];
        sprintf(msg, "userFloor: %d\r\n", userFloor);
        CDC_Transmit_FS((uint8_t*)msg, strlen(msg));

      } else if (strncmp(cmdBuffer, "GETuserReq", 11) == 0) {
        char msg[32];
        sprintf(msg, "userReq: %d\r\n", userReq);
        CDC_Transmit_FS((uint8_t*)msg, strlen(msg));

      } else if (strncmp(cmdBuffer, "GETCurrFloor,", 13) == 0) {
        int car = atoi(&cmdBuffer[13]);
        if (car >= 0 && car < 3) {
          char msg[32];
          sprintf(msg, "CurrFloor%d: %d\r\n", car, cReg[car].currFloor);
          CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
        } else {
          CDC_Transmit_FS((uint8_t*)"Invalid car\r\n", 14);
        }

      } else if (strncmp(cmdBuffer, "UP,", 3) == 0 || strncmp(cmdBuffer, "DOWN,", 5) == 0) {
        char dir[6];
        int from, to;
        if (sscanf(cmdBuffer, "%5[^,],%d,%d", dir, &from, &to) == 3) {
          if (from >= 0 && from < 8 && to >= 0 && to < 8 && from != to) {
            handleUserRequest(from, to);
            char msg[64];
            sprintf(msg, "Request %s from %d to %d\r\n", dir, from, to);
            CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
          } else {
            CDC_Transmit_FS((uint8_t*)"Invalid floor values\r\n", 23);
          }
        } else {
          CDC_Transmit_FS((uint8_t*)"Invalid command format\r\n", 25);
        }

      } else {
        CDC_Transmit_FS((uint8_t*)"Unknown command\r\n", 18);
      }

      memset(cmdBuffer, 0, sizeof(cmdBuffer));
    } else if (cmdIndex < sizeof(cmdBuffer) - 1) {
      cmdBuffer[cmdIndex++] = ch;
    }
  }

  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
  USBD_CDC_ReceivePacket(&hUsbDeviceFS);
  return (USBD_OK);
}
