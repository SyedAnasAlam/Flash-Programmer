/*
* Contains functions and constants that are speficic to the Ice Board
*/

#pragma once
#include <ftd2xx.h>
#include <LibFT4222.h>
#include <vector>
#include "Result.h"

enum FlashCommands
{
    ReadStatusRegisterCmd = 0x05,
    WakeUpCmd = 0xAB,
    WriteEnableCmd = 0x06,
    ChipEraseCmd = 0x60,
    ReadCmd = 0x03,
    PageProgramCmd = 0x02,
    DummyCmd = 0xFF
};

const int MAX_WAIT_TIME_MS = 500;
const int MAX_PAGE_PROGRAM_ATTEMPTS = 5;
const int FLASH_PAGE_SIZE = 256;

Result InitBoard();
Result WriteSPI(std::vector<uint8> writeBuffer, size_t bytesToWrite, bool isEndTransaction);
Result ReadSPI(std::vector<uint8>* readBuffer, size_t bytesToRead, bool isEndTransaction);
Result WaitForFlashReady();
Result WakeUpFlash();
Result EraseFlash();
Result WriteEnableFlash();
Result PageProgramFlash(int pageIndex, std::vector<uint8> writeBuffer);
Result ReadPageFlash(int pageIndex, std::vector<uint8>* readBuffer);
Result ProgramFlash(std::vector<uint8> fileBuffer);
Result ValidateFlash(std::vector<uint8> fileBuffer);
