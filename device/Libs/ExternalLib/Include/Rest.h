#ifndef REST_API_H
#define REST_API_H

#include "taskFlyport.h"
#include "cJSON.h"
#include <string.h>

struct HiveCommand
{
	cJSON* Name;
	cJSON* Parameters;	
	cJSON* ID;
};

cJSON * FormRegistrationRequest();
char* FormatRegistrationUrl(char* Buffer);
char* FormatInfoUrl(char* Buffer);
char* FormatCommandPollUrl(char* Buffer);
char* FormatNotificationUrl(char* Buffer);
char* FormatAckUrl(char* Buffer, cJSON* jID);

char* HandleServerInfo(cJSON* RawResponse);
void SetLastTimeStamp(const char* NewTimeStamp);
const char* GetLastTimeStamp();
struct HiveCommand HandleServerCommand(cJSON* json);
BOOL FetchServerCommand(cJSON* json, struct HiveCommand* dest);
char* FetchTimeStamp(cJSON * TimestampFromServer);
cJSON* FormNotificationRequest(const char * Name, cJSON* Parameters);
cJSON* FormParameter(const char * Name, double value);
cJSON* FormAckRequest(BOOL isResOK);

#endif






