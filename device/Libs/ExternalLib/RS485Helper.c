/* **************************************************************************																					
 *                                OpenPicus                 www.openpicus.com
 *                                                            italian concept
 * 
 *            openSource wireless Platform for sensors and Internet of Things	
 * **************************************************************************
 *  FileName:        RS485Helper.c
 *  Dependencies:    Microchip configs files
 *  Module:          FlyPort WI-FI
 *  Compiler:        Microchip C30 v3.12 or higher
 *
 *  Author               Rev.    Date              Comment
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Simone Marra	     1.0     5/23/2012		   First release  (core team)
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  Software License Agreement
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  This is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License (version 2) as published by 
 *  the Free Software Foundation AND MODIFIED BY OpenPicus team.
 *  
 *  ***NOTE*** The exception to the GPL is included to allow you to distribute
 *  a combined work that includes OpenPicus code without being obliged to 
 *  provide the source code for proprietary components outside of the OpenPicus
 *  code. 
 *  OpenPicus software is distributed in the hope that it will be useful, but 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details. 
 * 
 * 
 * Warranty
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * WE ARE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 **************************************************************************/
#ifndef __RS485_HELPER_LIB_C
#define __RS485_HELPER_LIB_C

#include "RS485Helper.h"
 
extern int *UMODEs[];
extern int *USTAs[];
extern int *UBRGs[];
extern int *UIFSs[];
extern int *UIECs[];
extern int *UTXREGs[];
extern int *URXREGs[]; 

static int wEnPin[4] = {0,0,0,0};
static int rEnPin[4] = {0,0,0,0};

/**
 * RS485Remap - Remap the provided FLYPORT pins to be used as RS485 serial.
 * \param port - the UART port to initialize. 
 * \param TXPin - the pin used as TX signal.
 * \param RXPin - the pin used as RX signal.
 * \param writeEnPin - the pin used as Write Enable signal.
 * \param readEnPin - the pin used as Read Enable signal.
 * \return None
 */
void RS485Remap(int port, int TXPin, int RXPin, int writeEnPin, int readEnPin)
{
	if((port<1) || (port >4))
		return;
	else
	{
		port--;
		
		rEnPin[port] = readEnPin;
		IOInit((int)rEnPin[port], out);
		IOPut((int)rEnPin[port], off);
		wEnPin[port] = writeEnPin;
		IOInit((int)wEnPin[port], out);
		IOPut((int)wEnPin[port], off);
		
		switch(port)
		{
			case 0:
				IOInit(TXPin, UART1TX);
				IOInit(RXPin, UART1RX);
				break;
			case 1:
				IOInit(TXPin, UART2TX);
				IOInit(RXPin, UART2RX);
				break;
			case 2:
				IOInit(TXPin, UART3TX);
				IOInit(RXPin, UART3RX);
				break;
				/*
			case 3:
				IOInit(TXPin, UART4TX);
				IOInit(RXPin, UART4RX);
				break;
				*/
		}
	}
}

 /**
 * RS485Init - Initializes the specified uart port with the specified baud rate to be used as RS485 serial.
 * \param port - the UART port to initialize. 
 * \param baud - the desired baudrate.
 * \param writeEnPin - the pin used as Write Enable signal
 * \param readEnPin - the pin used as Read Enable signal
 * \return None
 */
void RS485Init(int port,long int baud)
{
	UARTInit(port, baud);
}

void RS485SetParam(int port, int param, int value)
{
	port--;
	
	if(param == RS485_STOP_BITS)
	{
		if(value == RS485_ONE_STOP)
		{
			*UMODEs[port] = (*UMODEs[port] & 0xFFFE);
			*UMODEs[port] = (*UMODEs[port] | RS485_ONE_STOP);
		}
		else if(value == RS485_TWO_STOP)
		{
			*UMODEs[port] = (*UMODEs[port] & 0xFFFE);
			*UMODEs[port] = (*UMODEs[port] | RS485_TWO_STOP);
		}
	}
	else if(param == RS485_DATA_PARITY)
	{
		if(value == RS485_8BITS_PARITY_EVEN)
		{
			*UMODEs[port] = (*UMODEs[port] & 0xFFF9);
			*UMODEs[port] = (*UMODEs[port] | RS485_8BITS_PARITY_EVEN);
		}
		else if(value == RS485_8BITS_PARITY_ODD)
		{
			*UMODEs[port] = (*UMODEs[port] & 0xFFF9);
			*UMODEs[port] = (*UMODEs[port] | RS485_8BITS_PARITY_ODD);		
		}
		else if(value == RS485_8BITS_PARITY_NONE)
		{
			*UMODEs[port] = (*UMODEs[port] & 0xFFF9);
			*UMODEs[port] = (*UMODEs[port] | RS485_8BITS_PARITY_NONE);		
		}
		else if(value == RS485_9BITS_PARITY_NONE)
		{
			*UMODEs[port] = (*UMODEs[port] & 0xFFF9);
			*UMODEs[port] = (*UMODEs[port] | RS485_9BITS_PARITY_NONE);		
		}
	}
}

void RS485On(int port)
{
	UARTOn(port);
}

void RS485Off(int port)
{
	UARTOff(port);
}

void RS485Flush(int port)
{
	UARTFlush(port);
}

int RS485BufferSize(int port)
{
	return UARTBufferSize(port);
}

void RS485Write(int port, char *buffer)
{
	IOPut((int)wEnPin[port-1], on);
	IOPut((int)rEnPin[port-1], on);
	vTaskDelay(1);
	UARTWrite(port, buffer);
	vTaskDelay(1);
	IOPut((int)wEnPin[port-1], off);
	IOPut((int)rEnPin[port-1], off);
}

int RS485Read (int port , char *towrite , int count)
{
	return UARTRead(port, towrite, count);
}

void RS485WriteCh(int port, char chr)
{
	IOPut((int)wEnPin[port-1], on);
	IOPut((int)rEnPin[port-1], on);
	vTaskDelay(1);
	UARTWriteCh(port, chr);
	vTaskDelay(1);
	IOPut((int)wEnPin[port-1], off);
	IOPut((int)rEnPin[port-1], off);
}

#endif //__RS485_HELPER_LIB_C

