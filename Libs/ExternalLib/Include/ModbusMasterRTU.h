#ifndef MODBUS_MASTER_RTU_H
#define MODBUS_MASTER_RTU_H

#include "ModbusSerial.h"

enum MODBUS_FUNCTION_CODE
{
	FC_Read_Coils 				= 0x01,
	FC_Read_Discrete_Inputs 	= 0x02,
	FC_Read_Holding_Registers 	= 0x03,
	FC_Read_Input_Registers		= 0x04,
	FC_Write_Single_Coil 		= 0x05,
	FC_Write_Single_Register 	= 0x06,
	FC_Write_Multiple_Coils 	= 0x0F,	
	FC_Write_Multiple_Registers	= 0x10,		
	FC_Report_Slave_ID 			= 0x11
	
	// TODO: Add here all the remaining function codes
};

enum MODBUS_READ_TYPE
{
	Coils 		= FC_Read_Coils,
	DInputs	 	= FC_Read_Discrete_Inputs,
	HRegisters 	= FC_Read_Holding_Registers,
	IRegister 	= FC_Read_Input_Registers
};

enum MODBUS_ERROR_CODE
{
	EC_NO_ERROR = 0x00,
	EC_ILLEGAL_FUNCTION = 0x01,
/*
The function code received in the query is not an
allowable action for the server (or slave). This
may be because the function code is only
applicable to newer devices, and was not
implemented in the unit selected. It could also
indicate that the server (or slave) is in the wrong
state to process a request of this type, for
example because it is unconfigured and is being
asked to return register values.
*/

	EC_ILLEGAL_DATA_ADDRESS = 0x02,
/*	
The data address received in the query is not an
allowable address for the server (or slave). More
specifically, the combination of reference number
and transfer length is invalid. For a controller with
100 registers, the PDU addresses the first
register as 0, and the last one as 99. If a request
is submitted with a starting register address of 96
and a quantity of registers of 4, then this request
will successfully operate (address-wise at least)
on registers 96, 97, 98, 99. If a request is
submitted with a starting register address of 96
and a quantity of registers of 5, then this request
will fail with Exception Code 0x02 “Illegal Data
Address” since it attempts to operate on registers
96, 97, 98, 99 and 100, and there is no register
with address 100.
*/

	EC_ILLEGAL_DATA_VALUE = 0x03,
/*
A value contained in the query data field is not an
allowable value for server (or slave). This
indicates a fault in the structure of the remainder
of a complex request, such as that the implied
length is incorrect. It specifically does NOT mean
that a data item submitted for storage in a register
has a value outside the expectation of the
application program, since the MODBUS protocol
is unaware of the significance of any particular
value of any particular register.
*/

	EC_SLAVE_DEVICE_FAILURE = 0x04,
/*	
An unrecoverable error occurred while the server
(or slave) was attempting to perform the
requested action.
*/

	EC_ACKNOWLEDGE = 0x05,
/*	
Specialized use in conjunction with programming
commands.
The server (or slave) has accepted the request
and is processing it, but a long duration of time
will be required to do so. This response is
returned to prevent a timeout error from occurring
in the client (or master). The client (or master)
can next issue a Poll Program Complete message
to determine if processing is completed.
*/

	EC_SLAVE_DEVICE_BUSY = 0x06,
/*	
Specialized use in conjunction with programming
commands.
The server (or slave) is engaged in processing a
long–duration program command. The client (or
master) should retransmit the message later when
the server (or slave) is free.
*/

	EC_MEMORY_PARITY_ERROR = 0x08,
/*	
Specialized use in conjunction with function codes
20 and 21 and reference type 6, to indicate that
the extended file area failed to pass a consistency
check.
The server (or slave) attempted to read record
file, but detected a parity error in the memory.
The client (or master) can retry the request, but
service may be required on the server (or slave)
device.
*/

	EC_GATEWAY_PATH_UNAVAILABLE = 0x0A,
/*	
Specialized use in conjunction with gateways,
indicates that the gateway was unable to allocate
an internal communication path from the input
port to the output port for processing the request.
Usually means that the gateway is misconfigured
or overloaded.
*/

	EC_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND = 0x0B,
/*	
Specialized use in conjunction with gateways,
indicates that no response was obtained from the
target device. Usually means that the device is
not present on the network.	
*/

	EC_NO_RESPONSE = 0x11,
	EC_INTERBYTE_TIMEOUT = 0x12,
	EC_UNEXPECTED_DATA = 0x13,
	EC_CRC_MISMATCH = 0x14,
	EC_TOO_MANY_REGISTERS = 0x15	
};

struct ReadRegistersResp
{
	enum MODBUS_ERROR_CODE ec;
	short * payload;
	short Qnty;
};

void _MBMInit(const struct SerialPort * Serial);	
struct ReadRegistersResp _ReadHRegisters(unsigned char SlaveID, const short StartAddr, const short RequestedQnty);
struct ReadRegistersResp _ReadIRegisters(unsigned char SlaveID, const short StartAddr, const short RequestedQnty);
enum MODBUS_ERROR_CODE _WriteMultipleRegister(unsigned char SlaveID, const short StartAddr, const unsigned char Qnty, const short* NewValues);enum MODBUS_ERROR_CODE _WriteSingleRegister(unsigned char SlaveID, const short Addr, const short NewValue);
void dump(const unsigned char* buf, unsigned char len);	
unsigned short CRC16(unsigned char * pcBlock, unsigned short len);

void _SendRegistersReq(unsigned char SlaveID, unsigned char ReqType, const short StartAddr, const short RequestedQnty);
enum MODBUS_ERROR_CODE _SendWriteRegistersReq(unsigned char SlaveID, unsigned char ReqType, const short StartAddr, const unsigned char Qnty, const short* NewValues);
enum MODBUS_ERROR_CODE _ReadWriteConfirmResponse(unsigned char SlaveID, unsigned char ReqType, unsigned short* Address, unsigned short* Data);
enum MODBUS_ERROR_CODE _ReadModbusResponse(unsigned char SlaveID, unsigned char ReqType, unsigned char** PayloadPointer, unsigned char* PayloadSize);
enum MODBUS_ERROR_CODE _ReadResponseHeader(unsigned char* PayloadLen, unsigned char SlaveID, unsigned char ReqType);
enum MODBUS_ERROR_CODE _ReadAndCheckCRC(unsigned char PayloadLen);

struct ModbusMaster
{
	void (*Init)(const struct SerialPort * Serial);		
	struct ReadRegistersResp (*ReadHoldingRegisters)(unsigned char SlaveID, const short StartAddr, const short RequestedQnty);	
	struct ReadRegistersResp (*ReadInputRegisters)(unsigned char SlaveID, const short StartAddr, const short RequestedQnty);		
	enum MODBUS_ERROR_CODE (*WriteMultipleRegisters)(unsigned char SlaveID, const short StartAddr, const unsigned char Qnty, const short* NewValues);	
	enum MODBUS_ERROR_CODE (*WriteSingleRegister)(unsigned char SlaveID, const short Addr, const short NewValue);	
};


int MBM_Read(int v);

#endif








