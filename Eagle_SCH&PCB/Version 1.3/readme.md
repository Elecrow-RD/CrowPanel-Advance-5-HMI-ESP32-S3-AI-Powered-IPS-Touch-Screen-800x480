# 5.0-Inch ESP32-S3 Hardware Driver Guide

| Item | Details |
|---|---|
| Document Version | V1.0 |
| Applicable Hardware | ESP32 Display 5.0 inch V1.3 |
| Baseline Date | 2026-07-30 |
| Author | OpenAI Codex (compiled through cross-referencing project materials) |
| Purpose | Hardware maintenance, driver porting, production testing, and onboarding handoff |

## 1. Document Scope and Determination Criteria

This document is based on cross-verification of the following project materials:

1. `Version 1.3/ESP32 Display 5.0 inch V1.3.sch`: EAGLE 9.6.2 schematic source file, used as the primary hardware evidence for components and net connections.
2. `Version 1.3/ESP32 Display 5.0 inch V1.3.brd`: PCB netlist, used to verify schematic nets and component pad connections.
3. `Version 1.3/ESP32 Display 5.0 inch V1.3.pdf`: Single-page schematic, used for visual verification of functional sections, interface names, and component models.
4. Board-level examples under `Arduino/`: Used as the project’s verified driver baseline, primarily covering display/touch, TF card, I2S audio, DHT20, nRF24L01, SX1262 LoRaWAN, and UART/Zigbee examples.

Conflicts are resolved according to the following priority:

> Working code configurations in successfully tested examples > actual PCB nets > schematic nets/labels > comments and filenames.

However, obvious placeholder values, conflicting comments, or parameters that do not correspond to actual electrical connections in the code are not automatically treated as reliable hardware facts. Such items are identified separately as risks in this document.

### 1.1 Evidence Levels

| Level | Meaning |
|---|---|
| A | The schematic/PCB matches the project driver code, and the code performs actual initialization or read/write operations |
| B | The schematic/PCB has been confirmed, but the project has no corresponding ESP32 driver, or the function is managed only by the onboard control MCU |
| C | The project provides a driver example, but it targets an optional module connected through an interface rather than a permanently mounted motherboard component |
| D | Only the presence of hardware or reserved signals can be confirmed; key protocols or parameters are missing and must be tested before porting |

No build logs, test reports, or test result files were found in the repository. Therefore, “verified code” in this document refers specifically to the product example baseline provided by the repository and does not mean that all tests were rerun on physical hardware for this review.

## 2. Product Hardware Architecture

The main controller is an ESP32-S3-WROOM-1-N16R8. The display uses an 800 × 480 RGB565 parallel interface. The GT911, onboard STC8H1K28 control MCU, RTC, and external I2C interface share GPIO15/16. GPIO4/5/6 and GPIO19/20 are multiplexed among the TF card, I2S, digital microphone, and wireless expansion interface through two CH486F analog switches and DIP-switch selection.

The onboard STC8H1K28 is an important auxiliary controller. It connects to touch reset, LCD backlight enable/power, the buzzer, charging status indicators, and ESP32_EN, and provides a board-control command interface to the ESP32 at I2C address `0x30`. Do not mistake these functions for direct ESP32 GPIO control.

## 3. Peripheral Overview

