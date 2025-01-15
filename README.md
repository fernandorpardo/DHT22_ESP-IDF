# DHT22 FreeRTOS driver for ESP-IDF
This is a FreeRTOS C language driver of the DHT22 temperature & humidity sensor for the ESP32.

It works with interruption instead of active waiting and relays on 
the ["ESP Timer"](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/esp_timer.html) API for microseconds time measurement.

## Enviroment
The DHT22 device is powered with 3.3v and the DATA pin is wired to ESP32's GPIO 15 in the code delivered here.

GPIO is configure as open-drain for input & output with a 1k Ohm pull-up to 3.3v.

Code is generated using **ESP-IDF v5.4-dev-2194-gd7ca8b94c8** for Linux running on a Raspberry.

Code was developed and tested on a ESP-WROOM-32

## Description
The driver provides 2 functions

**void DHTinit(gpio_num_t gpio_num)**
is the initialization function that needs to be called once. It takes the GPIO number of choice as parameter and configures the GPIO as open drain for input & output (GPIO_MODE_INPUT_OUTPUT_OD), enables the pin for interruptions on falling edge (GPIO_INTR_NEGEDGE), enables the interruption handler function (ISR), and starts the ESP timer.

**int DHTget(uint16_t\* temperature, uint16_t\* humidity)**
is meant to be called anytime to get DHT's measurement. The function returns in around 20ms 
which is the time it takes the DHT to respond and transmit the data. The function is pre-emptive and is not blocking other tasks during this time.

When DHTget is called the ESP drives the GPIO as output to generate the start signal (it pulls the GPIO low and then up back).

```
          500us         20-40us   
 ___               _______________ - - - -
    \             /    
     \___________/                 
```
 
After the ESP32's start signal the DHT22 drives the DATA wire to send back the response. 

The DHT generates 41 down+up cycles.

The first down+up cycle means "ready to output" and is followed by 40 pulses meaning data 0 or 1

Data 0 or 1 is discriminated by the duration of the up pulse. Up time for 0 is 26-28us and up time for 1 is around 70us

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

The recorded down+up cycles are converted into bits. The post processing outcome is 5 bytes. The first two bytes are for the humidity and the second two is the temperature. 

The fifth byte is the checksum.


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
