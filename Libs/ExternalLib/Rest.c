#include "Rest.h"

static const char * DevId = "b55cf00a-ffff-aaaa-bbbb-8af2a112cb57";
static const char * DevKey = "b55cf00a-ffff-kkkk-bbbb-8af2a112cb57";
static const char * NtwKey = "b55cf00a-nnnn-kkkk-bbbb-8af2a112cb57";
static const char * BaseUrl = "ecloud.dataart.com";///ecapi8";


cJSON * FormRegistrationRequest()
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
