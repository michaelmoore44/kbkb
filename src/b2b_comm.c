#include "b2b_comm.h"

#include "buffer.h"
#include "keys.h"
#include "stm32f4xx_hal.h"
#include "terminal.h"

#include <string.h>

#define TX_BUF_SIZE (32)
#define RX_BUF_SIZE (32)

enum {
    MSG_NONE,
    MSG_ACK,
    MSG_NACK,
    MSG_KEYS,
};

enum {
    MSG_STATUS_TO_SEND,
    MSG_STATUS_SENT,
    MSG_STATUS_RETRY1,
    MSG_STATUS_R1_SENT,
    MSG_STATUS_RETRY2,
    MSG_STATUS_R2_SENT,
    MSG_STATUS_RETRY3,
    MSG_STATUS_DONE,
};

typedef struct
{
   uint8_t len;
   uint8_t status;
   uint8_t* buf;
} out_buf_t;

static uint8_t  output_buffer[2][32];

out_buf_t out_buf[] = {
    [0] = { .len = 0, .status = MSG_STATUS_DONE, .buf = &output_buffer[0][0]},
    [1] = { .len = 0, .status = MSG_STATUS_DONE, .buf = &output_buffer[1][0]},
};

static uint8_t  read;

static UART_HandleTypeDef Uart_Handle;

static buffer_t tx_buf;
static uint8_t  tx_buf_data[TX_BUF_SIZE];
static buffer_t rx_buf;
static uint8_t  rx_buf_data[RX_BUF_SIZE];


static uint8_t  msg_buf[32];
static uint8_t  msg_buf_tx[32];
static uint32_t bad_b2b_message_count;
static uint8_t  msg_idx;
static bool     msg_len;
static bool     msg_len_valid;

uint8_t* prev_keys_buf[NUM_KEY_BYTES];


void USART2_IRQHandler (void)
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


static result b2b_comm_putchar(uint8_t* data)
{
	result ret_val = FAIL;
	if(!buffer_is_full(&tx_buf)) {
		ret_val = buffer_write(&tx_buf, data);
	}
	return ret_val;
}

static uint32_t b2b_comm_puts(uint8_t* data, uint8_t len)
{
    uint8_t count;

	for(count = 0; count < len; count++)
	{
	    if(b2b_comm_putchar(data++))
	        break;
	}

    if(!buffer_is_empty(&tx_buf))
    {
        //enable transmit interrupt
        Uart_Handle.Instance->CR1 |= USART_CR1_TXEIE;
    }
	return count;
}


static result b2b_comm_getchar(uint8_t* data)
{
	result ret_val = FAIL;
	if(!buffer_is_empty(&rx_buf)) {
		*data = buffer_read(&rx_buf);
		ret_val = OK;
	}
	return ret_val;
}


static bool msg_to_be_sent(uint8_t status)
{
    bool ret_val = FALSE;
    if((status == MSG_STATUS_TO_SEND) ||
       (status == MSG_STATUS_RETRY1)  ||
       (status == MSG_STATUS_RETRY2)  ||
       (status == MSG_STATUS_RETRY3)) {
        ret_val = TRUE;
    }
	return ret_val;
}


static void b2b_print_msg(uint8_t* buf, uint8_t len)
{
    uint8_t i;
    print("\t");
    for(i = 0; i < len; i++) {
        print("%02X ", buf[i]);
    }
    print("\r\n");
    print("\tLength: %d\r\n", buf[0]);
    print("\tCommand: ");
    switch (buf[2]) {
        case MSG_NONE:
            print("NONE");
            break;
        case MSG_ACK:
            print("ACK");
            break;
        case MSG_NACK:
            print("NACK");
            break;
        case MSG_KEYS:
            print("KEYS");
            break;
        default:
            print("Unknown");
            break;
    }
    print("\r\n");
}


