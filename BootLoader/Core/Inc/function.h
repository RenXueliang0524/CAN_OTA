#ifndef FUNCTION__H_
#define FUNCTION__H_

#include "main.h"
/* OTA 跳转相关定义*/
#define APP1_ADDR 0x08020000U
#define APP1_SIZE 0x000E0000U

#define APP2_ADDR 0x08100000U
#define APP2_SIZE 0x000E0000U

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
    uint32_t magic;      // 判断 OTA Info 是否有效
    uint32_t bootAppSelect;   // 1 = App1, 2 = App2
    uint32_t version;    // 先预留
    uint32_t crc;        // 先预留
} OTAInfo;

const OTAInfo* getOTAInfo(void);
#endif

