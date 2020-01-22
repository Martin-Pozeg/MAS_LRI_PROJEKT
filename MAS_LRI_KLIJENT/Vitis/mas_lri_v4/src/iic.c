/*
 * INCLUDES
 */
#include <sleep.h>

#include "xparameters.h"
#include "xiicps.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"

#include "cameraInit.h"
#include "iic.h"

/*
 * CONSTANT DEFINITIONS
 */
#define IIC_DEVICE_ID		XPAR_XIICPS_0_DEVICE_ID																		//declaring the device ID

#define IIC_CAMERA_ADDR		0x21																						//declaring the IIC camera adress
#define IIC_SCLK_RATE		100000																						//declaring the IIC speed

/*
 * FUNCTION PROTOTYPE DECLARATIONS
 */
int iic_OV7670_init(void);
int write_reg(u8 register_address, u8 value);
int read_reg(u8 register_address);
void init_regs(struct regval_list *register_list);
void init_default_regs(struct regval_list* OV7670_default_regs);
void init_RGB565(struct regval_list* OV7670_RGB565);
void init_YUV(struct regval_list* OV7670_YUV422);
void init_test_bar(struct regval_list* OV7670_test_bar);
void init_image(struct regval_list* OV7670_image);

/*
 * VARIABLE DEFINITIONS
 */
XIicPs Iic;																												//declaring the instance of the IIC Device

/*
 * iic_OV7670_init
 *
 * This function sets up and initializes the IIC connection. After a successful initialization, it performs a soft reset on the OV7670.
 */
int iic_OV7670_init(void)
{
	int Status;																											//declaring the status variable
	int result;																											//declaring the result variable
	XIicPs_Config *Config;																								//declaring the pointer to the XIicPs_Config

	Config = XIicPs_LookupConfig(IIC_DEVICE_ID);																		//creating the CONFIG
	if(Config == NULL)																									//if the CONFIG isn't created, return error;
	{
		xil_printf("Error: XIicPs_LookupConfig()\n\r");																	//outputting the error message
		return XST_FAILURE;																								//returning the XST_FAILURE value
	}

	Status = XIicPs_CfgInitialize(&Iic, Config, Config->BaseAddress);													//initializing the CONFIG
	if(Status != XST_SUCCESS)																							//if the Status isn't XST_SUCCESS, return error;
	{
		xil_printf("Error: XIicPs_CfgInitialize()\n\r");																//outputting the error message
		return XST_FAILURE;																								//returning the XST_FAILURE value
	}

	Status = XIicPs_SelfTest(&Iic);																						//performing the self-test
	if(Status != XST_SUCCESS)																							//if the Status isn't XST_SUCCESS, return error;
	{
		xil_printf("Error: XIicPs_SelfTest()\n\r");																		//outputting the error message
		return XST_FAILURE;																								//returning the XST_FAILURE value
	}

	XIicPs_SetSClk(&Iic, IIC_SCLK_RATE);																				//set the IIC_SCLK_RATE
	xil_printf("I2C configuration done.\n\r");																			//outputting the message (initialization complete)

	xil_printf("Soft Reset OV7670.\n\r");																				//outputting the message (soft reset)
	result = write_reg(REG_COM7, COM7_RESET);																			//performing a soft reset of the camera
	if(result != XST_SUCCESS)																							//if the Status isn't XST_SUCCESS, return error;
	{
		xil_printf("Error: OV767 RESET\r\n");																			//outputting the error message
		return XST_FAILURE;																								//returning the XST_FAILURE value
	}
	sleep(1);																											//sleep for a second

	return XST_SUCCESS;																									//returning the XST_SUCCESS value
}

/*
 * write_reg
 *
 * This function writes a new value into OV7670 register.
 *
 * register_address - (u8) address of the register in OV7670
 * value - (u8) value to be written into the register
 */
int write_reg(u8 register_address, u8 value)
{
	int Status;																											//declaring the Status variable
	u8 buff[2];																											//declaring the buffer

	buff[0] = register_address;																							//first value in the buffer is the register address
	buff[1] = value;																									//second value in the buffer is the new value to be written in the register

	Status = XIicPs_MasterSendPolled(&Iic, buff, 2, IIC_CAMERA_ADDR);													//sending the buffer to the OV7670 (polled)

	if(Status != XST_SUCCESS)																							//if the Status isn't XST_SUCCESS, return error;
	{
		xil_printf("WriteReg:I2C Write Fail\n");																		//outputting the error message
		return XST_FAILURE;																								//returning the XST_FAILURE value
	}

	while(XIicPs_BusIsBusy(&Iic))																						//waiting until the bus is idle to start another transfer
	{
		/* NOP */
	}

	usleep(30*1000);																									//sleeping for 0.3s

	return XST_SUCCESS;																									//returning the XST_SUCCESS value
}

