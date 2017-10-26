/*
 * Flash.c
 *
 *  Created on: 14 Aug 2017
 *      Author: 99141145
 */

/*!
 *  @addtogroup Flash_module Flash module documentation
 *  @{
 */
#include "Flash.h"
#include "MK70F12.h"
#include <string.h>
#include <stdlib.h>


static uint8_t MemoryMap[FLASH_SIZE] =
  {CLEAR}; //set it to be clear.

/*! @brief Enables the Flash module.
 *
 *  @return bool - TRUE if the Flash was setup successfully.
 */
bool Flash_Init(void)
{
  //SIM_SCGC3 |= SIM_SCGC3_NFC_MASK; //enable NFC clock
  return true;
}

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
bool Flash_AllocateVar(volatile void** variable, const uint8_t size)
{
  //allocate some space and return address variable
  //provides offset for next available address space
  //FIXME
  //https://stackoverflow.com/questions/6938219/how-to-check-whether-all-bytes-in-a-memory-block-are-zero
  //for 1 byte
  int i;
  char clearBlock[4]; //a block of memory thats set to 0 to compare with memory map.
  //Convert this to a function
  switch (size)
  {
    case 1:
      for (i = 0; i < FLASH_SIZE; i++)
      {
        if (MemoryMap[i] == 0)
        {
          //found a byte that has not been allocated yet.
          MemoryMap[i] = 1;
          *variable = (void *)FLASH_DATA_START + i;
          return true;
          break;
        }
      }
      break;
    case 2:
    case 4:
      memset(clearBlock, 0, sizeof(clearBlock));
      for (i = 0; i < FLASH_SIZE; i++)
      {
        if (MemoryMap[i] == 0)
        {
          //found a byte that has not been allocated yet.
          //compare clear block with this block of memory found
          if (!memcmp(clearBlock, (void *)&MemoryMap[i], size))
          {
            //all 0,return true
            memset(&MemoryMap[i], 1, size);					//set this space as taken
            *variable = (void *)FLASH_DATA_START + i;
            return true;
          }
        }
      }
      break;
  }
  return false;
}

/*! @brief Writes a 32-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 32-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 4-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write32(volatile uint32_t* const address, const uint32_t data)
{
  // we have to check that the address is 8 byte aligned, probably by using modulo.
  // this gets the word and the next word and calls Modify_Phrase to write data
  uint32_t offset = (uint32_t)address - (uint32_t)FLASH_DATA_START;
  uint32_t sector = offset / 8;
  uint32_t sectorStart = (uint32_t)FLASH_DATA_START + (SECTOR_SIZE * sector);//get address at start of sector

  uint32_t index = (uint32_t)address - sectorStart;
  uint32_t divby4 = (uint32_t)address % 4;
  uint64union_t bit64;
  if (divby4 == 0)
  {
    if (index > 3)
    {
      bit64.s32.Hi = data;
      bit64.s32.Lo = *(address - 1);
    }
    else
    {
      bit64.s32.Hi = *(address + 1);
      bit64.s32.Lo = data;
    }
  }
  else
  {
    return false;
  }
//  assert((uint32_t)address % 64 == 0); //ensure that the address is 64 bit or sector aligned.
  //change this to start of sector
  return ModifyPhrase(sectorStart, bit64);
}

/*! @brief Writes a 16-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 16-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 2-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write16(volatile uint16_t* const address, const uint16_t data)
{
  //combine the passed 16 bit (half-word) with the next 16 bit block to call _Write32
  uint32_t offset = (uint32_t)address - (uint32_t)FLASH_DATA_START;
  uint32_t sector = offset / 8;
  uint32_t sectorStart = (uint32_t)FLASH_DATA_START + (SECTOR_SIZE * sector);//get address at start of sector

  uint32_t index = (uint32_t)address - sectorStart;
  uint32_t odd = (uint32_t)address % 2;
  uint32union_t bit32;
  if (!odd)
  {
    if (index == 2 || index == 6)
    {
      bit32.s.Lo = *(address - 1);
      bit32.s.Hi = data;
      return Flash_Write32((uint32_t *)(address - 1), bit32.l);
    }
    else
    {
      bit32.s.Lo = data;
      bit32.s.Hi = *(address + 1);
      return Flash_Write32((uint32_t *)(address), bit32.l);
    }
  }
  else
  {
    return false;
  }

  //uint64union_t tempData;
  //tempData.s32.Hi = 0;
  //tempData.s32.Lo = bit32.l;
  //ModifyPhrase((uint32_t)BLOCK2_SECT0_ABSOLUTE_ADDRESS, tempData);
}

/*! @brief Writes an 8-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 8-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write8(volatile uint8_t* const address, const uint8_t data)
{
  uint32_t evenAddress = ((uint32_t)address % 2) == 0;
  uint16union_t bit16;
  if (evenAddress)
  {
    bit16.s.Lo = data;
    bit16.s.Hi = *(address + 1);
    return Flash_Write16((uint16_t *)(address), bit16.l);
  }
  else
  {
    bit16.s.Lo = *(address - 1);
    bit16.s.Hi = data;
    return Flash_Write16((uint16_t *)(address - 1), bit16.l);
  }
}

/*! @brief Erases the entire Flash sector.
 *
 *  @return bool - TRUE if the Flash "data" sector was erased successfully.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Erase(void)
{
  bool success = true;
  for (int i=0;i < FLASH_SIZE;i+=8)
    success = success && EraseSector(FLASH_DATA_START + i);
  return success;
}

/*! @brief Higher level function which calls erase and write.
 *
 *  @return bool returns true if everything has been successfully executed.
 */
