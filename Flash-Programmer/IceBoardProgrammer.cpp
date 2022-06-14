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
#include "Result.h"
#include "IceBoard.h"

#define COUNT 5000


void handleResult(Result result)
{
    std::string prefix;

    if (result.code == DEBUG)
        prefix = "DEBUG";
    else if (result.code == SUCCESS)
        prefix = "SUCCESS";
    else
        prefix = "ERROR " + std::to_string(result.code);

    std::cout << prefix << ": " << result.msg << std::endl;

    if (result.code > 0)
        exit(EXIT_FAILURE);
}

int main(int argc, char const* argv[])
{
    /*
    if (argc != 2)
    {
        cout << "Usage: Flash-Programmer <Filename>.bin" << endl;
        return EXIT_FAILURE;
    }
    std::ifstream binfile("C:/Users/Saa03/Documents/Uni/6.Semester/Bachelorprojekt/Flash-Programmer/x64/Debug/Sqrt3600.bin", std::ifstream::binary);

    binfile.seekg(0, binfile.end);
    binfileSize = binfile.tellg();
    binfile.seekg(0, binfile.beg);
    binfileBuffer.resize(binfileSize);
    binfile.read((char *) & binfileBuffer[0], binfileSize);
    binfile.close();   
    */

    std::cout << std::showbase << std::internal << std::setfill('0');

    srand(time(NULL));

    int binfileSize = 65535;
    std::vector<uint8> binfileBuffer(binfileSize);
    for(int i = 0; i < binfileSize; i++)
        binfileBuffer[i] = rand() % 0xFF;
    
    handleResult(InitBoard());
    handleResult(WakeUpFlash());
    handleResult(EraseFlash());
    handleResult(ProgramFlash(binfileBuffer));
    handleResult(ValidateFlash(binfileBuffer));

    return EXIT_SUCCESS;
}
