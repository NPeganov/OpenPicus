#include "ModbusMasterRTU.h"
#include "RS485Helper.h"

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
#define MB_REQ_IND 2	

//The length of header bytes
#define MB_HEADER_LEN MB_ADDR_LEN + MB_FCODE_LEN + MB_PSIZE_LEN	

//The length of non-payload bytes
#define MB_NON_PAYLOAD_LEN MB_HEADER_LEN + MB_CRC_LEN

unsigned char ModbusBuf[256 + MB_NON_PAYLOAD_LEN];
static struct SerialPort SPort;
DWORD InterbyteTimeout_us = 0;
const DWORD StartByteTimeout_us = 10000000; // one second

void _MBMInit(const struct SerialPort * Serial)
{
	SPort = *Serial;
	
    if (SPort.GetBaud() < 19201)//Delay is (1 / (baudrate/11)) * 3.5 seconds, assumming that one real byte is equal to 11 bits
		InterbyteTimeout_us = (((11 * 3.5) / SPort.GetBaud()) * 1000000);
    else
		InterbyteTimeout_us = 1750;
		
	char loggBuff[64];		
	sprintf(loggBuff, "\r\nInterbyteTimeout_us is set to %lu us.\r\n", InterbyteTimeout_us);		
    UARTWrite(1, loggBuff);	
}

unsigned char _RegHi(unsigned short source)
{
	return (unsigned char)((source >> 8) & 0x00FF);	
}

unsigned char _RegLow(unsigned short source)
{
	return (unsigned char)((source) & 0x00FF);	
}


struct ReadRegistersResp _ReadHRegisters(unsigned char SlaveID, const short StartAddr, const short RequestedQnty)
{
	UARTWrite(1, "\r\n_ReadHRegisters...");		
	struct ReadRegistersResp res;
	res.ec = EC_NO_ERROR;
	res.payload = 0;
	res.Qnty = 0;
	unsigned char ReqType = FC_Read_Holding_Registers;	

	_SendRegistersReq(SlaveID, ReqType, StartAddr, RequestedQnty);

	unsigned char* PayloadPointer = 0;
	unsigned char PayloadSize = 0;
	res.ec = _ReadModbusResponse(SlaveID, ReqType, &PayloadPointer, &PayloadSize);
	
	if(res.ec == EC_NO_ERROR)
	{
		UARTWrite(1, "\r\n_ReadHRegisters... Response is correct");		
		res.Qnty = (PayloadSize >> 1);
		res.payload = (short*)malloc(sizeof(short) * res.Qnty);
		unsigned char i = 0;
		for(i = 0; i < res.Qnty; ++i)
		{
			short value = 0;
			
			value |= *PayloadPointer << 8;
			PayloadPointer++;
			value |= *PayloadPointer;
			PayloadPointer++;
			res.payload[i] = value;			
		}
	}
		
	return res;
}

struct ReadRegistersResp _ReadIRegisters(unsigned char SlaveID, const short StartAddr, const short RequestedQnty)
{
	UARTWrite(1, "\r\n_ReadHRegisters...");		
	struct ReadRegistersResp res;
	res.ec = EC_NO_ERROR;
	res.payload = 0;
	res.Qnty = 0;
	unsigned char ReqType = FC_Read_Input_Registers;

	_SendRegistersReq(SlaveID, ReqType, StartAddr, RequestedQnty);

	unsigned char* PayloadPointer = 0;
	unsigned char PayloadSize = 0;
	res.ec = _ReadModbusResponse(SlaveID, ReqType, &PayloadPointer, &PayloadSize);			
	
	if(res.ec == EC_NO_ERROR)
	{
		UARTWrite(1, "\r\n_ReadHRegisters... Response is correct");
	
		res.Qnty = (PayloadSize >> 1);
		res.payload = (short*)malloc(sizeof(short) * res.Qnty);
		unsigned char i = 0;
		for(i = 0; i < res.Qnty; ++i)
		{
			short value = 0;
			
			value |= *PayloadPointer << 8;
			PayloadPointer++;
			value |= *PayloadPointer;
			PayloadPointer++;
			res.payload[i] = value;			
		}
	}
		
