# vesc-control

Control funcions and routines for the VESC, written in pure C.

### Supported Firmwares
- 4.12
- 23.3 (Unity)

### Instructions

include the ```vesc``` folder in your project, include the ```vesc``` folder as sources, and the ```vesc/include``` as include files.
if your project does not have crc funcions do the same with the ```crc``` folder.

create the high level funcion to write bytes to the communication interface (like put_char()), name it vesc_write_byte();

now in your code call periodically the vesc_process_buffer(uint8_t* buffer, uint16_t length), this will unpack vesc messages and process them, it will call the callback funcions, make sure you link the callbacks you want.

### Instructions

This software is licensed under [MIT](LICENSE);