void b2b_send_pend_msg(void)
{
    if(msg_to_be_sent(out_buf[read].status)) {
        b2b_comm_puts(out_buf[read].buf, out_buf[read].len);
        print("\r\nMessage Sent\r\n");
        b2b_print_msg(out_buf[read].buf, out_buf[read].len);
        out_buf[read].status++;
    }

    //if the message that was just sent was not an ACK or NACK and if
    // the current message is done, and there is a message in the write buffer
    // then change over to the write buffer
    if((out_buf[read].buf[2] != MSG_ACK) && (out_buf[read].buf[2] != MSG_NACK)){
        if(out_buf[read].status == MSG_STATUS_DONE &&
           msg_to_be_sent(out_buf[!read].status) == TRUE)
        {
            read = !read;
        }
    }
    //if the message that was just sent was an ack or a nack and if
    // the current message is done
    else if((out_buf[read].buf[2] == MSG_ACK)   ||
             (out_buf[read].buf[2] == MSG_NACK)) {
        if(out_buf[read].status == MSG_STATUS_DONE &&
           msg_to_be_sent(out_buf[!read].status) == TRUE)
        {
            read = !read;
        }
    }
}

/*len = entire message
 *                2 bytes - Header - length of payload + complement of field
 *                n bytes - payload
 *                1 byte  - checksum*/
static void b2b_comm_put_msg_in_queue(uint8_t* buf, uint8_t len)
{
    if(out_buf[!read].len == 0) {
        memcpy(out_buf[!read].buf, buf, len);
        out_buf[!read].len = len;
        out_buf[!read].status = MSG_STATUS_TO_SEND;
    }
    else
        print("no empty buffer for message\r\n");
}


static uint8_t get_checksum8(uint8_t* buf, uint8_t len)
{
    uint8_t i;
    uint8_t cksum = buf[0];

    for(i = 1; i < (len - 1); i++) {
        cksum += buf[i];
    }

    return cksum;
}

/* length parameter:
 * len = length of payload
 * payload = 1 byte command + n bytes data
 *
 * length sent is entire message*/
static void b2b_send_msg(uint8_t* buf, uint8_t len)
{
    buf[0] = len;
    buf[1] = ~buf[0];
    buf[len + 2] = get_checksum8(&buf[2], len);


    b2b_comm_put_msg_in_queue(buf, len + 3);
}


static void b2b_send_ack(void)
{
    msg_buf_tx[2] = MSG_ACK;
    b2b_send_msg(msg_buf_tx, 1);
}


static void b2b_send_nack(void)
{
    msg_buf_tx[2] = MSG_NACK;
    b2b_send_msg(msg_buf_tx, 1);
}


void b2b_comm_send_keys(void)
{
    int val;
    uint8_t buf[NUM_KEY_BYTES + 4];

    keys_get_keys(&buf[3]);

    val =  memcmp(&buf[3], prev_keys_buf, NUM_KEY_BYTES);
    if(val) {
        msg_buf_tx[2] = MSG_KEYS;
        memcpy(&msg_buf_tx[3], &buf[3], NUM_KEY_BYTES);
        b2b_send_msg(msg_buf_tx, NUM_KEY_BYTES + 1);
        memcpy(prev_keys_buf, &buf[3], NUM_KEY_BYTES);
    }
}


static void b2b_process_msg(uint8_t* buf, uint8_t len)
{
    switch (buf[2]) {
        case MSG_KEYS:
            if((len - 4) == 5)
                keys_received(&buf[3], len - 4);
            else
                print("Keys received, but bad length");
            break;

        case MSG_ACK:
            out_buf[read].status = MSG_STATUS_DONE;
            break;

        case MSG_NACK:
            out_buf[read].status++;
            break;

        default:
            print("Message with unknown command received\r\n");
            break;
    }
}


static result checksum8(uint8_t* buf, uint8_t len)
{
    uint8_t i;
    uint8_t cksum = buf[0];
    result ret_val = FAIL;

    for(i = 1; i < (len - 1); i++) {
        cksum += buf[i];
    }

    if(cksum == buf[len - 1])
        ret_val = OK;

    return ret_val;
}