	return res;
}

enum MODBUS_ERROR_CODE _WriteSingleRegister(unsigned char SlaveID, const short Addr, const short NewValue)
{
	UARTWrite(1, "\r\n_WriteSingleRegister...");		
	unsigned char ReqType = FC_Write_Single_Register;

	_SendRegistersReq(SlaveID, ReqType, Addr, NewValue);

	unsigned short Address = 0;
	unsigned short Value = 0;
	return _ReadWriteConfirmResponse(SlaveID, ReqType, &Address, &Value);
}

enum MODBUS_ERROR_CODE _WriteMultipleRegister(unsigned char SlaveID, const short StartAddr, const unsigned char Qnty, const short* NewValues)
{
	UARTWrite(1, "\r\n_WriteMultipleRegister...");		
	unsigned char ReqType = FC_Write_Multiple_Registers;
	
	_SendWriteRegistersReq(SlaveID, ReqType, StartAddr, Qnty, NewValues);		
	unsigned short Address = 0;
	unsigned short Value = 0;
	return _ReadWriteConfirmResponse(SlaveID, ReqType, &Address, &Value);	
}

enum MODBUS_ERROR_CODE _SendWriteRegistersReq(unsigned char SlaveID, unsigned char ReqType, const short StartAddr, const unsigned char Qnty, const short* NewValues)
{
	UARTWrite(1, "\r\n_SendWriteRegistersReq...");	
	
	if(Qnty > 123)
	{
		UARTWrite(1, "\r\nError: Too many registers to write!!!...");	
		return EC_TOO_MANY_REGISTERS;		
	}
	
	ModbusBuf[MB_ADDR_IND] = SlaveID;
	ModbusBuf[MB_FCODE_IND] = ReqType;

	ModbusBuf[MB_REQ_IND + 0] = _RegHi(StartAddr);
	ModbusBuf[MB_REQ_IND + 1] = _RegLow(StartAddr);	
	
	ModbusBuf[MB_REQ_IND + 2] = _RegHi(Qnty);
	ModbusBuf[MB_REQ_IND + 3] = _RegLow(Qnty);
	ModbusBuf[MB_REQ_IND + 4] = Qnty * MB_REGISTER_LEN;	
	
	unsigned char* pNewValues = &ModbusBuf[MB_REQ_IND + 5];
	
	unsigned char i = 0;
	for(; i < Qnty; ++i)
	{	
		*pNewValues = _RegHi(NewValues[i]);++pNewValues;
		*pNewValues = _RegLow(NewValues[i]);++pNewValues;		
	}

	const unsigned char ReqLen = MB_ADDR_LEN + MB_FCODE_LEN + MB_STARTING_ADDRESS_LEN + MB_QUANTITY_LEN + MB_PSIZE_LEN/*Byte Count*/;	
	unsigned char total_len = ReqLen + ModbusBuf[MB_REQ_IND + 4];	
	
	unsigned short CRC = CRC16(&ModbusBuf[MB_ADDR_IND], total_len);		
	ModbusBuf[total_len + 0] = _RegLow(CRC);// LOW byte goes first!!!
	ModbusBuf[total_len + 1] = _RegHi(CRC);		
	
	UARTWrite(1, "\r\nSending request...");		
	dump(ModbusBuf, total_len + MB_CRC_LEN);	
	SPort.Write(ModbusBuf, total_len + MB_CRC_LEN);		
	return EC_NO_ERROR;
}

