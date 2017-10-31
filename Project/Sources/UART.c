/*!
** @file UART.c
** @version 1.0
 *  @brief I/O routines for UART communications on the TWR-K70F120M.
 *
 *  This contains the functions for operating the UART (serial port).
 *  @author 98112939, 99141145
 *  @date 8-08-2017
*/

/*!
*  @addtogroup UART_module UART module documentation
*  @{
*/

#include "UART.h"
#include "FIFO.h"
#include "math.h"
#include "LEDs.h"
#include "MK70F12.h"
#include "Cpu.h"
#include "../Library/OS.h"
//#define PORTE_MUX_MASK 0x180

static TFIFO RxFIFO;
static TFIFO TxFIFO;

OS_ECB *RxSemaphore;
OS_ECB *TxSemaphore;

static uint8_t TempVar;
/*! @brief Gets data from FIFO and transmits. Disables the interrupt if FIFO is empty.
 *
 */
static void SendData();


bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{

  RxSemaphore = OS_SemaphoreCreate(0);
  TxSemaphore = OS_SemaphoreCreate(1);
  uint16union_t sbr;
  float sbrFloat, brfd_decimal;
  uint8_t brfd, sbr_uint;

  //FIFO Init
  FIFO_Init(&RxFIFO);
  FIFO_Init(&TxFIFO);
  //enable UART2 and PORTE (PORT E shares pins with UART)
  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;
  //SET pins for UART2 configuration.
  PORTE_PCR16 |= PORT_PCR_MUX(0x3); //0x3 is Alternate 3 configuration
  PORTE_PCR17 |= PORT_PCR_MUX(0x3);
  //turn off Transmit and receive
  UART2_C2 &= ~UART_C2_TE_MASK;
  UART2_C2 &= ~UART_C2_RE_MASK;
  //set baud rate
  //UART baud rate = UART module clock / (16 × (SBR[12:0] + BRFD))

  //calculate the SBR and BRFA depending on function arguments.
  sbrFloat = (float)moduleClk / (float)(16 * baudRate); //do a float division to retain decimal
  sbr_uint = (uint8_t)floor(sbrFloat); //take the integer value of division
  sbr.l = sbr_uint;//write to sbr variable
  brfd_decimal = sbrFloat - sbr.l;//extract decimal value by subtracting integer to float number
  brfd = (uint8_t)ceil(brfd_decimal * 32);// brfd_decimal * 32 to get BRFA and round up to ensure accuracy.

  //write values to registers
  UART2_BDL = sbr.s.Lo;//write low value
  UART2_BDH = sbr.s.Hi;//write high value
  UART2_C4 |= brfd;//write decimal to BRFA

  UART2_C2 &= ~UART_C2_TIE_MASK; //disables the transmit interrupt
  UART2_C2 |= UART_C2_RIE_MASK; //enables the receive interrupt



  //enable Transmit and Receive
  UART2_C2 |= UART_C2_TE_MASK;
  UART2_C2 |= UART_C2_RE_MASK;
  //the value set is determined by IRQ % 32 (pg 99)
  //reset NVICI		49 = IRQ, 32 = Given by manual
  NVICICPR1 = (1 << (49 % 32)); //NVIC1 is used because IRQ%4 = 1; Indicates the NVIC's IPR register number used for this IRQ. (pg 99)
  //enable NVICI
  NVICISER1 = (1 << (49 % 32));

  return true;


}

/*! @brief Get a character from the receive FIFO if it is not empty.
 *
 *  @param dataPtr A pointer to memory to store the retrieved byte.
 *  @return bool - TRUE if the receive FIFO returned a character.
 *  @note Assumes that UART_Init has been called.
 */
bool UART_InChar(uint8_t * const dataPtr)
{
  return FIFO_Get(&RxFIFO, dataPtr);
}

