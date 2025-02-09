# DHT22 FreeRTOS driver for ESP-IDF
[![Apache-2.0 license](https://img.shields.io/badge/License-Apache_2.0-green.svg?style=flat-square)](https://www.apache.org/licenses/LICENSE-2.0)
[![ESP32](https://img.shields.io/badge/ESP-32-green.svg?style=flat-square)](https://www.espressif.com/en/products/socs/esp32)

This is a FreeRTOS DHT22 temperature & humidity sensor driver for ESP IDF.

It works with interruptions instead of active waiting and relays on 
the ["ESP Timer"](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/esp_timer.html) API for microseconds time measurement.

## Environment
The DHT22 device is powered with 3.3v and the DATA pin is wired to ESP32's GPIO 25 in the code delivered here.

GPIO is configured as open-drain for input & output with a 1k Ohm pull-up to 3.3v.

The code has been generated using **ESP-IDF v5.4-dev-2194-gd7ca8b94c8** for Linux running on a Raspberry Pi.

The code was developed and tested on a ESP-WROOM-32.

## Description
The driver provides 2 functions.

**void DHTinit(gpio_num_t gpio_num)**
is the initialization function that needs to be called once. It takes the GPIO number of choice as parameter and configures the GPIO as open drain for input & output (GPIO_MODE_INPUT_OUTPUT_OD), enables the pin for interruptions on falling edge (GPIO_INTR_NEGEDGE), enables the interruption handler function (ISR), and starts the ESP timer.

**int DHTget(uint16_t\* temperature, uint16_t\* humidity)**
is meant to be called anytime to get DHT's measurements. The function returns after approximately 20ms which is the time it takes the DHT to respond and transmit the data. The function is preemptive, that is, it is not blocking other tasks during its execution.

When DHTget is called the ESP drives the GPIO as output to generate the start signal (it pulls the GPIO low and then up back).

```
          500us         20-40us   
 ___               _______________ - - - -
    \             /    
     \___________/                 
```
 
After the start signal the ESP32 releases the wire and it is the turn of the DHT22 to drive it to send back the response. 

The DHT generates 41 down+up cycles.

The first down+up cycle means "ready to output" and is followed by 40 pulses meaning data 0 or 1.

Data 0 or 1 is discriminated by the duration of the up pulse. Up time for data 0 is 26-28us and up time for data 1 is around 70us.

```
data 0 bit
                         26-28us   
                   ____________
    \             /    
     \___________/                 

data 1 bit
                              70us   
                   ______________________ 
    \             /    
     \___________/    
```
The driver takes the duration of the down+up, that is, the time between two consecutive falling edges, to decide between 0 and 1.

The interrupt routine (ISR) is limited to counting cycles and recording the time between falling edges.

The recorded down+up cycles are converted into bits. The post processing outcome is 5 bytes. The first two bytes are for the humidity and the second two is the temperature. The fifth byte is the checksum.



## Build
Follow the regular process to generate and flash the code as described in the ESP IDF instructions ["Configure Your Projec"](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html#configure-your-project)

Run ESP-IDF script to make the tools usable from the command line and set the necessary environment variables.
```console
. $HOME/esp/esp-idf/export.sh
```
Go to your project directory, e.g.
```console
cd ~/esp/DHT22
```
Select your target
```console
idf.py set-target esp32
```
You can skip the menuconfig for the are no project specific variables to set up
```console
idf.py menuconfig
```
Compile and generate the code
```console
idf.py build
```
Execute (choose the right port for your enviroment)
```console
idf.py -p /dev/ttyUSB1 flash monitor
```
