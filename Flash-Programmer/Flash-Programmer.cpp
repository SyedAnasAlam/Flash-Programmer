#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <functional>
#include <ftd2xx.h>
#include <LibFT4222.h>
#include <fstream>
#include <iterator>
#include <algorithm>
#include "outcome.h"

#define IS_DEBUG 1

using namespace std;

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
const int FLASH_PAGE_SIZE = 256;
FT_HANDLE ft4222Handle = NULL;

//vector<unsigned char> binfileBuffer;
vector<uint8> binfileBuffer;
int binfileSize;

inline vector<uint8> intToByteArray(int x)
{
    vector<uint8> byte(3);
    byte[0] = x & 0x00FF0000;
    byte[1] = (x & 0x0000FF00) >> 8;
    byte[2] = (x & 0x000000FF) >> 16;

    return byte;
}

outcome OP_InitFT4222Device()
{
    outcome outcome = { };

    DWORD ftdiDdeviceCount = 0;                             // Number of FTDI (of any type) currently connected to hos
    size_t ft4222DeviceCount = 0;                            // Number of FT4222 devices currently connected to host
    std::vector<FT_DEVICE_LIST_INFO_NODE> ft4222Devices;    // Will store FT4222 devices that are currently connected to host
    
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

        const string deviceDescription = ftdiDeviceInfo.Description;
        if (deviceStatus == FT_OK && (deviceDescription == "FT4222" || deviceDescription == "FT4222 A"))
            ft4222Devices.push_back(ftdiDeviceInfo);
    }
    // Number of FT4222 devices
    ft4222DeviceCount = ft4222Devices.size();
    if (ft4222DeviceCount == 0)
    {
        outcome = { FT_DEVICE_NOT_FOUND, outcomeMessages[FT_DEVICE_NOT_FOUND]};
        return outcome;
    }
    
    // Connect/Open the chosen FT4222 device 
    // Simply connect to the first FT4222 device that was found 
    deviceStatus = FT_OpenEx((PVOID)ft4222Devices[0].SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &ft4222Handle);
    if (deviceStatus != FT_OK)
    {
        outcome = { (int)deviceStatus, outcomeMessages[deviceStatus]};
        return outcome;
    }
    
    deviceStatus = FT4222_SPIMaster_Init(ft4222Handle, SPI_IO_SINGLE, CLK_DIV_8, CLK_IDLE_HIGH, CLK_TRAILING, 0x01); // TODO: Verify last argument is correct
    if (deviceStatus != FT_OK)
    {
        outcome = { (int)deviceStatus, outcomeMessages[deviceStatus]};
        return outcome;
    }

    outcome = { SUCCESS, "Connected and initialized FT4222 device" };
    return outcome;
}
outcome OP_WaitForFlashReady()
{
    outcome outcome = { };

    uint8 receiveData[2];
    uint8 sendData[2] = { ReadStatusRegisterCmd, DummyCmd };
    uint16 bytesSent;

    for (int i = 0; i < MAX_WAIT_TIME_MS; i++)
    {
        FT4222_STATUS deviceStatus = FT4222_SPIMaster_SingleReadWrite(ft4222Handle, receiveData, sendData, sizeof(sendData), &bytesSent, true);
        if (deviceStatus != FT4222_OK)
        {
            outcome = { deviceStatus, outcomeMessages[deviceStatus]};
            return outcome;
        }
        else if (bytesSent != sizeof(sendData))
        {
            outcome = { INCORRECT_NUMBER_BYTES, outcomeMessages[INCORRECT_NUMBER_BYTES]};
            return outcome;
        }

        if((receiveData[1] & 0x01) == 0x00) // Busy flag is cleared
        {
            outcome = { DEBUG, "Flash is ready" };
            return outcome;
        }

        Sleep(1);
    } 

    return { TIME_OUT_ERROR, outcomeMessages[TIME_OUT_ERROR] };
}
outcome OP_WakeUpFlash()
{
    outcome outcome = { };

    uint8 sendData = WakeUpCmd;
    uint16 bytesSent;
  
    FT4222_STATUS deviceStatus = FT4222_SPIMaster_SingleWrite(ft4222Handle, &sendData, sizeof(sendData), &bytesSent, true);
    if (deviceStatus != FT4222_OK)
    {
        outcome = { deviceStatus, outcomeMessages[deviceStatus] };
        return outcome;
    }
    else if (bytesSent != sizeof(sendData))
    {
        outcome = { INCORRECT_NUMBER_BYTES, outcomeMessages[INCORRECT_NUMBER_BYTES] };
        return outcome;
    }
    
    return { DEBUG, "Flash wake up command sent"};
}
outcome OP_WriteEnable()
{
    outcome outcome = { };

    uint8 sendData = WriteEnableCmd;
    uint16 bytesSent;

    FT4222_STATUS deviceStatus = FT4222_SPIMaster_SingleWrite(ft4222Handle, &sendData, sizeof(sendData), &bytesSent, true);
    if (deviceStatus != FT4222_OK)
    {
        outcome = { deviceStatus, outcomeMessages[deviceStatus] };
        return outcome;
    }
    else if (bytesSent != sizeof(sendData))
    {
        outcome = { INCORRECT_NUMBER_BYTES, outcomeMessages[INCORRECT_NUMBER_BYTES] };
        return outcome;
    }

    return { DEBUG, "Write enable command sent" };
}
outcome OP_ChipErase()
{
    outcome outcome = { };

    uint8 sendData = ChipEraseCmd;
    uint16 bytesSent;

    FT4222_STATUS deviceStatus = FT4222_SPIMaster_SingleWrite(ft4222Handle, &sendData, sizeof(sendData), &bytesSent, true);
    if (deviceStatus != FT4222_OK)
    {
        outcome = { deviceStatus, outcomeMessages[deviceStatus] };
        return outcome;
    }
    else if (bytesSent != sizeof(sendData))
    {
        outcome = { INCORRECT_NUMBER_BYTES, outcomeMessages[INCORRECT_NUMBER_BYTES] };
        return outcome;
    }

    return { DEBUG, "Chip erase command sent" };
}
outcome OP_ProgramFlash()
{
    outcome outcome = { };

    int bufferPointer = 0;
    size_t pageCount = 1 + ((binfileSize - 1) / FLASH_PAGE_SIZE);

    uint8 sendCommand = PageProgramCmd;
    vector<uint8> sendAddress(3);
    uint16 bytesSent;
    FT4222_STATUS deviceStatus;

    for (int i = 0; i < pageCount; i++)
    {
        OP_WriteEnable();

        deviceStatus = FT4222_SPIMaster_SingleWrite(ft4222Handle, &sendCommand, sizeof(sendCommand), &bytesSent, false);
        if (deviceStatus != FT4222_OK)
        {
            outcome = { deviceStatus, outcomeMessages[deviceStatus] };
            return outcome;
        }
        else if (bytesSent != sizeof(sendCommand))
        {
            outcome = { INCORRECT_NUMBER_BYTES, outcomeMessages[INCORRECT_NUMBER_BYTES] };
            return outcome;
        }

        sendAddress = intToByteArray(bufferPointer);
        deviceStatus = FT4222_SPIMaster_SingleWrite(ft4222Handle, &sendAddress[0], sendAddress.size(), &bytesSent, false);
        if (deviceStatus != FT4222_OK)
        {
            outcome = { deviceStatus, outcomeMessages[deviceStatus] };
            return outcome;
        }
        else if (bytesSent != sendAddress.size())
        {
            outcome = { INCORRECT_NUMBER_BYTES, outcomeMessages[INCORRECT_NUMBER_BYTES] };
            return outcome;
        }
  
        int bytesToTransmit = (i == pageCount - 1) ? binfileSize - i*FLASH_PAGE_SIZE : FLASH_PAGE_SIZE;
        bool isEndTransaction = (i == pageCount - 1) ? true : false;

        deviceStatus = FT4222_SPIMaster_SingleWrite(ft4222Handle, &binfileBuffer[bufferPointer], bytesToTransmit, &bytesSent, true);
        if (deviceStatus != FT4222_OK)
        {
            outcome = { deviceStatus, outcomeMessages[deviceStatus] };
            return outcome;
        }
        else if (bytesSent != bytesToTransmit)
        {
            outcome = { INCORRECT_NUMBER_BYTES, outcomeMessages[INCORRECT_NUMBER_BYTES] };
            return outcome;
        }

        bufferPointer += bytesToTransmit;

        OP_WaitForFlashReady();
    }
   
    outcome = { DEBUG, "Transmitted all data bits to flash" };

    return outcome;
}
outcome OP_ReadFlash()
{
    outcome outcome = { };

    uint8* receiveData = new uint8[binfileSize];
    uint8 sendData[4] = { ReadCmd, 0, 0, 0 };
    uint16 bytesSent;
    uint16 bytesRead;


    FT4222_STATUS deviceStatus = FT4222_SPIMaster_SingleWrite(ft4222Handle, sendData, sizeof(sendData), &bytesSent, false);
    if (deviceStatus != FT4222_OK)
    {
        outcome = { deviceStatus, outcomeMessages[deviceStatus] };
        goto ret;
    }
    else if (bytesSent != sizeof(sendData))
    {
        outcome = { INCORRECT_NUMBER_BYTES, outcomeMessages[INCORRECT_NUMBER_BYTES] };
        goto ret;
    }

    deviceStatus = FT4222_SPIMaster_SingleRead(ft4222Handle, receiveData, binfileSize, &bytesRead, true);
    if (deviceStatus != FT4222_OK)
    {
        outcome = { deviceStatus, outcomeMessages[deviceStatus] };
        goto ret;
    }
    else if (bytesRead != binfileSize)
    {
        outcome = { INCORRECT_NUMBER_BYTES, outcomeMessages[INCORRECT_NUMBER_BYTES] };
        goto ret;
    }

    for (int i = 0; i < binfileSize; i++)
    {
        if (receiveData[i] != binfileBuffer[i])
        {
            outcome = { CORRUPTED_UPLOAD, outcomeMessages[CORRUPTED_UPLOAD] };
            goto ret;
        }
    }

    outcome = { SUCCESS, "Done programming flash" };

 ret:
    delete[] receiveData;
    return outcome;
}
  
