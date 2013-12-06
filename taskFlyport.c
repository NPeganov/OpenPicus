#include "taskFlyport.h"
#include "grovelib.h"
#include "RS485Helper.h"
#include "ModbusSerial.h"
#include "ModbusMasterRTU.h"
#include "CommonUtils.h"
#include "HTTPUtils.h"
#include "Rest.h"

#include <string.h>

#include "cJSON.h"

extern const struct SerialPort RS485;	
extern const struct ModbusMaster MBM;	
static const int port485 = 2;

static const char * DevId = "b55cf00a-ffff-aaaa-bbbb-8af2a112cb57";
static const char * DevKey = "b55cf00a-ffff-kkkk-bbbb-8af2a112cb57";
static const char * NtwKey = "b55cf00a-nnnn-kkkk-bbbb-8af2a112cb57";
static const char * BaseUrl = "ecloud.dataart.com";///ecapi8";
//static const char * BaseUrl = "www.google.com";

char loggBuff[255];
unsigned char inBuff[2000];
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
		
    UARTWrite(1, "calling APNConfig... ");		
	APNConfig("internet.beeline.ru", "beeline", "beeline", DYNAMIC_IP, DYNAMIC_IP, DYNAMIC_IP);
	ProcessComand();

	UARTWrite(1,"\r\nConnecting to a server... ");
	const unsigned char ConnectionAtteptsNum = 3;
	if(!EstablishHttpConnecion(&conn, BaseUrl, "80", ConnectionAtteptsNum))
	{
		UARTWrite(1, "Cannot connect to a server.\r\n Sleep and reset...");	
		vTaskDelay(300);
		Reset();		
	}

	char url[128];url[0] = 0;
	strcat(url, BaseUrl);
	strcat(url, "/ecapi8");	
	strcat(url, "/device/");	
	strcat(url, DevId);	
	UARTWrite(1, url);		
	UARTWrite(1, "\r\n");			

	UARTWrite(1, "Sending registration request... ");		
	cJSON * RegRequestJson = FormRegistrationRequest();	
	SendHttpJsonRequest(&conn, url, HTTP_PUT, RegRequestJson, inBuff, 100);
	
	for(;;)vTaskDelay(20);
	
	
	//char requestURL[] = "www.google.com/index.html";
	//char data1[] = "";
 
	//HTTPRequest(&conn, HTTP_GET, requestURL, data1, HTTP_NO_PARAM);	
	char *jsonStr = cJSON_PrintUnformatted(RegRequestJson);
	
	UARTWrite(1, "\r\nuformatted JSON\r\n");    
	UARTWrite(1, jsonStr);
	UARTWrite(1, "\r\n");	
	
	HTTPRequest(&conn, HTTP_PUT, url, jsonStr, HTTP_NO_PARAM);
	free(jsonStr);	
	ProcessComand();
	
	UARTWrite(1, "\r\n READING Socket:\r\n");	
	BOOL first_time = TRUE;
	int lenTemp = 0;
	do
	{
		do
		{		
			HTTPStatus(&conn);
			if(ProcessComand())
			{
				UARTWrite(1, "\r\n TCP Socket Status:\r\n");
				sprintf(loggBuff, "Status: %d\r\n", conn.status);
				UARTWrite(1, loggBuff);
				sprintf(loggBuff, "RxLen: %d\r\n", conn.rxLen);
				UARTWrite(1, loggBuff);			
			}
			if(!first_time)break;
		}while(conn.rxLen == 0);

		first_time = FALSE;
		
		if(conn.rxLen == 0)
		{
		    UARTWrite(1, (char*)inBuff);			
			break;
		}
		else
		{
			UARTWrite(1, "\r\n READING REAL DATA:\r\n");			
			HTTPReadData(&conn, (char*)&inBuff[lenTemp], conn.rxLen);
			lenTemp += conn.rxLen;
			sprintf(loggBuff, "lenTemp: %d\r\n", lenTemp);		
			UARTWrite(1, loggBuff);				
			sprintf(loggBuff, "rxLen: %d\r\n", conn.rxLen);						
			UARTWrite(1, loggBuff);				
		}
	}while(1);
	
	//sprintf(temp, "State machine: IDLE, Error type: %d\r\n", errorType);	
	vTaskDelay(20);		
		
	for(;;)vTaskDelay(20);
	
    while((LastConnStatus() != REG_SUCCESS) && (LastConnStatus() != ROAMING))
    {
    	vTaskDelay(20);
    	IOPut(p21, toggle);
    }
    IOPut(p21, on);
	vTaskDelay(20);
    UARTWrite(1,"Flyport registered on network!\r\n");
	
	// GROVE board
	void *board = new(GroveNest);
	
	// GROVE devices	
	// Digital Input
	void *button = new(Dig_io, IN);
	attachToBoard(board, button, DIG1);	
	
	RS485.Init(port485, 19200, RS485_TWO_STOP, RS485_8BITS_PARITY_NONE);
	MBM.Init(&RS485);
	MBM.ReadHRegisters(1,2,2);
	
    while(1)
    {
		MBM_Read(1);
 		// Check pressure of button
		if(get(button) != 0)
		{
			UARTWrite(1, "button pressed!\r\n");
			vTaskDelay(20);
		}
	}
}




