/*!
** @file FIFO.c
** @version 1.0
 *  @brief Routines to implement a FIFO buffer.
 *
 *  This contains the structure and "methods" for accessing a byte-wide FIFO.
 *  @author 98112939, 99141145
 *  @date 8-08-2017
*/

/*!
*  @addtogroup FIFO_module FIFO module documentation
*  @{
*/

#include "FIFO.h"
#include "Cpu.h"

#include "OS.h"

/*! @brief Initialize the FIFO before first use.
 *
 *  @param FIFO A pointer to the FIFO that needs initializing.
 *  @return void
 */
void FIFO_Init(TFIFO * const FIFO)
{
  FIFO->Start=0;
  FIFO->End=0;
  FIFO->NbBytes=0;
  FIFO->BufferAccess = OS_SemaphoreCreate(1);
  FIFO->SpaceAvailable = OS_SemaphoreCreate(FIFO_SIZE);
  FIFO->ItemsAvailable = OS_SemaphoreCreate(0);
}

/*! @brief Put one character into the FIFO.
 *
 *  @param FIFO A pointer to a FIFO struct where data is to be stored.
 *  @param data A byte of data to store in the FIFO buffer.
 *  @return bool - TRUE if data is successfully stored in the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
bool FIFO_Put(TFIFO * const FIFO, const uint8_t data)
{
	OS_SemaphoreWait(FIFO->SpaceAvailable, 0);
	OS_SemaphoreWait(FIFO->BufferAccess, 0);

//	EnterCritical();
	OS_DisableInterrupts();
	FIFO->Buffer[FIFO->End] = data;//write to the end of FIFO
	FIFO->End++;//increment the end
	if (FIFO->End == FIFO_SIZE - 1)
			FIFO->End = 0;
	FIFO->NbBytes++;//increment the size of the buffer.
	OS_EnableInterrupts();
	//	ExitCritical();

	OS_SemaphoreSignal(FIFO->BufferAccess);
	OS_SemaphoreSignal(FIFO->ItemsAvailable);

	return true;
}

/*! @brief Get one character from the FIFO.
 *
 *  @param FIFO A pointer to a FIFO struct with data to be retrieved.
 *  @param dataPtr A pointer to a memory location to place the retrieved byte.
 *  @return bool - TRUE if data is successfully retrieved from the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
bool FIFO_Get(TFIFO * const FIFO, uint8_t * const dataPtr)
{
	OS_SemaphoreWait(FIFO->ItemsAvailable, 0);
	OS_SemaphoreWait(FIFO->BufferAccess, 0);

//	EnterCritical();
	OS_DisableInterrupts();
	*dataPtr = FIFO->Buffer[FIFO->Start];//get value of the first index
	FIFO->Start++;//increment the start pointer of the buffer
	if (FIFO->Start == FIFO_SIZE - 1)
		FIFO->Start = 0;//go back to zero index after end of buffer
	FIFO->NbBytes--; //decrement size of buffer
//	ExitCritical();
	OS_EnableInterrupts();

	OS_SemaphoreSignal(FIFO->BufferAccess);
	OS_SemaphoreSignal(FIFO->SpaceAvailable);

	return true;
}
/*!
* @}
*/
