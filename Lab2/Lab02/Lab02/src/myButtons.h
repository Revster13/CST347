////////////////////////////////////////////////////////////////////////////////
//File: myButton.h
//Author:	Chad Revel
//This is the drivers header file of the second lab for CST 347
////////////////////////////////////////////////////////////////////////////////
#ifndef MYBUTTONS_H_
#define MYBUTTONS_H_

#include <asf.h>
#include <FreeRTOS.h>

//initializeButtonDriver and readButton
void initializeButtonDriver(void);
uint8_t readButton(uint8_t uiButtonNum);

#endif /* MYBUTTONS_H_ */