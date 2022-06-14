/*
* Structure and functions for handling errors and relaying info to the user
*/

#pragma once
#include <map>
#include <string>
#include <ftd2xx.h>
#include <LibFT4222.h>

// Custom result codes that are not found in the ftd2xx or LibFT4222 libraries
#define SUCCESS 0
#define DEBUG 0
#define INCORRECT_NUMBER_BYTES 19
#define TIME_OUT_ERROR 20
#define CORRUPTED_UPLOAD 21

struct Result {
	int code;
	std::string msg;
};

// TODO: Change error message to be about board not FTDI device

extern std::map<int, std::string> resultMessages;



