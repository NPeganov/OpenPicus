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

    UARTWrite(1,"Configuring RS485 interface...\r\n");	
	// GROVE board
	void *board = new(GroveNest);
	
	// GROVE devices	
	// Digital Input
	void *button = new(Dig_io, IN);
	attachToBoard(board, button, DIG1);	
	
	RS485.Init(port485, 19200, RS485_TWO_STOP, RS485_8BITS_PARITY_NONE);
	MBM.Init(&RS485);
	
	MBM.ReadHRegisters(1, 0, 1);	
	for(;;)vTaskDelay(20);
	
	
	
	
    UARTWrite(1, "calling APNConfig... ");		
	APNConfig("internet.beeline.ru", "beeline", "beeline", DYNAMIC_IP, DYNAMIC_IP, DYNAMIC_IP);
	ProcessCommand();

	UARTWrite(1,"\r\nConnecting to a server... ");
	const unsigned char ConnectionAtteptsNum = 3;
	if(!EstablishHttpConnecion(&conn, BaseUrl, "80", ConnectionAtteptsNum))
	{
		UARTWrite(1, "Cannot connect to a server.\r\n Sleep and reset...");	
		vTaskDelay(300);
		Reset();		
	}

	char url[128];url[0] = 0;
	struct HttpResponse result;	

	FormatInfoUrl(url);	
	UARTWrite(1, "Getting server info... ");	
	result = SendHttpDataRequest(&conn, url, HTTP_GET, NULL, inBuff, 100);	
	if(result.RsponseIsOK && result.Response)
	{
		char* pTimeStamp = 0;
		pTimeStamp = HandleServerInfo(cJSON_Parse(result.Response));
		if(pTimeStamp)
		{
			RunClock(pTimeStamp);		
		}
	}
	
	FormatRegistrationUrl(url);
	UARTWrite(1, "Sending registration request... ");		
	cJSON * RegRequestJson = FormRegistrationRequest();	
	SendHttpJsonRequest(&conn, url, HTTP_PUT, RegRequestJson, inBuff, 100);
	
	while(TRUE)
	{
		FormatCommandPollUrl(url);
		UARTWrite(1, "Getting commands for the device... ");	
		result = SendHttpDataRequest(&conn, url, HTTP_GET, NULL, inBuff, 100);	
		if(result.RsponseIsOK && result.Response)
		{	
			struct HiveCommand Command = HandleServerCommand(cJSON_Parse(result.Response));
		}
		
		MBM.ReadHRegisters(1, 0, 1);			
		

		FormatNotificationUrl(url);
		UARTWrite(1, "Sending device notification... ");	
		cJSON * NoifyRequestJson = FormNotificationRequest(3.1416);			
		result = SendHttpJsonRequest(&conn, url, HTTP_POST, NoifyRequestJson, inBuff, 100);			
		
		vTaskDelay(200);
	}
}






