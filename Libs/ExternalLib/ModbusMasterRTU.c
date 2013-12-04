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
		
	if(InterbyteDelay_us == 2005)
	    UARTWrite(1,"InterbyteDelay is set to 2005 us.\r\n");	
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
	struct ReadRegistersResp res;
	res.ec = 0;
	res.payload = 0;
	
	unsigned char total_len = 0;
	ModbusBuf[MB_ADDR_IND] = SlaveID;
	ModbusBuf[MB_FCODE_IND] = FC_Read_Input_Registers;
	ModbusBuf[MB_PSIZE_IND] = MB_STARTING_ADDRESS_LEN + MB_QUANTITY_LEN;	
	total_len = ModbusBuf[MB_PSIZE_IND];

	ModbusBuf[MB_PLOAD_IND + 0] = _RegHi(StartAddr);
	ModbusBuf[MB_PLOAD_IND + 1] = _RegLow(StartAddr);	
	
	ModbusBuf[MB_PLOAD_IND + 2] = _RegHi(RequestedQnty);
	ModbusBuf[MB_PLOAD_IND + 3] = _RegLow(RequestedQnty);
	
	unsigned short CRC = CRC16(&ModbusBuf[MB_PLOAD_IND + 0], ModbusBuf[MB_PSIZE_IND]);
	
	ModbusBuf[MB_PLOAD_IND + 4] = _RegHi(CRC);
	ModbusBuf[MB_PLOAD_IND + 5] = _RegLow(CRC);		
	
	total_len += MB_NON_PAYLOAD_LEN;
	
	RS485Port.Write(ModbusBuf, total_len);
	
	BOOL readRes = FALSE;
	
	readRes = RS485Port.Read(&ModbusBuf[MB_ADDR_IND], MB_ADDR_LEN, 1000000);
	if(!readRes)
	{
		res.ec = EC_NO_RESPONSE;
		return res;
	}
	
	if(ModbusBuf[MB_ADDR_IND] != SlaveID)
	{
		res.ec = EC_UNEXPECTED_DATA;
		return res;
	}	
	
	readRes = RS485Port.Read(&ModbusBuf[MB_FCODE_IND], MB_FCODE_LEN + MB_PSIZE_LEN, InterbyteDelay_us);
	if(!readRes)
	{
		res.ec = EC_INTERBYTE_TIMEOUT;
		return res;
	}
	
	if((ModbusBuf[MB_FCODE_IND] & 0x7F) != FC_Read_Input_Registers)
	{
		res.ec = EC_UNEXPECTED_DATA;
		return res;
	}	
	
	if(ModbusBuf[MB_FCODE_IND] & 0x80) // Modbus protocol error
	{
		res.ec = ModbusBuf[MB_PSIZE_IND];
		return res;
	}		
	
	readRes = RS485Port.Read(&ModbusBuf[MB_PLOAD_IND], ModbusBuf[MB_PSIZE_IND], InterbyteDelay_us);
	if(!readRes)
	{
		res.ec = EC_INTERBYTE_TIMEOUT;
		return res;
	}
	
unsigned char RecievedCRC[2] = {0, 0};
		
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