| Category | Component/Function | Primary Interface | ESP32 Resources | Onboard/External | Evidence |
|---|---|---|---|---|---|
| MCU | ESP32-S3-WROOM-1-N16R8 | Main controller | All board-level resources | Onboard | A |
| Auxiliary MCU | STC8H1K28-36I-LQFP32 | I2C board control | SDA15, SCL16; recovery pin GPIO1 indirectly associated with TP_INT | Onboard | A/B |
| Display | 5.0-inch 800×480 IPS RGB display | RGB565 parallel interface | GPIO3/7/9-14/17/18/21/38/45-48, 39-42 | Onboard | A |
| Touch | GT911 capacitive touch | I2C | SDA15, SCL16, INT GPIO1; RST controlled by STC | Onboard | A |
| Storage | MicroSD/TF card | SPI | MISO4, SCK5, MOSI6; see risk notes for CS | Onboard | A/discrepancy |
| RTC | PCF8563-compatible device | I2C | SDA15, SCL16, 32.768 kHz crystal, CR1220 backup | Onboard | B |
| Audio Output | NS4168 amplifier + speaker interface | I2S | DOUT4, BCLK5, LRCLK6; amplifier enabled by STC command | Onboard | A |
| Audio Input | LMD3526B261 digital MEMS microphone | I2S/PDM-type digital interface | CLK19, DATA20 | Onboard | B |
| Buzzer | BEEP_5025 passive buzzer | STC GPIO/PWM | No direct ESP32 connection; STC `P2.7/BEEP` | Onboard | B |
| Temperature and Humidity | DHT20 | I2C | SDA15, SCL16 | External | C |
| 2.4 GHz | nRF24L01 | SPI + CE | MISO4, SCK5, MOSI6, CSN19, CE20 | External | C |
| LoRa | SX1262 | SPI + DIO/RESET/BUSY | MISO4, SCK5, MOSI6, NSS8, DIO1 20, RESET19, BUSY2 | External | C |
| Zigbee/Serial Module | Model not specified | UART | RX19, TX20, 115200 8N1 | External | C/D |
| USB-to-Serial | CH340K | USB 2.0/UART0 | ESP_TXD0/ESP_RXD0, automatic download circuit | Onboard | B |
| Native USB | USB-C D+/D- | USB 2.0 | ESP32-S3 native USB fixed pins (nets routed through resistors) | Onboard | B |
| Power Input | USB-C 5 V, external 5 V/VIN | Power | Not GPIO | Onboard | B |
| Battery Charging | TP4059 + CR1220 RTC backup battery interface | Analog power/status | Status monitored by STC | Onboard | B |
| Main-Power DC/DC | RY3420/HM3416H | Step-down conversion | Not GPIO | Onboard | B |
| LCD Backlight | MT9201 | Boost constant-current + STC control | Indirectly controlled by ESP32 through I2C `0x30` | Onboard | A |
| Power/Charging LEDs | Red/green LEDs | STC GPIO | No direct ESP32 connection | Onboard | B |
| Buttons | BOOT, RESET | GPIO0, ESP32_EN | Active low | Onboard | B |
| DIP Switch | K1/DSHP02TS-S | Multiplexing selection | SEL0, SEL1, hardware configuration | Onboard | A/B |
| Expansion Interfaces | UART, I2C, GPIO, I2S/wireless headers | Multiple protocols | GPIO2/4/5/6/8/15/16/19/20, etc. | Onboard connectors | B/C |

## 4. ESP32-S3 GPIO Summary

| GPIO | Actual Project Function | Direction/Electrical Mode | Multiplexing and Restrictions |
|---:|---|---|---|
| 0 | BOOT; CS parameter passed by the SD example | Boot strapping pin, button active low | Peripherals should not pull it low during reset; see 6.4 regarding the validity of SD_CS |
| 1 | `IO1_TP_INT`, touch-interrupt net; examples use a low pulse to recover board control/touch | Normally input; open-drain-equivalent operation during fault recovery: output low for 120 ms, then return to input | Must not be continuously driven high in push-pull mode; code does not enable GT911 interrupt polling |
| 2 | Wireless expansion CS; SX1262 BUSY | Digital I/O | LoRa example uses it for BUSY; associated with J11 expansion functions |
| 3 | LCD R6 net, corresponding to one RGB565 red data bit | High-speed push-pull output | Dedicated to LCD |
| 4 | SPI MISO / I2S DOUT / wireless MISO | Input or push-pull output, depending on mode | Multiplexed through CH486F; functions are mutually exclusive |
| 5 | SPI SCK / I2S BCLK / wireless SCK | Push-pull clock output | Multiplexed through CH486F; functions are mutually exclusive |
| 6 | SPI MOSI / I2S LRCLK / wireless MOSI | Push-pull output | Multiplexed through CH486F; functions are mutually exclusive |
| 7 | LCD R3 | High-speed push-pull output | Dedicated to LCD |
| 8 | Wireless BUSY/expansion; SX1262 NSS | Digital I/O | LoRa code uses it as a push-pull chip select; verify DIP-switch and connector settings |
| 9-14 | LCD G2-G7 | High-speed push-pull output | Dedicated to LCD |
| 15 | I2C SDA | Bidirectional open-drain, with onboard 4.7 kΩ pull-up to 3.3 V | Shared by GT911, STC, RTC, and external I2C |
| 16 | I2C SCL | Open-drain clock, with onboard 4.7 kΩ pull-up to 3.3 V | Same as above |
| 17, 18 | LCD R4, R5 | High-speed push-pull output | Dedicated to LCD |
| 19 | nRF24 CSN / SX1262 RESET / UART RX / MIC CLK / LED example | Input or output, depending on use case | Multiplexed through CH486F; multiple examples cannot run simultaneously |
| 20 | nRF24 CE / SX1262 DIO1 / UART TX / MIC DATA | Input or output, depending on use case | Multiplexed through CH486F; multiple examples cannot run simultaneously |
| 21 | LCD B3 | High-speed push-pull output | Dedicated to LCD |
| 38, 45, 47, 48 | LCD B7, B6, B4, B5 | High-speed push-pull output | Dedicated to LCD; GPIO45/46 are boot-sensitive pins and must not have additional pull-ups or pull-downs |
| 39 | LCD PCLK/DCLK | High-speed push-pull clock output, 21 MHz | Dedicated to LCD |
| 40 | LCD HSYNC | High-speed push-pull output, configured active low | Dedicated to LCD |
| 41 | LCD VSYNC | High-speed push-pull output, configured active low | Dedicated to LCD |
| 42 | LCD DE | High-speed push-pull output | Dedicated to LCD |
| 46 | LCD R7 | High-speed push-pull output | Input-restricted/boot-sensitive pin, currently used only as an LCD output |