static bool ModifyPhrase(const uint32_t address, const uint64union_t phrase)
{
  //this calls EraseSector and writePhrase. We don't care about mismatched size cos only writing to one phrase.
  if (!EraseSector(address))
    return false;
  return WritePhrase(address, phrase);
}

/*! @brief Private function which writes the phrase when called by Flash_Write32
 *
 *  @return bool returns true if everything has been successfully executed.
 */
static bool WritePhrase(const uint32_t address, const uint64union_t phrase)
{
  //calls LaunchCommand
  //not ready to execute another command
  if (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK))
    return false;

  //clear error flags before executing command
  if (FTFE_FSTAT & FTFE_FSTAT_ACCERR_MASK)
    FTFE_FSTAT = FTFE_FSTAT_ACCERR_MASK;
  if (FTFE_FSTAT & FTFE_FSTAT_FPVIOL_MASK)
    FTFE_FSTAT = FTFE_FSTAT_FPVIOL_MASK;

  //write to TFFCOB struct with command
  TFCCOB phraseWrite;
  phraseWrite.cmd = FTFE_FCCOB0_CCOBn(CMD_PROGRAM_PHRASE);
  ContructPhraseWriteCommand(&phraseWrite, address, phrase);
  return LaunchCommand(&phraseWrite);

}

/*! @brief Private function which erases 1 sector of flash memory
 *
 *  @return bool returns true if everything has been successfully executed.
 */
static bool EraseSector(const uint32_t address)
{
  //calls LaunchCommand
  TFCCOB command;
  command.addressReg.address = address;
  command.cmd = FTFE_FCCOB0_CCOBn(CMD_CLEAR_SECTOR);
  return LaunchCommand(&command);
}

/*! @brief Function which interfaces with the K70 chip to execute flash commands.
 *
 *  @return bool returns true if everything has been successfully executed.
 */
static bool LaunchCommand(TFCCOB* commonCommandObject)
{
  //write command and address
  FTFE_FCCOB3 = commonCommandObject->addressReg.ADRStruct.reg0;	//handles translating to big endian.
  FTFE_FCCOB2 = commonCommandObject->addressReg.ADRStruct.reg1;
  FTFE_FCCOB1 = commonCommandObject->addressReg.ADRStruct.reg2;
  FTFE_FCCOB0 = commonCommandObject->cmd;

  //write data;
  FTFE_FCCOB4 = commonCommandObject->data.s8.Lo4;	//e
  FTFE_FCCOB5 = commonCommandObject->data.s8.Lo3;	//f
  FTFE_FCCOB6 = commonCommandObject->data.s8.Lo2;	//g
  FTFE_FCCOB7 = commonCommandObject->data.s8.Lo1;	//h

  FTFE_FCCOB8 = commonCommandObject->data.s8.Hi4;	//a
  FTFE_FCCOB9 = commonCommandObject->data.s8.Hi3;	//b
  FTFE_FCCOBA = commonCommandObject->data.s8.Hi2;	//c
  FTFE_FCCOBB = commonCommandObject->data.s8.Hi1;	//d

  FTFE_FSTAT = FTFE_FSTAT_CCIF_MASK; // Launch command sequence, by clearing CCIF
  while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK))
  {
  }	// wait until the command is complete.

  return !(FTFE_FSTAT & FTFE_FSTAT_ACCERR_MASK
      || FTFE_FSTAT & FTFE_FSTAT_FPVIOL_MASK); //return the result of the command, it will be false when either of the bits are set to true.
}

/*! @brief Function which allocates data to its corresponding FTFE_FCCOB register
 *
 *  @return void
 */
static void ContructPhraseWriteCommand(TFCCOB* commonCommandObject,
                                       const uint32_t address,
                                       const uint64union_t phrase)
{
  commonCommandObject->addressReg.address = address;
  commonCommandObject->data = phrase;
}
/*!
 * @}
 */
