/* **************************************************************************																					
 *                                OpenPicus                 www.openpicus.com
 *                                                            italian concept
 * 
 *            openSource wireless Platform for sensors and Internet of Things	
 * **************************************************************************
 *  FileName:        RS232Helper.c
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
#ifndef __RS232_HELPER_LIB_C
#define __RS232_HELPER_LIB_C

#include "RS232Helper.h"
 
extern int *UMODEs[];
extern int *USTAs[];
extern int *UBRGs[];
extern int *UIFSs[];
extern int *UIECs[];
extern int *UTXREGs[];
extern int *URXREGs[];
 
static int txPin [4] = {0,0,0,0};
static int rxPin [4] = {0,0,0,0};
static int rtsPin[4] = {0,0,0,0};
static int ctsPin[4] = {0,0,0,0};

/**
 * RS232Remap - Remap the provided FLYPORT pins to be used as RS232 serial.
 * \param port - the UART port to initialize. 
 * \param TXPin - the pin used as TX signal. 
 * \param RXPin - the pin used as RX signal.
 * \param RTSPin - the pin used as RTS signal
 * \param CTSPin - the pin used as CTS signal
 * \return None
 */
void RS232Remap(int port, int TXPin, int RXPin, int RTSPin, int CTSPin)
{
	if((port<1) || (port >4))
		return;
	else
	{
		port--;
		txPin[port] = TXPin;
		rxPin[port] = RXPin;
		rtsPin[port] = RTSPin;
		ctsPin[port] = CTSPin;
		
		switch(port)
		{
			case 0:
				IOInit(TXPin,  UART1TX);
				IOInit(RXPin,  UART1RX);
				IOInit(RTSPin, UART1RTS);
				IOInit(CTSPin, UART1CTS);
				break;
			case 1:
				IOInit(TXPin,  UART2TX);
				IOInit(RXPin,  UART2RX);
				IOInit(RTSPin, UART2RTS);
				IOInit(CTSPin, UART2CTS);
				break;
			case 2:
				IOInit(TXPin,  UART3TX);
				IOInit(RXPin,  UART3RX);
				IOInit(RTSPin, UART3RTS);
				IOInit(CTSPin, UART3CTS);
				break;
/*
			case 3:
				IOInit(TXPin,  UART4TX);
				IOInit(RXPin,  UART4RX);
				IOInit(RTSPin, UART4RTS);
				IOInit(CTSPin, UART4CTS);
				break;
*/
		}
	}
}
 
 /**
 * RS232Init - Initializes the specified uart port with the specified baud rate.
 * \param port - the UART port to initialize. <B>Note:</B> at the moment the Flyport Framework supports just one UART, but the hardware allows to create up to four UARTs. Others will be added in next release, however is possible to create them with standard PIC commands.
 * \param baud - the desired baudrate.
 * \return None
 */
void RS232Init(int port,long int baud)
{
	port--;
	long int brg , baudcalc , clk , err;
	clk = GetInstructionClock();
	brg = (clk/(baud*16ul))-1;
	baudcalc = (clk/16ul/(brg+1));
	err = (abs(baudcalc-baud)*100ul)/baud;

	int UMODEval = 0;
	
	if((rtsPin[port] != 0) || (ctsPin[port] != 0))
	{
		UMODEval = (*UMODEs[port] & 0xFE7F);
		UMODEval = (UMODEval | 0x0200); // Enable UxTX, UxRX, /UxCTS and /UxRTS
	}
	else
	{
		UMODEval = (*UMODEs[port] & 0xFE7F);
		UMODEval = (UMODEval | 0x0000); // Enable UxTX and UxRX but not /UxRTS and /UxCTS
	}
	
	if (err<2)
	{
		*UMODEs[port] = (0 | UMODEval);
		*UBRGs[port] = brg;
	}
	else
	{
		brg = (clk/(baud*4ul))-1;
		*UMODEs[port] = (0x8 | UMODEval);
		*UBRGs[port] = brg;
	}
}


void RS232SetParam(int port, int param, int value)
{
	port--;
	
	if(param == RS232_STOP_BITS)
	{
		if(value == RS232_ONE_STOP)
		{
			*UMODEs[port] = (*UMODEs[port] & 0xFFFE);
			*UMODEs[port] = (*UMODEs[port] | RS232_ONE_STOP);
		}
		else if(value == RS232_TWO_STOP)
		{
			*UMODEs[port] = (*UMODEs[port] & 0xFFFE);
			*UMODEs[port] = (*UMODEs[port] | RS232_TWO_STOP);
		}
	}
	else if(param == RS232_DATA_PARITY)
	{
		if(value == RS232_8BITS_PARITY_EVEN)
		{
			*UMODEs[port] = (*UMODEs[port] & 0xFFF9);
			*UMODEs[port] = (*UMODEs[port] | RS232_8BITS_PARITY_EVEN);
		}
		else if(value == RS232_8BITS_PARITY_ODD)
		{
			*UMODEs[port] = (*UMODEs[port] & 0xFFF9);
			*UMODEs[port] = (*UMODEs[port] | RS232_8BITS_PARITY_ODD);		
		}
		else if(value == RS232_8BITS_PARITY_NONE)
		{
			*UMODEs[port] = (*UMODEs[port] & 0xFFF9);
			*UMODEs[port] = (*UMODEs[port] | RS232_8BITS_PARITY_NONE);		
		}
		else if(value == RS232_9BITS_PARITY_NONE)
		{
			*UMODEs[port] = (*UMODEs[port] & 0xFFF9);
			*UMODEs[port] = (*UMODEs[port] | RS232_9BITS_PARITY_NONE);		
		}
	}
}

void RS232On(int port)
{
	UARTOn(port);
}

void RS232Off(int port)
{
	UARTOff(port);
}

void RS232Flush(int port)
{
	UARTFlush(port);
}

int RS232BufferSize(int port)
{
	return UARTBufferSize(port);
}

void RS232Write(int port, char *buffer)
{
	UARTWrite(port, buffer);
}

int RS232Read (int port , char *towrite , int count)
{
	return UARTRead(port, towrite, count);
}

void RS232WriteCh(int port, char chr)
{
	UARTWriteCh(port, chr);
}

#endif //__RS232_HELPER_LIB_C
