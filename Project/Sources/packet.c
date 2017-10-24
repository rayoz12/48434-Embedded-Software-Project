/*!
** @file packet.c
** @version 1.0
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *  @author 98112939, 99141145
 *  @date 8-08-2017
*/

/*!
*  @addtogroup Packet_module Packet module documentation
*  @{
*/

#include "packet.h"
#include "types.h"
#include "UART.h"
#include "stdbool.h"
#include "../Library/OS.h"

TPacket Packet;

OS_ECB *PacketSemaphore;

//Mask to flip the MSB in a byte
const uint8_t PACKET_ACK_MASK = 0x80;

static int PacketState = 0; //used in Packet_Get to maintain the packet byte counter.

static uint8_t Calc_Checksum(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3);

/*! @brief Initializes the packets by calling the initialization routines of the supporting software modules.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz
 *  @return bool - TRUE if the packet module was successfully initialized.
 */
bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
	PacketSemaphore = OS_SemaphoreCreate(1);
  return UART_Init(baudRate, moduleClk);
}

/*! @brief Attempts to get a packet from the received data.
 *
 *  @return bool - TRUE if a valid packet was received.
 */
bool Packet_Get(void)
{
  uint8_t recByte;//the received byte that is updated whenever UART_InChar is called.
  if(UART_InChar(&recByte)) //check if we can get a byte ftom UART and store it in recByte.
  {
    //switch depending on the the packet number of bytes received. (state machine)
    switch(PacketState)
    {
      case 0:
				Packet_Command = recByte;
				PacketState++;
				break;
      case 1:
				Packet_Parameter1 = recByte;
				PacketState++;
				break;
      case 2:
				Packet_Parameter2 = recByte;
				PacketState++;
				break;
      case 3:
				Packet_Parameter3 = recByte;
				PacketState++;
				break;
      case 4:
				Packet_Checksum = recByte;
				// checksum = Packet_Command ^ Packet_Parameter1 ^ Packet_Parameter2 ^ Packet_Parameter3;
				uint8_t checksum = Calc_Checksum(Packet_Command, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3); //calculate XOR checksum
				if(checksum != Packet_Checksum) //check if checksum is correct, if not shift up.
				{
					Packet_Command = Packet_Parameter1;
					Packet_Parameter1 = Packet_Parameter2;
					Packet_Parameter2 = Packet_Parameter3;
					Packet_Parameter3 = Packet_Checksum;
					return false;
				}
				else {
					PacketState = 0;
					return true;
				}
				break;
    }
  }
  return false;
}



/*! @brief Builds a packet and places it in the transmit FIFO buffer. Must also calculate checksum of arguments
 *
 *  @return bool - TRUE if a valid packet was sent.
 */
void Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
	OS_SemaphoreWait(PacketSemaphore,0);
  uint8_t checksum = Calc_Checksum(command, parameter1, parameter2, parameter3);
  UART_OutChar(command);
  UART_OutChar(parameter1);
  UART_OutChar(parameter2);
  UART_OutChar(parameter3);
  UART_OutChar(checksum);
  OS_SemaphoreSignal(PacketSemaphore);
  //return true;
}

/*! @brief Calculates the checksum of a packet.
 *
 *  @return unint8_t - the calculated checksum of a packet.
 */
static uint8_t Calc_Checksum(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
	return command ^ parameter1 ^ parameter2 ^ parameter3; //calculate XOR checksum
}
/*!
* @}
*/