/*! @brief Put a byte in the transmit FIFO if it is not full.
 *
 *  @param data The byte to be placed in the transmit FIFO.
 *  @return bool - TRUE if the data was placed in the transmit FIFO.
 *  @note Assumes that UART_Init has been called.
 */
bool UART_OutChar(const uint8_t data)
{
	return FIFO_Put(&TxFIFO, data);
//	bool success = FIFO_Put(&TxFIFO, data);
//	OS_SemaphoreSignal(TxSemaphore);
//	return success;
//	bool success  =
//	if (success && !(UART2_C2 & UART_C2_TIE_MASK)) //if FIFO has data and the UART interrupt has not been enabled, signal semaphore
//	{
//		OS_SemaphoreSignal(TxSemaphore);
//		UART2_C2 |= UART_C2_TIE_MASK; //kickstart the interrupt by calling senddata and enabling the interrupt
//	}
//	return success;
}

bool UART_OutString(const uint8_t data[])
{
  OS_DisableInterrupts();
  uint8_t currentChar = data[0];
  int i = 0;
  do
  {
    FIFO_Put(&TxFIFO, currentChar);
    currentChar = data[++i];
  } while(currentChar != '\0');
  OS_EnableInterrupts();
  return true;
}

/*! @brief Poll the UART status register to try and receive and/or transmit one character.
 *
 *  @return void
 *  @note Assumes that UART_Init has been called.

void UART_Poll(void)
{
  if (UART2_S1 & UART_S1_TDRE_MASK)
  {
    //transmitted byte
    FIFO_Get(&TxFIFO, (uint8_t *) &UART2_D);
  }
  if (UART2_S1 & UART_S1_RDRF_MASK)
  {
    //Received
    FIFO_Put(&RxFIFO, UART2_D);
  }
}
 */

void TransmitThread(void *arg)
{
	for(;;)
	{
		OS_SemaphoreWait(TxSemaphore, 0);
		FIFO_Get(&TxFIFO, (uint8_t *) &UART2_D);
		UART2_C2 |= UART_C2_TIE_MASK; //enable the UART interrupt if the FIFO buffer is not empty, disable is done before every transmit in UART2_ISR
	}
}

void ReceiveThread(void *arg)
{
	for(;;)
	{
		OS_SemaphoreWait(RxSemaphore, 0);
		FIFO_Put(&RxFIFO, TempVar);
		//UART2_C2 |= UART_C2_RIE_MASK;
	}
}


/*! @brief Interrupt service routine for the UART.
 *
 *  @note Assumes the transmit and receive FIFOs have been initialized.
 */
void __attribute__ ((interrupt)) UART_ISR(void)
{
	//EnterCritical(); //Ensure no other interrupt gets triggered, saves status register
	OS_ISREnter();
	if(UART2_C2 & UART_C2_TIE_MASK)
	{
		if (UART2_S1 & UART_S1_TDRE_MASK)
		{
			//SendData();
			OS_SemaphoreSignal(TxSemaphore);
			UART2_C2 &= ~UART_C2_TIE_MASK;
		}
	}
	if(UART2_C2 & UART_C2_RIE_MASK)
	{
		if (UART2_S1 & UART_S1_RDRF_MASK)
		{
			//Received
			TempVar = UART2_D;
			//FIFO_Put(&RxFIFO, &tempvar);
			OS_SemaphoreSignal(RxSemaphore);
		}
	}
	OS_ISRExit();
	//ExitCritical();
}

/*! @brief Gets data from FIFO and transmits. Disables the interrupt if FIFO is empty.
 *
 *  @return void
 */
void SendData()
{
	bool success = FIFO_Get(&TxFIFO, (uint8_t *) &UART2_D); //calls FIFO_Get to get data from FIFO and transmit it
	if(!success)
	{
		UART2_C2 &= ~UART_C2_TIE_MASK; //disables the UART interrupt if the FIFO buffer is empty
	}
}

/*!
* @}
*/
