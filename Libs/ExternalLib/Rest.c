#include "Rest.h"
#include "CommonUtils.h"

static const char * DevId = "b55cf00a-ffff-aaaa-bbbb-8af2a112cb57";
static const char * DevKey = "b55cf00a-ffff-kkkk-bbbb-8af2a112cb57";

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

cJSON* FormNotificationRequest(const char * Name, cJSON* Parameters)
{
	//UARTWrite(1, "\r\nFormNotificationRequest...");
	cJSON *root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "id", cJSON_CreateNumber(TickGet()));
	char time_dest[32];
	GetClockValue(time_dest);	
	cJSON_AddItemToObject(root, "datetime", cJSON_CreateString(time_dest));	
	cJSON_AddItemToObject(root, "notification", cJSON_CreateString(Name));
	cJSON_AddItemToObject(root, "parameters", Parameters);
	char *s_print = cJSON_Print(root);
	UARTWrite(1, "\r\nNOTIFICATION CREATED:\r\n");		
	UARTWrite(1, s_print);	
	free(s_print);
		
	//UARTWrite(1, "\r\n...FormNotificationRequest");		
	return root;	
}

cJSON* FormAckRequest(BOOL isResOK)
{
	//UARTWrite(1, "\r\nFormAckRequest...");
	cJSON *root = cJSON_CreateObject();
	if(isResOK)
	{
		cJSON_AddItemToObject(root, "status", cJSON_CreateString("success"));		
		cJSON_AddItemToObject(root, "r", cJSON_CreateNumber((double)1));	
	}
	else
	{
		cJSON_AddItemToObject(root, "status", cJSON_CreateString("failed"));		
		cJSON_AddItemToObject(root, "r", cJSON_CreateNumber((double)(0)));	
	}
	//UARTWrite(1, "\r\n...FormAckRequest");		
	return root;	
}

cJSON* FormParameter(const char* Name, double value)
{
	//UARTWrite(1, "\r\nFormParameter...");	
	cJSON *root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, Name, cJSON_CreateNumber(value));
	
	//UARTWrite(1, "\r\n...FormParameter");			
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

char* FormatNotificationUrl(char* Buffer)
{
	strcpy(Buffer, BaseUrl);
	strcat(Buffer, Prefix);	
	strcat(Buffer, "device/");
	strcat(Buffer, DevId);		
	strcat(Buffer, "/notification");		
	return Buffer;
}

char* FormatAckUrl(char* Buffer, cJSON* jID)
{
	strcpy(Buffer, BaseUrl);
	strcat(Buffer, Prefix);	
	strcat(Buffer, "device/");
	strcat(Buffer, DevId);		
	strcat(Buffer, "/command/");
	
	char* sID = cJSON_Print(jID);	
	strcat(Buffer, sID);			
	free(sID);
	
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
		return FetchTimeStamp(timestampJSON);
	}		
	return NULL;
}

void SetLastTimeStamp(const char* NewTimeStamp)
{
	strcpy(Timestamp, NewTimeStamp);			
}

const char* GetLastTimeStamp()
{
	return Timestamp;			
}

char* FetchTimeStamp(cJSON * TimestampFromServer)
{
	if(TimestampFromServer)
	{
		SetLastTimeStamp(TimestampFromServer->valuestring);		
		UARTWrite(1, "\n\rServer timestamp\n\r");
		UARTWrite(1, GetLastTimeStamp());	
		UARTWrite(1, "\n\r");
		return GetLastTimeStamp();
	}
	else
	{
		char *error = (char*)cJSON_GetErrorPtr();
		UARTWrite(1,"\n\r An error was encountered\n\r");
		UARTWrite(1,error);	
		UARTWrite(1,"\n\r Failed to update last timestamp\n\r");		
		return NULL;
	}		
}


struct HiveCommand HandleServerCommand(cJSON* json)
{
	struct HiveCommand res;
	res.Name = NULL;
	res.Parameters = NULL;
	
	char loggBuff[64];	
	
	if (!json)  
	{
		char *error = (char*)cJSON_GetErrorPtr();
		UARTWrite(1,"\n\r An error was encountered\n\r");
		UARTWrite(1,error);
		return res;
	}	
	else
	{	
		int size = cJSON_GetArraySize(json);	
		sprintf(loggBuff, "\n\rGot %d commands from server", size);		
		UARTWrite(1, loggBuff);
		
		if(size > 0)
		{
			cJSON* Command =  cJSON_GetArrayItem(json, 0);
			cJSON* timestampJSON = cJSON_GetObjectItem(Command, "timestamp");
			if(FetchTimeStamp(timestampJSON))
			{
				res.Name = cJSON_DetachItemFromObject(Command, "command");
				if(res.Name)
				{
					UARTWrite(1, "\r\nCommand name is: ");
					char * s_out = cJSON_Print(res.Name);
					UARTWrite(1, s_out);
					free(s_out);					
				}
				
				res.Parameters = cJSON_DetachItemFromObject(Command, "parameters");
				if(res.Parameters)
				{
					UARTWrite(1, "\r\nCommand parameters are: ");
					char * s_out = cJSON_Print(res.Parameters);
					UARTWrite(1, s_out);
					free(s_out);					
				}	
				
				res.ID = cJSON_DetachItemFromObject(Command, "id");
				if(res.ID)
				{
					UARTWrite(1, "\r\nCommand ID is: ");
					char * s_out = cJSON_Print(res.ID);
					UARTWrite(1, s_out);
					free(s_out);					
				}				
			}				
		}
	}		

	return res;
}


BOOL FetchServerCommand(cJSON* json, struct HiveCommand* dest)
{
	char loggBuff[64];	
	memset(dest, 0, sizeof(struct HiveCommand));		
	
	if (json)  
	{	
		int size = cJSON_GetArraySize(json);	
		
		if(size > 0)		
		{
			sprintf(loggBuff, "\n\rThere are %d commands from server left to handle.", size);		
			UARTWrite(1, loggBuff);			
			
			cJSON* Command = cJSON_GetArrayItem(json, 0);
			cJSON* timestampJSON = cJSON_GetObjectItem(Command, "timestamp");
			if(FetchTimeStamp(timestampJSON))
			{
				dest->Name = cJSON_DetachItemFromObject(Command, "command");
				if(dest->Name)
				{
					UARTWrite(1, "\r\nCommand name is: ");
					char * s_out = cJSON_Print(dest->Name);
					UARTWrite(1, s_out);
					free(s_out);					
				}
				
				dest->Parameters = cJSON_DetachItemFromObject(Command, "parameters");
				if(dest->Parameters)
				{
					UARTWrite(1, "\r\nCommand parameters are: ");
					char * s_out = cJSON_Print(dest->Parameters);
					UARTWrite(1, s_out);
					free(s_out);					
				}	
				
				dest->ID = cJSON_DetachItemFromObject(Command, "id");
				if(dest->ID)
				{
					UARTWrite(1, "\r\nCommand ID is: ");
					char * s_out = cJSON_Print(dest->ID);
					UARTWrite(1, s_out);
					free(s_out);					
				}
			}	

			cJSON_DeleteItemFromArray(json, 0);			
		}
		else
		{
			UARTWrite(1, "\n\rNo commands from server to handle. Will pull again.");
			cJSON_Delete(json);
		}
	}		

	if(dest->Name && dest->Parameters && dest->ID)
		return TRUE;
	else
	{
		if(dest->Name)cJSON_Delete(dest->Name);
		if(dest->Parameters)cJSON_Delete(dest->Parameters);		
		if(dest->ID)cJSON_Delete(dest->ID);				
		return FALSE;
	}
}







