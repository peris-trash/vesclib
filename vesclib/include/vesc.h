/*
 *	Copyright 2020 - Rafael Silva - silvagracarafael[at]gmail.com
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _VESC_H_
#define _VESC_H_

//#define _VESC_TIMEOUT_EN_
#define VESC_TIMEOUT_VAL 50
#define VESC_TICK_COUNT g_ullSystemTick

// needs CRC-16/XMODEM algorithm
#define VESC_CRC16_INC "crc.h"
#define VESC_CRC16_FUNC(buff, count) crc16(buff, count)

//#define _VESC_DYNAMIC_ALLOCATION_EN_
#define VESC_ALLOC_FUNC(size)   realloc(size)
#define VESC_FREE_FUNC(ptr)    free(ptr)
#define VESC_MAX_BUFFER 512

#include <stdlib.h>
#include <stdint.h>
#include VESC_CRC16_INC

typedef enum
{
    RECEIVE_TIMEOUT = 0,
    CRC_ERR,
    END_BYTE_ERR,
    UNHANDLED_MESSAGE,
    MEM_ERR
} vesc_error_t;

typedef void (* vesc_handle_err_cb_fn_t)(vesc_error_t);
typedef uint8_t (* vesc_handle_message_cb_fn_t)(uint8_t, uint8_t*, uint16_t);

void vesc_set_err_callback(vesc_handle_message_cb_fn_t pFn);
void vesc_set_message_callback(vesc_handle_message_cb_fn_t pFn);

void vesc_process_buffer(uint8_t* pubBuff, uint16_t usLen);

void vesc_send_payload(uint8_t* pubPayload, uint16_t usLen);

void _vesc_write_byte(uint8_t ubData);

#endif /* _VESC_H_ */