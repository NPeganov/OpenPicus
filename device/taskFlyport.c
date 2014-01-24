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

void SendAck(cJSON* ID, BOOL isOK);

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
	if(!EstablishHttpConnecion(&conn, BaseUrl, "8010", ConnectionAtteptsNum))
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
	
	while(TRUE)
	{
		FormatCommandPollUrl(url);
		UARTWrite(1, "\r\nGetting commands for the device...");	
		result = SendHttpDataRequest(&conn, url, HTTP_GET, NULL, SERVER_RESPONSE_TIMOUT_SEC);	
		
		
		if(!result.RsponseIsOK || !result.Response)
		{
			UARTWrite(1, "\r\nFailed to get commands for the device...");			
		}
		else
		{	
			cJSON* jResponse = cJSON_Parse(result.Response);
			struct HiveCommand Command;
			while(FetchServerCommand(jResponse, &Command))// will free jResponse if doesn't find any command left
			{
				if(Command.Name 
				&& Command.Name->type == cJSON_String 
				&& Command.ID 
				&& Command.ID->type == cJSON_Number 			
				&& Command.Parameters)
				{
					if(!strcmp(Command.Name->valuestring, "read"))
					{
						// Parsing command
						cJSON* jSlaveID = cJSON_DetachItemFromObject(Command.Parameters, "i");
						unsigned char SlaveAddr = jSlaveID->valueint;cJSON_Delete(jSlaveID);						
						
						cJSON* jRegType = cJSON_DetachItemFromObject(Command.Parameters, "t");	
						unsigned char RegType = jRegType->valueint;cJSON_Delete(jRegType);						
						
						cJSON* jStartAddress = cJSON_DetachItemFromObject(Command.Parameters, "a");
						unsigned short StartAddress = jStartAddress->valueint;cJSON_Delete(jStartAddress);
						
						cJSON* jRegQnty = cJSON_DetachItemFromObject(Command.Parameters, "c");	
						unsigned short RegQnty = jRegQnty->valueint;cJSON_Delete(jRegQnty);						
										
						// Reading registers
						struct ReadRegistersResp mb_res; mb_res.ec = EC_NO_ERROR;
						switch(RegType)
						{
							case FC_Read_Holding_Registers: {
								sprintf(loggBuff, "\n\rReading %d HOLDING registers, starting at 0x%04hX form slave 0x%02hX", RegQnty, StartAddress, SlaveAddr);																		
								mb_res = MBM.ReadHoldingRegisters(SlaveAddr, StartAddress, RegQnty);				
							}break;
							
							case FC_Read_Input_Registers: {
								sprintf(loggBuff, "\n\rReading %d INPUT registers, starting at 0x%04hX form slave 0x%02hX", RegQnty, StartAddress, SlaveAddr);																									
								mb_res = MBM.ReadInputRegisters(SlaveAddr, StartAddress, RegQnty);				
							}break;
						}
						
						// Sending notification
						if(mb_res.ec == EC_NO_ERROR && mb_res.payload != NULL && mb_res.Qnty > 0)
						{					
							FormatNotificationUrl(url);
							cJSON* jParams = cJSON_CreateObject();
							cJSON_AddItemToObject(jParams, "i", cJSON_CreateNumber(SlaveAddr));//SlaveID
							cJSON* jData = cJSON_CreateObject();//Data	
							cJSON_AddItemToObject(jData, "t", cJSON_CreateNumber(RegType));
							cJSON_AddItemToObject(jData, "v", cJSON_CreateIntArray(mb_res.payload, mb_res.Qnty));					
							cJSON_AddItemToObject(jParams, "d", jData);
							cJSON* jNoifyRequestJson = FormNotificationRequest("MODBUS Slave Report", jParams);											
							result = SendHttpJsonRequest(&conn, url, HTTP_POST, jNoifyRequestJson, SERVER_RESPONSE_TIMOUT_SEC);	
							cJSON_Delete(jNoifyRequestJson);										
							free(mb_res.payload);			
							
							SendAck(Command.ID, TRUE);										
						}
						else
						{
							SendAck(Command.ID, FALSE);											
						}
					} //if(!strcmp(Command.Name->valuestring, "read"))
					else if(!strcmp(Command.Name->valuestring, "write")) {
						// Parsing command
						cJSON* jSlaveID = cJSON_DetachItemFromObject(Command.Parameters, "i");
						unsigned char SlaveAddr = jSlaveID->valueint;cJSON_Delete(jSlaveID);						
									
						cJSON* jStartAddress = cJSON_DetachItemFromObject(Command.Parameters, "a");
						unsigned short StartAddress = jStartAddress->valueint;cJSON_Delete(jStartAddress);
						
						cJSON* jData = cJSON_DetachItemFromObject(Command.Parameters, "d");	
						unsigned short RegQnty = cJSON_GetArraySize(jData);
						short* pNewValues = (short*)malloc(sizeof(short) * RegQnty);
						
						unsigned char i = 0;
						for(; i < RegQnty; ++i)
						{
							cJSON* jItem = cJSON_GetArrayItem(jData, i);
							if(jItem) {				
								pNewValues[i] = jItem->valueint;
							}							
						}
						cJSON_Delete(jData);
						
						enum MODBUS_ERROR_CODE res = EC_NO_ERROR;					
						sprintf(loggBuff, "\n\rWriting %d HOLDING registers, starting at 0x%04hX form slave 0x%02hX", RegQnty, StartAddress, (unsigned short)SlaveAddr);																		
						UARTWrite(1,loggBuff);					
						res = MBM.WriteMultipleRegisters(SlaveAddr, StartAddress, RegQnty, pNewValues);	
						free(pNewValues);			
						
						if(res == EC_NO_ERROR)SendAck(Command.ID, TRUE);
						else SendAck(Command.ID, FALSE);
					}//if(!strcmp(Command.Name->valuestring, "write"))
					else if(!strcmp(Command.Name->valuestring, "write1")) {
						UARTWrite(1,"\n\rHANDLING WRITE1\n\r");	
						// Parsing command
						cJSON* jSlaveID = cJSON_DetachItemFromObject(Command.Parameters, "i");
						unsigned char SlaveAddr = jSlaveID->valueint;cJSON_Delete(jSlaveID);						
						
						cJSON* jAddress = cJSON_DetachItemFromObject(Command.Parameters, "a");
						unsigned short Address = jAddress->valueint;cJSON_Delete(jAddress);
						
						cJSON* jValue = cJSON_DetachItemFromObject(Command.Parameters, "v");	
						unsigned short Value = jValue->valueint;cJSON_Delete(jValue);	
						
						enum MODBUS_ERROR_CODE res = EC_NO_ERROR;					
						sprintf(loggBuff, "\n\rWriting value 0x%04hX into HOLDING register at address 0x%04hX to slave 0x%02hX", Value, Address, (unsigned short)SlaveAddr);																		
						UARTWrite(1,loggBuff);					
						res = MBM.WriteSingleRegister(SlaveAddr, Address, Value);							
						
						if(res == EC_NO_ERROR)SendAck(Command.ID, TRUE);
						else SendAck(Command.ID, FALSE);
						
					}// if(!strcmp(Command.Name->valuestring, "write1"))
					else if(!strcmp(Command.Name->valuestring, "ack")) {
						SendAck(Command.ID, TRUE);
					}// if(!strcmp(Command.Name->valuestring, "ack"))				
					else {						
						sprintf(loggBuff, "\n\rUnknown command from server: \"%s\"", Command.Name->valuestring);																		
						UARTWrite(1,loggBuff);											
						SendAck(Command.ID, FALSE);
					}// if(!strcmp(Command.Name->valuestring, "write1"))
					cJSON_Delete(Command.ID);
					cJSON_Delete(Command.Name);			
					cJSON_Delete(Command.Parameters);										
				}// if command is valid and has all necessary data
			}//while(FetchServerCommand(jResponse, &Command))
		}
		vTaskDelay(20);		
	}
}


void SendAck(cJSON* ID, BOOL isOK)
{
	char url[128];url[0] = 0;
	FormatAckUrl(url, ID);
	cJSON* jAck = FormAckRequest(isOK);
	SendHttpJsonRequest(&conn, url, HTTP_PUT, jAck, SERVER_RESPONSE_TIMOUT_SEC);	
	cJSON_Delete(jAck);			
}















