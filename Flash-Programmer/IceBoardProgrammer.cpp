#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <time.h>
#include "ftd2xx.h"
#include "LibFT4222.h"
#include "IceBoard.h"

void HandleStatus(int status)
{
    if (status != (int)FT_OK || status != (int)FT4222_OK)
    {
        std::cout << statusMessages[status] << std::endl;
        exit(EXIT_FAILURE);
    }
}

std::vector<uint8> OpenFile(std::string filePath)
{
    std::vector<uint8> fileBuffer;
    int fileSize;
    
    std::ifstream file(filePath, std::ifstream::binary);
    if (!file.good())
    {
        std::cout << "Error opening file" << std::endl;
        exit(EXIT_FAILURE);
    }

    file.seekg(0, file.end);
    fileSize = (int)file.tellg();
    file.seekg(0, file.beg);

    if (fileSize > FLASH_SIZE)
    {
        std::cout << "Too large file" << std::endl;
        exit(EXIT_FAILURE);
    }

    fileBuffer.resize(fileSize);
    file.read((char*)&fileBuffer[0], fileSize);
    file.close();

    return fileBuffer;
}

int main(int argc, char const* argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: Flash-Programmer <Filename>.bin" << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<uint8> fileBuffer;
    fileBuffer = OpenFile(argv[1]);
    
    HandleStatus(InitBoard());
    std::cout << "Connection established with Ice Board" << std::endl;
    HandleStatus(WakeUpFlash());
    HandleStatus(EraseFlash());
    std::cout << "Uploading " << fileBuffer.size() << " Bytes" << std::endl;
    HandleStatus(ProgramFlash(fileBuffer));
    HandleStatus(ValidateFlash(fileBuffer));
    std::cout << "Success! Flash is programmed" << std::endl;
    
    return EXIT_SUCCESS;
}
