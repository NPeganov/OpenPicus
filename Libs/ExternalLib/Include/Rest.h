#ifndef REST_API_H
#define REST_API_H

#include "taskFlyport.h"
#include "cJSON.h"
#include <string.h>

struct HiveCommand
{
	cJSON* Name;
	cJSON* Parameters;	
};

cJSON * FormRegistrationRequest();
char* FormatRegistrationUrl(char* Buffer);
char* FormatInfoUrl(char* Buffer);
char* FormatCommandPollUrl(char* Buffer);
char* FormatNotificationUrl(char* Buffer);

char* HandleServerInfo(cJSON* RawResponse);
void SetLastTimeStamp(const char* NewTimeStamp);
const char* GetLastTimeStamp();
struct HiveCommand HandleServerCommand(cJSON* json);
char* FetchTimeStamp(cJSON * TimestampFromServer);
cJSON* FormNotificationRequest(const char * Name, cJSON* Parameters);
cJSON* FormParameter(const char * Name, double value);

#endif



