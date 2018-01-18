## Wi-Fi module ESP8266 (ESP01 SoC) with ds18b20 temperature measurer firmware

#### Main idea ( description )
The project is created to connect WiFi module with temperature measurer and PC with TCP protocol in router ( AP ) sphere ( local area network ) and evaluate the temperature level in closed room to show when it would be nice to air

#### Steps
1. Trying to connect ESP01 to router ( AP ) with his SSID and password in Station mode
1. Prepare network configurations ( physical mode, MAC-address, TCP server IP and local/remote ports )
1. Trying to connect to TCP server in AP zone with prepared settings
1. Evaluate the temperature with ds18b20
1. Send message of temperature measures per 2 seconds

#### Toolchain and auxiliary programs
* ESP8266 NONOS SDK
* GNU Make Builder
* GCC C Compiler
* xtensa-lx106 toolchain
* Eclipse Luna
* Terminal || Cool Term - to debug and upgrade firmware with USB-UART
* Esptool || XTCOM UTIL - to create firmware
* SocketTest - to create TCP server on PC and get the temperature data

#### Schemes

##### Debug scheme
| USB-UART | ESP8266 |
| --- | --- |
| ACCV0 - 3.3v | VCC and CH_PD from resistor of 4K nominal |
| GND | GND ( and GPIO0 while debugging ) |
| RXD | TXD |
| TXD | RXD |

##### Release scheme
| ds18b20 | ESP8266 |
| --- | --- |
| GND | GND |
| DQ | GPIO2 and VCC from resistor of 4K nominal  |
| Vdd | VCC |

| energy ( 2xAA ) | ESP8266 |
| --- | --- |
| ACCV | VCC |
| GND | GND  |

| NB: now we can put our device to any place in radius of router ( AP ) zone to collect temperature data on the PC
