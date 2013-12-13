/* **************************************************************************																					
 *                                OpenPicus                 www.openpicus.com
 *                                                            italian concept
 * 
 *            openSource wireless Platform for sensors and Internet of Things	
 * **************************************************************************
 *  FileName:        RS485Helper.h
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
#include "HWlib.h"
#include "p24FJ256GA106.h"
#include "Tick.h"

#define		RS485_STOP_BITS		0
#define		RS485_DATA_PARITY	1

#define		RS485_ONE_STOP 		0
#define		RS485_TWO_STOP		1

#define		RS485_8BITS_PARITY_EVEN 4
#define		RS485_8BITS_PARITY_ODD  2
#define		RS485_8BITS_PARITY_NONE	0
#define		RS485_9BITS_PARITY_NONE	6
  
void RS485Remap(int port, int TXPin, int RXPin, int writeEnPin, int readEnPin);
void RS485Init(int port,long int baud);
void RS485SetParam(int port, int param, int value);
void RS485On(int port);
void RS485Off(int port);
void RS485Flush(int port);
int RS485BufferSize(int port);
void RS485Write(int port, char *buffer); 
int RS485Read (int port, char* towrite, int count);
void RS485WriteCh(int port, char chr);


