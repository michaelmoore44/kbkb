#include "main.h"
#include "b2b_comm.h"
#include "terminal.h"
#include "keycode.h"
#include "keys.h"
#include "usb_device.h"

static GPIO_InitTypeDef  GPIO_InitStruct;

static void SystemClock_Config(void);
static void Error_Handler(void);

USBD_HandleTypeDef USBD_Device;

enum {
    BOARD_NONE,
    MASTER_BOARD,
    SLAVE_BOARD,
};

uint8_t board;

int main(void)
{
    HAL_Init();
    uint8_t keys[8];
    uint8_t i;
    uint32_t j;

    /* Configure the system clock to 84 MHz */
    SystemClock_Config();

    //delay at startup
    HAL_Delay(1000);

    /* -1- Enable GPIOD Clock (to be able to program the configuration registers) */
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    //initialize board ID pin
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    //determine if board is master or slave
    if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9) == GPIO_PIN_RESET)
        board = SLAVE_BOARD;
    else
        board = MASTER_BOARD;

    term_init();
    print("KBKB v00.00.05\r\n");

    if(board == SLAVE_BOARD)
        print("Slave Keyboard");
    else
        print("Master Keyboard");
    print("\r\n");

    HAL_Delay(2000);

    if(board == MASTER_BOARD)
        MX_USB_DEVICE_Init();

    keys_init();

    b2b_comm_init();

    i = 0;
    j = 0;

    /* -3- Toggle PA05 IO in an infinite loop */
    while (1)
    {
        keys_scan();
        b2b_send_pend_msg();
        b2b_check_for_msg();

        if(i >= 10) {
            if(board == SLAVE_BOARD) {
                b2b_comm_send_keys();
            }
            else {
                keys_translate(keys);
                usb_send(keys, 8);
            }
            i = 0;
        }

        if(j >= 1000) {
            HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_4);
            j = 0;
        }

        HAL_Delay(1);
        ++i;
        ++j;
    }
}



/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
      RCC_OscInitTypeDef RCC_OscInitStruct;
      RCC_ClkInitTypeDef RCC_ClkInitStruct;

      __PWR_CLK_ENABLE();

      __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

      RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
      RCC_OscInitStruct.HSEState = RCC_HSE_ON;
      RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
      RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
      RCC_OscInitStruct.PLL.PLLM = 4;
      RCC_OscInitStruct.PLL.PLLN = 192;
      RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
      RCC_OscInitStruct.PLL.PLLQ = 8;
      HAL_RCC_OscConfig(&RCC_OscInitStruct);

      RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                                  |RCC_CLOCKTYPE_PCLK2;
      RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
      RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
      RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
      RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
      HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);

      HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

      HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

      /* SysTick_IRQn interrupt configuration */
      HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  while(1)
  {
  }
}

void _exit(int status)
{
	while(1);
}

/*
 sbrk
 Increase program data space.
 Malloc and related functions depend on this
 */
caddr_t _sbrk(int incr)
{
	caddr_t ret_val = 0;
	return ret_val;
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

