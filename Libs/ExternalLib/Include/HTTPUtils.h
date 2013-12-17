#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include "CommonUtils.h"
#include "cJSON.h"
#include <string.h>

struct HttpResponse
{
	BOOL RsponseIsOK;
	char * Response;	
};

enum SocketState
{
	CONNECTED,
	NOT_CONNECTED,
	GET_STATUS_FAILURE		
};

struct HttpResponse SendHttpJsonRequest(TCP_SOCKET* conn, const char* url, unsigned char type, cJSON* json, int timeout_sec);
struct HttpResponse SendHttpDataRequest(TCP_SOCKET* conn, const char* url, unsigned char type, unsigned char * data, int timeout_sec);
int GetHttpResponse(TCP_SOCKET* conn, unsigned char* BufForResp, int timeout_sec);
BOOL EstablishHttpConnecion(TCP_SOCKET* conn, const char* host, const char* port, unsigned char attempts_num);
enum SocketState GetHttpStatus(TCP_SOCKET* conn);
BOOL HTTPConnect(TCP_SOCKET* conn, const char* host, const char* port);
struct HttpResponse ParseResponse(unsigned char* BufForResp);
#endif

