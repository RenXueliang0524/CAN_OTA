#include "function.h"

static OTAContext otaCtx;
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

    if (!((app_reset >= APP1_ADDR && app_reset < (APP1_ADDR + APP1_SIZE)) ||
          (app_reset >= APP2_ADDR && app_reset < (APP2_ADDR + APP2_SIZE))))
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
    otaInfo.reserved0 = 0;
    otaInfo.reserved1 = 0;
    
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

/**
 * @brief  获取当前未使用的app地址
 * @retval uint32_t 未使用的app地址
 */
uint32_t getInactiveAppAddr(void)
{
    uint32_t bootAppAddr = getBootAppAddr();

    if (bootAppAddr == APP1_ADDR)
    {
        return APP2_ADDR;
    }

    return APP1_ADDR;
}
/**
 * @brief  擦除对应分区
 * @param  app_addr: 分区地址
 * @retval HAL_StatusTypeDef HAL_OK 擦除成功，HAL_ERROR 擦除失败
 */
HAL_StatusTypeDef eraseAppArea(uint32_t app_addr)
{
    FLASH_EraseInitTypeDef eraseInit;
    uint32_t sectorError = 0;

    eraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (app_addr == APP1_ADDR)
    {
        eraseInit.Banks = APP1_BANK;
        eraseInit.Sector = APP1_START_SECTOR;
        eraseInit.NbSectors = APP1_SECTOR_COUNT;
    }
    else if (app_addr == APP2_ADDR)
    {
        eraseInit.Banks = APP2_BANK;
        eraseInit.Sector = APP2_START_SECTOR;
        eraseInit.NbSectors = APP2_SECTOR_COUNT;
    }
    else
    {
        return HAL_ERROR;
    }

    HAL_FLASH_Unlock();

    if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return HAL_ERROR;
    }

    HAL_FLASH_Lock();

    return HAL_OK; 
}
/**
 * @brief  写入 App 数据到 Flash
 * @param  addr 写入 Flash 的目标地址
 * @param  data 数据缓冲区地址
 * @param  len  数据长度，必须是 32 字节对齐
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef writeAppData(uint32_t addr, uint8_t *data, uint32_t len)
{
     uint32_t offset = 0;

    if ((addr % 32U) != 0U)
    {
        return HAL_ERROR;
    }

    if ((len % 32U) != 0U)
    {
        return HAL_ERROR;
    }

    HAL_FLASH_Unlock();

    while (offset < len)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                              addr + offset,
                              (uint32_t)(data + offset)) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return HAL_ERROR;
        }

        offset += 32U;
    }

    HAL_FLASH_Lock();

    return HAL_OK;
}

/**
 * @brief  ota参数初始化
 */
void otaInit(void)
{
    otaCtx.targetAddr = 0;
    otaCtx.writeAddr = 0;
    otaCtx.totalSize = 0;
    otaCtx.receivedSize = 0;
    otaCtx.flashBufLen = 0;
    otaCtx.state = OTA_IDLE;

    memset(otaCtx.flashBuf, 0xFF, OTA_FLASH_WORD_SIZE);
}

/**
 * @brief  固件写入函数
 * @param  
 * @retval 
 */
HAL_StatusTypeDef otaProcessCanFrame(uint32_t can_id, uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0)
    {
        return HAL_ERROR;
    }

    switch (can_id)
    {
        case OTA_CAN_ID_START:
        {
            if (len < 4)
            {
                return HAL_ERROR;
            }

            otaCtx.totalSize = ((uint32_t)data[0]) |
                               ((uint32_t)data[1] << 8) |
                               ((uint32_t)data[2] << 16) |
                               ((uint32_t)data[3] << 24);

            otaCtx.targetAddr = getInactiveAppAddr();
            otaCtx.writeAddr = otaCtx.targetAddr;
            otaCtx.receivedSize = 0;
            otaCtx.flashBufLen = 0;
            otaCtx.state = OTA_RECEIVING;

            memset(otaCtx.flashBuf, 0xFF, OTA_FLASH_WORD_SIZE);

            if (eraseAppArea(otaCtx.targetAddr) != HAL_OK)
            {
                otaCtx.state = OTA_ERROR;
                return HAL_ERROR;
            }

            break;
        }

        case OTA_CAN_ID_DATA:
        {
            uint32_t i;

            if (otaCtx.state != OTA_RECEIVING)
            {
                return HAL_ERROR;
            }

            for (i = 0; i < len; i++)
            {
                if (otaCtx.receivedSize >= otaCtx.totalSize)
                {
                    break;
                }

                otaCtx.flashBuf[otaCtx.flashBufLen++] = data[i];
                otaCtx.receivedSize++;

                if (otaCtx.flashBufLen == OTA_FLASH_WORD_SIZE)
                {
                    if (writeAppData(otaCtx.writeAddr,
                                     otaCtx.flashBuf,
                                     OTA_FLASH_WORD_SIZE) != HAL_OK)
                    {
                        otaCtx.state = OTA_ERROR;
                        return HAL_ERROR;
                    }

                    otaCtx.writeAddr += OTA_FLASH_WORD_SIZE;
                    otaCtx.flashBufLen = 0;
                    memset(otaCtx.flashBuf, 0xFF, OTA_FLASH_WORD_SIZE);
                }
            }

            break;
        }

        case OTA_CAN_ID_END:
        {
            if (otaCtx.state != OTA_RECEIVING)
            {
                return HAL_ERROR;
            }

            if (otaCtx.receivedSize != otaCtx.totalSize)
            {
                otaCtx.state = OTA_ERROR;
                return HAL_ERROR;
            }

            if (otaCtx.flashBufLen > 0)
            {
                memset(&otaCtx.flashBuf[otaCtx.flashBufLen],
                       0xFF,
                       OTA_FLASH_WORD_SIZE - otaCtx.flashBufLen);

                if (writeAppData(otaCtx.writeAddr,
                                 otaCtx.flashBuf,
                                 OTA_FLASH_WORD_SIZE) != HAL_OK)
                {
                    otaCtx.state = OTA_ERROR;
                    return HAL_ERROR;
                }
            }

            if (otaCtx.targetAddr == APP1_ADDR)
            {
                if (setBootAppAddr(OTA_BOOT_APP1) != HAL_OK)
                {
                    otaCtx.state = OTA_ERROR;
                    return HAL_ERROR;
                }
            }
            else if (otaCtx.targetAddr == APP2_ADDR)
            {
                if (setBootAppAddr(OTA_BOOT_APP2) != HAL_OK)
                {
                    otaCtx.state = OTA_ERROR;
                    return HAL_ERROR;
                }
            }
            else
            {
                otaCtx.state = OTA_ERROR;
                return HAL_ERROR;
            }

            otaCtx.state = OTA_FINISHED;

            NVIC_SystemReset();

            break;
        }

        default:
            return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief  获取 App 是否请求进入 OTA
 * @retval 1: 请求 OTA, 0: 未请求
 */
uint8_t getOtaRequest(void)
{
    uint32_t flag = *(volatile uint32_t *)OTA_REQUEST_ADDR;

    if (flag == OTA_REQUEST_MAGIC)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief  清除 OTA 请求标志
 * @retval None
 */
void clearOtaRequest(void)
{
    *(volatile uint32_t *)OTA_REQUEST_ADDR = 0;
}
