#ifndef REST_API_H
#define REST_API_H

#include "taskFlyport.h"
#include "cJSON.h"
#include <string.h>

cJSON * FormRegistrationRequest();
char* FormatRegistrationUrl(char* Buffer);
char* FormatInfoUrl(char* Buffer);
char* FormatCommandPollUrl(char* Buffer);
char* HandleServerInfo(cJSON* RawResponse);

#endif

