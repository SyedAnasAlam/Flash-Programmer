#include <string>
#include <iostream>
#include <map>
#include "IceBoard.h"

FT_HANDLE IceBoardHandle;

inline std::vector<unsigned char> IntToByteVec(int x)
{
    std::vector<unsigned char> byte(3);
    byte[0] = (x & 0x00FF0000) >> 16;
    byte[1] = (x & 0x0000FF00) >> 8;
    byte[2] = (x & 0x000000FF);

    return byte;
}

FT_STATUS InitBoard()
{
    FT_STATUS status = FT_OK;

    DWORD ftdiDdeviceCount = 0;                          // Number of FTDI (of any type) currently connected to hos
    size_t ft4222DeviceCount = 0;                        // Number of FT4222 devices currently connected to host
    std::vector<FT_DEVICE_LIST_INFO_NODE> ft4222Devices; // Will store FT4222 devices that are currently connected to host

    status = FT_CreateDeviceInfoList(&ftdiDdeviceCount);

    // Loop through all connected FTDI devices and find those who are of type FT4222
    for (DWORD i = 0; i < ftdiDdeviceCount; ++i)
    {
        FT_DEVICE_LIST_INFO_NODE ftdiDeviceInfo;
        memset(&ftdiDeviceInfo, 0, sizeof(ftdiDeviceInfo));

        status = FT_GetDeviceInfoDetail(
            i,
            &ftdiDeviceInfo.Flags,
            &ftdiDeviceInfo.Type,
            &ftdiDeviceInfo.ID,
            &ftdiDeviceInfo.LocId,
            ftdiDeviceInfo.SerialNumber,
            ftdiDeviceInfo.Description,
            &ftdiDeviceInfo.ftHandle);

        const std::string deviceDescription = ftdiDeviceInfo.Description;
        if (status == FT_OK && (deviceDescription == "FT4222" || deviceDescription == "FT4222 A"))
            ft4222Devices.push_back(ftdiDeviceInfo);
    }
    // Number of FT4222 devices
    ft4222DeviceCount = ft4222Devices.size();
    if (ft4222DeviceCount == 0)
        return FT_DEVICE_NOT_FOUND;

    // Connect/Open the chosen FT4222 device 
    // Simply connect to the first FT4222 device that was found 
    status = FT_OpenEx((PVOID)ft4222Devices[0].SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &IceBoardHandle);
    if (status != FT_OK)
        return status;

    status = FT4222_SPIMaster_Init(IceBoardHandle, SPI_IO_SINGLE, CLK_DIV_2, CLK_IDLE_HIGH, CLK_TRAILING, 0x01);
    if (status != FT_OK)
        return status;

    return status;
}
FT4222_STATUS WriteSPI(std::vector<uint8> writeBuffer, size_t bytesToWrite, bool isEndTransaction)
{
    FT4222_STATUS status = FT4222_OK;
    uint16 bytesTransferred;
    
    status = FT4222_SPIMaster_SingleWrite(IceBoardHandle, &writeBuffer[0], (uint16)bytesToWrite, &bytesTransferred, isEndTransaction);
    if (status != FT4222_OK)
        return status;
    else if (bytesTransferred != bytesToWrite)
        return FT4222_INORRECT_TRANSFER_SIZE;

    return status;
}
FT4222_STATUS ReadSPI(std::vector<uint8>* readBuffer, size_t bytesToRead, bool isEndTransaction)
{
    FT4222_STATUS status;
    uint16 bytesRead;

    status = FT4222_SPIMaster_SingleRead(IceBoardHandle, &(*readBuffer)[0], (uint16)bytesToRead, &bytesRead, isEndTransaction);
    if (status != FT4222_OK)
        return status;
    else if (bytesRead != bytesToRead)
        return FT4222_INORRECT_TRANSFER_SIZE;

    return status;
}
FT4222_STATUS WaitForFlashReady()
{
    FT4222_STATUS status;

    std::vector<uint8> readBuffer(1);

    for (int i = 0; i < MAX_WAIT_TIME_MS; i++)
    {
        status = WriteSPI({ ReadStatusRegisterCmd }, 1, false);
        if (status != FT4222_OK)
            return status;

        status = ReadSPI(&readBuffer, readBuffer.size(), true);
        if (status != FT4222_OK)
            return status;

        if ((readBuffer[0] & 0x01) == 0x00)
            return FT4222_OK;

        Sleep(1);
    }

    return FT4222_TIME_OUT_ERROR;
}
FT4222_STATUS WakeUpFlash()
{
    FT4222_STATUS status = WriteSPI({ WakeUpCmd }, 1, true);
    if (status != FT4222_OK)
        return status;

    return status;
}
FT4222_STATUS EraseFlash()
{
    FT4222_STATUS status;

    status = WriteEnableFlash();
    if (status != FT4222_OK)
        return status;

    status = WriteSPI({ ChipEraseCmd }, 1, true);
    if (status != FT4222_OK)
        return status;

    status = WaitForFlashReady();
    if (status != FT4222_OK)
        return status;

    return status;
}
FT4222_STATUS EraseSector(int sectorIndex)
{
    FT4222_STATUS status;
    
    status = WriteEnableFlash();
    if (status != FT4222_OK)
        return status;

    std::vector<uint8> writeBuffer = IntToByteVec(sectorIndex * FLASH_SECTOR_SIZE);
    writeBuffer.insert(writeBuffer.begin(), SectorEraseCmd);

    status = WriteSPI(writeBuffer, writeBuffer.size(), true);
    if (status != FT4222_OK)
        return status;

    status = WaitForFlashReady();
    if (status != FT4222_OK)
        return status;

    return status;
}
FT4222_STATUS WriteEnableFlash()
{
    FT4222_STATUS status = WriteSPI({ WriteEnableCmd }, 1, true);
    if (status != FT4222_OK)
        return status;

    return status;
}
FT4222_STATUS PageProgramFlash(int pageIndex, std::vector<uint8> writeBuffer)
{
    FT4222_STATUS status;

    std::vector<uint8> readBuffer(FLASH_PAGE_SIZE);
    int startAddress = pageIndex * FLASH_PAGE_SIZE;
    std::vector<uint8> commandAndAddressBuffer = IntToByteVec(startAddress);
    commandAndAddressBuffer.insert(commandAndAddressBuffer.begin(), PageProgramCmd);

    status = WriteEnableFlash();
    if (status != FT4222_OK)
        return status;

    status = WriteSPI(commandAndAddressBuffer, commandAndAddressBuffer.size(), false);
    if (status != FT4222_OK)
        return status;

    status = WriteSPI(writeBuffer, FLASH_PAGE_SIZE, true);
    if (status != FT4222_OK)
        return status;

    status = WaitForFlashReady();
    if (status != FT4222_OK)
        return status;


    return status;
}
FT4222_STATUS SectorProgramFlash(int sectorIndex, std::vector<uint8> sectorBuffer)
{
    FT4222_STATUS status = FT4222_OK;

    int pageCount = (int)sectorBuffer.size() < FLASH_SECTOR_SIZE ? 1 + (((int)sectorBuffer.size() - 1) / FLASH_PAGE_SIZE) : FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE;
    int pageStartIndex = sectorIndex * (FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE);
    
    for (int i = 0; i < pageCount; i++)
    {
        std::vector<uint8>::const_iterator first = sectorBuffer.begin() + i*FLASH_PAGE_SIZE;
        std::vector<uint8>::const_iterator last = i == pageCount - 1 ? sectorBuffer.end() : sectorBuffer.begin() + (i + 1) * FLASH_PAGE_SIZE;
        std::vector<uint8> pageBuffer(first, last);

        status = PageProgramFlash(pageStartIndex + i, pageBuffer);
        if (status != FT4222_OK)
            return status;
    }

    return status;
}
FT4222_STATUS ReadSectorFlash(int sectorIndex, std::vector<uint8>* readBuffer)
{
    FT4222_STATUS status;

    std::vector<uint8> commandAndAddressBuffer = IntToByteVec(sectorIndex*FLASH_SECTOR_SIZE);
    commandAndAddressBuffer.insert(commandAndAddressBuffer.begin(), ReadCmd);

    status = WriteSPI(commandAndAddressBuffer, commandAndAddressBuffer.size(), false);
    if (status != FT4222_OK)
        return status;

    readBuffer->resize(FLASH_SECTOR_SIZE);
    status = ReadSPI(readBuffer, FLASH_SECTOR_SIZE, true);
    if (status != FT4222_OK)
        return status;

    return status;
}
FT4222_STATUS ProgramFlash(std::vector<uint8> fileBuffer)
{
    FT4222_STATUS status = FT4222_OK;

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

            bytesToTransmit = (i == sectorCount - 1) ? (int)fileBuffer.size() - i * FLASH_SECTOR_SIZE : FLASH_SECTOR_SIZE;

            status = SectorProgramFlash(i, sectorBuffer);
            if (status != FT4222_OK)
                return status;

            WaitForFlashReady();

            status = ReadSectorFlash(i, &readBuffer);
            if (status != FT4222_OK)
                return status;

            for (int j = 0; j < bytesToTransmit; j++)
            {
                if (readBuffer[j] != sectorBuffer[j])
                    errorCount++;
            }

            if (errorCount > 0)
            {
                std::cout << "Failed to program sector " <<  i << ", " << errorCount << " Errors" << std::endl;
                status = EraseSector(i);
                if (status != FT4222_OK)
                    return status;
            }
            
            success = errorCount == 0;
                
            attempts++;
            if (attempts == MAX_SECTOR_PROGRAM_ATTEMPTS)
                return FT4222_CORRUPTED_UPLOAD;

        }
        bufferPointer += FLASH_SECTOR_SIZE;
    }
    return status;
}
FT4222_STATUS ValidateFlash(std::vector<uint8> fileBuffer)
{
    FT4222_STATUS status = FT4222_OK;

    std::vector<uint8> readBuffer(fileBuffer.size());
    std::vector<uint8> commandAndAddressBuffer;
    int n = 1 + (((int)fileBuffer.size() - 1) / MAX_READ_SIZE);
    int pointer = 0;


    for (int i = 0; i < n; i++)
    {
        commandAndAddressBuffer = IntToByteVec(pointer);
        commandAndAddressBuffer.insert(commandAndAddressBuffer.begin(), ReadCmd);
        int bytesToRead = i == n - 1 ? (int)fileBuffer.size() - i * MAX_READ_SIZE : MAX_READ_SIZE;
        std::vector<uint8> tempBuffer(bytesToRead);

        status = WriteSPI(commandAndAddressBuffer, commandAndAddressBuffer.size(), false);
        if (status != FT4222_OK)
            return status;

        status = ReadSPI(&tempBuffer, bytesToRead, true);
        if (status != FT4222_OK)
            return status;

        for (int j = 0; j < bytesToRead; j++)
            readBuffer[pointer + j] = tempBuffer[j];

        pointer += MAX_READ_SIZE;
    }
    
    for (int i = 0; i < fileBuffer.size(); i++)
    {
        if (readBuffer[i] != fileBuffer[i])
        {
            std::cout << "Corruption at address " + std::to_string(i) << std::endl;
            return FT4222_CORRUPTED_UPLOAD;
        }
            
    }

    return status;
}


