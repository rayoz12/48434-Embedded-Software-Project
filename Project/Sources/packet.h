/*! @file
 *
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *
 *  @author PMcL
 *  @date 2015-07-23
 */

#ifndef PACKET_H
#define PACKET_H

/*!
*  @addtogroup Packet_module Packet module documentation
*  @{
*/

// new types
#include "types.h"
#include "../Library/OS.h"

// Packet structure
/*!
 * The byte size of a packet
 */
#define PACKET_NB_BYTES 5

#pragma pack(push)
#pragma pack(1)

/*!
 * @union TPacket packet.h
 */
typedef union
{
  uint8_t bytes[PACKET_NB_BYTES];     /*!< The packet as an array of bytes. */
  struct
  {
    uint8_t command;		      /*!< The packet's command. */
    union
    {
      struct
      {
        uint8_t parameter1;	      /*!< The packet's 1st parameter. */
        uint8_t parameter2;	      /*!< The packet's 2nd parameter. */
        uint8_t parameter3;	      /*!< The packet's 3rd parameter. */
      } separate;
      struct
      {
        uint16_t parameter12;         /*!< Parameter 1 and 2 concatenated. */
        uint8_t parameter3;
      } combined12;
      struct
      {
        uint8_t paramater1;
        uint16_t parameter23;         /*!< Parameter 2 and 3 concatenated. */
      } combined23;
    } parameters;
    uint8_t checksum;
  } packetStruct;
} TPacket;

#pragma pack(pop)
/*!
 * The command byte of packet
 */
#define Packet_Command     Packet.packetStruct.command
/*!
 * The first parameter byte of packet
 */
#define Packet_Parameter1  Packet.packetStruct.parameters.separate.parameter1
/*!
 * The second parameter of packet
 */
#define Packet_Parameter2  Packet.packetStruct.parameters.separate.parameter2
/*!
 * The third parameter of packet
 */
#define Packet_Parameter3  Packet.packetStruct.parameters.separate.parameter3
/*!
 * Combined first and second parameter of packet
 */
#define Packet_Parameter12 Packet.packetStruct.parameters.combined12.parameter12
/*!
 * Combined second and third parameter of packet
 */
#define Packet_Parameter23 Packet.packetStruct.parameters.combined23.parameter23
/*!
 * The checksum parameter of packet
 */
#define Packet_Checksum    Packet.packetStruct.checksum

extern TPacket Packet;

extern OS_ECB *PacketSemaphore;

// Acknowledgment bit mask
extern const uint8_t PACKET_ACK_MASK;

/*! @brief Initializes the packets by calling the initialization routines of the supporting software modules.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz
 *  @return bool - TRUE if the packet module was successfully initialized.
 */
bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk);

/*! @brief Attempts to get a packet from the received data.
 *
 *  @return bool - TRUE if a valid packet was received.
 */
bool Packet_Get(void);

/*! @brief Builds a packet and places it in the transmit FIFO buffer. Must also calculate checksum of arguments
 *
 *  @return bool - TRUE if a valid packet was sent.
 */
void Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3);
/*!
* @}
*/
#endif
