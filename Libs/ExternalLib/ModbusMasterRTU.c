#include "ModbusMasterRTU.h"
#include "RS485Helper.h"
#include "CRC16Calc.h"

#define MB_CRC_LEN 2		//The length of slave CRC16 field

#define MB_ADDR_LEN 1		//The length of slave address field
#define MB_FCODE_LEN 1		//The length of function code field
#define MB_PSIZE_LEN 1		//The length of payload size field

#define MB_STARTING_ADDRESS_LEN 2	
#define MB_QUANTITY_LEN 2	
#define MB_REGISTER_LEN 2	

#define MB_ADDR_IND 0		
#define MB_FCODE_IND 1	
#define MB_PSIZE_IND 2	
#define MB_ERROR_IND 2
#define MB_PLOAD_IND 3	

//The length of header bytes
#define MB_HEADER_LEN MB_ADDR_LEN + MB_FCODE_LEN + MB_PSIZE_LEN	

//The length of non-payload bytes
#define MB_NON_PAYLOAD_LEN MB_HEADER_LEN + MB_CRC_LEN

unsigned char ModbusBuf[512];
static struct SerialPort RS485Port;
static unsigned long InterbyteDelay_us = 0;

void _MBMInit(const struct SerialPort * Serial)
{
	RS485Port = *Serial;
	
    if (RS485Port.GetBaud() < 19201)//Delay is (1 / (baudrate/11)) * 3.5 seconds, assumming that one real byte is equal to 11 bits
		InterbyteDelay_us = (((11 * 3.5)/RS485Port.GetBaud()) * 1000000);
    else
		InterbyteDelay_us = 1750;
		
	char loggBuff[32];		
	sprintf(loggBuff, "\r\nInterbyteDelay is set to %ld us.", InterbyteDelay_us);		
    UARTWrite(1, loggBuff);	
}

unsigned char _RegHi(unsigned short source)
{
	return (unsigned char)((source >> 8) & 0x0F);	
}

unsigned char _RegLow(unsigned short source)
{
	return (unsigned char)((source) & 0x0F);	
}


struct ReadRegistersResp _ReadHRegisters(unsigned char SlaveID, const short StartAddr, const short RequestedQnty)
{
	UARTWrite(1, "\r\n_ReadHRegisters...");		
	char loggBuff[300];	
	struct ReadRegistersResp res;
	res.ec = 0;
	res.payload = 0;
	unsigned char fcode = FC_Read_Holding_Registers;
	
	unsigned char total_len = 0;
	ModbusBuf[MB_ADDR_IND] = SlaveID;
	ModbusBuf[MB_FCODE_IND] = fcode;
	ModbusBuf[MB_PSIZE_IND] = MB_STARTING_ADDRESS_LEN + MB_QUANTITY_LEN;	
	total_len = ModbusBuf[MB_PSIZE_IND];

	ModbusBuf[MB_PLOAD_IND + 0] = _RegHi(StartAddr);
	ModbusBuf[MB_PLOAD_IND + 1] = _RegLow(StartAddr);	
	
	ModbusBuf[MB_PLOAD_IND + 2] = _RegHi(RequestedQnty);
	ModbusBuf[MB_PLOAD_IND + 3] = _RegLow(RequestedQnty);
	
	UARTWrite(1, "\r\nCalculating CRC...");		
	unsigned short CRC = CRC16(&ModbusBuf[MB_PLOAD_IND + 0], ModbusBuf[MB_PSIZE_IND]);
	
	ModbusBuf[MB_PLOAD_IND + 4] = _RegHi(CRC);
	ModbusBuf[MB_PLOAD_IND + 5] = _RegLow(CRC);		
	
	total_len += MB_NON_PAYLOAD_LEN;
	
	UARTWrite(1, "\r\nSending request...");			
	RS485Port.Write(ModbusBuf, total_len);
	
	BOOL readRes = FALSE;
	
