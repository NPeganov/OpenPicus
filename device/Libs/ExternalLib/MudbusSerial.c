#include "ModbusSerial.h"

#define		TX_485		p5
#define		RX_485		p7
#define		DE_485		p2
#define		RE_485		p17

#define		TX_232		p4
#define		RX_232		p6
#define		CTS_232		p11
#define		RTS_232		p9

static unsigned char Port_ = 0;
static unsigned int Baud_ = 0;

void _232PortInit(unsigned char Port, int baud, unsigned char stopBits, unsigned char Parity)
{	
    //UARTWrite(1, "_232PortInit...");		
	Port_ = Port;
	Baud_ = baud;
	
	// Initialize the serial port
	RS232Remap(Port_, TX_232, RX_232, 0, 0);
	RS232Init(Port_, Baud_);
    UARTInit(Port_, Baud_);  		
	RS232SetParam(Port_, RS232_STOP_BITS, stopBits);
	RS232SetParam(Port_, RS232_DATA_PARITY, Parity);
	RS232On(Port_);	
	vTaskDelay(100);
	
    //UARTWrite(1, "..._SerialPortInit");		
}

void _485PortInit(unsigned char Port, int baud, unsigned char stopBits, unsigned char Parity)
{	
    //UARTWrite(1, "_485PortInit...");		
	Port_ = Port;
	Baud_ = baud;
	
	// Initialize the serial port
	RS485Remap(Port_, TX_485, RX_485, DE_485, RE_485);
	RS485Init(Port_, Baud_);
    UARTInit(Port_, Baud_);  		
	RS485SetParam(Port_, RS485_STOP_BITS, stopBits);
	RS485SetParam(Port_, RS485_DATA_PARITY, Parity);
	RS485On(Port_);	
	vTaskDelay(100);
	
    //UARTWrite(1, "..._SerialPortInit");		
}

void _485Write(const unsigned char* Data, short len)
{
	char loggBuff[256];		
	sprintf(loggBuff, "\r\n_485Write... going to write %d bytes to port # %d", (int)len, (int)Port_);		
    UARTWrite(1, loggBuff);		
	
	IOPut((int)DE_485, on);
	IOPut((int)RE_485, on);// RE is inverted, so, ON value turns it off

	vTaskDelay(1);
	short ind = len;
	do
	{
		UARTWriteCh(Port_, Data[ind - len]);		
	}while(--len);
	vTaskDelay(1);

	UARTWrite(1, "\r\n_SerialWrite... enabling reciever");	
	
	IOPut((int)DE_485, off);
	IOPut((int)RE_485, off);		
}

void _232Write(const unsigned char* Data, short len)
{
	char loggBuff[256];		
	sprintf(loggBuff, "\r\n_232Write... going to write %d bytes to port # %d", (int)len, (int)Port_);		
    UARTWrite(1, loggBuff);		

	vTaskDelay(1);
	short ind = len;
	do
	{
		UARTWriteCh(Port_, Data[ind - len]);		
	}while(--len);
	vTaskDelay(1);	
}

BOOL _SerialReadByte(unsigned char* Dest, DWORD timeout_us)
{
	int bytesReady = 0;
	DWORD tim1 = TickGet();
	
	do
	{
		if( ((TickGet() - tim1) * 16) > timeout_us)		
			return FALSE;		
			
		bytesReady = RS485BufferSize(Port_);			
	}while(bytesReady < 1);

	RS485Read(Port_, (char *)Dest, 1);	
	return TRUE;
}

BOOL _SerialRead(unsigned char* Dest, unsigned short len, DWORD timeout_us)
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
	_485PortInit,
	_485Write,
	_SerialReadByte,
	_SerialRead,
	_GetBaud
};

const struct SerialPort RS232 = 
{
	_232PortInit,
	_232Write,
	_SerialReadByte,
	_SerialRead,
	_GetBaud
};





