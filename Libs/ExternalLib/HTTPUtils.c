#include "HTTPUtils.h"
#include "taskFlyport.h"

struct HttpResponse SendHttpJsonRequest(TCP_SOCKET* conn, const char* url, unsigned char type, cJSON* json, unsigned char* BufForResp, int timeout_sec)
{	
	struct HttpResponse res;

	char *jsonStr = cJSON_Print(json);
	UARTWrite(1, "\r\nSending json\r\n");    
	UARTWrite(1, jsonStr);
	UARTWrite(1, "\r\n");		
	free(jsonStr);			
	
	UARTWrite(1, "\r\nto URL\r\n");    	
	UARTWrite(1, (char*)url);    		
	UARTWrite(1, "... ");    	
	
	jsonStr = cJSON_PrintUnformatted(json);	
	res = SendHttpDataRequest(conn, url, type, (unsigned char*)jsonStr, BufForResp, timeout_sec);
	free(jsonStr);	
	
	return res;
}

struct HttpResponse SendHttpDataRequest(TCP_SOCKET* conn, const char* url, unsigned char type, unsigned char* data, unsigned char* BufForResp, int timeout_sec)
{
	struct HttpResponse res;
	res.RsponseIsOK = FALSE;
	res.Response = 0;
	
	UARTWrite(1, "\r\nSending raw data\r\n");    
	UARTWrite(1, (char*)data);
	UARTWrite(1, "\r\n");
	
	UARTWrite(1, "\r\nto URL\r\n");    	
	UARTWrite(1, (char*)url);    		
	UARTWrite(1, "... ");    	

	
	HTTPRequest(conn, type, url, (char*)data, HTTP_NO_PARAM);	
	ProcessComand();
	int respLen = GetHttpResponse(conn, BufForResp, timeout_sec);
	if(respLen)
	{
		UARTWrite(1, "Raw response arrived:\r\n");    
		UARTWrite(1, (char*)BufForResp);
		UARTWrite(1, "\r\n");	
		
		res = ParseResponse(BufForResp);		
	}
	else
	{
		UARTWrite(1, "\r\nNo response.\r\n"); 		
	}
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
			if(GetHttpStatus(conn) != CONNECTED)
			{
				UARTWrite(1, "\r\nGetHttpResponse: Bad socket status\r\n");				
				return 0;
			}
			
			if(!first_time)break;
			
			if(TickGetDiv64K() - start_sec > timeout_sec)
			{
				UARTWrite(1, "\r\nGetHttpResponse: TIMEOUT\r\n");
				return 0;
			}
		}while(conn->rxLen == 0);

		first_time = FALSE;
		
		if(conn->rxLen == 0)
		{
			break;
		}
		else
		{
			UARTWrite(1, "\r\n READING REAL DATA:\r\n");			
			HTTPReadData(conn, (char*)&BufForResp[lenTemp], conn->rxLen);
			lenTemp += conn->rxLen;
			sprintf(loggBuff, "lenTemp: %d\r\n", lenTemp);		
			UARTWrite(1, loggBuff);				
			sprintf(loggBuff, "rxLen: %d\r\n", conn->rxLen);						
			UARTWrite(1, loggBuff);				
		}
	}while(1);
	
	return lenTemp;
}


enum SocketState GetHttpStatus(TCP_SOCKET* conn)
{
	char loggBuff[32];
	
	UARTWrite(1, "\r\nGetting HttpStatus... ");	
	HTTPStatus(conn);
	if(ProcessComand())
	{
		UARTWrite(1, "TCP Socket Status:\r\n");
		sprintf(loggBuff, " - Status: %d\r\n", conn->status);
		UARTWrite(1, loggBuff);
		sprintf(loggBuff, " - RxLen: %d\r\n", conn->rxLen);
		UARTWrite(1, loggBuff);	
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
	char loggBuff[32];

	HTTPClose(conn);
	while(LastExecStat() == OP_EXECUTION)
		vTaskDelay(1);		
	
	UARTWrite(1, "\r\n\r\nConnecting to \""); 
	UARTWrite(1, host); 	
	UARTWrite(1, "\", using port "); 		
	UARTWrite(1, port); 
	UARTWrite(1, "... ");
	HTTPOpen(conn, host, port);   
	if(ProcessComand())
	{
		UARTWrite(1, "\r\n HTTPOpen OK \r\n");
		UARTWrite(1, "Socket Number: ");
		sprintf(loggBuff, "%d", conn->number);
		UARTWrite(1, loggBuff);	
		
		return TRUE;
	}

	return FALSE;			
}

BOOL EstablishHttpConnecion(TCP_SOCKET* conn, const char* host, const char* port, unsigned char attempts_num)
{
	do
	{
		if(HTTPConnect(conn, host, port) && (GetHttpStatus(conn) == CONNECTED))
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
	char loggBuff[32];	
	
	char subst[8];
	char* start = 0;
	char* end = 0;
	char* respEnd = (char*)&BufForResp[strlen((char*)BufForResp)];
		
	strcpy(subst, "HTTP/1.");
	if(start = strstr((char*)&BufForResp[0], subst) && 
	(start + strlen("HTTP/1.1 200")) < respEnd)
	{	
		UARTWrite(1, "START now is\r\n"); 		
		UARTWrite(1, start);		
		UARTWrite(1, "\r\n"); 				
		int start1 = 0;
		start1 = (int)(start - (char*)&BufForResp[0]);
		sprintf(loggBuff, "start1 pos: %d\r\n%s\r\n", start1, start);	
		UARTWrite(1, loggBuff); 			
		
		end = start + strlen("HTTP/1.1 200");
		
		UARTWrite(1, "END now is\r\n"); 		
		UARTWrite(1, end);		
		UARTWrite(1, "\r\n"); 				
		
		
		int end1 = 0;
		end1 = (int)(end - (char*)BufForResp);		
		sprintf(loggBuff, "end1 pos: %d\r\n%s\r\n", end1, end);			
		UARTWrite(1, loggBuff); 			
		
		*end = 0;
		start += strlen(subst) + 2;	
		
		int start2 = 0;
		start2 = (int)(start - (char*)BufForResp);		
		sprintf(loggBuff, "start2 pos: %d\r\n%s\r\n", start2, start);
		UARTWrite(1, loggBuff); 					
		
		UARTWrite(1, "\r\nResponse status: ");	
		UARTWrite(1, start);				
		UARTWrite(1, "\r\n");	
		if(!strcmp(start, "200") || !strcmp(start, "204"))
			res.RsponseIsOK = TRUE;			
	}
	else
	{
		UARTWrite(1, "\r\nCannot find status in response\r\n");							
	}
		
	strcpy(subst, "\r\n\r\n");
	if(start = strstr((char*)BufForResp, subst) && 
	(start + strlen(subst)) < respEnd)
	{
		start += strlen(subst);
		sprintf(loggBuff, "start3 pos: %d\r\n%s\r\n", (int)(start - (char*)BufForResp), start);			
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
