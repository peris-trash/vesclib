/*
 *	Copyright 2020 - Rafael Silva - silvagracarafael[at]gmail.com
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "vesc.h"

static vesc_handle_err_cb_fn_t pfn_vesc_handle_err_cb = NULL;
static vesc_handle_message_cb_fn_t pfn_vesc_handle_msg_cb = NULL;

typedef enum
{
    PROCESS_BUFFER_GET_START = 0,
    PROCESS_BUFFER_GET_LEN_SHORT,
    PROCESS_BUFFER_GET_LEN_LONG,
    PROCESS_BUFFER_GET_PAYLOAD,
    PROCESS_BUFFER_GET_CRC,
    PROCESS_BUFFER_GET_END
} process_buffer_state_t;

void vesc_set_err_callback(vesc_handle_message_cb_fn_t pFn)
{
    pfn_vesc_handle_err_cb = pFn;
}
void vesc_set_message_callback(vesc_handle_message_cb_fn_t pFn)
{
    pfn_vesc_handle_msg_cb = pFn;
}

void vesc_handle_error(vesc_error_t xErr)
{
    if(pfn_vesc_handle_err_cb)
        pfn_vesc_handle_err_cb(xErr);
}
void vesc_handle_message(uint8_t* pubPayload, uint16_t usPayloadLen)
{
    uint8_t ubMsgId = *(pubPayload++);

    if(pfn_vesc_handle_msg_cb)
        if(pfn_vesc_handle_msg_cb(ubMsgId, pubPayload, usPayloadLen - 1))
            return;

    vesc_handle_error(UNHANDLED_MESSAGE);
}

void vesc_process_buffer(uint8_t* pubBuff, uint16_t usLen)
{
    static process_buffer_state_t xProcessBuffState = PROCESS_BUFFER_GET_START;

    #ifdef _VESC_DYNAMIC_ALLOCATION_EN_
    static uint8_t* spubPayloadBuff;
    #else
    static uint8_t spubPayloadBuff[VESC_MAX_BUFFER];
    #endif

    #ifdef _VESC_TIMEOUT_EN_
        static uint64_t ullLastReceiveTick = 0;
        if(VESC_TICK_COUNT > (ullLastReceiveTick + VESC_TIMEOUT_VAL) && xProcessBuffState != PROCESS_BUFFER_GET_START)
        {
            // timed out
            xProcessBuffState = PROCESS_BUFFER_GET_START;

            #ifdef _VESC_DYNAMIC_ALLOCATION_EN_
            if(spubPayloadBuff) VESC_FREE_FUNC(spubPayloadBuff);
            #endif
            vesc_handle_error(RECEIVE_TIMEOUT);
        }

        if(usLen || xProcessBuffState == PROCESS_BUFFER_GET_START)
            ullLastReceiveTick = VESC_TICK_COUNT;
    #endif

    while(usLen--)
    {
        static uint8_t subLenFirstByte;
        static uint16_t susPacketLen;

        static uint8_t subPayloadFirstByte;
        static uint16_t susPayloadCount;

        static uint8_t* spubPayloadBuffPtr = NULL;

        static uint8_t subCrcFirstByte;
        static uint16_t susPacketCrc;

        switch(xProcessBuffState)
        {
        case PROCESS_BUFFER_GET_START:
            if(*pubBuff == 0x02)
                xProcessBuffState = PROCESS_BUFFER_GET_LEN_SHORT;
            else if(*pubBuff == 0x03)
            {
                xProcessBuffState = PROCESS_BUFFER_GET_LEN_LONG;
                subLenFirstByte = 1;
            }
            break;

        case PROCESS_BUFFER_GET_LEN_SHORT:
            susPacketLen = *pubBuff;
            subPayloadFirstByte = 1;
            xProcessBuffState = PROCESS_BUFFER_GET_PAYLOAD;
            break;

        case PROCESS_BUFFER_GET_LEN_LONG:
            if(subLenFirstByte)
            {
                susPacketLen = (uint16_t)*pubBuff << 8;
                subLenFirstByte = 0;
            }
            else
            {
                susPacketLen |= *pubBuff;
                subPayloadFirstByte = 1;
                xProcessBuffState = PROCESS_BUFFER_GET_PAYLOAD;
            }
            break;

        case PROCESS_BUFFER_GET_PAYLOAD:
            if(subPayloadFirstByte)
            {
                if(susPacketLen > VESC_MAX_BUFFER)
                {
                    vesc_handle_error(MEM_ERR);
                    return;
                }

                #ifdef _VESC_DYNAMIC_ALLOCATION_EN_
                spubPayloadBuff = (uint8_t*)VESC_ALLOC_FUNC(susPacketLen);

                if(!spubPayloadBuff)
                {
                    vesc_handle_error(MEM_ERR);
                    return;
                }
                #endif

                spubPayloadBuffPtr = spubPayloadBuff;

                susPayloadCount = 0;
                subPayloadFirstByte = 0;

                break;
            }

            *(spubPayloadBuffPtr++) = *pubBuff;
            susPayloadCount++;

            if(susPayloadCount >= susPacketLen)
            {
                subCrcFirstByte = 1;
                xProcessBuffState = PROCESS_BUFFER_GET_CRC;
            }
            break;

        case PROCESS_BUFFER_GET_CRC:
            if(subCrcFirstByte)
            {
                susPacketCrc = (uint16_t)*pubBuff << 8;
                subCrcFirstByte = 0;
            }
            else
            {
                susPacketCrc |= *pubBuff;
                xProcessBuffState = PROCESS_BUFFER_GET_END;
            }
            break;

        case PROCESS_BUFFER_GET_END:
            if(*pubBuff == 0x03)
            {
                if(VESC_CRC16_FUNC(spubPayloadBuff, susPacketLen) == susPacketCrc)
                {
                    vesc_handle_message(spubPayloadBuff, susPacketLen);
                    #ifdef _VESC_DYNAMIC_ALLOCATION_EN_
                    if(spubPayloadBuff) VESC_FREE_FUNC(spubPayloadBuff);
                    #endif
                    break;
                }
                vesc_handle_error(CRC_ERR);
            }
            else
                vesc_handle_error(END_BYTE_ERR);

            #ifdef _VESC_DYNAMIC_ALLOCATION_EN_
            if(spubPayloadBuff) VESC_FREE_FUNC(spubPayloadBuff);
            #endif
            break;

            default:
            xProcessBuffState = PROCESS_BUFFER_GET_START;
            break;
        }

        pubBuff++;
    }
}

void vesc_send_payload(uint8_t* pubPayload, uint16_t usLen)
{
    uint16_t usPayloadCrc = VESC_CRC16_FUNC(pubPayload, usLen);

    if(usLen < 256)
    {
        _vesc_write_byte(0x02);
        _vesc_write_byte(usLen & 0xFF);

    }
    else
    {
        _vesc_write_byte(0x03);
        _vesc_write_byte(usLen >> 8);
        _vesc_write_byte(usLen & 0xFF);
    }

    while(usLen--)
    {
        _vesc_write_byte(*(pubPayload++));
    }

    _vesc_write_byte(usPayloadCrc >> 8);
    _vesc_write_byte(usPayloadCrc & 0xFF);

    _vesc_write_byte(0x03);
}