	UARTWrite(1, "\r\nReading response...");
	ModbusBuf[MB_ADDR_IND] = 55;
	readRes = RS485Port.Read(&ModbusBuf[MB_ADDR_IND], MB_ADDR_LEN, 1000000);
	if(!readRes)
	{
		UARTWrite(1, "\r\nEC_NO_RESPONSE");			
		res.ec = EC_NO_RESPONSE;
		return res;
	}
	
	if(ModbusBuf[MB_ADDR_IND] != SlaveID)
	{	
		sprintf(loggBuff, "\r\nEC_UNEXPECTED_DATA: SlaveID: %d instead of %d.", (int)ModbusBuf[MB_ADDR_IND], (int)SlaveID);			
		UARTWrite(1, loggBuff);					
		res.ec = EC_UNEXPECTED_DATA;
		return res;
	}	
	
	readRes = RS485Port.Read(&ModbusBuf[MB_FCODE_IND], MB_FCODE_LEN + MB_PSIZE_LEN, InterbyteDelay_us);
	if(!readRes)
	{
		UARTWrite(1, "\r\nEC_INTERBYTE_TIMEOUT: while getting response header");				
		res.ec = EC_INTERBYTE_TIMEOUT;
		return res;
	}
	
	unsigned char registers_num = ModbusBuf[MB_PSIZE_IND] >> 1;
	sprintf(loggBuff, "\r\nAbout to read %d register values from slave device.", (int)registers_num);	
	
	if((ModbusBuf[MB_FCODE_IND] & 0x7F) != FC_Read_Input_Registers)
	{
		sprintf(loggBuff, "\r\nEC_UNEXPECTED_DATA: Function code is: %d instead of %d.", (int)(ModbusBuf[MB_FCODE_IND] & 0x7F), (int)fcode);			
		UARTWrite(1, loggBuff);			
		res.ec = EC_UNEXPECTED_DATA;
		return res;
	}	
	
	if(ModbusBuf[MB_FCODE_IND] & 0x80) // Modbus protocol error
	{
		sprintf(loggBuff, "\r\nModbus error: code: %d", (int)ModbusBuf[MB_PSIZE_IND]);			
		UARTWrite(1, loggBuff);			
		res.ec = ModbusBuf[MB_PSIZE_IND];
		return res;
	}		
	unsigned char payload_len = ModbusBuf[MB_PSIZE_IND];
	sprintf(loggBuff, "\r\nModbus payload length: %d", (int)payload_len);		
	readRes = RS485Port.Read(&ModbusBuf[MB_PLOAD_IND], ModbusBuf[MB_PSIZE_IND], InterbyteDelay_us);
	if(!readRes)
	{
		UARTWrite(1, "\r\nEC_INTERBYTE_TIMEOUT: while getting response payload");		
		res.ec = EC_INTERBYTE_TIMEOUT;
		return res;
	}
	
	unsigned char RecievedCRC[2] = {0, 0};
	readRes = RS485Port.Read(&RecievedCRC[0], MB_CRC_LEN, InterbyteDelay_us);
	if(!readRes)
	{
		UARTWrite(1, "\r\nEC_INTERBYTE_TIMEOUT: while getting response CRC");			
		res.ec = EC_INTERBYTE_TIMEOUT;
		return res;
	}	
	
	unsigned short PayloadCRC = CRC16(&ModbusBuf[MB_PLOAD_IND + 0], payload_len);
	
	if(RecievedCRC[0] != _RegHi(PayloadCRC) || RecievedCRC[1] != _RegLow(PayloadCRC))
	{		
		UARTWrite(1, "\r\EC_CRC_MISMATCH");			
		res.ec = EC_CRC_MISMATCH;
		return res;			
	}
	
	ModbusBuf[payload_len + MB_NON_PAYLOAD_LEN] = 0;	
	sprintf(loggBuff, "\r\nGot response from slave # %d\r\n%s", (int)SlaveID, (char*)ModbusBuf);			
		
	res.payload = (short*)&ModbusBuf[MB_PLOAD_IND];
	return res;
}

int MBM_Read(int v)
{
	return 0;
}

const struct ModbusMaster MBM = 
{
	_MBMInit,
	_ReadHRegisters
};





