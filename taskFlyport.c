#include "taskFlyport.h"
#include "grovelib.h"
#include "RS485Helper.h"
#include "ModbusSerial.h"
#include "ModbusMasterRTU.h"

#include <string.h>

#include "cJSON.h"

extern const struct SerialPort RS485;	
extern const struct ModbusMaster MBM;	
static const int port485 = 2;

static const char * DevId = "b55cf00a-FFFF-AAAA-BBBB-8af2a112cb57";
static const char * DevKey = "b55cf00a-FFFF-AAAA-BBBB-8af2a112cb57";
//static const char * BaseUrl = "http://ecloud.dataart.com/ecapi8";
static const char * BaseUrl = "http://httpbin.org";

void FlyportTask()
{
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
		
	TCP_SOCKET conn;
	HTTPOpen(&conn, BaseUrl, "80");
    
	UARTWrite(1,"Connecting to a server!\r\n");	
	while(LastExecStat() == OP_EXECUTION)
	{
    	vTaskDelay(20);
    	IOPut(p21, toggle);
	}
	// Check on success for the operation
	if(LastExecStat() == OP_SUCCESS)
		UARTWrite(1, "Connected");
	else
		UARTWrite(1, "Failed to connect");	

	cJSON *root = cJSON_CreateObject();
    cJSON *network = cJSON_CreateObject();	
    cJSON *deviceClass = cJSON_CreateObject();		
    cJSON *data = cJSON_CreateObject();			
    cJSON *devClassData = cJSON_CreateObject();	
	       
	/*add  objects to the JSON */
	//cJSON_AddItemToObject(root,"menu",menu_1);
	cJSON_AddItemToObject(root, "id", cJSON_CreateString(DevId));
	cJSON_AddItemToObject(root, "key", cJSON_CreateString("SomeDevKey"));	
	cJSON_AddItemToObject(root, "name", cJSON_CreateString("OpenPicus MODBUS RTU Master Device"));	
	cJSON_AddItemToObject(root, "status", cJSON_CreateString("Device is running"));	
	
	cJSON_AddItemToObject(root, "data", data);		

	cJSON_AddItemToObject(network, "id", cJSON_CreateNumber(235));
	cJSON_AddItemToObject(network, "key", cJSON_CreateString("SomeNetworkKey"));	
	cJSON_AddItemToObject(network, "name", cJSON_CreateString("OpenPicus MODBUS Network"));	
	cJSON_AddItemToObject(network, "description", cJSON_CreateString("OpenPicus MODBUS Network"));	
	cJSON_AddItemToObject(root, "network", network);
		
	cJSON_AddItemToObject(deviceClass, "id", cJSON_CreateNumber(5323));
	cJSON_AddItemToObject(deviceClass, "name", cJSON_CreateString("OpenPicus MODBUS RTU Master Class"));	
	cJSON_AddItemToObject(deviceClass, "version", cJSON_CreateString("OpenPicus MODBUS Version 1.0.1"));	
	cJSON_AddItemToObject(deviceClass, "isPermanent", cJSON_CreateBool(0));		
	cJSON_AddItemToObject(deviceClass, "offlineTimeout", cJSON_CreateNumber(60 * 30));			
	cJSON_AddItemToObject(deviceClass, "data", devClassData);				
	cJSON_AddItemToObject(root, "deviceClass", deviceClass);
	
	/*print the JSON  created*/
	char *s_out = cJSON_Print(root);
	UARTWrite(1, "\r\nthe JSON created\r\n");    
	UARTWrite(1, s_out);
	UARTWrite(1, "\r\n");		

	/*
	char url[128];url[0] = 0;
	strcat(url, BaseUrl);
	strcat(url, "/device/");
	strcat(url, DevId);	
	*/
	
	char url[128];url[0] = 0;
	strcat(url, BaseUrl);
	strcat(url, "/ip/");
	//strcat(url, DevId);	
	
	UARTWrite(1, url);		
	UARTWrite(1, "\r\n");			
	
	HTTPRequest(&conn, HTTP_GET, url, 0, "application/x-www-form-urlencoded");	
	
	UARTWrite(1, "Sending registration request!\r\n");	
	while(LastExecStat() == OP_EXECUTION)
	{
    	vTaskDelay(20);
    	IOPut(p21, toggle);
	}
	// Check on success for the operation
	if(LastExecStat() == OP_SUCCESS)
		UARTWrite(1, "registration request succsessfully sent!\r\n");
	else
	{
		UARTWrite(1, "Failed to send registration request\r\n");	
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
	
	//sprintf(temp, "State machine: IDLE, Error type: %d\r\n", errorType);	
	
	/*free the memory allocated*/
	free(s_out);
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






