/** \file HILOlib.c
 *  \brief library to manage GSM Connection and IMEI
 */

/**
\addtogroup GSM
@{
*/
/* **************************************************************************																					
 *                                OpenPicus                 www.openpicus.com
 *                                                            italian concept
 * 
 *            openSource wireless Platform for sensors and Internet of Things	
 * **************************************************************************
 *  FileName:        HILOlib.c
 *  Dependencies:    Microchip configs files
 *  Module:          FlyPort GPRS/3G
 *  Compiler:        Microchip C30 v3.12 or higher
 *
 *  Author               Rev.    Date              Comment
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *  Gabriele Allegria    1.0     02/08/2013		   First release  (core team)
 *  Simone Marra
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
/// @cond debug
#include "HWlib.h"
#include "Hilo.h"
#include "HILOlib.h"
#include "GSMData.h"
#include "DATAlib.h"

extern int Cmd;
extern int mainGSMStateMachine;
extern OpStat	mainOpStatus;
extern GSMModule mainGSM;

extern int CheckCmd(int countData, int chars2read, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern int CheckEcho(int countData, const DWORD tick, char* cmdReply, const char* msg2send, const BYTE maxtimeout);
extern void CheckErr(int result, BYTE* smInt, DWORD* tickUpdate);
extern void gsmDebugPrint(char*);

static BYTE smInternal = 0; // State machine for internal use of callback functions
static BYTE maxtimeout = 2;

//static BOOL connVal = FALSE;
/// @endcond

/**
\defgroup HILO
@{
Provides IMEI and Network Connection functions for GSM
*/

/**
 * Provides modem IMEI
 * \param None
 * \return char* with Hilo IMEI
 */
char* GSMGetIMEI()
{
	return (char*)mainGSM.IMEI;
}

/**
 * Turns GSM module OFF
 * \param None
 * \return None
 */
void GSMHibernate()
{
	BOOL opok = FALSE;
	
	if(mainGSM.HWReady != TRUE)
		return;
	if(mainGSMStateMachine == SM_GSM_HIBERNATE)
		return;
	
	//	Function cycles until it is not executed
	while (!opok)
	{
		while (xSemaphoreTake(xSemFrontEnd,0) != pdTRUE);		//	xSemFrontEnd TAKE

		// Check mainOpStatus.ExecStat
		if (mainOpStatus.ExecStat != OP_EXECUTION)
		{		
			mainOpStatus.ExecStat = OP_EXECUTION;
			mainOpStatus.Function = 28;
			mainOpStatus.ErrorCode = 0;
			
			xQueueSendToBack(xQueue,&mainOpStatus.Function,0);	//	Send COMMAND request to the stack
			
			xSemaphoreGive(xSemFrontEnd);						//	xSemFrontEnd GIVE, the stack can answer to the command
			opok = TRUE;
		}
		else
		{
			xSemaphoreGive(xSemFrontEnd);
			taskYIELD();
		}
	}
}


