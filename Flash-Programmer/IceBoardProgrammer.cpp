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
#include <stdlib.h>
#include <time.h>
#include <iomanip>
#include "IceBoard.h"

//std::map<int, std::string> statusMessages;

void HandleStatus(int status)
{
    if (status != (int)FT_OK || status != (int)FT4222_OK)
    {
        std::cout << statusMessages[status] << std::endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const* argv[])
{
    /*
    if (argc != 2)
    {
        std::cout << "Usage: Flash-Programmer <Filename>.bin" << std::endl;
        return EXIT_FAILURE;
    }
    */

    
    std::ifstream binfile("C:/Users/Saa03/Documents/Uni/6.Semester/Bachelorprojekt/Flash-Programmer/x64/Debug/Multiply.bin", std::ifstream::binary);
    std::vector<uint8> binfileBuffer;
    
    binfile.seekg(0, binfile.end);
    int binfileSize = binfile.tellg();
    binfile.seekg(0, binfile.beg);
    binfileBuffer.resize(binfileSize);
    binfile.read((char *) & binfileBuffer[0], binfileSize);
    binfile.close();   
    

    /*srand((unsigned int)time(NULL));
    int binfileSize = 135536;
    std::vector<uint8> binfileBuffer(binfileSize);
    for (int i = 0; i < binfileSize; i++)
        binfileBuffer[i] = i % 0xFF;*/
    
    FT_STATUS ftStatus = InitBoard();
    std::cout << "Connection established with Ice Board" << std::endl;
   
    HandleStatus(WakeUpFlash());
    std::cout << "Flash is waken up" << std::endl;

    HandleStatus(EraseFlash());
    std::cout << "Flash is erased" << std::endl;

    HandleStatus(ProgramFlash(binfileBuffer));
    std::cout << "Uploaded all file bits to flash" << std::endl;

    HandleStatus(ValidateFlash(binfileBuffer));
    std::cout << "Success! Flash is programmed" << std::endl;

    return EXIT_SUCCESS;
}
