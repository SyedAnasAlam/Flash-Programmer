# IceBoard-Programmer
A tool to upload bitstreams to the [Ice Board FPGA board](https://github.com/SyedAnasAlam/Ice-Board)

## Build
The application uses two libraries from FTDI:
- ftd2xx: Linked statically
- LibFT4222: Linked dynamically
  - [```LibFT4222-64.dll```](Dependencies/LibFT4222/dll/) should be copied into your build folder
 
## Usage
```./IceBoard-Programmer.exe <file> ```
