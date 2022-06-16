/*
* Contains functions and constants that are speficic to the Ice Board
*/

#pragma once
#include <vector>
#include <map>
#include "ftd2xx.h"
#include "LibFT4222.h"

enum FlashCommands
{
    ReadStatusRegisterCmd = 0x05,
    WakeUpCmd = 0xAB,
    WriteEnableCmd = 0x06,
    ChipEraseCmd = 0x60,
    SectorEraseCmd = 0x20,
    ReadCmd = 0x03,
    PageProgramCmd = 0x02,
    DummyCmd = 0xFF
};

const int FLASH_SIZE = 262144;
const int FLASH_PAGE_SIZE = 256;
const int FLASH_SECTOR_SIZE = 4096;
const int MAX_READ_SIZE = 65535;
const int MAX_WAIT_TIME_MS = 500;
const int MAX_SECTOR_PROGRAM_ATTEMPTS = 5;

extern std::map<int, std::string> statusMessages;

FT_STATUS InitBoard();
FT4222_STATUS WriteSPI(std::vector<uint8> writeBuffer, size_t bytesToWrite, bool isEndTransaction);
FT4222_STATUS ReadSPI(std::vector<uint8>* readBuffer, size_t bytesToRead, bool isEndTransaction);
FT4222_STATUS WaitForFlashReady();
FT4222_STATUS WakeUpFlash();
FT4222_STATUS EraseFlash();
FT4222_STATUS EraseSector(int startAddress);
FT4222_STATUS WriteEnableFlash();
FT4222_STATUS PageProgramFlash(int pageIndex, std::vector<uint8> writeBuffer);
FT4222_STATUS SectorProgramFlash(int sectorIndex, std::vector<uint8> writeBuffer);
FT4222_STATUS ReadSectorFlash(int sectorIndex, std::vector<uint8>* readBuffer);
FT4222_STATUS ProgramFlash(std::vector<uint8> fileBuffer);
FT4222_STATUS ValidateFlash(std::vector<uint8> fileBuffer);

