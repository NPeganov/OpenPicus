#ifndef MODBUS_SERIAL_H
#define MODBUS_SERIAL_H

#include "RS485Helper.h"
#include "RS232Helper.h"

void _232PortInit(unsigned char Port, int baud, unsigned char stopBits, unsigned char Parity);	
void _485PortInit(unsigned char Port, int baud, unsigned char stopBits, unsigned char Parity);	
void _232Write(const unsigned char* Data, short len);	
void _485Write(const unsigned char* Data, short len);	
BOOL _SerialReadByte(unsigned char* Dest, DWORD timeout_us);	
BOOL _SerialRead(unsigned char* Dest, unsigned short len, DWORD timeout_us);
int _GetBaud();

struct SerialPort
{
	void (*Init)(unsigned char Port, int baud, unsigned char stopBits, unsigned char Parity);		
	void (*Write)(const unsigned char* Data, short len);	
	BOOL (*ReadByte)(unsigned char* Dest, DWORD timeout_us);		
	BOOL (*Read)(unsigned char* Dest, unsigned short len, DWORD timeout_us);	
	int (*GetBaud)();
};

#endif

