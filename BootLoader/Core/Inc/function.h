#ifndef FUNCTION__H_
#define FUNCTION__H_

#include "main.h"
#include <string.h>
/* OTA 跳转相关定义*/
#define APP1_ADDR 0x08020000U
#define APP1_SIZE 0x000E0000U

#define APP2_ADDR 0x08100000U
#define APP2_SIZE 0x000E0000U

/* 跳转函数相关*/
typedef void (*pFunction)(void);

typedef enum
{
    JUMP_OK = 0,
    JUMP_INVALID_STACK,
    JUMP_INVALID_RESET
} JumpStatus;

JumpStatus jumpToApp(uint32_t app_addr);

/* OTA状态相关定义，用来判断当前应该跳转至哪个分区*/
#define OTA_INFO_ADDR  0x081E0000U
#define OTA_MAGIC      0x4F544131U
#define OTA_BOOT_APP1 1U
#define OTA_BOOT_APP2 2U

typedef struct
{
    uint32_t magic;
    uint32_t bootAppSelect;
    uint32_t version;
    uint32_t size;
    uint32_t crc;
    uint32_t state;
    uint32_t reserved0;
    uint32_t reserved1;
} OTAInfo;

const OTAInfo* getOTAInfo(void);
uint32_t getBootAppAddr(void);
HAL_StatusTypeDef setBootAppAddr(uint32_t app_addr);

/* 判断当前正在运行的分区，擦除未使用分区，并选择固件写入位置*/
#define APP1_BANK           FLASH_BANK_1
#define APP1_START_SECTOR   FLASH_SECTOR_1
#define APP1_SECTOR_COUNT   7U

#define APP2_BANK           FLASH_BANK_2
#define APP2_START_SECTOR   FLASH_SECTOR_0
#define APP2_SECTOR_COUNT   7U

#define OTA_RX_BUF_SIZE 32U

uint32_t getInactiveAppAddr(void);
HAL_StatusTypeDef eraseAppArea(uint32_t app_addr);
HAL_StatusTypeDef writeAppData(uint32_t addr, uint8_t *data, uint32_t len);

/* ota接收写入相关*/
#define OTA_CAN_ID_START  0x100U
#define OTA_CAN_ID_DATA   0x101U
#define OTA_CAN_ID_END    0x102U

#define OTA_FLASH_WORD_SIZE 32U

typedef enum
{
    OTA_IDLE = 0,
    OTA_RECEIVING,
    OTA_FINISHED,
    OTA_ERROR
} OTAState;

typedef struct
{
    uint32_t targetAddr;
    uint32_t writeAddr;
    uint32_t totalSize;
    uint32_t receivedSize;
    uint8_t flashBuf[OTA_FLASH_WORD_SIZE];
    uint32_t flashBufLen;
    OTAState state;
} OTAContext;

void otaInit(void);
HAL_StatusTypeDef otaProcessCanFrame(uint32_t can_id, uint8_t *data, uint32_t len);

/* 升级标志位*/
#define OTA_REQUEST_ADDR   0x2001FFF0U
#define OTA_REQUEST_MAGIC  0xA5A55A5AU

uint8_t getOtaRequest(void);
void clearOtaRequest(void);

#endif
