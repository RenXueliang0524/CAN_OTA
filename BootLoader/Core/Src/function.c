#include "function.h"
/**
 * @brief  跳转到指定应用程序
 * @param  app_addr 应用程序起始地址
 * @retval JumpStatus 跳转状态
 */
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

/**
 * @brief  获取OTA Info的结构体数据
 * @retval OTAInfo* 指向OTA Info的结构体指针
 */
const OTAInfo* getOTAInfo(void)
{
    return (const OTAInfo *)OTA_INFO_ADDR;
}

/**
 * @brief  获取要跳转的app地址
 * @retval uint32_t 要跳转的app地址
 */
uint32_t getBootAppAddr(void)
{
    uint32_t app_addr = APP1_ADDR;
    const OTAInfo *otaInfo = NULL;//OTAInfo结构体指针

    otaInfo = getOTAInfo();
    if(otaInfo->magic == OTA_MAGIC)
    {
        if(otaInfo->bootAppSelect==OTA_BOOT_APP1)
            app_addr = APP1_ADDR;
        else if(otaInfo->bootAppSelect==OTA_BOOT_APP2)
            app_addr = APP2_ADDR;
    }
    return app_addr;
}

/**
 * @brief  设置要跳转的app分区
 * @param  appChoose 选填 OTA_BOOT_APP1 或 OTA_BOOT_APP2
 * @retval HAL_StatusTypeDef HAL_OK 设置成功，HAL_ERROR 设置失败
 */
HAL_StatusTypeDef setBootAppAddr(uint32_t appChoose)
{
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError = 0;
    OTAInfo otaInfo;

    if(appChoose != OTA_BOOT_APP1 && appChoose != OTA_BOOT_APP2)
    {
        return HAL_ERROR;
    }

    otaInfo.magic = OTA_MAGIC;
    otaInfo.bootAppSelect = appChoose;
    otaInfo.version = 0;
    otaInfo.size = 0;
    otaInfo.crc = 0;
    otaInfo.state = 0;
    
    HAL_FLASH_Unlock();

    eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    eraseInit.Banks = FLASH_BANK_2;
    eraseInit.Sector = FLASH_SECTOR_7;
    eraseInit.NbSectors = 1;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return HAL_ERROR;
    }

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                          OTA_INFO_ADDR,
                          (uint32_t)&otaInfo) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return HAL_ERROR;
    }

    HAL_FLASH_Lock();

    return HAL_OK;
}
