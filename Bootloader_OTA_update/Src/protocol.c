/*
 * protocol.c
 * ─────────────────────────────────────────────────────────────
 * Packet Format:
 *   SOF  (1 byte ) : 0xAA
 *   SEQ  (2 bytes) : little-endian
 *   LEN  (2 bytes) : little-endian, 1–512
 *   DATA (N bytes) : firmware payload
 *   CRC  (4 bytes) : CRC32 of data little-endian
 */

#include "stm32f4xx.h"
#include <stdint.h>
#include <string.h>
#include "flash_if.h"
#include "uart6.h"
#include "crc.h"
#include "metadata.h"
#include "uart2.h"
#include <stdio.h>

// CONFIGS
#define MAX_DATA_SIZE       512
#define WRITE_BUFFER_SIZE   2048

#define SOF_BYTE            0xAA
#define ACK_BYTE            0xFA
#define NACK_BYTE           0xFB
#define START_BYTE 			0xFC

#define SEQ_SIZE            2
#define LEN_SIZE            2

// PARSER STATES
typedef enum {
    STATE_WAIT_SOF = 0,
    STATE_WAIT_SEQ_0,
    STATE_WAIT_SEQ_1,
    STATE_WAIT_LEN_0,
    STATE_WAIT_LEN_1,
    STATE_RECV_DATA,
    STATE_WAIT_CRC_0,
    STATE_WAIT_CRC_1,
    STATE_WAIT_CRC_2,
    STATE_WAIT_CRC_3,
} ParserState;

// Packet Struct
typedef struct {
    ParserState state;
    uint16_t    seq;
    uint16_t    len;
    uint8_t     data[MAX_DATA_SIZE];
    uint16_t    data_index;
    uint32_t    crc_received;
    uint16_t    expected_seq;
} PacketCtx;


// WRITE BUFFER CONTEXT

typedef struct {
    uint8_t  buf[WRITE_BUFFER_SIZE];
    uint16_t index;
    uint32_t flash_address;
} WriteBufferCtx;

static PacketCtx      pkt;
static WriteBufferCtx wbuf;
static uint32_t       run_crc,packet_crc,temp_run_crc;   // maintaining 2 crc states , one is temp for efficient calculation
uint32_t total_fw_len = 0;
uint32_t total_fw_crc = 0;
uint32_t total_data_received=0;


static void Send_ACK()
{
    uart6_write(ACK_BYTE);

}

static void Send_NACK()
{
	uart6_write(NACK_BYTE);
}


// FLUSH WRITE BUFFER , Pads to 4-byte boundary with 0xFF then calls Flash_Direct_Write.
static void FlushWriteBuffer(void)
{
    if (wbuf.index == 0) return;

    while (wbuf.index % 4 != 0)
        wbuf.buf[wbuf.index++] = 0xFF;

    uint32_t word_count = wbuf.index / 4;
    Flash_Direct_Write(wbuf.flash_address, (uint32_t *)wbuf.buf, word_count);

    wbuf.flash_address += wbuf.index;
    wbuf.index = 0;
}


//  APPEND DATA TO WRITE BUFFER , Auto-flushes to flash when buffer fills up.
static void BufferAppend(const uint8_t *data, uint16_t len)
{
    uint16_t written = 0;

    while (written < len) {
        uint16_t space = WRITE_BUFFER_SIZE - wbuf.index;
        uint16_t chunk = ((len - written) < space) ? (len - written) : space;

        memcpy(&wbuf.buf[wbuf.index], &data[written], chunk);
        wbuf.index += chunk;
        written    += chunk;

        if (wbuf.index == WRITE_BUFFER_SIZE){
        	FlushWriteBuffer();
        }

    }
}


static void Process_Packet(void)
{
    if (pkt.seq != pkt.expected_seq) {
        Send_NACK();
        printf("%d packet out of seq\r\n",pkt.seq);
        return;
    }

    packet_crc=0xFFFFFFFF; // reset packet crc
    run_crc=temp_run_crc; // commit running crc

    BufferAppend(pkt.data, pkt.len);

    pkt.expected_seq++;
    printf("%d packet received and verified\r\n",pkt.seq);
    Send_ACK();
}



void Protocol_Init(uint32_t flash_addr)
{
    memset(&pkt,  0, sizeof(pkt));
    memset(&wbuf, 0, sizeof(wbuf));

    pkt.state          = STATE_WAIT_SOF;
    pkt.expected_seq   = 0;
    wbuf.flash_address = flash_addr;
    run_crc = 0xFFFFFFFF;
    packet_crc=0xFFFFFFFF;
    temp_run_crc=0xFFFFFFFF;
}



