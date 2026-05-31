#ifndef FUNCTION__H_
#define FUNCTION__H_

#include "main.h"

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

#endif
