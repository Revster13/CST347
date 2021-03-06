////////////////////////////////////////////////////////////////////////////////
//File: myTasks.c
//Author:	Chad Revel
//This is the task file of the third lab for CST 347
////////////////////////////////////////////////////////////////////////////////

#include <asf.h>
#include <FreeRTOS.h>
#include "myTasks.h"


//gonna need 2 delay variables. 1 with 500ms, and 1 with 1000ms. Still need the while loop for the toggle.
//Block for 500ms being xDelay, 1000ms for xDelay2
//xDelay2 is for the taskHeartbeat, so LED0 will flash every second

const TickType_t xDelay2 = 1000 / portTICK_PERIOD_MS;
const TickType_t xDelay3 = 100 / portTICK_PERIOD_MS;

int SW_Debounce = 0;

extern const char* uartBuffer1D;
extern const char* uartBuffer2D;
extern const char* uartBuffer3D;

extern const char* uartBuffer1I;
extern const char* uartBuffer2I;
extern const char* uartBuffer3I; 

extern const char* uartBuffer1A;
extern const char* uartBuffer2A;
extern const char* uartBuffer3A;

//This is the main control for the task system
void taskSystemControl(void * pvParamaters)
{
	/*
	The MainControl task will poll the status of all three switches
	(just as we did in lab 2). The switches should be debounced with
	a three stage strategy: Read the switch, Delay 10ms using a vTaskDelay,
	then read switch again to verify the change.
	
	In addition you should implement a lockout feature in your driver
	only allowing a switch to perform a single action on a press.
	There will be three of these tasks (only 1 active at a time). 
	Each of these task will send messages via a queue to its corresponding LED task.
	The switches will perform the following function.
	*/
	struct controlStruct * controlParams = (struct controlStruct *) pvParamaters;
	QueueHandle_t ledQueueParam = controlParams->ledQ;
	QueueHandle_t uartQueueParam = controlParams->uartQ;
	TaskHandle_t nextTaskHandleParam = *controlParams->nextTask;
	uint8_t ledParam = controlParams->ledNum;
               
	//declaration for either increasing or decreasing the speed			                     
	enum timeDelay incDec;

	while (true)
	{
		//the first case will be for sw1
		//it will create the task for each of the leds.
		/*
		EXT_SW1
		This Switch will decrease the delay time of the corresponding LED task.
		It will accomplish this by sending a message to this task via the queue setup for this
		pair using the API function xQueueSendToBack() function.
		The message will tell the LED task to decrease its delay time.
		*/
		if (readButton(SW1) == 1)
		{
			if (SW_Debounce < maxSWDebounce) SW_Debounce++;
			else
			{
				SW_Debounce = 0;
				//send led1 back to the end of the queue
				incDec = DECREASE;
				xQueueSendToBack(ledQueueParam, (void *) &incDec, (TickType_t) 10);
			}
		}

		//if sw 2 is pressed, then check each led from the highest led to the lowest, then delete the task
		/*
		EXT_SW2
		This Switch will increase the delay time of the corresponding LED task.
		It will accomplish this by sending a message to this task via the queue setup for this pair.
		It will accomplish this by calling xQueueSendToBack() function.
		*/
		else if (readButton(SW2) == 1)
		{
			if (SW_Debounce < maxSWDebounce) SW_Debounce++;
			else
			{
				SW_Debounce = 0;
				incDec = INCREASE;
				//send led1 back to the end of the queue
				xQueueSendToBack(ledQueueParam, (void *) &incDec, (TickType_t) 10);
			}
		}

		//For the on board switch, if it's pressed, then suspend, or release the leds
		/*
		SW0
		This Switch will cause the next LED to be selected.
		It will accomplish this by resuming the next MainControl (vTaskResume) Task and then suspending (vTaskSuspend) itself.
		So if LED1 is the current MainControl Task then after the SUSPEND/RESUME LED2 will be the active MainControl Task.
		During this operation a message will also be sent to the UART task to identify the new active LED.
		This message will be in the form of a char * and contain a message stating �LED N IS NOW ACTIVE�.
		Make sure that it includes the appropriate Line feeds to move cursor to the next line.

		The polling of the switches will happen in sequence and actions will be taken accordingly.
		After all three switches have been polled, the task will perform a vTaskDelay() for 100ms.
		Error checking must be implemented to assure that the Message was sent to the appropriate queue with the vQueueSendtoBack().
		*/
		else if (readButton(SW0) == 0)
		{

			if (SW_Debounce < maxSWDebounce) SW_Debounce++;
			else
			{
				SW_Debounce = 0;
				if(ledParam == LED1)
				{
					xQueueSendToBack(uartQueueParam, uartBuffer2A, (TickType_t) 0);
					vTaskResume(nextTaskHandleParam);
					vTaskSuspend(NULL);
				}
				else if(ledParam == LED2)
				{					
					xQueueSendToBack(uartQueueParam, uartBuffer3A, (TickType_t) 0);
					vTaskResume(nextTaskHandleParam);
					vTaskSuspend(NULL);
				}
				else if(ledParam == LED3)
				{
					xQueueSendToBack(uartQueueParam, uartBuffer1A, (TickType_t) 0);
					vTaskResume(nextTaskHandleParam);
					vTaskSuspend(NULL);
				}
			}

		}

		//delay for 100ms after all 3 switches
		vTaskDelay(xDelay3);
	}

}