/*
 * read_reg
 *
 * This function reads the value from the selected OV7670 register
 *
 * register_address - (u8) address of the register in OV7670
 */
int read_reg(u8 register_address)
{
	int Status;																											//declaring the Status variable
	u8 buff[1];																											//declaring the buffer

	buff[0] = register_address;																							//value in the buffer is the register address

	Status = XIicPs_MasterSendPolled(&Iic, buff, 1, IIC_CAMERA_ADDR);															//writing the register address into the OV7670

	if(Status != XST_SUCCESS)																							//if the Status isn't XST_SUCCESS, return error;
	{
		xil_printf("WriteReg:I2C Write Fail\n");																		//outputting the error message
		return XST_FAILURE;																								//returning the XST_FAILURE value
	}

	while(XIicPs_BusIsBusy(&Iic))																						//waiting until the bus is idle to start another transfer
	{
		/* NOP */
	}

	Status = XIicPs_MasterRecvPolled(&Iic, buff, 1, IIC_CAMERA_ADDR);															//reading the value from the selected OV7670 register

	if(Status != XST_SUCCESS)																							//if the Status isn't XST_SUCCESS, return error;
	{
		xil_printf("WriteReg:I2C Read Fail\n");																			//outputting the error message
		return XST_FAILURE;																								//returning the XST_FAILURE value
	}

	while(XIicPs_BusIsBusy(&Iic))																						//waiting until the bus is idle to start another transfer
	{
		/* NOP */
	}

	return buff[0];																										//return the read value
}

/*
 * init_regs
 *
 * This functions sets the OV7670 registers in reg_list to desired values.
 *
 * reg_list - (regval_list*) a pointer to the selected regval_list in cameraInit.h
 */
void init_regs(struct regval_list* register_list)
{
	int i = 0;																											//declaring the iterator

	while(register_list[i].reg_num != 0xff)																		//performing this loop as long as we don't reach 0xff address (end marker)
	{
		write_reg(register_list[i].reg_num, register_list[i].reg_val);													//writing the combination into the OV7670
		xil_printf("Value: %X to register %X\r\n", register_list[i].reg_val, register_list[i].reg_num);					//outputting the message about the set register
		i++;																											//incrementing the iterator
	}
}

/*
 * init_default_regs
 *
 * This function sets all register in OV7670 to their default values.
 *
 * OV7670_default_regs - (regval_list*) a pointer to the selected regval_list in cameraInit.h
 */
void init_default_regs(struct regval_list* OV7670_default_regs)
{
	init_regs(OV7670_default_regs);																						//writing all the desired registers
}

/*
 * init_RGB565
 *
 * This function sets all register in OV7670 for the RGB565 format.
 *
 * OV7670_RGB565 - (regval_list*) a pointer to the selected regval_list in cameraInit.h
 */
void init_RGB565(struct regval_list* OV7670_RGB565)
{
	init_regs(OV7670_RGB565);																							//writing all the desired registers
}

/*
 * init_YUV422
 *
 * This function sets all register in OV7670 for the YUV422 format.
 *
 * OV7670_YUV422 - (regval_list*) a pointer to the selected regval_list in cameraInit.h
 */
void init_YUV(struct regval_list* OV7670_YUV422)
{
	init_regs(OV7670_YUV422);																							//writing all the desired registers
}

/*
 * init_test_bar
 *
 * This function sets all register in OV7670 for the test bar.
 *
 * OV7670_test_bar - (regval_list*) a pointer to the selected regval_list in cameraInit.h
 */
void init_test_bar(struct regval_list* OV7670_test_bar)
{
	init_regs(OV7670_test_bar);																							//writing all the desired registers
}

/*
 * init_image
 *
 * This function sets all register in OV7670 for the image.
 *
 * OV7670_image - (regval_list*) a pointer to the selected regval_list in cameraInit.h
 */
void init_image(struct regval_list* OV7670_image)
{
	init_regs(OV7670_image);																								//writing all the desired registers
}