void _SendRegistersReq(unsigned char SlaveID, unsigned char ReqType, const short StartAddr, const short RequestedQnty)
{
	const unsigned char RequestLen = MB_ADDR_LEN + MB_FCODE_LEN + MB_STARTING_ADDRESS_LEN + MB_QUANTITY_LEN + MB_CRC_LEN;
	unsigned char RequestBuffer[RequestLen];
	UARTWrite(1, "\r\n_SendRegistersReq...");		
	
	RequestBuffer[MB_ADDR_IND] = SlaveID;
	RequestBuffer[MB_FCODE_IND] = ReqType;

	RequestBuffer[MB_REQ_IND + 0] = _RegHi(StartAddr);
	RequestBuffer[MB_REQ_IND + 1] = _RegLow(StartAddr);	
	
	RequestBuffer[MB_REQ_IND + 2] = _RegHi(RequestedQnty);
	RequestBuffer[MB_REQ_IND + 3] = _RegLow(RequestedQnty);

	unsigned char total_len = MB_ADDR_LEN + MB_FCODE_LEN + MB_STARTING_ADDRESS_LEN + MB_QUANTITY_LEN;	
	
	unsigned short CRC = CRC16(&RequestBuffer[MB_ADDR_IND], total_len);		
	RequestBuffer[MB_REQ_IND + 4] = _RegLow(CRC);// LOW byte goes first!!!
	RequestBuffer[MB_REQ_IND + 5] = _RegHi(CRC);		
	
	UARTWrite(1, "\r\nSending request...");		
	dump(RequestBuffer, RequestLen);	
	SPort.Write(RequestBuffer, RequestLen);
}

enum MODBUS_ERROR_CODE _ReadResponseHeader(unsigned char* PayloadLen, unsigned char SlaveID, unsigned char ReqType)
{
	char loggBuff[128];
	BOOL readRes = FALSE;
	
	UARTWrite(1, "\r\nReading response header...");
	readRes = SPort.Read(&ModbusBuf[MB_ADDR_IND], MB_ADDR_LEN, StartByteTimeout_us);
	if(!readRes)
	{
		UARTWrite(1, "\r\nEC_NO_RESPONSE");			
		return EC_NO_RESPONSE;
	}
	
	if(ModbusBuf[MB_ADDR_IND] != SlaveID)
	{	
		sprintf(loggBuff, "\r\nEC_UNEXPECTED_DATA: SlaveID: 0x%02hhX instead of 0x%02hhX.", ModbusBuf[MB_ADDR_IND], SlaveID);			
		UARTWrite(1, loggBuff);					
		return EC_UNEXPECTED_DATA;
	}	
	
	readRes = SPort.Read(&ModbusBuf[MB_FCODE_IND], MB_FCODE_LEN + MB_PSIZE_LEN, InterbyteTimeout_us);
	if(!readRes)
	{
		UARTWrite(1, "\r\nEC_INTERBYTE_TIMEOUT: while getting response header");				
		return EC_INTERBYTE_TIMEOUT;
	}
	
	if((ModbusBuf[MB_FCODE_IND] & 0x7F) != ReqType)
	{
		sprintf(loggBuff, "\r\nEC_UNEXPECTED_DATA: Function code is: 0x%02hhX instead of 0x%02hhX.", (ModbusBuf[MB_FCODE_IND] & 0x7F), ReqType);			
		UARTWrite(1, loggBuff);			
		return EC_UNEXPECTED_DATA;
	}	
	
	UARTWrite(1, "\r\nGot modbus response header:");
	dump(ModbusBuf, MB_HEADER_LEN);	
	
	if(ModbusBuf[MB_FCODE_IND] & 0x80) // Modbus protocol error
	{
		sprintf(loggBuff, "\r\nModbus error: code: 0x%02hhX", ModbusBuf[MB_PSIZE_IND]);			
		UARTWrite(1, loggBuff);			
		return ModbusBuf[MB_PSIZE_IND];
	}		
	
	sprintf(loggBuff, "\r\nPayload size is 0x%02hhX.", ModbusBuf[MB_PSIZE_IND]);	
	UARTWrite(1, loggBuff);		
	
	*PayloadLen = ModbusBuf[MB_PSIZE_IND];
	return EC_NO_ERROR;	
}

enum MODBUS_ERROR_CODE _ReadWriteConfirmResponse(unsigned char SlaveID, unsigned char ReqType, unsigned short* Address, unsigned short* Data)
{
	char loggBuff[128];
	BOOL readRes = FALSE;
	enum MODBUS_ERROR_CODE mb_res = EC_NO_ERROR;	
	
