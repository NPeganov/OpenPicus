#include "taskFlyport.h"
#include "grovelib.h"
#include "RS485Helper.h"

#include "RS232Helper.h"

#include "ModbusSerial.h"
#include "ModbusMasterRTU.h"
#include "CommonUtils.h"
#include "HTTPUtils.h"
#include "Rest.h"

#include <string.h>

#include "cJSON.h"


#define		TX_232		p4
#define		RX_232		p6
#define		CTS_232		p11
#define		RTS_232		p9

#define		DE_485		p2
#define		RE_485		p17

#define		TX_485		p5
#define		RX_485		p7

extern const struct SerialPort RS232;
extern const struct SerialPort RS485;	
extern const struct ModbusMaster MBM;	
static const int port232 = 2;
static const int port485 = 3;

static const char * BaseUrl = "ecloud.dataart.com";///ecapi8";
//static const char * BaseUrl = "www.google.com";

char loggBuff[255];
TCP_SOCKET conn;

void FlyportTask()
{
	conn.number = INVALID_SOCKET;	
	
	vTaskDelay(20);
    UARTWrite(1,"Flyport Task Started...\r\n");

	// wait for registration success
    UARTWrite(1,"Waiting for registration success...\r\n");	
	
	while(LastConnStatus() != REG_SUCCESS)
	{
		vTaskDelay(20);
		IOPut(p21, toggle);
	}	
    UARTWrite(1,"Registered successfully!\r\n");	

/*	
	// GROVE board
	void *board = new(GroveNest);
	
	// GROVE devices	
	// Digital Input
	void *button = new(Dig_io, IN);
	attachToBoard(board, button, DIG1);	
*/	

    //UARTWrite(1,"Configuring RS485 interface...\r\n");		
	//RS485.Init(port485, 19200, RS485_ONE_STOP, RS485_8BITS_PARITY_NONE);		
	//MBM.Init(&RS485);	
	UARTWrite(1,"Configuring RS232 interface...\r\n");		
	RS232.Init(port232, 19200, RS232_ONE_STOP, RS232_8BITS_PARITY_NONE);
	MBM.Init(&RS232);
	
    UARTWrite(1, "calling APNConfig... ");		
	APNConfig("internet.beeline.ru", "beeline", "beeline", DYNAMIC_IP, DYNAMIC_IP, DYNAMIC_IP);
	ProcessCommand();

	UARTWrite(1,"\r\nConnecting to a server... ");
	const unsigned char ConnectionAtteptsNum = 3;
	if(!EstablishHttpConnecion(&conn, BaseUrl, "80", ConnectionAtteptsNum))
	{
		UARTWrite(1, "Cannot connect to a server.\r\n Sleep and reset...\r\n");	
		vTaskDelay(300);
		Reset();		
	}

	char url[128];url[0] = 0;
	struct HttpResponse result;	

	FormatInfoUrl(url);	
	UARTWrite(1, "Getting server info... ");	
	result = SendHttpDataRequest(&conn, url, HTTP_GET, NULL, 100);	
	if(result.RsponseIsOK && result.Response)
	{
		char* pTimeStamp = 0;
		cJSON* jServerInfo = cJSON_Parse(result.Response);
		pTimeStamp = HandleServerInfo(jServerInfo);
		cJSON_Delete(jServerInfo);		
		if(pTimeStamp)
		{
			RunClock(pTimeStamp);		
		}
	}
	
	FormatRegistrationUrl(url);
	UARTWrite(1, "Sending registration request... ");		
	cJSON * RegRequestJson = FormRegistrationRequest();	
	SendHttpJsonRequest(&conn, url, HTTP_PUT, RegRequestJson, 100);
	cJSON_Delete(RegRequestJson);
	
	const unsigned char SlaveAddr = 1;
	unsigned short StartAddress = 1;
	unsigned short RegQnty = 1;	
	unsigned char RegType = FC_Read_Holding_Registers;		
	
	while(TRUE)
	{
		FormatCommandPollUrl(url);
		UARTWrite(1, "Getting commands for the device... ");	
		result = SendHttpDataRequest(&conn, url, HTTP_GET, NULL, 100);	
		if(result.RsponseIsOK && result.Response)
		{	cJSON* jResponse = cJSON_Parse(result.Response);
			struct HiveCommand Command = HandleServerCommand(jResponse);
			cJSON_Delete(jResponse);			
			if(Command.Name 
			&& Command.Name->type == cJSON_String 
			&& Command.ID 
			&& Command.ID->type == cJSON_Number 			
			&& Command.Parameters 
			&& !strcmp(Command.Name->valuestring, "set"))
			{
				UARTWrite(1, "\r\nGot command to set new parameters.");					
				cJSON* jRegType = cJSON_DetachItemFromObject(Command.Parameters, "RegType");				
				cJSON* jStartAddress = cJSON_DetachItemFromObject(Command.Parameters, "StartAddress");
				cJSON* jRegQnty = cJSON_DetachItemFromObject(Command.Parameters, "RegQnty");	
				cJSON* jResultObj = cJSON_CreateObject();
				
				BOOL sendACK = FALSE;
				
				FormatNotificationUrl(url);					
				if(jRegType && jRegType->type == cJSON_Number)
				{
					RegType = jRegType->valueint;		
					cJSON_Delete(jRegType);						
					UARTWrite(1, "\r\nRequest type changed.");
					cJSON_AddItemToObject(jResultObj, "NewRequestType", cJSON_CreateNumber((double)RegType));	
					sendACK = TRUE;
				}
				
				if(jStartAddress && jStartAddress->type == cJSON_Number)
				{
					StartAddress = jStartAddress->valueint;
					cJSON_Delete(jStartAddress);						
					UARTWrite(1, "\r\nStart address changed.");
					cJSON_AddItemToObject(jResultObj, "NewStartAddress", cJSON_CreateNumber((double)StartAddress));	
					sendACK = TRUE;						
				}	

				if(jRegQnty && jRegQnty->type == cJSON_Number)
				{
					RegQnty = jRegQnty->valueint;
					cJSON_Delete(jRegQnty);					
					UARTWrite(1, "\r\nRegisters quantity changed.");
					cJSON_AddItemToObject(jResultObj, "NewRegistersQnty", cJSON_CreateNumber((double)RegQnty));	
					sendACK = TRUE;						
				}
				
				if(sendACK)
				{
					FormatAckUrl(url, Command.ID);
					cJSON* jAck = FormAckRequest(jResultObj);
					result = SendHttpJsonRequest(&conn, url, HTTP_PUT, jAck, 100);	
					UARTWrite(1, "\r\nDeletting jAck");					
					cJSON_Delete(jAck);		
					UARTWrite(1, "\r\nDeletting Command.ID");											
					cJSON_Delete(Command.ID);
					UARTWrite(1, "\r\nDeletting Command.Name");																
					cJSON_Delete(Command.Name);			
					UARTWrite(1, "\r\nDeletting Command.Parameters");											
					cJSON_Delete(Command.Parameters);	
				}
			}
		}
		
		struct ReadRegistersResp mb_res; mb_res.ec = EC_NO_ERROR;
		switch(RegType)
		{
			case FC_Read_Holding_Registers: {
				mb_res = MBM.ReadHoldingRegisters(SlaveAddr, StartAddress, RegQnty);				
			}break;
			
			case FC_Read_Input_Registers: {
				mb_res = MBM.ReadInputRegisters(SlaveAddr, StartAddress, RegQnty);				
			}break;
		}
		
		if(mb_res.ec == EC_NO_ERROR && mb_res.payload != NULL && mb_res.Qnty > 0)
		{
			FormatNotificationUrl(url);
			unsigned char i = 0;
			for(i = 0; i < mb_res.Qnty; ++i)
			{	
				char textBuf[128];
				sprintf(textBuf, "Value of register 0x%02hhX", i + 1);
				cJSON* jNumericParam = FormParameter(textBuf, (double)mb_res.payload[i]);				
				cJSON* NoifyRequestJson = FormNotificationRequest("MODBUS Slave Report", jNumericParam);
				result = SendHttpJsonRequest(&conn, url, HTTP_POST, NoifyRequestJson, 100);	
				cJSON_Delete(NoifyRequestJson);
			}
			
			free(mb_res.payload);			
		}
		
		vTaskDelay(200);
	}
}












