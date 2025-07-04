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
- [ ] SD library (WIP)