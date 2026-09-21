# KAOS
## KAOS is no longer in developtment. If you want to emulate Skylanders, please continue using KAOS for now.
## In the future please use [PTPE](https://github.com/NicoAICP/PTPE), since this will replace KAOS.
Pi Pico Skylander Portal Emulator

Supports following types of Skylander formats:
 + .bin
 + .dmp
 + .dump
 + .sky (created via RPCS3)

## Usage
To Emulate the portal, just plug in the Microcontroller into a USB Port of a Console. As soon as the RP2040 is ready it will be detected as a Portal.

What does which button do?

![Board Diagram with Numbers](https://media.discordapp.net/attachments/943504899956703323/989223408011198554/unknown.png?width=496&height=671)

 + Button 1: Changes the selected slot to the Left (Player 2 -> Player 1)
 + Button 2: Changes the selected slot to the Right (Player 1 -> Player 2)
 + Button 3: Sends the Sense Command to the Game (Tells the game how many Skylanders are on the Portal. Only use if the game doesnt detect the skylander)
 + Button 4: Change File Selection to the Left (If File 2 would be Spyro.bin and File 1 Kaos.bin, it would switch Spyro for Kaos)
 + Button 5: Change File Selection to the Right (If File 2 would be Spyro.bin and File 1 Kaos.bin, it would switch Kaos for Spyro)
 + Button 6: Load/Unload Skylander from selected slot

## Known Issues:

 + Slot selection does not work (Currently, if you load a skylander, after one is already loaded, it gets assigned the next Slot)
 + Unloading Skylanders does not work (To switch your skylander, you need to unplug the RP2040)

## Creating the Pico Board
Required Things for the Breadboard (atleast how I built it):
 + RP2040 Based Microcontroller
 + 6 Buttons
 + SPI SD Card Reader Module
 + LCD1602 With I2C Backpack
 + 2 Breadboards with 830 Contacts each
 + Jumpercables

Connect all the things like the Connection Diagram is telling you

Connection Diagram

![Connection Diagram](https://raw.githubusercontent.com/NicoAICP/KAOS/main/Images/unknown.png)
 
it should look something like this:

![Nicos Breadboard](https://media.discordapp.net/attachments/943501791612522527/989222203579052042/20220622_193502.jpg?width=895&height=671)

## Installation
Copy the KAOS.uf2 File onto the Microcontroller, while it is in flash mode.

## Building
To build you need the pi pico sdk

You need to copy the tusb_config.h into this location <pico-sdk_location>\lib\tinyusb\src\tusb_config.h

## Bill Update

Pin assigments:
SD Card: (SPI1)
  14. Sclk. GPIO 10
  15. MOSI (Tx).  GPio 11
  16. MISO (Rx). GPIO 12.  Pull up to 3.3V. ????? TBD
  17. Csn.  GPIO 13. Pull up to 3.3V
  5V VCC
LCD: (I2C0)
  6: SDA (GPIO 4)
  7: SCL (GPIO 5)
  5V VCC
UART (UART0)
  1: Tx GPIO 0
  2: Rx GPIO 1
Neopixel: PIO
  9:  GPIO 6
  5V VCC
Buttons: PIO
  19: GPIO 14
  20: GPIO 15 Slot up
  24: GPIO 18 Start
  25: GPIO 19 Figure up
  26: GPIO 20
  27: GPIO 21 Select