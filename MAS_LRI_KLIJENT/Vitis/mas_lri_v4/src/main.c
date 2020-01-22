/*****************************************************
mas_lri

Terminal Settings:
   -Baud: 115200
   -Data bits: 8
   -Parity: no
   -Stop bits: 1

27/12/19: Created by Kamerica.inc (Luka Mrkovic)
****************************************************/

/*
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sleep.h>
#include <xgpio.h>

#include "platform.h"
#include "xparameters.h"

#include "cameraInit.h"
#include "iic.h"


/*
 * FUNCTION PROTOTYPES
 */
void clear_screen();
void camera_initalize();
void take_picture(int mode, XGpio* camera);

/*
 * MACROS
 * for fast reading from the camera Video port
 */
#define READ_CAM XGpio_DiscreteRead(camera, 1)

#define CAMERA_isVSYNdown(data) ((data & 1024) == 0)
#define CAMERA_isVSYNup(data) ((data & 1024) != 0)

#define CAMERA_isHREFdown(data) ((data & 512) == 0)
#define CAMERA_isHREFup(data) ((data & 512) != 0)

#define CAMERA_isPCLKdown(data) ((data & 256) == 0)
#define CAMERA_isPCLKup(data) ((data & 256) != 0)

/*
 * clear_screen
 *
 * This function clears the terminal.
 */
void clear_screen()
{
	xil_printf("\e[1;1H\e[2J");																							//clearing the terminal
}

/*
 * camera_initialize
 *
 * This function sets the registers in the camera to desired values.
 */
void camera_initalize()
{
	clear_screen();																										//clearing the terminal
	iic_OV7670_init();																									//initializing the IIC connection to the camera
	init_default_regs(ov7670_default_regs);																				//Initializing the default values for the camera registers
	init_YUV(ov7670_fmt_yuv422);																						//Initializing the registers for the YUV422 format
	sleep(1);																											//delay
}

/*
 * take_picture
 *
 * This function takes a picuture on the camera. Saves the picture to Y array.
 *
 * mode - (int) mode for the picture taking
 * camera (XGpio*) pointer to the XGpio instance connected to the camera
 */
void take_picture(int mode, XGpio* camera)
{
	int i, z, y, r;																										//declaring iterators
	u32 tt, tt1;																										//declaring read values
	u8 Y[614400];																										//declaring the image array
	u8 old;

	z = 0;																												//initializing the z value to 0

	while(CAMERA_isVSYNdown(READ_CAM));																					//waiting for VSYNC value to be 0
	while(CAMERA_isVSYNup(READ_CAM));																					//waiting for VSYNC value to be 1

	for(y = 0; y < 480; y++)																							//iterating by the pixel lines
	{
		do
		{
			tt = READ_CAM;																								//reading the camera

			if(CAMERA_isVSYNup(tt))																						//if VSYNC gets asserted before time, an error is reported
			{
				xil_printf("VSYNC error [%d]\n\r",y);																	//outputting the error
				sleep(2);																								//sleeping for the error to be noticed
				goto l2;																								//exiting the loop
			}
		}
		while(CAMERA_isHREFdown(tt));																					//waiting for HREF value to be 1 (indicates the valid pixel values)

		for(r = 0; r < 1280; r++)																						//iterating by the individual pixel in the line
		{
			do
			{
				tt = READ_CAM;																							//reading the camera
			}
			while(CAMERA_isPCLKdown(tt));																				//waiting for the PCLK to be 1

			do
			{
				tt1 = READ_CAM;																							//reading the camera

				if(CAMERA_isHREFdown(tt1) && r!=1279)																	//if HREF gets pulled down before time, an error is reported
				{
					xil_printf("HREF error [%d, %d]\n\r",r, y);															//outputting the error
					sleep(2);																							//sleeping for the error to be noticed
					goto l2;																							//exiting the loop
				}
			}
			while(CAMERA_isPCLKup(tt1));																				//waiting for the PCLK to be 0

			Y[z] = (u8)(tt);																							//saving the value to the array
			z++;																										//incrementing the z value
		}

		while(CAMERA_isHREFup(READ_CAM));																				//waiting for the HREF to be 0
	}

	xil_printf("OK !!!\n\r");																							//outputting the OK message

	old = 0;

//	for(i = 0; i < 640; i++)
//	{
//		if(Y[2 * i] != old)
//		{
//			old = Y[2 * i];
//			xil_printf("%4d", Y[2 * i]);
//		}
//	}
//
//	sleep(15);

	l2:																													//label for exiting the loop

	sleep(1);																											//delay
}

