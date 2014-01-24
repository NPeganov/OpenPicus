#include "HTTPUtils.h"
#include "taskFlyport.h"

#define TCP_BUF_SIZE 1024

//char _host[128];
//char _port[8];
char* _host = NULL;
char* _port = NULL;

unsigned char TCP_Buff[TCP_BUF_SIZE];
static const char * AuthInfo = "Authorization: Basic ZGhhZG1pbjpkaGFkbWluXyM5MTE=";

struct HttpResponse SendHttpJsonRequest(TCP_SOCKET* conn, const char* url, unsigned char type, cJSON* json, int timeout_sec)
{	
	struct HttpResponse res;
	char *jsonStr = 0;
	
	if(json)
	{
		jsonStr = cJSON_Print(json);
		UARTWrite(1, "\r\nSending json\r\n");    
		UARTWrite(1, jsonStr);
		UARTWrite(1, "\r\n");		
		free(jsonStr);			
	}
	
	UARTWrite(1, "\r\nto URL\r\n");    	
	UARTWrite(1, url);    		
	UARTWrite(1, "... ");    	

	if(json)
	{	
		jsonStr = cJSON_PrintUnformatted(json);	
	}
	res = SendHttpDataRequest(conn, url, type, (unsigned char*)jsonStr, timeout_sec);
	free(jsonStr);	
	
	return res;
}

struct HttpResponse SendHttpDataRequest(TCP_SOCKET* conn, const char* url, unsigned char type, unsigned char* data, int timeout_sec)
{
	struct HttpResponse res;
	res.RsponseIsOK = FALSE;
	res.Response = 0; 	

	memset(TCP_Buff, 0, TCP_BUF_SIZE);	
	if(type != HTTP_GET)
	{
		strcpy(TCP_Buff, "Content-Length: ");
		char contentlength[5];
		sprintf(contentlength, "%d", strlen((char*)data));
		strcat(TCP_Buff, contentlength);
		strcat(TCP_Buff, "\r\nContent-Type: application/json\r\nAccept: */*\r\n");
	}
	else
	{
		strcpy(TCP_Buff, "Accept: */*\r\n");		
	}
	strcat(TCP_Buff, AuthInfo);						
	strcat(TCP_Buff, "\r\n");												
	strcat(TCP_Buff, "\r\n\r\n");
	strcat(TCP_Buff, (char*)data);
	
	int respLen = 0;
	unsigned char attempts_counter = 0;
	const unsigned char attempts_failure_limit = 5;
	do
	{
		if(conn->number == INVALID_SOCKET || GetHttpStatus(conn) != CONNECTED)
		{
			unsigned char ConnectionAtteptsNum = 2;
			if(!EstablishHttpConnecion(conn, _host, _port, ConnectionAtteptsNum))
			{
				UARTWrite(1, "Cannot establish a connection.\r\n Sleep and reset...\r\n");	
				vTaskDelay(300);
				Reset();		
			}		
		}	
		
		if(data)
		{
			UARTWrite(1, "\r\nSending raw data\r\n");    
			UARTWrite(1, (char*)data);
			UARTWrite(1, "\r\n");
		}
		else
		{
			UARTWrite(1, "\r\nSending empty request");    
		}
		
		char* urlToSend = (char*)malloc(sizeof(char) * (strlen(url) + 1));	
		strcpy(urlToSend, url); // there is a need to make a copy of an url, since the call to HTTPRequest will modify it.
			
		UARTWrite(1, "\r\nto URL\r\n");    	
		UARTWrite(1, urlToSend);    		
		UARTWrite(1, "... "); 		
		
		HTTPRequest(conn, type, urlToSend, NULL, (char*)TCP_Buff);	
		ProcessCommand();
		free(urlToSend);
	
		memset(TCP_Buff, 0, TCP_BUF_SIZE);	
		respLen = GetHttpResponse(conn, TCP_Buff, timeout_sec);
		
		if(respLen > 0)
		{
			UARTWrite(1, "Raw response arrived:\r\n");    
			UARTWrite(1, (char*)TCP_Buff);
			UARTWrite(1, "\r\n");	
		
			res = ParseResponse(TCP_Buff);		
		}
		else if(respLen == 0)
		{
			UARTWrite(1, "\r\nNo response: Timeout."); 			
		}
	
		UARTWrite(1, "\r\nClosing socket... "); 			
		HTTPClose(conn);
		ProcessCommand();
		conn->number = INVALID_SOCKET;	
	
		if(++attempts_counter > attempts_failure_limit)	
		{
			UARTWrite(1, "\r\n\r\nToo many attempts for a transimission!\r\nResetting the device!\r\n\r\n"); 				
			Reset();			
		}		
	}while(respLen < 0);
	
	return res;
}

