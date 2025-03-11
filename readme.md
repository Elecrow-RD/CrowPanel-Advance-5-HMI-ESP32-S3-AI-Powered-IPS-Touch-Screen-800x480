1, Product picture

![ESP32 Advance HMI 5inch AI display](https://www.elecrow.com/media/catalog/product/cache/9e67447b006ee4d9559353b91d12add5/e/s/esp32_advance_hmi_5.0inch_display.jpg)

2, Product version number

|      | Hardware | Software | Remark |
| ---- | -------- | -------- | ------ |
| 1    | V1.0     | V1.0     | latest |

3, product information

| **Main Chip-**ESP32-S3-WROOM-1-N16R8         |                                                              |
| -------------------------------------------- | ------------------------------------------------------------ |
| CPU/SoC                                      | high-performance Xtensa 32-bit LX7 dual-core processor, with up to 240MHz |
| System Memory                                | 512KB SRAM、8M PSRAM                                         |
| Memory                                       | 16M Flash，384KB ROM                                         |
| Development Language                         | MicroPython、C/C++                                           |
| Development Environment                      | ESP-IDF、Arduino IDE、PlatformIO、Micro Python、LVGL         |
| **Screen**                                   |                                                              |
| Size                                         | 5.0 inch                                                     |
| Diver IC                                     | ST7262                                                       |
| Resolution                                   | 800*480                                                      |
| Display Panel                                | IPS Panel                                                    |
| Touch Panel                                  | Capacitive Single Touch                                      |
| Viewing Angle                                | 178°                                                         |
| Brightness                                   | 400 cd/m²(Typ.)                                              |
| Color Depth                                  | 16-bit                                                       |
| **Wireless Communication - Onboard Antenna** |                                                              |
| WiFi                                         | Support 2.4GHz, 802.11a/b/g/n                                |
| Bluetooth                                    | Support Bluetooth 5.0 and BLE                                |
| Other                                        | **Zigbee、LoRa、nRF2401、Matter、Thread and Wi-Fi 6 (Optional)** |
| **Interface/Function**                       |                                                              |
| Interface                                    | USB port, UART, I2C, SD card slot, battery socket, speaker port, microphone, etc. |
| Function                                     | RTC clock, audio amplifier, volume control, battery charge management, USB to UART, etc. |
| **Button/LED Indicator**                     |                                                              |
| Reset Button                                 | Yes, press to reset device                                   |
| Boot Button                                  | Yes, press and hold the power button to burn the program     |
| PWR                                          | Power indicator                                              |
| CHG                                          | Lithium battery charging status, completion indication       |
| **Other**                                    |                                                              |
| Installation method                          | Back hanging, fixed hole                                     |
| Operating temperature                        | -20~70 °C                                                    |
| Storage temperature                          | -30~80 °C                                                    |
| Power Input                                  | 5V/2A, USB or UART terminal                                  |
| Active Area                                  | 108mm*65mm                                                   |
| Dimensions                                   | 136.4*84.7*15.4mm                                            |

4, Use the driver module

| Name   | dependency library                      |
| ------ | --------------------------------------- |
| LVGL   | lvgl/lvgl@8.3.3                         |
| ST7789 | Adafruit GFX Library<br/>version=1.11.0 |

5,Quick Start



6,Folder structure.

|--3D file： Contains 3D model files (.stp) for the hardware. These files can be used for visualization, enclosure design, or integration into CAD software.

|--Datasheet: Includes datasheets for components used in the project, providing detailed specifications, electrical characteristics, and pin configurations.

|--Eagle_SCH&PCB: Contains **Eagle CAD** schematic (`.sch`) and PCB layout (`.brd`) files. These are used for circuit design and PCB manufacturing.

|--example: Provides example code and projects to demonstrate how to use the hardware and libraries. These examples help users get started quickly.

|--factory_firmware: Stores pre-compiled factory firmware that can be directly flashed onto the device. This ensures the device runs the default functionality.

|--factory_sourcecode:  Contains the source code for the factory firmware, allowing users to modify and rebuild the firmware as needed.

|--libraries: Includes necessary libraries required for compiling and running the project. These libraries provide drivers and additional functionalities for the hardware.

7,Pin definition

#define HSPI_MISO  4
#define HSPI_MOSI  6
#define HSPI_SCLK  5
#define HSPI_SS    19

#define SD_MOSI 6
#define SD_MISO 4
#define SD_SCK 5
#define SD_CS 0 //The chip selector pin is not connected to IO

#define I2S_DOUT 4
#define I2S_BCLK 5
#define I2S_LRC 6

​      cfg.pin_d0 = GPIO_NUM_21;    // B0
​      cfg.pin_d1 = GPIO_NUM_47;    // B1
​      cfg.pin_d2 = GPIO_NUM_48;   // B2
​      cfg.pin_d3 = GPIO_NUM_45;    // B3
​      cfg.pin_d4 = GPIO_NUM_38;    // B4
​      cfg.pin_d5 = GPIO_NUM_9;    // G0
​      cfg.pin_d6 = GPIO_NUM_10;    // G1
​      cfg.pin_d7 = GPIO_NUM_11;    // G2
​      cfg.pin_d8 = GPIO_NUM_12;   // G3
​      cfg.pin_d9 = GPIO_NUM_13;   // G4
​      cfg.pin_d10 = GPIO_NUM_14;   // G5
​      cfg.pin_d11 = GPIO_NUM_7;  // R0
​      cfg.pin_d12 = GPIO_NUM_17;  // R1
​      cfg.pin_d13 = GPIO_NUM_18;  // R2
​      cfg.pin_d14 = GPIO_NUM_3;  // R3
​      cfg.pin_d15 = GPIO_NUM_46;  // R4

      cfg.pin_henable = GPIO_NUM_42;
      cfg.pin_vsync = GPIO_NUM_41;
      cfg.pin_hsync = GPIO_NUM_40;
      cfg.pin_pclk = GPIO_NUM_39;


​     cfg.pin_sda = GPIO_NUM_15;
​      cfg.pin_scl = GPIO_NUM_16;