/*
 * msc_app.c
 *
 *  Created on: Feb 19, 2020
 *      Author: ibrahim.erturk
 */

#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_msc.h"
#include "ff.h"

#include <string.h>
#include <stdbool.h>

#include <uart_drv.h>

#include "queue.h"

#define MSCAPP_THREAD_PRIO  ( tskIDLE_PRIORITY + 5 )

osThreadId mscTaskHandle;


FIL myFile;
FRESULT res;
UINT byteswritten, bytesread;
char rwtext[256];
uint8_t sub_state = 0;

extern ApplicationTypeDef Appli_state;
extern USBH_HandleTypeDef hUsbHostHS;
extern char USBHPath[4];
extern FATFS USBHFatFS;


tiva_msg_t mscUartBuff = {};
extern QueueHandle_t serBuffQueueHandle;

bool UsbTest_Write(void)
{
	//Open or Create file for writing
	if(f_open(&myFile, "TEST2.TXT", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
	{
		return 0;
	}
	//Copy test Text to my temporary read/write buffer
	sprintf(rwtext, "Hello from STM32F4 Discovery");
	//Write to text file
	res = f_write(&myFile, (const void *)rwtext, strlen(rwtext), &byteswritten);
	if((res != FR_OK) || (byteswritten == 0))
	{
		return 0;
	}

	f_close(&myFile);
	return 1; //Success


}

//2. USB test Read function
bool UsbTest_Read(void)
{
	//Open file for reading
	if(f_open(&myFile, "TEST2.TXT", FA_READ) != FR_OK)
	{
		return 0;
	}

	//Read text from files until NULL
	for(uint8_t i=0; i<100; i++)
	{
		res = f_read(&myFile, (uint8_t*)&rwtext[i], 1, &bytesread);
		if(rwtext[i] == 0x00) // NULL string
		{
			bytesread = i;
			break;
		}
	}
	//Reading error handling
	if(bytesread==0) return 0;

	//Close file
	f_close(&myFile);
	return 1;  // success

}


bool msc_create_file(void)
{
	//Open or Create file for writing
	if(f_open(&myFile, "TEST01.TXT", FA_OPEN_APPEND | FA_WRITE) != FR_OK)
	{
		return 0;
	}

	res = f_lseek(&myFile, f_size(&myFile));
	// f_close(&myFile);
	return 1; //Success
}

bool msc_write_test_data(void)
{
	static uint16_t i = 0;

	//Copy test Text to my temporary read/write buffer
	sprintf(rwtext, "%d - Hello from STM32F4 Discovery..............\r\n", i++);
	//Write to text file
	res = f_write(&myFile, (const void *)rwtext, strlen(rwtext), &byteswritten);
	if((res != FR_OK) || (byteswritten == 0))
	{
		return 0;
	}
	return 1; //Success
}


bool msc_write_data(void)
{
	while(xQueueReceive(serBuffQueueHandle, &mscUartBuff, portMAX_DELAY)) {
		// int len = mscUartBuff.len;
		// mscUartBuff.buff[len] = 0;
		res = f_write(&myFile, (const void *)mscUartBuff.buff, mscUartBuff.len, &byteswritten);
		if((res != FR_OK) || (byteswritten == 0))
		{
			// return 0;
		}
		f_sync(&myFile);
	}
	return 1;
}

void mscapp_thread(void)
{
	switch(Appli_state)
	{
		case APPLICATION_IDLE:
			break;

		case APPLICATION_START:
			if(f_mount(&USBHFatFS, (TCHAR const*)USBHPath, 0) == FR_OK)
			{
				sub_state = 0;
			}
			break;

		case APPLICATION_READY:

			switch(sub_state) {
			case 0:
				msc_create_file();
				sub_state = 1;
				break;

			case 1:
				msc_write_data();
				// msc_write_test_data();
				// f_sync(&myFile);
				break;


			default:
				break;

			}
			break;

		case APPLICATION_DISCONNECT:
			break;
	}
}
