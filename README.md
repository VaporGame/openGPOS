# openGPOS

a general purpose, open source operating system I am making for the RP2040.

bootloader and startup code based on vxj9800's code

configure and compile with
```
cmake -S . -B build
cmake --build build
```

features
- [x] UART
- [x] SPI
- [ ] USB serial (planned)
- [x] SD library (WIP)
- [x] FAT32 library (WIP)
- [x] ELF support (WIP)

TODO:
- [ ] Replace Cmake with a python script