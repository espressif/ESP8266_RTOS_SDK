#include "../csro/csro_common.h"

#ifndef MODBUS_H_
#define MODBUS_H_

#define ADDRESS_BROADCAST    				(  0 )   /*! Modbus broadcast address. */
#define ADDRESS_MIN         				(  1 )   /*! Smallest possible slave address. */
#define ADDRESS_MAX          				( 247)   /*! Biggest possible slave address. */

#define FUNC_NONE                          	(  0 )
#define FUNC_READ_COILS                    	(  1 )
#define FUNC_READ_DISCRETE_INPUTS          	(  2 )
#define FUNC_WRITE_SINGLE_COIL             	(  5 )
#define FUNC_WRITE_MULTIPLE_COILS          	( 15 )
#define FUNC_READ_HOLDING_REGISTER         	(  3 )
#define FUNC_READ_INPUT_REGISTER           	(  4 )
#define FUNC_WRITE_REGISTER                	(  6 )
#define FUNC_WRITE_MULTIPLE_REGISTERS      	( 16 )
#define FUNC_READWRITE_MULTIPLE_REGISTERS  	( 23 )
#define FUNC_DIAG_READ_EXCEPTION           	(  7 )
#define FUNC_DIAG_DIAGNOSTIC               	(  8 )
#define FUNC_DIAG_GET_COM_EVENT_CNT        	( 11 )
#define FUNC_DIAG_GET_COM_EVENT_LOG        	( 12 )
#define FUNC_OTHER_REPORT_SLAVEID          	( 17 )
#define FUNC_ERROR                         	( 128)

#define MODBUS_TIMEOUT						( 50)	// 50 == 0.5 second

uint16_t modbus_crc16(uint8_t *frame, uint8_t length);

#endif
