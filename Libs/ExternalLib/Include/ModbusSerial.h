#ifndef MODBUS_SERIAL_H
#define MODBUS_SERIAL_H

#include "RS485Helper.h"

void _SerialPortInit(unsigned char Port, int baud, unsigned char stopBits, unsigned char Parity);	
void _SerialWrite(const unsigned char* Data, short len);	
BOOL _SerialReadByte(unsigned char* Dest, unsigned long timeout_us);	
BOOL _SerialRead(unsigned char* Dest, unsigned short len, unsigned long long timeout_us);
int _GetBaud();

struct SerialPort
{
	unsigned char PortNum;
	void (*Init)(unsigned char Port, int baud, unsigned char stopBits, unsigned char Parity);		
	void (*Write)(const unsigned char* Data, short len);	
	BOOL (*ReadByte)(unsigned char* Dest, unsigned long timeout_us);		
	BOOL (*Read)(unsigned char* Dest, unsigned short len, unsigned long long timeout_us);	
	int (*GetBaud)();
};

#endif