//		
void taskHeartBeat (void * pvParamaters)		
{
/*
The heartbeat task will be responsible for toggling the onboard LED every second.
This will give you a visual clue that the FreeRTOS system is still running.
*/	

	//this is the heartbeat for LED 0 to happen once every second
	while (true)
	{
		toggleLED(LED0);
		vTaskDelay(xDelay2);
	}	
}

void taskLED(void * pvParameters)
{
/*
The LED tasks will blink the corresponding LED on and off. 
It will do this at an initial rate of 500ms. 
It will also check its Queue to see if it has a message waiting (uxQueueMessagesWaiting). 
If there is a message waiting it should read the message (xQueueReceive) and perform the necessary action. 
The wait time to the xQueueReceive() should be set to 0. 
The action will be either to increase or decrease the delay time in increments of 50ms. 
This should be bounded to a MAX delay of 1000ms and a MIN delay of 200ms. 
There will be three of these tasks as well. 
The task should use your LED Driver from Lab 2.
*/

	struct ledStruct * controlParams = (struct ledStruct *) pvParameters;
	QueueHandle_t ledQ = controlParams->ledQ;
	QueueHandle_t uartQ = controlParams->uartQ;
	uint8_t ledNum = controlParams->ledNum;
	int defaultMS = 500;
	TickType_t xDelay = defaultMS / portTICK_PERIOD_MS;

	timeDelay getDelay;

	while(true)
	{
		toggleLED(ledNum);
		
		//this will go through the queue for each of the leds and either increase or decrease the delay

		if(uxQueueMessagesWaiting(ledQ))
		{
			//if the queue receive of queue handle[i] goes into &getDelay and it's == to pbTrue, increase or decrease
			if (xQueueReceive(ledQ, &getDelay, 0))
			{
				//if the task is to decrease the led, then decrease it for the current led
				if(getDelay == DECREASE)
				{
					if (ledNum == LED1)
					{
						xQueueSendToBack(uartQ, uartBuffer1D, (TickType_t) 0);
					}
					else if (ledNum == LED2)
					{
						xQueueSendToBack(uartQ, uartBuffer2D, (TickType_t) 0);
					}
					else if (ledNum == LED3)
					{
						xQueueSendToBack(uartQ, uartBuffer3D, (TickType_t) 0);
					}
					xDelay = (defaultMS - 50) / portTICK_PERIOD_MS;
					if(xDelay < 200)
					{
						xDelay = 200;
					}
				}
				//if the task is to increase the led, then increase it for the current led
				else if (getDelay == INCREASE)
				{
					if (ledNum == LED1)
					{
						xQueueSendToBack(uartQ, uartBuffer1I, (TickType_t) 0);
					}
					else if (ledNum == LED2)
					{
						xQueueSendToBack(uartQ, uartBuffer2I, (TickType_t) 0);
					}
					else if (ledNum == LED3)
					{
						xQueueSendToBack(uartQ, uartBuffer3I, (TickType_t) 0);
					}
					xDelay = (defaultMS + 50) / portTICK_PERIOD_MS;
					if(xDelay == 1000)
					{
						xDelay = 1000;
					}
				}
			}
		}
		vTaskDelay(xDelay);
	}
}

void taskUART(void *pvParameters)
{
/*
The UART task will block until a message is ready for it. To accomplish this blocking
set the last parameter of the xQueueReceive() to portMAX_DELAY. It will then get the message
and write it out on the UART using your UARTPutStr(). You will need to write a very rudimentary
UART driver for your program. The filenames for the driver will be uartdrv.c and uartdrv.h.
This should include three functions. The functions should be initUART(), UARTPutC() and UARTPutStr().
The initUART() function will initialize the UART. The UARTPutC() will print a single byte (char) to the UART.
The UARTPutStr() function will use the UARTPutC() to write a complete string to the UART.
*/	

QueueHandle_t uartTempQueue = (QueueHandle_t) pvParameters;

char tempUART[50];

	while(true)
	{
		if(uxQueueMessagesWaiting(uartTempQueue))
		{
			xQueueReceive(uartTempQueue, &tempUART, portMAX_DELAY);
			UARTPutStr(EDBG_UART, tempUART, sizeof(tempUART));
		}
	}
}