void b2b_check_for_msg(void)
{
    uint8_t temp;
    if(b2b_comm_getchar(&msg_buf[msg_idx]) == OK) {
        msg_idx++;
        //print("\r\n%02X - index: %d - len: %d", msg_buf[msg_idx - 1], msg_idx, msg_len);

        //check for header
        if(msg_idx == 2) {
            temp = (uint8_t) ~msg_buf[0];
            if(msg_buf[1] == temp) {
                //the message length in the header refers to the payload
                //so the number of overhead bytes must be added to the payload
                msg_len = msg_buf[0] + 3;
                msg_len_valid = TRUE;
                print("\r\nFound Header - index: %d", msg_idx);
            }
            else { //start over
                temp = (uint8_t) ~msg_buf[0];
                print("\r\nKeep Looking - %02X not= %02X", msg_buf[1], temp);
                msg_buf[0] = msg_buf[1];
                msg_idx = 1;
                msg_len_valid = FALSE;
            }
        }

        //check for end of message
        if((msg_len_valid == TRUE) && (msg_idx == msg_len)) {
            print("\r\nMessage Received:\r\n\t");
            b2b_print_msg(msg_buf, msg_len);
            if(checksum8(&msg_buf[2], msg_len - 2) == OK) {
                //valid message received
                print("\tChecksum Ok\r\n");
                b2b_process_msg(msg_buf, msg_len);
                if((msg_buf[2] != MSG_ACK) && (msg_buf[2] != MSG_NACK)) {
                    print("\r\nSending ack");
                    b2b_send_ack();
                }
                msg_idx = 0;
                msg_len_valid = FALSE;
            }
            else {
                print("\tChecksum Bad\r\n");
                if((msg_buf[2] != MSG_ACK) && (msg_buf[2] != MSG_NACK)) {
                    print("\r\nSending nack");
                    b2b_send_nack();
                }
                //send a nack and start over
                msg_buf[0] = msg_buf[1];
                msg_idx = 0;
                msg_len_valid = FALSE;
                bad_b2b_message_count++;
            }
        }
    }
}

//initialize terminal
void b2b_comm_init(void)
{
    bad_b2b_message_count = 0;
    msg_idx = 0;
    msg_len_valid = TRUE;

    read = 0;

    tx_buf.buff = tx_buf_data;
    tx_buf.size = TX_BUF_SIZE;
    buffer_init(&tx_buf);
    rx_buf.buff = rx_buf_data;
    rx_buf.size = RX_BUF_SIZE;
    buffer_init(&rx_buf);

    Uart_Handle.Instance          = USART2;

    Uart_Handle.Init.BaudRate     = 115200;
    Uart_Handle.Init.WordLength   = UART_WORDLENGTH_8B;
    Uart_Handle.Init.Parity       = UART_PARITY_NONE;
    Uart_Handle.Init.StopBits     = UART_STOPBITS_1;
    Uart_Handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    Uart_Handle.Init.Mode         = UART_MODE_TX_RX;
    Uart_Handle.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_UART_Init(&Uart_Handle);

//    uint16_t i;
//    HAL_Delay(5000);
//    while(1) {
//        //send data
//        if(Uart_Handle.Instance->SR & USART_SR_TXE) {
//            //send data
//            Uart_Handle.Instance->DR = 'A';
//            Uart_Handle.Instance->SR &= ~USART_SR_TXE;
//        }
//
//        for(i = 0; i < 1000; i ++) {
//            //wait for received data
//            if(Uart_Handle.Instance->SR & USART_SR_RXNE)
//            {
//                print("%c\r\n", (uint8_t)Uart_Handle.Instance->DR);
//            }
//            HAL_Delay(1);
//        }
//    }


    //enable interrupts
    Uart_Handle.Instance->CR1 |= (USART_CR1_TXEIE | USART_CR1_RXNEIE);

//    uint8_t buf[] = {'A', 'B'};
//    uint8_t in_buf[2];
//    uint16_t i;
//
//    HAL_Delay(1000);
//    while(1) {
//        b2b_comm_puts (buf, 2);
//        for(i = 0; i < 1000; i++) {
//            HAL_Delay(1);
//            if(b2b_comm_getchar(in_buf) == OK)
//                print("%c\r\n", in_buf[0]);
//        }
//    }
}