/// @cond debug
int cGSMHibernate()
{
	// AT*PSCPOF -> response: OK
	char cmdReply[200];
	char msg2send[200];
	int resCheck = 0;
	DWORD tick;
	int countData = 0;
	int chars2read = 1;
	
	switch (smInternal)
	{
		case 0:
			if(GSMBufferSize() > 0)
			{
				// Parse Unsol Message
				mainGSMStateMachine = SM_GSM_CMD_PENDING;
				return -1;
			}
			else
				smInternal++;
		
		case 1:
			sprintf(msg2send, "AT*PSCPOF\r");
			GSMWrite(msg2send);
			// Start timeout count
			tick = TickGetDiv64K(); // 1 tick every seconds
			// Real max timeout for this command: 5 seconds
			maxtimeout = 10;
			smInternal++;
			
		case 2:
			vTaskDelay(2);
			// Check ECHO 
			countData = 0;
			resCheck = CheckEcho(countData, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			
		case 3:
			// Get reply (OK, BUSY, ERROR, etc...)
			vTaskDelay(20);

			// Get OK
			sprintf(msg2send, "OK");
			chars2read = 2;
			countData = 2; // GSM buffer should be: <CR><LF>OK<CR><LF>
			resCheck = CheckCmd(countData, chars2read, tick, cmdReply, msg2send, maxtimeout);
			
			CheckErr(resCheck, &smInternal, &tick);
			
			if(resCheck)
			{
				return mainOpStatus.ErrorCode;
			}
			else
			{
				
				// Patch for registration:
				mainGSM.ConnStatus = NO_REG;
				
				HILO_POK_IO = 0;
			   	vTaskDelay(250);
			    HILO_POK_IO = 1;
			    
				int ctsTris = 0;
				if(HILO_CTS_TRIS != 0)
					ctsTris = 0;
				
				HILO_CTS_TRIS = 1;
				int cnt = 30;
				
				while ((cnt > 0)&&(HILO_CTS_IO != 0))
				{
					cnt--;
					vTaskDelay(20);
				}
				HILO_CTS_TRIS = ctsTris;
			}
				
		default:
			break;
	}
	
	smInternal = 0;
	// Cmd = 0 only if the last command successfully executed
	mainOpStatus.ExecStat = OP_SUCCESS;
	mainOpStatus.Function = 0;
	mainOpStatus.ErrorCode = 0;
	mainGSMStateMachine = SM_GSM_HIBERNATE;
	return -1;
	
}
/// @endcond

/**
 * Turns GSM module OFF and PIC microcontroller in Sleep mode 
 * \param None
 * \return None
 */
void GSMSleep()
{
	BOOL opok = FALSE;
	
	if(mainGSM.HWReady != TRUE)
		return;
	
	//	Function cycles until it is not executed
	while (!opok)
	{
		while (xSemaphoreTake(xSemFrontEnd,0) != pdTRUE);		//	xSemFrontEnd TAKE

		// Check mainOpStatus.ExecStat
		if (mainOpStatus.ExecStat != OP_EXECUTION)
		{		
			mainOpStatus.ExecStat = OP_EXECUTION;
			mainOpStatus.Function = 28; //29;
			mainOpStatus.ErrorCode = 0;
			
			xQueueSendToBack(xQueue,&mainOpStatus.Function,0);	//	Send COMMAND request to the stack
			
			xSemaphoreGive(xSemFrontEnd);						//	xSemFrontEnd GIVE, the stack can answer to the command
			opok = TRUE;
			taskYIELD();
		}
		else
		{
			xSemaphoreGive(xSemFrontEnd);
			taskYIELD();
		}
	}
	while (mainOpStatus.ExecStat == OP_EXECUTION);		// wait for callback Execution...	

	if(mainGSMStateMachine == SM_GSM_HIBERNATE)
	{
		// Set Sleep for PIC 
		// PIC sleep mode
		RCONbits.VREGS = 0;
		vTaskDelay(50);
		asm("PWRSAV #0");
	}
}

/**
 * Turns GSM module ON after hibernation or sleep 
 * \param None
 * \return None
 */
void GSMOn()
{
	BOOL opok = FALSE;
	
	if(mainGSM.HWReady != TRUE)
		return;
	if(mainGSMStateMachine != SM_GSM_HIBERNATE)
		return;
	
	//	Function cycles until it is not executed
	while (!opok)
	{
		while (xSemaphoreTake(xSemFrontEnd,0) != pdTRUE);		//	xSemFrontEnd TAKE

		// Check mainOpStatus.ExecStat
		if (mainOpStatus.ExecStat != OP_EXECUTION)
		{		
			mainOpStatus.ExecStat = OP_EXECUTION;
			mainOpStatus.Function = 29;
			mainOpStatus.ErrorCode = 0;
			
			xQueueSendToBack(xQueue,&mainOpStatus.Function,0);	//	Send COMMAND request to the stack
			
			xSemaphoreGive(xSemFrontEnd);						//	xSemFrontEnd GIVE, the stack can answer to the command
			opok = TRUE;
		}
		else
		{
			xSemaphoreGive(xSemFrontEnd);
			taskYIELD();
		}
	}
}

/// @cond debug
int cGSMOn()
{
	while(cSTDModeEnable());

	mainGSMStateMachine = SM_GSM_IDLE;
	return -1;
}
/// @endcond

/*! @} */

/*! @} */