	UARTWrite(1, "\r\nReading write request confirmation...");
	readRes = SPort.Read(&ModbusBuf[MB_ADDR_IND], MB_ADDR_LEN, StartByteTimeout_us);
	if(!readRes)
	{
		UARTWrite(1, "\r\nEC_NO_RESPONSE");			
		return EC_NO_RESPONSE;
	}
	
	if(ModbusBuf[MB_ADDR_IND] != SlaveID)
	{	
		sprintf(loggBuff, "\r\nEC_UNEXPECTED_DATA: SlaveID: 0x%02hhX instead of 0x%02hhX.", ModbusBuf[MB_ADDR_IND], SlaveID);			
		UARTWrite(1, loggBuff);					
		return EC_UNEXPECTED_DATA;
	}	
	
	readRes = SPort.Read(&ModbusBuf[MB_FCODE_IND], MB_FCODE_LEN + (MB_STARTING_ADDRESS_LEN / 2)/*=1*/, InterbyteTimeout_us);
	if(!readRes)
	{
		UARTWrite(1, "\r\nEC_INTERBYTE_TIMEOUT: while getting write request confirmation");				
		return EC_INTERBYTE_TIMEOUT;
	}
	
	if((ModbusBuf[MB_FCODE_IND] & 0x7F) != ReqType)
	{
		sprintf(loggBuff, "\r\nEC_UNEXPECTED_DATA: Function code is: 0x%02hhX instead of 0x%02hhX.", (ModbusBuf[MB_FCODE_IND] & 0x7F), ReqType);			
		UARTWrite(1, loggBuff);			
		return EC_UNEXPECTED_DATA;
	}	
	
	UARTWrite(1, "\r\nGot modbus response header:");
	dump(ModbusBuf, MB_HEADER_LEN);	
	
	if(ModbusBuf[MB_FCODE_IND] & 0x80) // Modbus protocol error
	{
		sprintf(loggBuff, "\r\nModbus error: code: 0x%02hhX", ModbusBuf[MB_PSIZE_IND]);			
		UARTWrite(1, loggBuff);			
		return ModbusBuf[MB_PSIZE_IND];
	}		

	readRes = SPort.Read(&ModbusBuf[MB_PLOAD_IND], (MB_STARTING_ADDRESS_LEN / 2)/*=1*/ + MB_REGISTER_LEN, InterbyteTimeout_us);
	if(!readRes)
	{
		UARTWrite(1, "\r\nEC_INTERBYTE_TIMEOUT: while getting the rest of a write request confirmation");				
		return EC_INTERBYTE_TIMEOUT;
	}	
	
	mb_res = _ReadAndCheckCRC((MB_STARTING_ADDRESS_LEN / 2)/*=1*/ + MB_REGISTER_LEN);
	if(mb_res != EC_NO_ERROR)
		return mb_res;
	
	*Address = (ModbusBuf[MB_FCODE_IND] << 8) | ModbusBuf[MB_PLOAD_IND];
	*Data = (ModbusBuf[MB_PLOAD_IND + 1] << 8) | ModbusBuf[MB_PLOAD_IND + 2];	
	
	sprintf(loggBuff, "\r\nWrite request confirmation: (start) address 0x%04hX, quantity 0x%04hX", *Address, *Data);	
	UARTWrite(1, loggBuff);			
	
	return EC_NO_ERROR;	
}

enum MODBUS_ERROR_CODE _ReadAndCheckCRC(unsigned char PayloadLen)
{
	const unsigned char MB_CRC_IND = MB_HEADER_LEN + PayloadLen;
	BOOL readRes = SPort.Read(&ModbusBuf[MB_CRC_IND], MB_CRC_LEN, InterbyteTimeout_us);
	if(!readRes)
	{
		UARTWrite(1, "\r\nEC_INTERBYTE_TIMEOUT: while getting response CRC");			
		return EC_INTERBYTE_TIMEOUT;
	}	
	
