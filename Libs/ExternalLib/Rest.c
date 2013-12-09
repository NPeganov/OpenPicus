#include "Rest.h"

static const char * DevId = "b55cf00a-ffff-aaaa-bbbb-8af2a112cb57";
static const char * DevKey = "b55cf00a-ffff-kkkk-bbbb-8af2a112cb57";
static const char * NtwKey = "b55cf00a-nnnn-kkkk-bbbb-8af2a112cb57";
static const char * BaseUrl = "ecloud.dataart.com";///ecapi8";
static const char * Prefix = "/ecapi8/";

static char Timestamp[28];


cJSON* FormRegistrationRequest()
{
	cJSON *root = cJSON_CreateObject();
    cJSON *network = cJSON_CreateObject();	
    cJSON *deviceClass = cJSON_CreateObject();		
    cJSON *data = cJSON_CreateObject();			
    cJSON *devClassData = cJSON_CreateObject();	
	       
	cJSON_AddItemToObject(root, "id", cJSON_CreateString(DevId));
	cJSON_AddItemToObject(root, "key", cJSON_CreateString(DevKey));	
	cJSON_AddItemToObject(root, "name", cJSON_CreateString("OpenPicus MODBUS RTU Master Device"));	
	cJSON_AddItemToObject(root, "status", cJSON_CreateString("Device is running"));
	
	cJSON_AddItemToObject(data, "IMEI", cJSON_CreateString(GSMGetIMEI()));		
	cJSON_AddItemToObject(root, "data", data);		

	//cJSON_AddItemToObject(network, "id", cJSON_CreateNumber(235));
	cJSON_AddItemToObject(network, "key", cJSON_CreateString(""));	
	cJSON_AddItemToObject(network, "name", cJSON_CreateString("OpenPicus MODBUS Network"));	
	cJSON_AddItemToObject(network, "description", cJSON_CreateString("OpenPicus MODBUS Network"));	
	cJSON_AddItemToObject(root, "network", network);
		
	//cJSON_AddItemToObject(deviceClass, "id", cJSON_CreateNumber(5323));
	cJSON_AddItemToObject(deviceClass, "name", cJSON_CreateString("OpenPicus MODBUS RTU Master Class"));	
	cJSON_AddItemToObject(deviceClass, "version", cJSON_CreateString("OpenPicus MODBUS Version 1.0.1"));	
	cJSON_AddItemToObject(deviceClass, "isPermanent", cJSON_CreateBool(0));		
	cJSON_AddItemToObject(deviceClass, "offlineTimeout", cJSON_CreateNumber(60 * 30));			
	cJSON_AddItemToObject(deviceClass, "data", devClassData);				
	cJSON_AddItemToObject(root, "deviceClass", deviceClass);

	return root;	
}

char* FormatRegistrationUrl(char* Buffer)
{
	strcpy(Buffer, BaseUrl);
	strcat(Buffer, Prefix);	
	strcat(Buffer, "device/");	
	strcat(Buffer, DevId);	
	return Buffer;
}

char* FormatInfoUrl(char* Buffer)
{
	strcpy(Buffer, BaseUrl);
	strcat(Buffer, Prefix);	
	strcat(Buffer, "info/");	
	return Buffer;
}

char* FormatCommandPollUrl(char* Buffer)
{
	strcpy(Buffer, BaseUrl);
	strcat(Buffer, Prefix);	
	strcat(Buffer, "device/");
	strcat(Buffer, DevId);		
	strcat(Buffer, "/command/poll\?timestamp=");		
	strcat(Buffer, Timestamp);		
	strcat(Buffer, "&waitTimeout=5");
	return Buffer;
}

char* HandleServerInfo(cJSON* json)
{
	if (!json)  
	{
		char *error = (char*)cJSON_GetErrorPtr();
		UARTWrite(1,"\n\r An error was encountered\n\r");
		UARTWrite(1,error);
		return NULL;
	}	
	else
	{	
		cJSON *timestampJSON = cJSON_GetObjectItem(json, "serverTimestamp");
		
		if(timestampJSON)
		{
			strcpy(Timestamp, timestampJSON->valuestring);		
			UARTWrite(1, "\n\rServer timestamp\n\r");
			UARTWrite(1, Timestamp);	
			UARTWrite(1, "\n\r");
			return Timestamp;
		}
		else
		{
			char *error = (char*)cJSON_GetErrorPtr();
			UARTWrite(1,"\n\r An error was encountered\n\r");
			UARTWrite(1,error);	
			return NULL;
		}
	}		
	return NULL;
}
