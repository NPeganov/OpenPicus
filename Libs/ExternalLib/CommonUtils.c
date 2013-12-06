#include "CommonUtils.h"

void GetError()
{
	switch(LastExecStat())
	{
	case OP_SUCCESS: UARTWrite(1, "Last function executed correctly\r\n");break;
	case OP_EXECUTION: UARTWrite(1, "Function still executing\r\n");break;
	case OP_LL: UARTWrite(1, "evel mode is activate\r\n");break;
	case OP_TIMEOUT: UARTWrite(1, "Timeout error: GPRS module has not answered within the required timeout for the operation\r\n");break;
	case OP_SYNTAX_ERR: UARTWrite(1, "GPRS module reported a syntax error\r\n");break;
	case OP_CMS_ERR: UARTWrite(1, "GPRS module reported a CMS error\r\n");break;
	case OP_CME_ERR: UARTWrite(1, "GPRS module reported a CME error\r\n");break;
	case OP_NO_CARR_ERR: UARTWrite(1, "GPRS module reported NO CARRIER\r\n");break;
	case OP_SMTP_ERR: UARTWrite(1, "Error in sending the email\r\n");break;
	case OP_FTP_ERR: UARTWrite(1, "Error message received in FTP operation\r\n");break;
	case OP_HIB_ERR: UARTWrite(1, "GPRS module is turned off and cannot reply to commands\r\n");break;
	}	
}


BOOL ProcessComand()
{
	do 
	{
		vTaskDelay(20);		
    	IOPut(p21, toggle);				
	}while(LastExecStat() == OP_EXECUTION);
	
	if(LastExecStat() == OP_SUCCESS)
	{
		UARTWrite(1, "Succeed.\r\n");
		return TRUE;
	}
	else
	{
		UARTWrite(1, "Failed.\r\n");		
		GetError();
		return FALSE;		
	}		
}