	unsigned short PayloadCRC = CRC16(&ModbusBuf[MB_ADDR_IND], MB_HEADER_LEN + PayloadLen);
	
	if(ModbusBuf[MB_CRC_IND] != _RegLow(PayloadCRC) || ModbusBuf[MB_CRC_IND + 1] != _RegHi(PayloadCRC))//LOW byte goes first
	{		
		UARTWrite(1, "\r\nEC_CRC_MISMATCH");			
		return EC_CRC_MISMATCH;
	}	
	
	UARTWrite(1, "\r\nGot CRC:");
	dump(&ModbusBuf[MB_CRC_IND], MB_CRC_LEN);	

	return EC_NO_ERROR;	
}

enum MODBUS_ERROR_CODE _ReadModbusResponse(unsigned char SlaveID, unsigned char ReqType, unsigned char** PayloadPointer, unsigned char* PayloadSize)
{
	char loggBuff[64];
	enum MODBUS_ERROR_CODE mb_res = EC_NO_ERROR;
	unsigned char PayloadLen = 0;
	
	mb_res = _ReadResponseHeader(&PayloadLen, SlaveID, ReqType);
	if(mb_res != EC_NO_ERROR)
		return mb_res;				

	BOOL readRes = SPort.Read(&ModbusBuf[MB_PLOAD_IND], PayloadLen, InterbyteTimeout_us);
	if(!readRes)
	{
		UARTWrite(1, "\r\nEC_INTERBYTE_TIMEOUT: while getting response payload");		
		return EC_INTERBYTE_TIMEOUT;
	}
	else
	{
		UARTWrite(1, "\r\nGot response payload:");
		dump(&ModbusBuf[MB_PLOAD_IND], PayloadLen);	
	}
	
	mb_res = _ReadAndCheckCRC(PayloadLen);
	if(mb_res != EC_NO_ERROR)
		return mb_res;				
	
	sprintf(loggBuff, "\r\nGot response from slave 0x%02hhX:", SlaveID);
	UARTWrite(1, loggBuff);	
	dump(ModbusBuf, MB_HEADER_LEN + PayloadLen + MB_CRC_LEN);
	
	*PayloadPointer = &ModbusBuf[MB_PLOAD_IND];
	*PayloadSize = PayloadLen;
	return EC_NO_ERROR;	
}

int MBM_Read(int v)
{
	return 0;
}

void dump(const unsigned char* buf, unsigned char len)
{
	char loggBuff[8];
	UARTWrite(1, loggBuff);
	unsigned char i = 0;
	UARTWrite(1, "\r\n");	
	for(i = 0; i < len; ++i)
	{
		sprintf(loggBuff, "0x%02hhX ", buf[i]);
		UARTWrite(1, loggBuff);			
	}
	UARTWrite(1, "\r\n");		
}

unsigned short CRC16(unsigned char * pcBlock, unsigned short len)
{
	char loggBuff[64];
	
	int i = 0, i8 = 0;
	unsigned short CRC = 0xFFFF;
	unsigned char *ptr_ib = pcBlock;
	unsigned char CRC_S;

	for (i = 0; i < len; i++)
	{
		CRC = (CRC & 0xFF00) | ((CRC & 0x00FF) ^ *ptr_ib++);
		for(i8 = 0; i8 < 8; i8++)
        {
			CRC_S = CRC & 0x0001;
			CRC = CRC >> 1;
			if(CRC_S == 1)
                CRC = CRC ^ 0xA001;
        }
	}
	
	sprintf(loggBuff, "CRC: 0x%04hX, CRC_H: 0x%02hhX, CRC_L: 0x%02hhX", CRC, (unsigned char)(CRC >> 8), (unsigned char)(CRC & 0x00FF));	
	//UARTWrite(1, "\r\n");		
	//UARTWrite(1, loggBuff);			
	//UARTWrite(1, "\r\n");  
  
  return CRC;
}


const struct ModbusMaster MBM = 
{
	_MBMInit,
	_ReadHRegisters,
	_ReadIRegisters,
	_WriteMultipleRegister,
	_WriteSingleRegister
};




