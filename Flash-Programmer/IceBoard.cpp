#include <string>
#include <iostream>
#include "IceBoard.h"

FT_HANDLE IceBoardHandle;

inline std::vector<uint8> IntToByteVec(int x)
{
    std::vector<uint8> byte(3);
    byte[0] = x & 0x00FF0000;
    byte[1] = (x & 0x0000FF00) >> 8;
    byte[2] = (x & 0x000000FF) >> 16;

    return byte;
}

Result InitBoard()
{
    Result result = { };

    DWORD ftdiDdeviceCount = 0;                     // Number of FTDI (of any type) currently connected to hos
    size_t ft4222DeviceCount = 0;                   // Number of FT4222 devices currently connected to host
    std::vector<FT_DEVICE_LIST_INFO_NODE> ft4222Devices; // Will store FT4222 devices that are currently connected to host

    FT_STATUS deviceStatus = FT_CreateDeviceInfoList(&ftdiDdeviceCount);

    // Loop through all connected FTDI devices and find those who are of type FT4222
    for (DWORD i = 0; i < ftdiDdeviceCount; ++i)
    {
        FT_DEVICE_LIST_INFO_NODE ftdiDeviceInfo;
        memset(&ftdiDeviceInfo, 0, sizeof(ftdiDeviceInfo));

        deviceStatus = FT_GetDeviceInfoDetail(
            i,
            &ftdiDeviceInfo.Flags,
            &ftdiDeviceInfo.Type,
            &ftdiDeviceInfo.ID,
            &ftdiDeviceInfo.LocId,
            ftdiDeviceInfo.SerialNumber,
            ftdiDeviceInfo.Description,
            &ftdiDeviceInfo.ftHandle);

        const std::string deviceDescription = ftdiDeviceInfo.Description;
        if (deviceStatus == FT_OK && (deviceDescription == "FT4222" || deviceDescription == "FT4222 A"))
            ft4222Devices.push_back(ftdiDeviceInfo);
    }
    // Number of FT4222 devices
    ft4222DeviceCount = ft4222Devices.size();
    if (ft4222DeviceCount == 0)
    {
        result = { FT_DEVICE_NOT_FOUND, resultMessages[FT_DEVICE_NOT_FOUND] };
        return result;
    }

    // Connect/Open the chosen FT4222 device 
    // Simply connect to the first FT4222 device that was found 
    deviceStatus = FT_OpenEx((PVOID)ft4222Devices[0].SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &IceBoardHandle);
    if (deviceStatus != FT_OK)
    {
        result = { (int)deviceStatus, resultMessages[deviceStatus] };
        return result;
    }

    deviceStatus = FT4222_SPIMaster_Init(IceBoardHandle, SPI_IO_SINGLE, CLK_DIV_512, CLK_IDLE_HIGH, CLK_TRAILING, 0x01); // TODO: Verify last argument is correct
    if (deviceStatus != FT_OK)
    {
        result = { (int)deviceStatus, resultMessages[deviceStatus] };
        return result;
    }

    result = { SUCCESS, "Connected and initialized Ice Board" };
    return result;
}

Result WriteSPI(std::vector<uint8> writeBuffer, size_t bytesToWrite, bool isEndTransaction)
{
    Result result = { };

    FT4222_STATUS boardStatus;
    uint16 bytesTransferred;
    
    boardStatus = FT4222_SPIMaster_SingleWrite(IceBoardHandle, &writeBuffer[0], (uint16)bytesToWrite, &bytesTransferred, isEndTransaction);
    if (boardStatus != FT4222_OK)
    {
        result = { boardStatus, resultMessages[boardStatus] };
        return result;
    }
    else if (bytesTransferred != bytesToWrite)
    {
        result = { INCORRECT_NUMBER_BYTES, resultMessages[INCORRECT_NUMBER_BYTES] };
        return result;
    }

    return result;
}

Result ReadSPI(std::vector<uint8>* readBuffer, size_t bytesToRead, bool isEndTransaction)
{
    Result result = { };

    FT4222_STATUS boardStatus;
    uint16 bytesRead;

    boardStatus = FT4222_SPIMaster_SingleRead(IceBoardHandle, &(*readBuffer)[0], (uint16)bytesToRead, &bytesRead, isEndTransaction);
    if (boardStatus != FT4222_OK)
    {
        result = { boardStatus, resultMessages[boardStatus] };
        return result;
    }
    else if (bytesRead != bytesToRead)
    {
        result = { INCORRECT_NUMBER_BYTES, resultMessages[INCORRECT_NUMBER_BYTES] };
        return result;
    }

    return result;
}

Result WaitForFlashReady()
{
    Result result = { };

    std::vector<uint8> readBuffer(1);

    for (int i = 0; i < MAX_WAIT_TIME_MS; i++)
    {
        result = WriteSPI({ ReadStatusRegisterCmd }, 1, false);
        if (result.code)
            return result;

        result = ReadSPI(&readBuffer, readBuffer.size(), true);
        if (result.code)
            return result;

        if ((readBuffer[0] & 0x01) == 0x00)
        {
            result = { DEBUG, "Flash is ready" };
            return result;
        }

        Sleep(1);
    }

    return { TIME_OUT_ERROR, resultMessages[TIME_OUT_ERROR] };
}