## 5. Shared Buses and Resource Conflicts

### 5.1 I2C0: GPIO15/GPIO16

- SDA: GPIO15, schematic net `IO15_SDA_P2_4`.
- SCL: GPIO16, schematic net `IO16_SCL_P2_5`.
- Electrical mode: 3.3 V open-drain bus, each line with an onboard 4.7 kΩ pull-up.
- Known devices: GT911 `0x5D` (the code also identifies `0x14` as another possible address), board-control STC `0x30`, PCF8563-compatible RTC (commonly `0x51`, not verified by project code), and external DHT20 (the library default address is typically `0x38`, but the project does not explicitly hard-code it).
- Code initialization: `Wire.begin(15, 16)`; the LovyanGFX GT911 configuration uses I2C0 at 400 kHz.
- Sharing rules: All devices must use the same pin pair; the application layer must not repeatedly call `Wire.end()`. If any device supports only 100 kHz, the entire bus should be reduced to 100 kHz.
- Startup threshold: The example continuously probes `0x30` and `0x5D`; if either fails to respond, the main application does not start.

### 5.2 CH486F Multiplexing Matrix

The two CH486F devices, U11 and U8, route shared GPIOs to the following functional nets:

| Shared GPIO | Selectable Nets |
|---|---|
| GPIO4 | `IO4_SD_MISO`, `IO4_I2S_SDIN`, `IO4_W_MISO` |
| GPIO5 | `IO5_SD_SCK`, `IO5_I2S_BCLK`, `IO5_W_CLK` |
| GPIO6 | `IO6_SD_MOSI`, `IO6_I2S_LRCLK`, `IO6_W_MOSI` |
| GPIO19 | `IO19_MIC_CLK`, `IO19_RX_IRQ` |
| GPIO20 | `IO20_MIC_SD`, `IO20_TX_CE` |

The K1 DIP switch generates `SEL0/SEL1`, which are supplied to both CH486F devices. The project code does not control SEL0/SEL1 in software, so the correct setting must be selected according to the PCB silkscreen/product documentation before startup. Power must be disconnected before changing the DIP-switch setting. An incorrect setting may result in no SPI response, no I2S audio, no microphone data, or nonfunctional external wireless interrupts.

### 5.3 Mutually Exclusive Usage Matrix

| Function Combination | Conclusion | Reason |
|---|---|---|
| LCD + touch | Can be used simultaneously | RGB and I2C use different resources |
| Touch + RTC + DHT20 | Can theoretically be used simultaneously | They share I2C, but their addresses do not conflict; bus frequency must be managed |
| TF + I2S speaker | Cannot be used directly at the same time | GPIO4/5/6 and CH486F channels conflict |
| TF + nRF24/SX1262 | Cannot be used directly at the same time | They share SPI data/clock lines and have conflicting hardware-channel selections |
| I2S speaker + digital microphone | Requires dedicated verification of the DIP-switch and clock configuration | They use 4/5/6 and 19/20 respectively, but both are controlled by the same SEL group |
| nRF24 + Zigbee UART | Cannot be used directly at the same time | GPIO19/20 are occupied by CSN/CE and RX/TX respectively |
| SX1262 + Zigbee UART | Cannot be used directly at the same time | GPIO19/20 are occupied by RESET/DIO1 and RX/TX respectively |

## 6. Detailed Onboard Peripheral Drivers

### 6.1 ESP32-S3 Main Controller

**Device**: Schematic reference U5, ESP32-S3-WROOM-1; the product functional block is labeled N16R8, indicating the version with 16 MB Flash and 8 MB PSRAM.

**Software Layer**: Arduino-ESP32 Core; the low-level display uses ESP-IDF RGB/I2C driver wrappers, and the examples do not perform direct register operations. The detailed display configuration enables PSRAM: `cfg.use_psram = 1`.

**Startup/Download**:

- BOOT: GPIO0, button K3, active-low download boot.
- RESET: ESP32_EN, button K4, active-low reset.
- The CH340K + UMH3NTN automatic download circuit connects UART0, DTR, and RTS.
- The example debug serial port normally uses `Serial.begin(115200)`; the basic serial example in lesson-01 is an exception and uses 9600.

**Note**: When porting to PlatformIO/ESP-IDF, an ESP32-S3 N16R8 partition and memory configuration with PSRAM must be selected. Otherwise, allocation of full frames, large images, or LVGL buffers may fail.

