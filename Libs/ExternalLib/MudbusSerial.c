#include "ModbusSerial.h"

#define		DE_485		p2
#define		RE_485		p17
#define		TX_485		p5
#define		RX_485		p7

static unsigned char Port_ = 0;
static unsigned int Baud_ = 0;

void _SerialPortInit(unsigned char Port, int baud, unsigned char stopBits, unsigned char Parity)
{
	Port_ = Port;
	Baud_ = baud;
	
	// Initialize the RS485
	RS485Remap(Port_, TX_485, RX_485, DE_485, RE_485);
	RS485Init(Port_, Baud_);
	RS485SetParam(Port_, RS485_STOP_BITS, stopBits);
	RS485SetParam(Port_, RS485_DATA_PARITY, Parity);
	RS485On(Port_);	
	vTaskDelay(100);	
}

void _SerialWrite(const unsigned char* Data, short len)
{
	char loggBuff[256];		
	sprintf(loggBuff, "\r\n_SerialWrite... going to write %d bytes to port # %d", (int)len, (int)Port_);		
    UARTWrite(1, loggBuff);		
	
	IOPut((int)DE_485, on);
	IOPut((int)RE_485, on);

	vTaskDelay(1);
	short ind = len;
	do
	{
		//sprintf(loggBuff, "\r\n_SerialWrite... writing byte %d.", (int)(ind - len));	
		//UARTWrite(1, loggBuff);			
		UARTWriteCh(Port_, Data[ind - len]);		
	}while(--len);
	RS485Flush(Port_);	
	vTaskDelay(1);
	
	UARTWrite(1, "\r\n_SerialWrite... enabling reciever");	
	
	IOPut((int)DE_485, off);
	IOPut((int)RE_485, off);	
	
	UARTWrite(1, "\r\n..._SerialWrite");		
}

BOOL _SerialReadByte(unsigned char* Dest, unsigned long timeout_us)
{
	char loggBuff[256];		
	int bytesReady = 0;
	DWORD tim1 = TickGet();
	
	do
	{
		if( ((TickGet() - tim1) * 16) > timeout_us)
			return FALSE;		
			
		bytesReady = RS485BufferSize(Port_);			
		sprintf(loggBuff, "\r\n_SerialReadByte... bytes ready: %d, time: %d", bytesReady, (int)(((TickGet() - tim1) * 16)));	
		UARTWrite(1, loggBuff);			
	}while(bytesReady < 1);

	RS485Read(Port_, (char *)Dest, 1);	
	return TRUE;
}

BOOL _SerialRead(unsigned char* Dest, unsigned short len, unsigned long long timeout_us)
{
	unsigned short ind = len;
	do
	{
		if(!_SerialReadByte(&Dest[ind - len], timeout_us))
			return FALSE;		
	}while(--len);

	return TRUE;
}

int _GetBaud()
{
	return Baud_;
}

const struct SerialPort RS485 = 
{
	2,
	_SerialPortInit,
	_SerialWrite,
	_SerialReadByte,
	_SerialRead,
	_GetBaud
};


