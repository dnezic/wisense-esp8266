# wisense-esp8266
Source code for ESP8266-12E with BME280 used for weather station.
Flashing of ESP8266 is done using Raspberry Pi. There are some brief instructions how to connect and install everything below.

## Build docker image
We are going to use following docker image as toolchain:
```bash
# clone Dockerfile
git clone https://github.com/dnezic/docker-esp-open-sdk
cd docker-esp-open-sdk
# build docker image
sudo docker build -t dnezic/docker-esp-open-sdk:0.1 .
```
Flashing of the firmware will be done using Raspberry's UART port, therefore we are using this docker image only
for build purposes.

## Building the code
Clone this repo, or some other source code:
```bash
git clone https://github.com/dnezic/wisense-esp8266.git
```
Start container with `<path_to_src>` which points to your source code or previously cloned repo folder:
```bash
sudo docker run --name esp-builder -it -v <path_to_src>:/home/espbuilder/code/ dnezic/docker-esp-open-sdk:0.1
# or, afterwards, run shell inside existing container
sudo docker exec -it esp-builder /bin/bash
# cd to 'code' folder
cd code
# build code
make clean && make
```
Copy files in **firmware** folder somewhere to your Raspberry Pi:
* 0x00000.bin
* 0x40000.bin

## Flashing and running
It is presumed that **esptool** is installed on your Raspberry Pi.
Esptool can be installed from here: [https://github.com/themadinventor/esptool]

Firstly, Raspberry has to be setup to release UART as terminal console:
* disable serial port login (remove from /etc/inittab the line with ttyAMA0)
* disable serial port bootup messages (remove from /boot/cmdline.txt all parameters with ttyAMA0)

Connect Raspberry with ESP8266-12E in order to establish serial connection:
* RPi pin 10 (RXD) <---> ESP8266-12E TXD
* RPi pin 8  (TXD) <---> ESP8266-12E RXD
* Rpi pin 6  (GND) <---> ESP8266-12E GND

ESP8266-12E wiring description:
* GPIO0 connected to GND on powerup for flashing mode or
* GPIO0 connected to VCC on powerup for boot from flash mode (normal mode)
* GPIO15 to GND
* GPIO16 to RST to enable deep sleep used by weather station code
* CH_EN and GPIO2 to VCC

ESP8266-12E to BME280:
* GPIO14 to BME280 SCL
* GPIO2 to BME280 SDA

Check connection with ESP8266 in flashing mode:
```bash
./esptool.py --port /dev/ttyAMA0 read_mac
```
If it works, flash firmware:
```bash
./esptool.py --port /dev/ttyAMA0  write_flash 0x00000 ~/0x00000.bin 0x40000 ~/0x40000.bin
```
Monitor logs using terminal emulation software:
```bash
picocom --baud 115200  /dev/ttyAMA0
```

## Configuration
Configuration is placed in `user/user_config.h`
```c
#define USE_DNS false // if true, device will try to resolve REMOTE_HOST, otherwise REMOTE_IP will be used.
#define REMOTE_PORT 80 // server port
#define REMOTE_HOST "wisense" // only if USE_DNS true
#define REMOTE_IP "192.168.0.11" // only used if USE_DNS false
#define HOST_NAME "svecomp17" // name of this device
#define DATA_URL "/data" // relative PATH of POST request.
#define SSID "svesoftware" // wifi name
#define PASSWORD "" // wifi password
#define SLEEP_SEC 600 // 10 minutes
```

## Testing
In folder **test** there is little python TCP server that listens on port 80.
Adjust this port number to match settings in `user_config.h`.