### 6.2 RGB LCD

**Interface**: 800 × 480, RGB565, 16-bit parallel, DE/HSYNC/VSYNC/PCLK.

| LovyanGFX Signal | GPIO | Schematic LCD Net |
|---|---:|---|
| D0..D4 (5 blue bits) | 21, 47, 48, 45, 38 | B3..B7 |
| D5..D10 (6 green bits) | 9, 10, 11, 12, 13, 14 | G2..G7 |
| D11..D15 (5 red bits) | 7, 17, 18, 3, 46 | R3..R7 |
| DE | 42 | DE |
| VSYNC | 41 | VSYNC |
| HSYNC | 40 | HSYNC |
| PCLK | 39 | DCLK |

The code comments label the data bits above as B0..B4, G0..G5, and R0..R4, while the schematic labels them by their positions in an 8-bit color scale as B3..B7, G2..G7, and R3..R7. These are simply different naming conventions for RGB565 bit ordering. The actual GPIO connections are consistent and do not represent a hardware conflict.

**Key Timing**:

```cpp
cfg.freq_write = 21000000;
cfg.hsync_polarity = 0;
cfg.hsync_front_porch = 8;
cfg.hsync_pulse_width = 4;
cfg.hsync_back_porch = 8;
cfg.vsync_polarity = 0;
cfg.vsync_front_porch = 8;
cfg.vsync_pulse_width = 4;
cfg.vsync_back_porch = 8;
cfg.pclk_idle_high = 1;
```

**Initialization Sequence**: First ensure that the board-control I2C/touch devices respond and configure the backlight, then call `gfx.init()` and `gfx.initDMA()`, and finally clear the screen or create the LVGL display object. The LVGL example uses double buffering, 40 lines per buffer, RGB565, and `LV_DISPLAY_RENDER_MODE_PARTIAL`; the buffers are allocated from internal 8-bit RAM.

**Software Dependencies**: LovyanGFX v1 (`Bus_RGB`, `Panel_RGB`), with optional LVGL.

### 6.3 GT911 Capacitive Touch

**Connections**: SDA GPIO15, SCL GPIO16, INT net GPIO1, and RST controlled by STC `P1.7/TP_RST`. The touch FPC also includes 3.3 V and GND.

**Actual Code Configuration**:

```cpp
cfg.i2c_port = I2C_NUM_0;
cfg.pin_sda = GPIO_NUM_15;
cfg.pin_scl = GPIO_NUM_16;
cfg.pin_int = -1;
cfg.pin_rst = -1;
cfg.freq = 400000;
cfg.i2c_addr = 0x5D;
```

The code uses polling mode. Therefore, although the schematic connects TP_INT to GPIO1, LovyanGFX still sets `pin_int=-1`. Reset is not driven directly by the ESP32, so `pin_rst=-1`.

**Fault-Recovery Sequence**: If probing `0x30` or `0x5D` fails, the example first sends command 250 to `0x30`, then configures GPIO1 as a low output for 120 ms, returns it to input mode, and waits 100 ms. This operation is equivalent to pulling only the shared/interrupt net low; do not change it to a continuously driven push-pull high level.

**Coordinates**: X 0..800, Y 0..480, `offset_rotation=0`; LovyanGFX maps the coordinates after screen rotation.

### 6.4 MicroSD/TF Card

**Connections**: MISO GPIO4, SCK GPIO5, MOSI GPIO6, with 3.3 V power. The lines are switched through U11 CH486F. DA1/DA2 have pull-up/reserved nets.

**Actual Code**: Arduino `SPIClass` + `SD`/`FS`; the 5.0-inch example selects FSPI or HSPI depending on the platform. The stable example specifies 40 MHz:

```cpp
#define SD_MOSI 6
#define SD_MISO 4
#define SD_SCK  5
#define SD_CS   0
#define SD_SPI_FREQ 40000000
SD_SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
SD.begin(SD_CS, SD_SPI, SD_SPI_FREQ);
```

**Key Discrepancy/Risk**: The schematic labels the card-socket CS net as `SDCS`, connecting J5, R29, and R33, but does not show a direct connection to ESP32 GPIO0. The code also explicitly comments that the “chip selector pin is not connected to IO,” yet still passes `0` to `SD.begin()`. This indicates that the hardware may operate without an independent CS or with a fixed selected state through the DIP switch/analog switch. `GPIO0` is more likely an API placeholder than a reliable electrical connection to the card-socket CS. Maintenance personnel must not use this as justification for wiring the signal to GPIO0. If the low-level driver is rewritten or the bus is shared with other SPI devices, use a multimeter/oscilloscope to confirm the actual card-socket CS level and its relationship to U11 selection.