void PerformOperations(vector<function<outcome()>> operations)
{
    for (const auto& op : operations)
    {
        outcome outcome = op();
        
        string prefix; 
    
        if (outcome.code == DEBUG)
            prefix = "DEBUG";
        else if (outcome.code == SUCCESS)
            prefix = "SUCCESS";
        else
            prefix = "ERROR " + to_string(outcome.code);
      
        if (outcome.code == DEBUG)
        {
            if (IS_DEBUG == 1)
                cout << prefix << ": " << outcome.msg << endl;
        }
        else 
            cout << prefix << ": " << outcome.msg << endl;
        
        if (outcome.code > 0)
        {
            exit(EXIT_FAILURE);
        }
    }
}


int main(int argc, char const* argv[])
{
    if (argc != 2)
    {
        //cout << "Usage: Flash-Programmer <Filename>.bin" << endl;
        //return EXIT_FAILURE;
    }
    
    std::ifstream binfile("C:/Users/Saa03/Documents/Uni/6.Semester/Bachelorprojekt/Flash-Programmer/x64/Debug/Sqrt3600.bin", std::ifstream::binary);
     
    // get length of file:
    binfile.seekg(0, binfile.end);
    binfileSize = binfile.tellg();
    binfile.seekg(0, binfile.beg);

    // allocate memory:
    binfileBuffer.resize(binfileSize);

    // read data as a block:
    binfile.read((char *) & binfileBuffer[0], binfileSize);

    binfile.close();   
    

    vector<function<outcome()>> operations 
    {
       &OP_InitFT4222Device,
       &OP_WaitForFlashReady,
       &OP_WakeUpFlash,
       &OP_WriteEnable,
       &OP_ChipErase,
       &OP_WaitForFlashReady,
       &OP_WriteEnable,
       &OP_ProgramFlash,
       &OP_WaitForFlashReady,
       &OP_ReadFlash
    };

    PerformOperations(operations);

    return EXIT_SUCCESS;
}
