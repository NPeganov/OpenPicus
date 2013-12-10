#include "ModbusSerial.h"

#define		DE_485		p2
#define		RE_485		p17
#define		TX_485		p5
#define		RX_485		p7
#define		TX_232		p4
#define		RX_232		p6
#define		CTS_232		p11
#define		RTS_232		p9

static unsigned char Port_ = 0;
static unsigned int Baud_ = 0;

static int readEnPin_ = 0;
static int readWnPin_ = 0;

//static int wEnP[4] = {0,0,0,0};
//static int rEnP[4] = {0,0,0,0};
/*
extern static int wEnPin[4];
extern static int rEnPin[4];
*/

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
}

void _SerialWrite(const unsigned char* Data, short len)
{
	char loggBuff[256];		
	sprintf(loggBuff, "\r\n_SerialWrite... going to write %d bytes to port # %d", (int)len, (int)Port_);		
    UARTWrite(1, loggBuff);		
	
	IOPut((int)DE_485, on);
	IOPut((int)RE_485, on);
/*
	IOPut((int)wEnPin[Port_-1], on);
	IOPut((int)rEnPin[Port_-1], on);
*/	
	vTaskDelay(1);
	short ind = len;
	do
	{
		sprintf(loggBuff, "\r\n_SerialWrite... writing byte %d.", (int)(ind - len));	
		UARTWrite(1, loggBuff);			
		UARTWriteCh(Port_, Data[ind - len]);		
	}while(--len);
	vTaskDelay(1);
	
	UARTWrite(1, "\r\n_SerialWrite... enabling reciever");	
	
	/*
	IOPut((int)wEnPin[Port_-1], off);
	IOPut((int)rEnPin[Port_-1], off);	
	*/
	
	IOPut((int)DE_485, off);
	IOPut((int)RE_485, off);	
	
	UARTWrite(1, "\r\n..._SerialWrite");		
}

BOOL _SerialReadByte(unsigned char* Dest, unsigned long timeout_us)
{
	int bytesReady = RS485BufferSize(Port_);
	DWORD tim1 = TickGet();
	
	do
	{
		if( ((TickGet() - tim1) * 16) > timeout_us)
			return FALSE;		
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