A copy in the repository root previously used 80 MHz, while the lesson-04 version explicitly changed it to 40 MHz. Following the principles of maintainability, robustness, and preference for later code revisions, this document recommends 40 MHz. For long wiring, adapters, or low-quality TF cards, reduce the frequency to 20 MHz first when troubleshooting.

### 6.5 Board-Control STC8H1K28 and Backlight

**Device and Connections**: U13, STC8H1K28-36I-LQFP32; I2C connects to GPIO15/16 and is visible to the ESP32 at address `0x30`. It also connects to TP_RST, LCD_BK_PWR, LED_BK_EN, BEEP, charging status, ESP32_EN, independent UART TXD/RXD, and other signals.

**Known Commands**:

| Command Byte | Meaning in Project Comments |
|---:|---|
| 0..245 | Backlight brightness: 0 is brightest and 245 is off; larger values are dimmer |
| 248 | Enable the speaker/amplifier path |
| 250 | Activate/recover the touchscreen control path |

```cpp
Wire.beginTransmission(0x30);
Wire.write(command);
uint8_t error = Wire.endTransmission();
```

**Limitation**: The repository does not contain STC firmware source code or a complete protocol table, so commands not listed above cannot be inferred. The brightness polarity is opposite to typical PWM intuition. When porting, preserve the protocol semantics of “0 is brightest, 245 is off.”

### 6.6 PCF8563 RTC

**Device**: U4, schematic value `PCF8563MDTR(XBLW)`, component library name BM8563EMA; external Y1 32.768 kHz crystal and CR1220 backup battery.

**Connections**: SDA GPIO15, SCL GPIO16, 3.3 V main power; the backup domain is powered through a diode/battery.

**Driver Status**: The project does not include RTC example code. The PCF8563 family commonly uses the 7-bit address `0x51`, but this is only general device information and must be confirmed by an I2C scan on production boards. The Arduino `Rtc_Pcf8563`/`PCF8563` library or the ESP-IDF I2C master driver is recommended. On first use, check the VL (low-voltage) flag, write the date and time, and then read them back to verify timekeeping.

**Risk**: The CR1220 is a non-rechargeable coin-cell battery and must not be connected to a lithium-battery charging circuit. After replacing the battery, reset the time and verify that the crystal oscillator starts correctly.

### 6.7 I2S Audio Output and NS4168

**Data Connections**: GPIO4 is the audio data output, GPIO5 is BCLK, and GPIO6 is LRCLK; U11 switches them to the I2S nets. The NS4168 (U10) drives the right-channel speaker connector J14 and is managed by the STC `NS_CTRL`/power-control net.

**Initialization**:

```cpp
sendI2CCommand(248);          // Enable the speaker path first
audio.setPinout(5, 6, 4);     // BCLK, LRCLK, DOUT
audio.setVolume(20);          // Library range: 0..21
```

The example uses the `ESP32-audioI2S` library to decode network MP3 audio and depends on Arduino WiFi. The sample rate and bit width are dynamically configured by the audio stream and library. `audio.loop()` must be called frequently, or playback will stutter.

**Risk**: Volume 20/21 is near full scale. When first connecting a speaker with a different impedance, start at a low volume and increase it gradually. The NS4168 provides a power-amplified output and must not be connected to headphones or directly to a line-level input. The TF card and speaker I2S share GPIO4/5/6 and cannot be used simultaneously merely by selecting different SPI/I2S peripheral controllers.### 6.
8 Digital MEMS Microphone

**Device**: MIC1, LMD3526B261-OFA01; 3.3 V filtered through FB1.

**Connections**: GPIO19 → `IO19_MIC_CLK`, GPIO20 ← `IO20_MIC_SD`; signals are routed through U8 CH486F. The schematic contains no analog ADC audio path, so this device should be treated as a digital clock/data microphone.

**Driver Status**: The repository contains no recording example. The code does not confirm whether the interface is standard I2S, left-justified, or PDM, nor does it confirm the sampling edge. Before porting, consult the component datasheet and use a logic analyzer to verify CLK and DATA. Do not directly reuse the GPIO4/5/6 configuration from the audio output example.

### 6.9 Passive Buzzer

**Device**: B2 (BEEP_5025), driven by Q4, with the control signal connected to STC `P2.7/BEEP` and a D5 protection/clamping network.

**Driver Status**: There is no direct GPIO connection to the ESP32, and the repository does not provide a public buzzer I2C command. If the product requires audible alerts, the STC protocol or firmware should be extended. Do not directly configure any ESP32 GPIO as BEEP.

### 6.10 USB, UART, and Firmware Download

**USB-C**: J1 provides 5 V input and USB D+/D-. U1 CH340K provides USB-to-UART0 conversion, while U9 and Q1/Q2 form the automatic download/level-control path. ESP_TXD0/ESP_RXD0 are also routed to J2, where the external interface uses 3.3 V UART levels.