/*
 * main
 *
 * The main function.
 */
int main()
{
   XGpio input, output, camera;																							//declaring the XGpio variables
   int button_data = 0;																									//declaring the button data value
   int button_data_1 = 0;																								//declaring button data values for DEBOUNCING
   int button_data_2 = 0;
   int led_data = 0;																									//declaring the led data value
   int i;																												//declaring the iterator

   XGpio_Initialize(&input, XPAR_AXI_GPIO_0_DEVICE_ID);																	//initialize input XGpio variable
   XGpio_Initialize(&output, XPAR_AXI_GPIO_0_DEVICE_ID);																//initialize output XGpio variable

   XGpio_SetDataDirection(&input, 1, 0xF);																				//set first channel TRISTATE buffer to input
   XGpio_SetDataDirection(&output, 2, 0x0);																				//set second channel TRISTATE buffer to output

   camera_initalize();																									//initializing the camera IIC comunications and registers

   XGpio_Initialize(&camera, XPAR_AXI_GPIO_1_DEVICE_ID);																//initialize camera XGpio variable
   XGpio_SetDataDirection(&camera, 1, 0x7ff);																			//set first channel TRISTATE buffer to input/output (iii_iiii_iiii)


   init_platform();																										//initializing the FPGA platform

   led_data = 0b1111;																									//initially, all four buttons are available (all four green)
   XGpio_DiscreteWrite(&output, 2, led_data);

   clear_screen();																										//clearing the terminal
   xil_printf("Choose a MODE and take a photo!\n\rMODE 1 - a regular photo\n\rMODE 2\n\rMODE 3\n\rMODE 4\n\r");			//outputting the main menu

   while(1){																											//main loop

	  /*
	   * DEBOUNCING
	   */
	  do
	  {
		  button_data_1 = XGpio_DiscreteRead(&input, 1);																//get button data
		  usleep(20000);																								//wait for 20ms
		  button_data_2 = XGpio_DiscreteRead(&input, 1);																//get button data again
	  } while (button_data_1 != button_data_2);																			//running this as long as button_data_1 and button_data_2 aren't the same

	  button_data = button_data_2;																						//button data is the DEBOUNCED value

      /*
       * TAKING A PICTURE (depending on what button is pressed)
       */
      if(button_data == 0b0000)																							//if none of the buttons are pressed
      {
    	  /* NOP */
      }

      else if(button_data == 0b1000)																					//if the first button is pressed
      {
    	  clear_screen();																								//clearing the terminal

    	  xil_printf("Taking a photo... [MODE 1]\n\r");																	//outputting the message about the selected mode [MODE 1]

      	  led_data = 0b1000;																							//changing the led output (signaling the selected mode)
      	  XGpio_DiscreteWrite(&output, 2, led_data);																	//outputting the new led output

      	  take_picture(1, &camera);																						//taking a picture [MODE 1]

      	  clear_screen();																								//clearing the terminal

      	  xil_printf("Photo taken! [MODE 1]\n\r");																		//outputting the message that the photo is taken

      	  sleep(1);																										//sleeping for the message to be noticed

      	  led_data = 0b1111;																							//changing the led output (all 4 modes available)
      	  XGpio_DiscreteWrite(&output, 2, led_data);																	//outputting the new led output

      	  clear_screen();																								//clearing the terminal
      	  xil_printf("Choose a MODE and take a photo!\n\rMODE 1 - a regular photo\n\rMODE 2\n\rMODE 3\n\rMODE 4\n\r");	//outputting the main menu
      }

      else if(button_data == 0b0100)																					//if the second button is pressed
      {
    	  clear_screen();																								//clearing the terminal

    	  xil_printf("Taking a photo... [MODE 2]\n\r");																	//outputting the message about the selected mode [MODE 2]

    	  led_data = 0b0100;																							//changing the led output (signaling the selected mode)
    	  XGpio_DiscreteWrite(&output, 2, led_data);																	//outputting the new led output

    	  take_picture(2, &camera);																						//taking a picture [MODE 2]

    	  clear_screen();																								//clearing the terminal

    	  xil_printf("Photo taken! [MODE 2]\n\r");																		//outputting the message that the photo is taken

    	  sleep(1);																										//sleeping for the message to be noticed

    	  led_data = 0b1111;																							//changing the led output (all 4 modes available)
    	  XGpio_DiscreteWrite(&output, 2, led_data);																	//outputting the new led output

    	  clear_screen();																								//clearing the terminal
    	  xil_printf("Choose a MODE and take a photo!\n\rMODE 1 - a regular photo\n\rMODE 2\n\rMODE 3\n\rMODE 4\n\r");	//outputting the main menu
      }

      else if(button_data == 0b0010)																					//if the third button is pressed
      {
    	  clear_screen();																								//clearing the terminal

    	  xil_printf("Taking a photo... [MODE 3]\n\r");																	//outputting the message about the selected mode [MODE 3]

    	  led_data = 0b0010;																							//changing the led output (signaling the selected mode)
    	  XGpio_DiscreteWrite(&output, 2, led_data);																	//outputting the new led output

    	  take_picture(3, &camera);																						//taking a picture [MODE 3]

    	  clear_screen();																								//clearing the terminal

    	  xil_printf("Photo taken! [MODE 3]\n\r");																		//outputting the message that the photo is taken

    	  sleep(1);																										//sleeping for the message to be noticed

    	  led_data = 0b1111;																							//changing the led output (all 4 modes available)
    	  XGpio_DiscreteWrite(&output, 2, led_data);																	//outputting the new led output

    	  clear_screen();																								//clearing the terminal
    	  xil_printf("Choose a MODE and take a photo!\n\rMODE 1 - a regular photo\n\rMODE 2\n\rMODE 3\n\rMODE 4\n\r");	//outputting the main menu
      }

      else if(button_data == 0b0001)																					//if the fourth button is pressed
      {
    	  clear_screen();																								//clearing the terminal

    	  xil_printf("Taking a photo... [MODE 4]\n\r");																	//outputting the message about the selected mode [MODE 4]

    	  led_data = 0b0001;																							//changing the led output (signaling the selected mode)
    	  XGpio_DiscreteWrite(&output, 2, led_data);																	//outputting the new led output

    	  take_picture(4, &camera);																						//taking a picture [MODE 4]

    	  clear_screen();																								//clearing the terminal

    	  xil_printf("Photo taken! [MODE 4]\n\r");																		//outputting the message that the photo is taken

    	  sleep(1);																										//sleeping for the message to be noticed

    	  led_data = 0b1111;																							//changing the led output (all 4 modes available)
    	  XGpio_DiscreteWrite(&output, 2, led_data);																	//outputting the new led output

    	  clear_screen();																								//clearing the terminal
    	  xil_printf("Choose a MODE and take a photo!\n\rMODE 1 - a regular photo\n\rMODE 2\n\rMODE 3\n\rMODE 4\n\r");	//outputting the main menu
      }

      else{

    	  clear_screen();																								//clearing the terminal

    	  xil_printf("Multiple buttons pressed!\n\r");																	//outputting the error message about multiple modes selected

    	  /*
    	   * FLASHING THE LEDS
    	   */
    	  led_data = 0b1111;																							//changing the led output (all modes)

    	  for(i = 0; i < 10; i++){																						//repeating the flashing for 5 times

    		  led_data = (i % 2) * 0b1111;																				//calculating the new led output
    		  XGpio_DiscreteWrite(&output, 2, led_data);																//outputting the new led output

    		  usleep(100000);																							//sleeping 0.1s between state changes
    	  }

    	  clear_screen();																								//clearing the terminal
    	  xil_printf("Choose a MODE and take a photo!\n\rMODE 1 - a regular photo\n\rMODE 2\n\rMODE 3\n\rMODE 4\n\r");	//outputting the main menu
      }
   }

   cleanup_platform();																									//cleaning up the FPGA platform

   return 0;
}
