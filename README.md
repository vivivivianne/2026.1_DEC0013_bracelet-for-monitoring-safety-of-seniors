# Bracelet for monitoring safety of seniors with Sinric Pro

![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)
![PlatformIO](https://img.shields.io/badge/PlatformIO-Build%20%26%20Flash-orange)
![Language: C++](https://img.shields.io/badge/Language-C++-purple.svg)

---

## Project Overview
This is a prototype for a security bracelet for the elderly and vulnerable that monitors 
heartrate, oxigen levels, falls and emmits an emergency alert to Sinric Pro when needed.

![IRL Circuit](img/circuit_irl.png)

The Hardware consists of an ESP32 microcontroller paired togheter with a MPU6050 for measuring acceleration, a MAX10102 for measuring heartrate and O2 levels and an emergency button.

![Circuit Diagram](img/circuit.png)

The Firmware is written in C++ and uses the PlatformIO framework with arduino and freertos libs, reading data from the sensors with I2C. 

![Code Output Example](img/output.png)

It reads data as often at a frequency of 1hz from the mpu6050 for better collision, it also filters the data by comparing the current acceleration value with an average of the values during the last 2 seconds, this allows us to notice when a value is an outlier, but also helps ignoring some false positives.
It also waits an interval of 10 seconds between alerts to avoid spamming the Sinric Pro API.

---

## Setup

- Install PlatformIO following their [documentation](https://platformio.org/install). 

- Create an account on [Sinric Pro](https://sinric.pro/) and create a new device, select switch as the type and ensure the notification setting is enabled. After that you can go to your device page and get it's credentials.

![Sinric Device Creation](img/sinric.png)

- Clone this repo 

- Go to bracelet/src/credentials.h and substitute your Sinric Pro credentials accordingly there.

- Build / Flash by running the following command inside bracelet/

``pio run -t upload``

[! Recommended for testing, prints a lot of data!]

To Build / Flash with debugging enabled run,

``pio run -e debug -t upload``

## Project structure
```
.
├── 📁bracelet
│   ├── 📄compile_commands.json
│   ├── 📄diagram.json
│   ├── 📄platformio.ini
│   ├── 📄README.md
│   ├── 📁src
│   │   ├── 📄credentials.h
│   │   ├── 📄main.cpp
│   │   └── 📄types.h
│   └── 📄wokwi.toml
├── 📁img
│   ├── 🖼️circuit.png
│   └── 🖼️sinric.png
├── 📁PCBlayout
│   └── 📄README.md
└── 📄README.md```

---

## Authors

- Vivianne Coelho Fernandes
- Grégori Silva Grazziani de Oliveira
- Vinicius Caio Rodrigues

### To-do
- [x] code review / cleanup / merge
- [x] test and adjust the final code
- [x] Document how to setup platformio and compile the project
- [x] Document file structure
- [x] Update project documentation
- [ ] Extra code cleanup and build features
- [ ] Remove readme from bracelet folder.