**Initialization**: Application debugging typically uses `Serial.begin(115200)`. If flashing fails, hold BOOT, press RESET, and then release BOOT.

**Risks**: The UART expansion port does not use RS-232/RS-485 voltage levels; do not connect it directly to a ±12 V serial interface. Bus contention may occur if an external device and USB drive RX/TX simultaneously.

### 6.11 Power, Charging, and Indicator LEDs

| Function | Device/Net | Control Relationship |
|---|---|---|
| USB 5 V input | J1 `+VBUS1` | Enters VIN through the Schottky/power path |
| External 5 V | J10 `+5V_IN` | Connected to the system VIN power path; verify polarity before connection |
| Li-ion battery connector | J3 `VBAT` | TP4059 charging and power path; the cell specifications must match the design |
| Charge management | U2 TP4059 | DONE/CHRG status is sent to the STC and is not read directly by the ESP32 |
| 3.3 V step-down regulator | U3 RY3420/HM3416H | System 3.3 V rail; not software-controlled |
| LCD backlight boost converter | U6 MT9201 | STC controls `LED_BK_EN` and `LCD_BK_PWR` |
| Indicator LEDs | D1 dual-color, D14 red | Partially managed by the STC; the repository provides no ESP32 API |

Power management software should use the known protocol of the board-control MCU and must not bypass the STC to operate its nets directly. When connecting external sensors, use `3V3_OUT` whenever possible and verify the total load. The schematic does not provide a system-level guarantee for the expansion port’s continuous output current.

## 7. Detailed Drivers for External/Optional Modules

### 7.1 DHT20 Temperature and Humidity Sensor

**Interface**: External I2C using shared GPIO15/16; the library is `Crowbits_DHT20`.

**Initialization and Sampling**: After `Wire.begin(15,16)`, call `dht20.begin()`. The example reads `getTemperature()` and `getHumidity()` every 1000 ms and converts the values to integers for display. The address is not explicitly specified in the application code.

**Notes**: The sensor shares the bus with the board controller and touchscreen. If initialization fails, it must not block touchscreen and backlight operation. Production products should add value-range validation, CRC/error status handling, and disconnection detection.

### 7.2 nRF24L01

**Pins**: MISO4, MOSI6, SCLK5, CSN19, CE20. IRQ is not used in the example.

**Configuration**: RF24 library, address pipe `"00001"`, maximum PA level, 250 kbps, and RF channel 50. The receiver uses pipe 0 and calls `startListening()`; the transmitter calls `openWritingPipe()` and `stopListening()`. The payload buffer is 32 bytes.

```cpp
RF24 radio(20, 19);
hspi->begin(5, 4, 6, 19);
radio.setPALevel(RF24_PA_MAX);
radio.setDataRate(RF24_250KBPS);
radio.setChannel(50);
```

**Risks**: The code uses GPIO19 both as the SS argument to SPI begin and as the RF24 CSN pin, which is semantically consistent. At PA_MAX, the module draws substantial transient current during transmission, so local decoupling is required near the external module. Standard nRF24L01 modules must use only 3.3 V power and 3.3 V logic.

### 7.3 SX1262 LoRa/LoRaWAN

**Pins**: SCK5, MISO4, MOSI6, NSS8, DIO1 20, NRESET19, BUSY2.

**Software Layer**: RadioLib; `SX1262 radio = new Module(8, 20, 19, 2, SPI)`. After initialization, the code sets overcurrent protection to 140 mA and the TCXO voltage to 3.3 V. The example supports EU868 and US915. By default, it creates the node for EU868, subBand 1, and uses AT commands to switch between ABP/OTAA and configure ADR, DR, transmit power, and duty cycle.

**Risks**: A code comment refers to “HMI Advance v1.0,” while the hardware covered by this document is V1.3. The pins generally correspond to the V1.3 expansion nets, but the SX1262 is an external module, so its header pinout must still be verified. The antenna, regional frequency band, transmit power, duty cycle, and dwell time must comply with regulations in the deployment region. `setTCXO(3.3)` applies only to modules whose TCXO supply voltage is actually 3.3 V.

### 7.4 Zigbee/UART Module

**Pins and Format**: ESP32 GPIO19 is `Serial1 RX`, and GPIO20 is `Serial1 TX`; 115200 baud, 8 data bits, no parity, and 1 stop bit.

```cpp
Serial1.begin(115200, SERIAL_8N1, 19, 20);
```

The example only receives data terminated by a newline and forwards it to the debug serial port. It does not include a specific Zigbee protocol, module initialization, flow control, or transmission logic. The module model, voltage, and AT protocol must be added according to the actual BOM/module specifications. Therefore, this example must not be treated as a complete Zigbee driver.

