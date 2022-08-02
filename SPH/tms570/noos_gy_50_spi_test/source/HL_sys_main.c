/** @file HL_sys_main.c 
*   @brief Application main file
*   @date 11-Dec-2018
*   @version 04.07.01
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/* 
* Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com  
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "HL_sys_common.h"

/* USER CODE BEGIN (1) */
#include "HL_system.h"
#include "HL_sci.h"
#include "HL_rti.h"

#include <stdio.h>
#include <string.h>

#include "gy_50_intf_init.h"
#include "gy_50_rw.h"
/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */
#define UART        sciREG1

void sci_display_text (sciBASE_t *sci, uint8 *text, uint32 length);
void wait (uint32 time);

unsigned int buf_len;
char buf[128];
volatile uint8_t raw_xyz[6]={0,};
volatile uint16_t acc_x, acc_y, acc_z;
volatile boolean get_data_flg;
/* USER CODE END */

int main(void)
{
/* USER CODE BEGIN (3) */
    uint8_t whoami = 0;

    sciInit();

    sprintf(buf, "SCI Init Success!\n\r\0");
    buf_len = strlen(buf);
    sci_display_text(UART, (uint8 *)buf, buf_len);

    rtiInit();
    sprintf(buf, "RTI Init Success!\n\r\0");
    buf_len = strlen(buf);
    sci_display_text(UART, (uint8 *)buf, buf_len);

    /*rti 관련 함수*/
    rtiEnableNotification(rtiREG1, rtiNOTIFICATION_COMPARE0);
    _enable_IRQ_interrupt_();
    rtiStartCounter(rtiREG1, rtiCOUNTER_BLOCK0);

    gy_50_spi_init();

    sprintf(buf, "SPI Init Success!\n\r\0");
    buf_len = strlen(buf);
    sci_display_text(UART, (uint8 *)buf, buf_len);

    //gy-50의 ctrl_reg1을 통해 data rate = 800Hz, cut-off = 110로 설정
    //x,y,z 데이터 출력 활성화
    enable_spi_bus(WRITE, SPI_FMT_0, 0xFE);
    write_gy_50((uint16_t)MULTIRW_OFF, (uint16_t)CTRL_REG1, 0xFF);

    //gy-50의 ctrl_reg4 설정
    //MSB, LSB 읽기전까지 output 레지스터 업데이트 막기
    //엔디언 : little endian
    //스케일 : 2000dps
    //셀프테스트 모드 : 정상모드
    //spi 모드 : 3선식
    enable_spi_bus(WRITE, SPI_FMT_0, 0xFE);
    write_gy_50((uint16_t)MULTIRW_OFF, (uint16_t)CTRL_REG4, 0x41);

    enable_spi_bus(READ, SPI_FMT_0, 0xFE);
    whoami = read_gy_50((uint16_t)MULTIRW_OFF, (uint16_t)WHO_AM_I);
    sprintf(buf, "gy-50 ID : %d\n\r\0", whoami);
    buf_len = strlen(buf);
    sci_display_text(UART, (uint8 *)buf, buf_len);

    while(1)
    {
        if(get_data_flg)
        {
            acc_x = raw_xyz[0];
            acc_x |= raw_xyz[1] << 8;
            acc_y = raw_xyz[2];
            acc_y |= raw_xyz[3] << 8;
            acc_z = raw_xyz[4];
            acc_z |= raw_xyz[5] << 8;
            sprintf(buf, "acc_x : %d, acc_y : %d, acc_z : %d\n\r\0", acc_x, acc_y, acc_z);
            buf_len = strlen(buf);
            sci_display_text(UART, (uint8 *)buf, buf_len);
            get_data_flg = FALSE;
        }
        wait(1000000);
    }
/* USER CODE END */

    return 0;
}


/* USER CODE BEGIN (4) */
void rtiNotification(rtiBASE_t *rtiREG, uint32_t notification)
{
    //x축 raw 데이터 수신
    enable_spi_bus(READ, SPI_FMT_0, 0xFE);
    raw_xyz[0] = read_gy_50(MULTIRW_OFF, (uint16_t)OUT_X_L);

    enable_spi_bus(READ, SPI_FMT_0, 0xFE);
    raw_xyz[1] = read_gy_50(MULTIRW_OFF, (uint16_t)OUT_X_H);

    enable_spi_bus(READ, SPI_FMT_0, 0xFE);
    raw_xyz[2] = read_gy_50(MULTIRW_OFF, (uint16_t)OUT_Y_L);

    enable_spi_bus(READ, SPI_FMT_0, 0xFE);
    raw_xyz[3] = read_gy_50(MULTIRW_OFF, (uint16_t)OUT_Y_H);

    enable_spi_bus(READ, SPI_FMT_0, 0xFE);
    raw_xyz[4] = read_gy_50(MULTIRW_OFF, (uint16_t)OUT_Z_L);

    enable_spi_bus(READ, SPI_FMT_0, 0xFE);
    raw_xyz[5] = read_gy_50(MULTIRW_OFF, (uint16_t)OUT_Z_H);

    get_data_flg = TRUE;
}

void sci_display_text (sciBASE_t *sci, uint8 *text, uint32 length)
{
    while (length--)
    {
        while ((UART->FLR & 0x4) == 4)
            ;

        sciSendByte(UART, *text++);
    }
}

void wait (uint32 time)
{
    int i;

    for (i = 0; i < time; i++)
        ;
}
/* USER CODE END */