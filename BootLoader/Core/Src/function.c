#include "function.h"

JumpStatus jumpToApp(uint32_t app_addr)
{
    //获取app1的栈顶和重启地址
    uint32_t app_stack = *(volatile uint32_t *)app_addr;
    uint32_t app_reset = *(volatile uint32_t *)(app_addr + 4);
    pFunction appEntry;
    //判断是否程序运行在APP的地址空间
    if ((app_stack & 0x2FFE0000U) != 0x20000000U)
    {
        return JUMP_INVALID_STACK;
    }

    if (app_reset < APP1_ADDR || app_reset >= (APP1_ADDR + APP1_SIZE) &&
        app_reset < APP2_ADDR || app_reset >= (APP2_ADDR + APP2_SIZE))
    {
        return JUMP_INVALID_RESET;
    }
    //时钟和外设逆初始化
    HAL_RCC_DeInit();
    HAL_DeInit();
    //关闭中断
    __disable_irq();
    //关闭systick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    //设置中断向量表地址
    SCB->VTOR = app_addr;
    //设置堆栈指针
    __set_MSP(app_stack);
    //设置重启地址
    appEntry = (pFunction)app_reset;
    __enable_irq();
    appEntry();

    return JUMP_OK;
}