### 7.5 GPIO LED/Actuator Example

lesson-05 configures GPIO19 as a push-pull output and controls an LED using a temperature threshold of 30 °C. GPIO19 is a multiplexed expansion resource, not a dedicated onboard user LED in the schematic. Relays, motors, or high-power lights require an additional transistor/MOSFET, flyback diode, and independent power supply; they must not be driven directly by a GPIO.

## 8. Schematic and Code Discrepancy List

| No. | Discrepancy | Adopted Conclusion | Possible Cause/Resolution |
|---|---|---|---|
| D-01 | LCD code comments specify B0..B4/G0..G5/R0..R4, while the schematic specifies B3..B7/G2..G7/R3..R7 | Use the GPIO mapping from the code; electrical net consistency has been confirmed | RGB565 internal bit numbering differs from the panel’s 8-bit color-level naming |
| D-02 | GT911 INT/RST are both -1 in the code; in the schematic, INT is connected to GPIO1 and RST to the STC | Use the polling and STC-reset architecture implemented in the code | Touch interrupt is disabled, and reset is proxied by the board controller |
| D-03 | TF code uses `SD_CS=0`; the schematic does not show `SDCS` directly connected to GPIO0, and a code comment states that CS is not connected to an IO | Retain the proven API parameter, but do not treat GPIO0 as the confirmed electrical CS connection | Fixed selection, analog-switch architecture, or an API placeholder; physical verification is required |
| D-04 | The root-level TF example uses 80 MHz, while lesson-04 uses 40 MHz | Recommend 40 MHz | The later example is more conservative, improving card compatibility and signal integrity |
| D-05 | GPIO1 is not used as INT in LovyanGFX, but is driven low during failure recovery | Retain the 120 ms low pulse, then restore the pin to input mode | Board-controller/GT911 recovery timing; the pin must not remain configured as an output |
| D-06 | The SX1262 comment refers to HMI Advance v1.0 | Use it only as an external-module example and verify it before deployment on V1.3 | The example was reused across hardware versions without updating the comment |
| D-07 | The nRF24, LoRa, Zigbee, microphone, and LED examples all reuse GPIO19/20 | Treat them as mutually exclusive functions | CH486F/DIP-switch hardware multiplexing is part of the product design |
| D-08 | The STC controls multiple board-level functions, but the repository does not include the STC firmware or a complete command table | Use only the three known command categories: 0..245, 248, and 250 | The auxiliary MCU firmware was not released with the main-controller examples |

## 9. Recommended Unified Initialization Sequence

1. Configure serial logging and print the hardware/firmware versions and multiplexing-position requirements.
2. Initialize I2C on GPIO15/16 and wait at least 50 ms.
3. Probe the board controller at `0x30` and the GT911 at `0x5D`. If necessary, perform the recovery sequence of command 250 + drive GPIO1 low for 120 ms + release it to input mode for 100 ms, and limit the maximum number of retries.
4. Send the appropriate backlight value to the board controller according to the product scenario; send 248 if the speaker is required.
5. Initialize the RGB LCD and DMA, then initialize LVGL/application buffers.
6. Initialize touch input; use LovyanGFX polling by default.
7. Initialize only one multiplexed group according to the K1 switch function: TF, I2S, microphone, or wireless expansion.
8. Initialize the RTC/external I2C sensors and record the address-scan results.
9. Perform an observable self-test for each peripheral and configure timeouts to prevent any disconnected device from causing an infinite loop.

It is recommended to centralize board-level definitions in a single `board_v1_3.h` and select mutually exclusive functions through build macros. Do not redefine GPIOs independently in each example. Example:

```cpp
static constexpr int PIN_I2C_SDA = 15;
static constexpr int PIN_I2C_SCL = 16;
static constexpr int PIN_MUX_D0  = 4;
static constexpr int PIN_MUX_CLK = 5;
static constexpr int PIN_MUX_D1  = 6;
```

## 10. Risks and Precautions

### 10.1 High Priority

1. **Multiplexing Conflicts**: GPIO4/5/6/19/20 are not freely available concurrent resources. The firmware configuration must match the actual K1 switch position, and TF, I2S, wireless, and microphone implementations must be designed according to a mutual-exclusion matrix.
2. **Uncertain SD CS**: Do not treat `SD_CS=0` in the example as a schematic-confirmed connection. GPIO0 is also a boot strapping pin, and an incorrect external connection may prevent startup or firmware download.
3. **I2C Startup Infinite Loops**: Existing examples wait indefinitely for successful communication with `0x30` and `0x5D`. Production firmware should limit retries and enter a degraded mode or fault page; otherwise, a damaged touchscreen may prevent the entire device from starting.
4. **GPIO1 Drive Mode**: It may only be pulled low briefly and must then be restored to a high-impedance state to avoid contention with touchscreen/board-controller outputs.
5. **Voltage Levels**: The ESP32 and expansion ports use 3.3 V logic and are not 5 V tolerant; the UART does not use RS-232 voltage levels.