Result WakeUpFlash()
{
    Result result = WriteSPI({ WakeUpCmd }, 1, true);
    if (result.code)
        return result;

    return { DEBUG, "Flash wake up command sent" };
}

Result EraseFlash()
{
    Result result = { }; 

    result = WriteEnableFlash();
    if (result.code)
        return result;

    result = WriteSPI({ ChipEraseCmd }, 1, true);
    if (result.code)
        return result;

    result = WaitForFlashReady();
    if (result.code)
        return result;

    return { DEBUG, "Chip erase command sent" };
}

Result EraseSector(int startAddress)
{
    Result result = { };
    
    result = WriteEnableFlash();
    if (result.code)
        return result;

    std::vector<uint8> writeBuffer = IntToByteVec(startAddress);
    writeBuffer.insert(writeBuffer.begin(), ChipEraseCmd);

    result = WriteSPI(writeBuffer, writeBuffer.size(), true);
    if (result.code)
        return result;

    result = WaitForFlashReady();
    if (result.code)
        return result;

    return { DEBUG, "Erased 4KB sector starting from address " + std::to_string(startAddress) };
}

Result WriteEnableFlash()
{
    Result result = WriteSPI({ WriteEnableCmd }, 1, true);
    if (result.code)
        return result;

    return { DEBUG, "Write enable command sent" };
}

Result PageProgramFlash(int pageIndex, std::vector<uint8> writeBuffer)
{
    Result result = { };

    std::vector<uint8> readBuffer(FLASH_PAGE_SIZE);
    int startAddress = pageIndex * FLASH_PAGE_SIZE;
    std::vector<uint8> commandAndAddressBuffer = IntToByteVec(startAddress);
    commandAndAddressBuffer.insert(commandAndAddressBuffer.begin(), PageProgramCmd);

    result = WriteEnableFlash();
    if (result.code)
        return result;

    result = WriteSPI(commandAndAddressBuffer, commandAndAddressBuffer.size(), false);
    if (result.code)
        return result;

    result = WriteSPI(writeBuffer, FLASH_PAGE_SIZE, true);
    if (result.code)
        return result;

    result = WaitForFlashReady();
    if (result.code)
        return result;


    return { DEBUG, "Programmed page number " + std::to_string(pageIndex) };
}

Result SectorProgramFlash(int sectorIndex, std::vector<uint8> sectorBuffer)
{
    Result result = { };

    int pageCount = sectorBuffer.size() < FLASH_SECTOR_SIZE ? 1 + ((sectorBuffer.size() - 1) / FLASH_PAGE_SIZE) : FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE;
    int pageStartIndex = sectorIndex / FLASH_PAGE_SIZE;
    
    for (int i = 0; i < pageCount; i++)
    {
        std::vector<uint8>::const_iterator first = sectorBuffer.begin() + i*FLASH_PAGE_SIZE;
        std::vector<uint8>::const_iterator last = i == pageCount - 1 ? sectorBuffer.end() : sectorBuffer.begin() + (i + 1) * FLASH_PAGE_SIZE;
        std::vector<uint8> pageBuffer(first, last);

        result = PageProgramFlash(pageStartIndex + i, pageBuffer);
        if (result.code)
            return result;
    }

    return {DEBUG, "Programmed sector " + std::to_string(sectorIndex)};
}

Result ReadPageFlash(int pageIndex, std::vector<uint8>* readBuffer)
{
    Result result = { };

    std::vector<uint8> commandAndAddressBuffer = IntToByteVec(pageIndex);
    commandAndAddressBuffer.insert(commandAndAddressBuffer.begin(), ReadCmd);

    result = WriteSPI(commandAndAddressBuffer, commandAndAddressBuffer.size(), false);
    if (result.code)
        return result;

    readBuffer->resize(FLASH_PAGE_SIZE);
    result = ReadSPI(readBuffer, FLASH_PAGE_SIZE, true);
    if (result.code)
        return result;

    return { DEBUG, "Read page number " + std::to_string(pageIndex) };
}

Result ReadSectorFlash(int sectorIndex, std::vector<uint8>* readBuffer)
{
    Result result = { };

    std::vector<uint8> commandAndAddressBuffer = IntToByteVec(sectorIndex*FLASH_SECTOR_SIZE);
    commandAndAddressBuffer.insert(commandAndAddressBuffer.begin(), ReadCmd);

    result = WriteSPI(commandAndAddressBuffer, commandAndAddressBuffer.size(), false);
    if (result.code)
        return result;

    readBuffer->resize(FLASH_SECTOR_SIZE);
    result = ReadSPI(readBuffer, FLASH_SECTOR_SIZE, true);
    if (result.code)
        return result;

    return { DEBUG, "Read sector number" + std::to_string(sectorIndex) };
}

