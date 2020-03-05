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

typedef enum
{
    PROCESS_BUFFER_GET_START,
    PROCESS_BUFFER_GET_LEN_SHORT,
    PROCESS_BUFFER_GET_LEN_LONG,
    PROCESS_BUFFER_GET_PAYLOAD,
    PROCESS_BUFFER_GET_CRC,
    PROCESS_BUFFER_GET_END
} process_buffer_state_t;

void vesc_process_buffer(uint8_t* buff, uint16_t len)
{
    while(len--)
    {
        static process_buffer_state_t process_buffer_state;

        static uint8_t len_first;
        static uint16_t packet_len;

        static uint8_t payload_first;
        static uint16_t payload_count;

        static uint8_t* payload_buff;

        static uint8_t crc_first;
        static uint16_t packet_crc;

        switch(process_buffer_state)
        {
        case PROCESS_BUFFER_GET_START:
            if(*buff == 0x02)
                process_buffer_state = PROCESS_BUFFER_GET_LEN_SHORT;
            else if(*buff == 0x03)
            {
                process_buffer_state = PROCESS_BUFFER_GET_LEN_LONG;
                len_first = 1;
            }
            break;

        case PROCESS_BUFFER_GET_LEN_SHORT:
            packet_len = *buff;
            payload_first = 1;
            process_buffer_state = PROCESS_BUFFER_GET_PAYLOAD;
            break;

        case PROCESS_BUFFER_GET_LEN_LONG:
            if(len_first)
            {
                packet_len = (uint16_t)*buff << 8;
                len_first = 0;
            }
            else
            {
                packet_len |= *buff;
                payload_first = 1;
                process_buffer_state = PROCESS_BUFFER_GET_PAYLOAD;
            }
            break;

        case PROCESS_BUFFER_GET_PAYLOAD:
            if(payload_first)
            {
                if(payload_buff) free(payload_buff);

                payload_buff = (uint8_t*)malloc(packet_len);

                payload_count = 0;
                payload_first = 0;

                break;
            }

            *(payload_buff++) = *buff;
            payload_count++;/* condition */

            if(payload_count >= packet_len)
            {
                crc_first = 1;
                process_buffer_state = PROCESS_BUFFER_GET_CRC;
            }
            break;

        case PROCESS_BUFFER_GET_CRC:
            if(crc_first)
            {
                packet_crc = (uint16_t)*buff << 8;
                crc_first = 0;
            }
            else
            {
                packet_crc |= *buff;
                process_buffer_state = PROCESS_BUFFER_GET_END;
            }
            break;

        default:
            if(*buff == 0x03)
            {
                if(crc16(payload_buff, packet_len) == packet_crc)
                {
                    vesc_unpack(payload_buff, packet_len);
                    break;
                }
                // crc error
            }
            // end byte_error

            // call error handler

            if(payload_buff) free(payload_buff);
            break;
        }

        buff++;
    }
}

vesc_unpack(uint8_t* buff, uint16_t len)
{

}