int GetHttpResponse(TCP_SOCKET* conn, unsigned char* BufForResp, int timeout_sec)
{
	char loggBuff[255];	
	BufForResp[0] = 0;		
	UARTWrite(1, "\r\n READING Socket:\r\n");	
	BOOL first_time = TRUE;
	int lenTemp = 0;
	int start_sec = TickGetDiv64K();

	do
	{
		do
		{		
			UARTWrite(1, "\r\nReading data from the socket.");			
			if(GetHttpStatus(conn) != CONNECTED)
			{
				UARTWrite(1, "\r\nGetHttpResponse: Bad socket status\r\n");				
				return -1;
			}
			
			if(!first_time)break;
			
			if(TickGetDiv64K() - start_sec > timeout_sec)
			{
				UARTWrite(1, "\r\nGetHttpResponse: TIMEOUT\r\n");
				return 0;
			}
			UARTWrite(1, "No data in the socket yet.\r\n");	
		}while(conn->rxLen == 0);

		first_time = FALSE;
		
		if(conn->rxLen == 0)
		{
			break;
		}
		else
		{
			UARTWrite(1, "\r\n READING REAL DATA:\r\n");	
			if((lenTemp + conn->rxLen) >= TCP_BUF_SIZE)
			{
				UARTWrite(1, "\r\n RESPONSE IS TOO LONG:\r\n");					
				break;
			}
			HTTPReadData(conn, (char*)&BufForResp[lenTemp], conn->rxLen);
			lenTemp += conn->rxLen;
			sprintf(loggBuff, "lenTemp: %d\r\n", lenTemp);		
			UARTWrite(1, loggBuff);				
			sprintf(loggBuff, "rxLen: %d\r\n", conn->rxLen);						
			UARTWrite(1, loggBuff);		
			BufForResp[lenTemp] = 0;

			if(lenTemp == 934)break;
			DelayMs(1000);
			/*
			UARTWrite(1, "DATA READ: \r\n");	
			UARTWrite(1, (char*)&BufForResp[0]);				
			*/
		}
	}while(1);
	
	return lenTemp;
}


enum SocketState GetHttpStatus(TCP_SOCKET* conn)
{
	//UARTWrite(1, "\r\n\r\nGetHttpStatus...\r\n"); 		
	char loggBuff[32];
	
	//UARTWrite(1, "\r\nGetting HttpStatus... ");	
	HTTPStatus(conn);
	if(ProcessCommand())
	{
		if(conn->status != SOCK_CONNECT || conn->rxLen > 0)
		{
			UARTWrite(1, "TCP Socket Status:\r\n");
			sprintf(loggBuff, " - Status: %d\r\n", conn->status);
			UARTWrite(1, loggBuff);
			sprintf(loggBuff, " - RxLen: %d\r\n", conn->rxLen);
			UARTWrite(1, loggBuff);	
		}
		if(conn->status == SOCK_CONNECT)
			return CONNECTED;
		else
			return NOT_CONNECTED;
	}	
	else
		return GET_STATUS_FAILURE;
}

BOOL HTTPConnect(TCP_SOCKET* conn, const char* host, const char* port)
{		
//	UARTWrite(1, "\r\n\r\nHTTPConnect...\r\n"); 	
	char loggBuff[32];

	HTTPClose(conn);
	while(LastExecStat() == OP_EXECUTION)
		vTaskDelay(1);		
	
	UARTWrite(1, "\r\n\r\nConnecting to \""); 
	UARTWrite(1, host); 	
	UARTWrite(1, "\", using port "); 		
	UARTWrite(1, port); 
	UARTWrite(1, "... ");
	conn->number = INVALID_SOCKET;
	HTTPOpen(conn, host, port);   
	if(ProcessCommand())
	{
		UARTWrite(1, "\r\n HTTPOpen OK \r\n");
		UARTWrite(1, "Socket Number: ");
		sprintf(loggBuff, "%d\r\n", conn->number);
		UARTWrite(1, loggBuff);	
		
		return TRUE;
	}

//	UARTWrite(1, "\r\n...HTTPConnect\r\n"); 		
	return FALSE;			
}

BOOL EstablishHttpConnecion(TCP_SOCKET* conn, const char* host, const char* port, unsigned char attempts_num)
{
	UARTWrite(1, "\r\nEstablishHttpConnecion..."); 	
	if(!_host)
	{
		_host = (char*)malloc(sizeof(char) * (strlen(host) + 1));	
		strcpy(_host, host);
	}

	if(!_port)
	{
		_port = (char*)malloc(sizeof(char) * (strlen(port) + 1));		
		strcpy(_port, port);
	}	
	
//	strcpy(_host, host);
//	strcpy(_port, port);	
	do
	{
		if(HTTPConnect(conn, _host, _port) && (GetHttpStatus(conn) == CONNECTED))
		{
			UARTWrite(1, "Connection has been established.\r\n"); 				
			return TRUE;
		}
	}while(--attempts_num);
	
	return FALSE;	
}

struct HttpResponse ParseResponse(unsigned char* BufForResp)
{
	struct HttpResponse res;	
	
	char subst[8];
	char* start = 0;
	char* end = 0;
	char* respEnd = (char*)&BufForResp[strlen((char*)BufForResp)];
	          
	strcpy(subst, "HTTP/1.");            
    start = strstr((char*)&BufForResp[0], subst);    

	if(start && (respEnd - (start + strlen("HTTP/1.1 200"))) >= 0)
	{	
		end = start + strlen("HTTP/1.1 200");		
		start = start + strlen(subst) + 2;
		
		char tmp = *end;
		*end = 0;			
		
		UARTWrite(1, "\r\nResponse status: ");	
		UARTWrite(1, start);				
		UARTWrite(1, "\r\n");		
		
		if(!strcmp(start, "200") || !strcmp(start, "201") || !strcmp(start, "204"))
			res.RsponseIsOK = TRUE;	
		
		*end = tmp;
	}
	else
	{
		UARTWrite(1, "\r\nCannot find status in response\r\n");							
	}
		
	strcpy(subst, "\r\n\r\n");
	start = strstr((char*)&BufForResp[0], subst); 

	if(start && (respEnd - (start + strlen(subst))) >= 0)
	{
		start += strlen(subst);

		UARTWrite(1, "\r\nResponse payload: ");	
		UARTWrite(1, start);				
		UARTWrite(1, "\r\n");			
		
		res.Response = start;
	}
	else
	{
		UARTWrite(1, "\r\nCannot find payload in response\r\n");				
	}	
	
	return res;
}








