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

#define SERVER_RESPONSE_TIMOUT_SEC 60

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
	result = SendHttpDataRequest(&conn, url, HTTP_GET, NULL, SERVER_RESPONSE_TIMOUT_SEC);	
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
	
	BOOL ReportStarted = FALSE;	
	
	while(TRUE)
	{
		FormatCommandPollUrl(url);
		UARTWrite(1, "\r\nGetting commands for the device...");	
		result = SendHttpDataRequest(&conn, url, HTTP_GET, NULL, SERVER_RESPONSE_TIMOUT_SEC);	
		
		BOOL sendACK = FALSE;		
		
		if(!result.RsponseIsOK || !result.Response)
		{
			UARTWrite(1, "\r\nFailed to get commands for the device...");			
		}
		else
		{	
			cJSON* jResponse = cJSON_Parse(result.Response);
			struct HiveCommand Command = HandleServerCommand(jResponse);
			cJSON_Delete(jResponse);			
			if(Command.Name 
			&& Command.Name->type == cJSON_String 
			&& Command.ID 
			&& Command.ID->type == cJSON_Number 			
			&& Command.Parameters)
			{
				cJSON* jResultObj = cJSON_CreateObject();
				if(!strcmp(Command.Name->valuestring, "set") || !strcmp(Command.Name->valuestring, "start"))
				{
					BOOL isSet = !strcmp(Command.Name->valuestring, "set");
					if(isSet)
						UARTWrite(1, "\r\nGot command to set new parameters.");					
					else
					{
						UARTWrite(1, "\r\nGot command to start mesurment loop.");	
						ReportStarted = TRUE;
					}
						
					cJSON* jRegType = cJSON_DetachItemFromObject(Command.Parameters, "RegType");				
					cJSON* jStartAddress = cJSON_DetachItemFromObject(Command.Parameters, "StartAddress");
					cJSON* jRegQnty = cJSON_DetachItemFromObject(Command.Parameters, "RegQnty");						
				
					FormatNotificationUrl(url);					
					if(jRegType && jRegType->type == cJSON_Number)
					{
						RegType = jRegType->valueint;		
						cJSON_Delete(jRegType);						
						UARTWrite(1, "\r\nRequest type changed.");
						if(isSet)
							cJSON_AddItemToObject(jResultObj, "NewRequestType", cJSON_CreateNumber((double)RegType));	
						else
							cJSON_AddItemToObject(jResultObj, "InitialRequestType", cJSON_CreateNumber((double)RegType));	
							
						sendACK = TRUE;
					}
					
					if(jStartAddress && jStartAddress->type == cJSON_Number)
					{
						StartAddress = jStartAddress->valueint;
						cJSON_Delete(jStartAddress);						
						UARTWrite(1, "\r\nStart address changed.");
						if(isSet)						
							cJSON_AddItemToObject(jResultObj, "NewStartAddress", cJSON_CreateNumber((double)StartAddress));	
						else
							cJSON_AddItemToObject(jResultObj, "InitialStartAddress", cJSON_CreateNumber((double)StartAddress));	
							
						sendACK = TRUE;						
					}	

					if(jRegQnty && jRegQnty->type == cJSON_Number)
					{
						RegQnty = jRegQnty->valueint;
						cJSON_Delete(jRegQnty);					
						UARTWrite(1, "\r\nRegisters quantity changed.");
						if(isSet)
							cJSON_AddItemToObject(jResultObj, "NewRegistersQnty", cJSON_CreateNumber((double)RegQnty));	
						else
							cJSON_AddItemToObject(jResultObj, "InitialRegistersQnty", cJSON_CreateNumber((double)RegQnty));	
						sendACK = TRUE;						
					}			
					
				}		
				
				if(sendACK)
				{
					FormatAckUrl(url, Command.ID);
					cJSON* jAck = FormAckRequest(jResultObj);
					result = SendHttpJsonRequest(&conn, url, HTTP_PUT, jAck, SERVER_RESPONSE_TIMOUT_SEC);	
					cJSON_Delete(jAck);		
				}	
				
				cJSON_Delete(Command.ID);
				cJSON_Delete(Command.Name);			
				cJSON_Delete(Command.Parameters);										
			}
		
			if(ReportStarted)
			{
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
					cJSON* jValuesArray = cJSON_CreateArray();					
					cJSON* jParams = cJSON_CreateObject();
					cJSON_AddItemToObject(jParams, "SlaveID", cJSON_CreateNumber((double)SlaveAddr));						
					cJSON_AddItemToObject(jParams, "Data", jValuesArray);						
					cJSON* jNoifyRequestJson = FormNotificationRequest("MODBUS Slave Report", jParams);					
					for(i = 0; i < mb_res.Qnty; ++i)
					{
						cJSON* jRegister = cJSON_CreateObject();										
						char addrBuf[5];
						sprintf(addrBuf, "0x%02hhX", StartAddress + i);		
						cJSON_AddItemToObject(jRegister, "Addr", cJSON_CreateString(addrBuf));										
						cJSON_AddItemToObject(jRegister, "Val", cJSON_CreateNumber((double)mb_res.payload[i]));						
						cJSON_AddItemToArray(jValuesArray, jRegister);												
					}
					
					result = SendHttpJsonRequest(&conn, url, HTTP_POST, jNoifyRequestJson, SERVER_RESPONSE_TIMOUT_SEC);	
					cJSON_Delete(jNoifyRequestJson);										
					free(mb_res.payload);			
				}
			}
			
			vTaskDelay(200);
		}
	}
}












