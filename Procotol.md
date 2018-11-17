# USB2Snes websocket protocol

It's better to use the USB2snes websocket protocol to access usb2snes as it allows for multiple application to access the device at the same time. It also allows to access it through javascript in a webpage.

## Connection

The server is accessible on `ws://localhost:8080`. It's a regular no encrypted websocket server.

Request are send in json format. Data must be send in binary

## Performing a Request

Request are in json format and follow this schema :

``` json
{
    Opcode : "opcode name",
    Space : "SNES",
    Flags : ["FLAG1", "FLAGS2"],
    Operands : ["arg1", "arg2", ...]
}
```

`Opcode` is the command you want to be done. MANDATORY
`Space` can be SNES or CMD, but most of the time you want SNES. MANDATORY
`Flags` are specific usb2snes firmware flags, you rarely need them. OPTIONNAL
`Operands` are for the arguments of the command

## Reply

The websocket server send you back a json reply this form

``` json
{
    Results : ["data1", "data2"]
}
```

## Errors

If an error occur, the server will just close your connection.

## First commands

### Get devices list

Your first command should be `DeviceList` to received the list of available device. In the case of Usb2snes original server you will only get sd2snes hardware as their port name. QUsb2Snes support more 'device' so their name is important as you cannot just list your serial device ports.

Example

```json
{
    Opcode : "DeviceList",
    Space : "SNES"
}
```
usb2snes

```json
{
    Results :  ["COM3", "COM5"]
}
```
QUsb2snes
```json
{
    Results: ["SD2SNES COM3", "SNES Classic", "EMU SNES9x"]
}
```

### Attach to the device

Next command is `Attach` to associate yourself with the device you want.

```json
{
    Opcode : "Attach",
    Space : "SNES",
    Operands : ["SD2SNES COM3"]
}
```

You will get no reply if the command succeeded, a failure will be the server closing the connection

## Command list

The full command list can be found here in https://github.com/Skarsnik/QUsb2snes/blob/master/usb2snes.h#L84 we will just go over the most basic one.

### Name [ApplicationName]

It's better to use a Name since both usb2snes and QUsb2snes can display the client connected. 

### Info

This give you information on what the device is running.
Returns you the firmware version string, the version, the current running rom, and a flags list.
The current running rom is interesting since for usb2snes device `/sd2snes/menu.bin` tell you the device is not running a ROM.
For QUsb2snes non usb2snes devices you will mostly get garbage informations

### GetAddress [offset, size]

To read memory from the device. Offset and size must be both encoded in hexadecimal. Offset are not Snes address, they are specific on how usb2snes store the various information (more later).

This read the first 256 bytes from the WRAM

```json
{
    Opcode : "GetAddress",
    Space : "SNES",
    Operands : ["F50000", "100"]
}
```

The server will reply directly with binary data corresponding to what you requested. Be careful, usb2snes implementation make it send data by chunck of 1024 bytes so don't execpt the full data in one go if you request more than this value.

### PutAddress [offset, size]

This work like GetAddress for argument. After sending the json request, send your binary data. Again with the original usb2snes server don't send more than 1024 bytes per binary send, send the data per chunk of 1024.

## Usb2snes address

ROM start at  `0x000000`
WRAM start at `0xF50000`
SRAM start at `0xE00000`