Result ProgramFlash(std::vector<uint8> fileBuffer)
{
    Result result = { };

    int bufferPointer = 0;
    std::vector<uint8> readBuffer;
    size_t sectorCount = 1 + ((fileBuffer.size() - 1) / FLASH_SECTOR_SIZE);
    bool success = false;
    int attempts = 0;
    int bytesToTransmit = 0;
    int errorCount = 0;

    for (int i = 0; i < sectorCount; i++)
    {
        success = false;
        attempts = 0;

        while (!success)
        {
            errorCount = 0;
            std::vector<uint8>::const_iterator first = fileBuffer.begin() + bufferPointer;
            std::vector<uint8>::const_iterator last = i == sectorCount - 1 ? fileBuffer.end() : fileBuffer.begin() + bufferPointer + FLASH_SECTOR_SIZE;
            std::vector<uint8> sectorBuffer(first, last);

            bytesToTransmit = (i == sectorCount - 1) ? fileBuffer.size() - i * FLASH_SECTOR_SIZE : FLASH_SECTOR_SIZE;

            result = SectorProgramFlash(i, sectorBuffer);
            if (result.code)
                return result;

            WaitForFlashReady();

            result = ReadSectorFlash(i, &readBuffer);
            if (result.code)
                return result;

            for (int j = 0; j < bytesToTransmit; j++)
            {
                if(readBuffer[j] != sectorBuffer[j])
                {
                    errorCount++;
                    std::cout << "Flash[" << bufferPointer + j << "] = " << (int)readBuffer[j] << " ; Should be " << (int)sectorBuffer[j] << std::endl;
                }
            }

            if (errorCount > 0)
            {
                success = false;
                std::cout << "Failed to program page " << bufferPointer + i << ", " << errorCount << " Errors" << std::endl;
                EraseSector(bufferPointer);
            }
            else
            {
                success = true;
            }
                
            attempts++;
            if (attempts == MAX_PAGE_PROGRAM_ATTEMPTS)
            {
                result = { CORRUPTED_UPLOAD, resultMessages[CORRUPTED_UPLOAD] };
                return result;
            }
        }
        bufferPointer += FLASH_SECTOR_SIZE;
    }
    return { DEBUG, "Programmed all file bits to flash" };
}

/*
Result ProgramFlash(std::vector<uint8> fileBuffer)
{
    Result result = { };

    int bufferPointer = 0;
    std::vector<uint8> readBuffer;
    bool success = false;
    int errorCount = 0;
    int attempts = 0;
    size_t pageCount = 1 + ((fileBuffer.size() - 1) / FLASH_PAGE_SIZE);

    for (int i = 0; i < pageCount; i++)
    {
        success = false;
        attempts = 0;
        while(!success)
        { 
            errorCount = 0;
            std::vector<uint8>::const_iterator first = fileBuffer.begin() + bufferPointer;
            std::vector<uint8>::const_iterator last = i == pageCount - 1 ? 
                                                    fileBuffer.end() :
                                                    fileBuffer.begin() + bufferPointer + FLASH_PAGE_SIZE;
            std::vector<uint8> writeBuffer(first, last);
            int bytesToTransmit = (i == pageCount - 1) ? fileBuffer.size() - i * FLASH_PAGE_SIZE : FLASH_PAGE_SIZE;

            result = PageProgramFlash(bufferPointer, writeBuffer);
            if (result.code)
                return result;

            WaitForFlashReady();

            result = ReadPageFlash(bufferPointer, &readBuffer);
            if (result.code)
                return result;
            
            for (int j = 0; j < bytesToTransmit; j++)
            {
                if (readBuffer[j] != writeBuffer[j])
                {
                    errorCount++;
                    std::cout << "Flash[" << bufferPointer + j << "] = " << (int)readBuffer[j] << " ; Should be " << (int)writeBuffer[j] << std::endl;
                }
            }
            if (errorCount > 0)
                std::cout << "Failed to program page " << i << ", " << errorCount << " Errors" << std::endl;

            attempts++;
            if (attempts == MAX_PAGE_PROGRAM_ATTEMPTS)
            {
                result = { CORRUPTED_UPLOAD, resultMessages[CORRUPTED_UPLOAD] };
                return result;
            }

            success = errorCount > 0 ? false : true;
        }
        bufferPointer += FLASH_PAGE_SIZE;
    }

    return { DEBUG, "Programmed all file bits to flash" };
}
*/
Result ValidateFlash(std::vector<uint8> fileBuffer)
{
    Result result = {};
    
    std::vector<uint8> readBuffer(fileBuffer.size());

    result = WriteSPI({ ReadCmd, 0, 0, 0 }, 4, false);
    if (result.code)
        return result;

    result = ReadSPI(&readBuffer, fileBuffer.size(), true);
    if (result.code)
        return result;

    for (int i = 0; i < fileBuffer.size(); i++)
    {
        if (readBuffer[i] != fileBuffer[i])
        {
            result = { CORRUPTED_UPLOAD, resultMessages[CORRUPTED_UPLOAD] };
            return result;
        }
    }

    return { SUCCESS, "Sucessfully programmed flash" };
}