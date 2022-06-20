# IceBoard-Programmer
A tool to upload bitstreams to the [Ice Board FPGA board](https://github.com/SyedAnasAlam/Ice-Board)

## Build
The application is developed using Visual Studio 2022 Community Edition, and compiled iwth MSVC 2014.
Two libraries from FTDI are used:
- ftd2xx: Linked statically
- LibFT4222: Linked dynamically
  - [```LibFT4222-64.dll```](Dependencies/LibFT4222/dll/) should be copied into your build folder
 
## Usage
```./IceBoard-Programmer.exe <file> ```
