/*! @file
 *
 *  @brief Routines for erasing and writing to the Flash.
 *
 *  This contains the functions needed for accessing the internal Flash.
 *
 *  @author PMcL
 *  @date 2015-08-07
 */



#ifndef FLASH_H
#define FLASH_H

/*!
*  @addtogroup Flash_module Flash module documentation
*  @{
*/

// new types
#include "types.h"
#include <stdbool.h>

// FLASH data access
/*!
 * Macro to cast to byte address
 */
#define _FB(flashAddress)  *(uint8_t  volatile *)(flashAddress)//byte
/*!
 * Macro to cast to half word address
 */
#define _FH(flashAddress)  *(uint16_t volatile *)(flashAddress)//half word
/*!
 * Macro to cast to word address
 */
#define _FW(flashAddress)  *(uint32_t volatile *)(flashAddress)//word
/*!
 * Macro to cast to phrase address
 */
#define _FP(flashAddress)  *(uint64_t volatile *)(flashAddress)//phrase
/*!
 * The length of the flash
 */
#define FLASH_SIZE 8
/*!
 * the command to write a phrase to flash
 */
#define CMD_PROGRAM_PHRASE 0x07
/*!
 * the command to clear a sector in flash
 */
#define CMD_CLEAR_SECTOR 0x09
/*!
 * Constant value that represents an unallocated place in memory
 */
#define CLEAR 0
/*!
 * Constant value that represents an allocated place in memory
 */
#define SETBIT 1

#define SECTOR_SIZE 8


//all flash commands we'll need
//0x09 = clear sector
//0x07 = program phrase


//struct for the 12 FCCOB registers, reg0 = FCCOB3, reg1 = FCCOB2, reg3 = FCCOB1, cmd = command
/*!
 * @struct TFCCOB Flash.h
 */
typedef struct
{
	uint8_t cmd;
	union {
		uint32_t address;
		struct
		{
			uint8_t reg0; //width: 8 (0:7)
			uint8_t reg1; //width: 8 (8:15)
			uint8_t reg2; //register width: 8, rest of the bits don't need to be used
			uint8_t reg3;
		} ADRStruct;
	} addressReg;
	uint64union_t data;
} TFCCOB;


//sector is 64 bits

//8 -> 16 -> 32
//32 handles actually writing
//32 - erase phrase, write lo word, write high word.

//launch command is the function that interfaces with chip

// Address of the start of the Flash block we are using for data storage
/*!
 * The start address of flash memory
 */
#define FLASH_DATA_START 0x00080000LU
//
/*!
 * The End address of flash memory
 */
#define FLASH_DATA_END   0x00080023LU
//This is the value of the flash if there is no data.
/*!
 *	The value of a clear byte of Flash
 */
#define CLEAR_DATA1 0xFFu

#define CLEAR_DATA2 0xFFFFu

#define CLEAR_DATA4 0xFFFFFFFFu

/*! @brief Enables the Flash module.
 *
 *  @return bool - TRUE if the Flash was setup successfully.
 */
bool Flash_Init(void);
 
/*! @brief Allocates space for a non-volatile variable in the Flash memory.
 *
 *  @param variable is the address of a pointer to a variable that is to be allocated space in Flash memory.
 *         The pointer will be allocated to a relevant address:
 *         If the variable is a byte, then any address.
 *         If the variable is a half-word, then an even address.
 *         If the variable is a word, then an address divisible by 4.
 *         This allows the resulting variable to be used with the relevant Flash_Write function which assumes a certain memory address.
 *         e.g. a 16-bit variable will be on an even address
 *  @param size The size, in bytes, of the variable that is to be allocated space in the Flash memory. Valid values are 1, 2 and 4.
 *  @return bool - TRUE if the variable was allocated space in the Flash memory.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_AllocateVar(volatile void** variable, const uint8_t size);

/*! @brief Writes a 32-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 32-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 4-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write32(volatile uint32_t* const address, const uint32_t data);
 
/*! @brief Writes a 16-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 16-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 2-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write16(volatile uint16_t* const address, const uint16_t data);

/*! @brief Writes an 8-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 8-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write8(volatile uint8_t* const address, const uint8_t data);

/*! @brief Erases the entire Flash sector.
 *
 *  @return bool - TRUE if the Flash "data" sector was erased successfully.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Erase(void);

//higher level function that calls erase & write
/*! @brief Higher level function which calls erase and write.
 *
 *  @return bool returns true if everything has been successfully executed.
 */
static bool ModifyPhrase(const uint32_t address, const uint64union_t phrase);

//these following functions call launch command with appropriate params
//private write phrase which will be called by write32
/*! @brief Private function which writes the phrase when called by Flash_Write32
 *
 *  @return bool returns true if everything has been successfully executed.
 */
static bool WritePhrase(const uint32_t address, const uint64union_t phrase);

//Erase Sector (only 1 sector to erase)
/*! @brief Private function which erases 1 sector of flash memory
 *
 *  @return bool returns true if everything has been successfully executed.
 */
static bool EraseSector(const uint32_t address);

//interfaces with K70 via Flash commands
/*! @brief Function which interfaces with the K70 chip to execute flash commands.
 *
 *  @return bool returns true if everything has been successfully executed.
 */
static bool LaunchCommand(TFCCOB* commonCommandObject);

/*! @brief Function which allocates data to its corresponding FTFE_FCCOB register
 *
 *  @return void
 */
static void ContructPhraseWriteCommand(TFCCOB* commonCommandObject, const uint32_t address, const uint64union_t phrase);
/*!
* @}
*/

#endif