void Protocol_ReceiveByte(uint8_t byte)
{
    switch (pkt.state)
    {
        // SOF
        case STATE_WAIT_SOF:
            if (byte == SOF_BYTE)
                pkt.state = STATE_WAIT_SEQ_0;
            // any garbage byte before SOF is silently ignored
            break;

        // SEQ
        case STATE_WAIT_SEQ_0:
            pkt.seq   = (uint16_t)byte;
            pkt.state = STATE_WAIT_SEQ_1;
            break;

        case STATE_WAIT_SEQ_1:
            pkt.seq  |= (uint16_t)byte << 8;
            pkt.state = STATE_WAIT_LEN_0;
            break;

        // LEN
        case STATE_WAIT_LEN_0:
            pkt.len   = (uint16_t)byte;
            pkt.state = STATE_WAIT_LEN_1;
            break;

        case STATE_WAIT_LEN_1:
            pkt.len |= (uint16_t)byte << 8;

            if (pkt.len == 0 || pkt.len > MAX_DATA_SIZE) {
                //Send_NACK(); SEEE THISSS
                pkt.state = STATE_WAIT_SOF;
                printf("%d packet len exceeded/corrupted\r\n",pkt.seq);
                break;
            }

            pkt.data_index = 0;
            pkt.state      = STATE_RECV_DATA;
            break;

        // DATA
        case STATE_RECV_DATA:
        	total_data_received++;
            pkt.data[pkt.data_index++] = byte;
            packet_crc=crc32_update(packet_crc, byte);
            temp_run_crc=crc32_update(temp_run_crc, byte);
            if (pkt.data_index == pkt.len)
                pkt.state = STATE_WAIT_CRC_0;
            break;

        // CRC (4 bytes, little-endian)
        case STATE_WAIT_CRC_0:
            pkt.crc_received  = (uint32_t)byte;
            pkt.state         = STATE_WAIT_CRC_1;
            break;

        case STATE_WAIT_CRC_1:
            pkt.crc_received |= (uint32_t)byte << 8;
            pkt.state         = STATE_WAIT_CRC_2;
            break;

        case STATE_WAIT_CRC_2:
            pkt.crc_received |= (uint32_t)byte << 16;
            pkt.state         = STATE_WAIT_CRC_3;
            break;

        case STATE_WAIT_CRC_3:
        {
            pkt.crc_received |= (uint32_t)byte << 24;


            if (~packet_crc != pkt.crc_received) {
                Send_NACK();
                temp_run_crc=run_crc;
                printf("%d packet crc wrong\r\n",pkt.seq);
            } else {
                Process_Packet();
            }

            // Reset parser for next packet regardless of outcome
            pkt.state        = STATE_WAIT_SOF;
            pkt.data_index   = 0;
            pkt.len          = 0;
            pkt.crc_received = 0;
            break;
        }

        default:
            pkt.state = STATE_WAIT_SOF;
            break;
    }
}


uint8_t Protocol_Finalize(uint32_t expected_fw_crc)
{
    FlushWriteBuffer();     // flush any remaining bytes

    if (~run_crc != expected_fw_crc) {
        Send_NACK();
        return 0;
    }

    Send_ACK();
    return 1;
}


uint8_t comm_init(void) {
    // 1. Wait for START
	uint8_t temp;
    while (uart6_read() != START_BYTE);

    // 2. Respond with ACK
    Send_ACK();
    const volatile BootMetadata_t *meta=read_metadata();
    if(meta->active_slot==0){
    	uart6_write(temp=1); // send slot B
    }
    else{
    	uart6_write(temp=0); // send slot A
    }

    // 3. Receive Firmware Length (4 bytes - Little Endian)
    total_fw_len = 0;
    total_fw_len |= ((uint32_t)uart6_read() << 0);
    total_fw_len |= ((uint32_t)uart6_read() << 8);
    total_fw_len |= ((uint32_t)uart6_read() << 16);
    total_fw_len |= ((uint32_t)uart6_read() << 24);

    // 4. Receive Full Firmware CRC (4 bytes - Little Endian)
    total_fw_crc = 0;
    total_fw_crc |= ((uint32_t)uart6_read() << 0);
    total_fw_crc |= ((uint32_t)uart6_read() << 8);
    total_fw_crc |= ((uint32_t)uart6_read() << 16);
    total_fw_crc |= ((uint32_t)uart6_read() << 24);

    // 5. Final ACK
    Flash_Direct_Erase_Sector(temp);

    Send_ACK();
    uint32_t flash_addr = (temp==1)? 0x08040000 : 0x0800C000;
    Protocol_Init(flash_addr);
    return temp;
}

uint8_t start_protocol(){

	while(total_data_received<total_fw_len || pkt.state != STATE_WAIT_SOF){
		Protocol_ReceiveByte(uart6_read());
	}
	return Protocol_Finalize(total_fw_crc);
}


