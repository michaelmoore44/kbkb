#include "terminal.h"

#include "buffer.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "stm32f4xx_hal.h"


#define TX_BUF_SIZE (256)
#define RX_BUF_SIZE (256)

static uint8_t term_data[TX_BUF_SIZE];

static UART_HandleTypeDef Uart_Handle;

static buffer_t tx_buf;
static uint8_t  tx_buf_data[TX_BUF_SIZE];
static buffer_t rx_buf;
static uint8_t  rx_buf_data[RX_BUF_SIZE];

//initialize terminal
void term_init(void)
{

	tx_buf.buff = tx_buf_data;
	tx_buf.size = TX_BUF_SIZE;
	buffer_init(&tx_buf);
	rx_buf.buff = rx_buf_data;
	rx_buf.size = RX_BUF_SIZE;
	buffer_init(&rx_buf);

    Uart_Handle.Instance          = USART1;

    Uart_Handle.Init.BaudRate     = 115200;
    Uart_Handle.Init.WordLength   = UART_WORDLENGTH_8B;
    Uart_Handle.Init.Parity       = UART_PARITY_NONE;
    Uart_Handle.Init.StopBits     = UART_STOPBITS_1;
    Uart_Handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    Uart_Handle.Init.Mode         = UART_MODE_TX_RX;
    Uart_Handle.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&Uart_Handle);

    //enable interrupts
    Uart_Handle.Instance->CR1 |= (USART_CR1_TXEIE | USART_CR1_RXNEIE);
}


result term_putchar(uint8_t* data)
{
	result ret_val = FAIL;
	if(!buffer_is_full(&tx_buf))
		ret_val = buffer_write(&tx_buf, data);
	return ret_val;
}


uint32_t term_puts(uint8_t* data, uint32_t len)
{
    uint32_t count;
	for (count = 0; count < len; count++)
	{
		if(term_putchar(data++))
			break;
	}

	if(!buffer_is_empty(&tx_buf))
	{
		//enable transmit interrupt
		Uart_Handle.Instance->CR1 |= USART_CR1_TXEIE;
	}
    return count;
}


uint32_t print_va (const char *format, va_list ap)
{
    uint32_t len;

    len = (uint32_t)vsnprintf((char*)term_data, TX_BUF_SIZE, format, ap);

    if (len > TX_BUF_SIZE)
        len = TX_BUF_SIZE;

    term_puts(term_data, len);

    return len;
}


uint32_t print(const char *format, ...)
{
    va_list ap;
    uint32_t res;

    va_start (ap, format);
    res = print_va (format, ap);
    va_end (ap);

    return res;
}


void USART1_IRQHandler (void)
{
	//check if tx interrupt is active - check TXE == 1
	if(Uart_Handle.Instance->SR & USART_SR_TXE)
	{
		//if more data to send, put data in tx buffer
		if(!buffer_is_empty(&tx_buf))
			Uart_Handle.Instance->DR = buffer_read(&tx_buf);

		//clear the TXE flag
		Uart_Handle.Instance->SR &= ~USART_SR_TXE;
		if(buffer_is_empty(&tx_buf))
		{
			//disable transmit interrupt
			Uart_Handle.Instance->CR1 &= ~USART_CR1_TXEIE;
		}
	}

	//check if rx interrupt is active - check RXNE
	if(Uart_Handle.Instance->SR & USART_SR_RXNE)
	{
		//put data in rx buffer
		buffer_write(&rx_buf, (uint8_t*)&(Uart_Handle.Instance->DR));
	}
}
