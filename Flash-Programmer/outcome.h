#pragma once
#include <map>
#include <string>
#include <ftd2xx.h>
#include <LibFT4222.h>

#define SUCCESS 0
#define DEBUG -1
#define INCORRECT_NUMBER_BYTES 19
#define TIME_OUT_ERROR 20
#define CORRUPTED_UPLOAD 21

using std::string;
using std::map;

struct outcome {
	int code;
	string msg;
};


map<int,std::string> outcomeMessages =
{
	{FT_INVALID_HANDLE, "Invalid handle",},
	{FT_DEVICE_NOT_FOUND, "Did not find FTDI device",},
	{FT_DEVICE_NOT_OPENED, "FTDI device not opened on host",},
	{FT_IO_ERROR, "FTDI device IO error",},
	{FT_INSUFFICIENT_RESOURCES, "FTDI device has insufficient resources",},
	{FT_INVALID_PARAMETER, "FTDI device was initialized with invalid paramters",},
	{FT_INVALID_BAUD_RATE, "Invalid baud rate for FTDI device",},
	{FT_DEVICE_NOT_OPENED_FOR_ERASE, "FTDI device is not opened for erase",},
	{FT_DEVICE_NOT_OPENED_FOR_WRITE, "FTDI device is not opened for write",},
	{FT_FAILED_TO_WRITE_DEVICE, "Failed to write to FTDI device",},
	{FT_EEPROM_READ_FAILED, "Failed reading from FTDI device EEPROM",},
	{FT_EEPROM_WRITE_FAILED, "Failed to write to FTDI device EEPROM",},
	{FT_EEPROM_ERASE_FAILED, "Failed to erase FTDI device EEPROM",},
	{FT_EEPROM_NOT_PRESENT, "FTDI device EEPROM not present",},
	{FT_EEPROM_NOT_PROGRAMMED, "FTDI device EEPROM not programmed",},
	{FT_INVALID_ARGS, "Invalid arguments",},
	{FT_NOT_SUPPORTED, "FTDI device not supported",},
	{FT_OTHER_ERROR, " ",},
	{FT4222_DEVICE_NOT_SUPPORTED, "FT4222 device not supported",},
	{FT4222_CLK_NOT_SUPPORTED, "Specified clock rate is not supported, SPI master does not support 80Mhz/2",},
	{FT4222_VENDER_CMD_NOT_SUPPORTED, "FT4222 device vendor not supported",},
	{FT4222_IS_NOT_SPI_MODE, "FT4222 device is not in SPI mode",},
	{FT4222_IS_NOT_I2C_MODE, "FT4222 device is not in I2C mode",},
	{FT4222_IS_NOT_SPI_SINGLE_MODE, "Trying to perform SPI 1-bit operation when FT4222 is not in 1-bit mode",},
	{FT4222_IS_NOT_SPI_MULTI_MODE, "Trying to perform SPI multi-bit operation when FT4222 is not in multi-bit mode",},
	{FT4222_WRONG_I2C_ADDR, "Incorrect I2C address",},
	{FT4222_INVAILD_FUNCTION, "Function not supported for this FT4222 device",},
	{FT4222_INVALID_POINTER, "Invalid pointer to FT4222 device",},
	{FT4222_EXCEEDED_MAX_TRANSFER_SIZE, "Exceeded max transfer size",},
	{FT4222_FAILED_TO_READ_DEVICE, "Failed to read from FT4222 device",},
	{FT4222_I2C_NOT_SUPPORTED_IN_THIS_MODE, "I2C not supported in this FT4222 chip configuration See the possible data streams depending on pins DCNF0 and DCNF1 on the device datasheet",},
	{FT4222_GPIO_NOT_SUPPORTED_IN_THIS_MODE, "GPIO not supported in this FT4222 chip configuration. See the possible data streams depending on pins DCNF0 and DCNF1 on the device datasheet",},
	{FT4222_GPIO_EXCEEDED_MAX_PORTNUM, "Exeeded max number GPIO ports supported in this FT4222 chip configuration. See the possible data streams depending on pins DCNF0 and DCNF1 in the device datasheet"},
	{FT4222_GPIO_WRITE_NOT_SUPPORTED, "GPIO write not supported",},
	{FT4222_GPIO_PULLUP_INVALID_IN_INPUTMODE, "GPIO pull-up invalid in input mode",},
	{FT4222_GPIO_PULLDOWN_INVALID_IN_INPUTMODE, "GPIO pull-down invalid in input mode",},
	{FT4222_GPIO_OPENDRAIN_INVALID_IN_OUTPUTMODE, "GPIO open-drain invalid in output mode",},
	{FT4222_INTERRUPT_NOT_SUPPORTED, "Interrupts not supported",},
	{FT4222_GPIO_INPUT_NOT_SUPPORTED, "GPIO input not supported",},
	{FT4222_EVENT_NOT_SUPPORTED, "Event not supported",},
	{FT4222_FUN_NOT_SUPPORT, "FUN Not supported",},
	{INCORRECT_NUMBER_BYTES, "The number of bytes sent was not equal to the number of bytes in the data to send",},
	{TIME_OUT_ERROR, "Time out error while waiting for flash device to get ready",},
	{CORRUPTED_UPLOAD, "The read data bits from the flash are not the same data bits that were programmed",}
};