### 10.2 Medium Priority

1. **Inverted Backlight Protocol**: Command 0 is the brightest setting, while 245 turns the backlight off. Converting values as if they were ordinary PWM percentages will produce the opposite effect.
2. **Speaker Power**: Start at a low volume. Verify speaker impedance and power rating; it must not be used as a headphone output.
3. **Wireless Power Supply**: nRF24 PA_MAX and SX1262 transmission transients may cause the 3.3 V rail to sag. Provide sufficient decoupling near the expansion module and verify the available power margin.
4. **Boot-Sensitive GPIOs**: External modules must not apply incorrect levels to GPIO0, 45, or 46 during reset.
5. **RTC Battery**: The CR1220 is non-rechargeable. RTC time is unreliable until the low-voltage flag has been cleared.
6. **PSRAM Dependency**: Display, large-image, and LVGL projects must enable the correct N16R8 PSRAM configuration and check buffer-allocation return values.

### 10.3 Documentation Gaps

1. The STC8H1K28 firmware source code, protocol version, and complete I2C command table are missing.
2. A product-level DIP-switch truth table/silkscreen position description for the CH486F is missing.
3. Separate datasheets and supplier part numbers for the 5.0-inch display and GT911 module are missing.
4. Digital microphone timing/format information and a recording example are missing.
5. ESP32 application-layer drivers for the RTC, buzzer, and charging status are missing.
6. Locked build versions, physical test reports, and production test records corresponding to the repository examples are missing.

These gaps should be addressed in the next hardware documentation package. The hardware version, STC firmware version, DIP-switch position, and Arduino Core/library versions should all be included in the release baseline.

## 11. Porting and Maintenance Checklist

- [ ] Confirm that the target board is the 5.0 inch V1.3, not the 4.3/7.0-inch model or an older HMI revision.
- [ ] Confirm that the ESP32-S3 Flash/PSRAM configuration matches N16R8.
- [ ] Verify each RGB 16-bit data, DE, VSYNC, HSYNC, and PCLK pin against this table.
- [ ] Start with a 21 MHz PCLK and 8/4/8 porch parameters.
- [ ] Use GPIO15/16 for I2C, confirm `0x30` and `0x5D`, and scan for the RTC/sensors.
- [ ] Preserve GPIO1’s default high-impedance state and controlled recovery timing.
- [ ] Confirm that the K1 switch position matches the compiled function.
- [ ] Keep the initial TF frequency at or below 40 MHz and physically verify CS behavior.
- [ ] Send board-controller command 248 before I2S output and validate volume from low to high.
- [ ] Verify the wireless module’s 3.3 V requirements, current consumption, antenna, and regional regulatory compliance.
- [ ] Ensure that all initialization loops have timeouts, error codes, and degraded-mode paths.
- [ ] Record the hardware version, STC protocol version, library versions, and test results.

## 12. Key Source Code Index

| Content | Project Path |
|---|---|
| RGB/GT911 board-level configuration | `Arduino/SD_CrowPanel_ESP32_Advance_HMI_4_3_5_0_7_0/LovyanGFX_Driver.h` |
| Stable TF card example | `Arduino/lesson-04/SD_CrowPanel_ESP32_Advance_HMI_4_3_5_0_7_0/SD_CrowPanel_ESP32_Advance_HMI_4_3_5_0_7_0.ino` |
| LVGL display and touch | `Arduino/lesson-03/BigInch_LVGL/BigInch_LVGL.ino` |
| DHT20/output GPIO | `Arduino/lesson-05/Port_CrowPanel_ESP32_Advance_HMI_4_3_5_0_7_0/Port_CrowPanel_ESP32_Advance_HMI_4_3_5_0_7_0.ino` |
| I2S network audio | `Arduino/lesson-02/OnlineAudio_large/OnlineAudio_large.ino` |
| nRF24 transmit/receive | `Arduino/lesson-06/READ/...`, `Arduino/lesson-06/WRITE/...` |
| SX1262/LoRaWAN | `Arduino/lesson-07/code/sendATcommands_7.0/config.h`, `sendATcommands_7.0.ino` |
| UART/Zigbee | `Arduino/lesson-09/zigbee_7.0/zigbee_7.0.ino` |
| Schematic/PCB | `Version 1.3/ESP32 Display 5.0 inch V1.3.sch`, `.brd`, `.pdf` |

---

**Maintenance Policy**: If subsequent physical validation disproves any item in this document, update the unified board-level definitions and this discrepancy table first, and include oscilloscope/logic-analyzer records, the board number, and the firmware commit ID. Do not modify only an individual lesson example without updating the product baseline.