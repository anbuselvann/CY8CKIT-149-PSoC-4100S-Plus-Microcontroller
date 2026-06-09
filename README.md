# PSoC 4100S Plus Embedded Projects

This repository contains bare-metal embedded projects developed using the Infineon PSoC 4100S Plus microcontroller. All projects are implemented using direct register programming without external libraries.

## 1. Digital Dice

A simple electronic dice that generates a random number from 1 to 6 when a push button is pressed.

### Features

* Push-button controlled dice roll
* LCD display output
* LED indication during roll
* Random number generation using software counter

### Hardware Used

* PSoC 4100S Plus
* Push Button
* LED
* 16x2 I2C LCD (PCF8574)

---

## 2. I2C LCD Interface

A bare-metal implementation of a 16x2 LCD interface using I2C bit-banging.

### Features

* Software I2C communication
* PCF8574 I/O expander support
* LCD initialization and control
* Text display on LCD

### Hardware Used

* PSoC 4100S Plus
* 16x2 LCD
* PCF8574 I2C Backpack

---

## 3. Smart Farm Monitoring System

A monitoring system for basic agricultural applications.

### Features

* Soil moisture detection
* LDR-based light intensity monitoring
* Ultrasonic tank level measurement
* LCD-based status display
* Multiple screen display modes

### Hardware Used

* PSoC 4100S Plus
* Soil Moisture Sensor
* LDR Sensor
* HC-SR04 Ultrasonic Sensor
* 16x2 I2C LCD

---

## 4. Smart Dustbin (Ultrasonic + Servo Motor)

An automatic dustbin that opens its lid when an object is detected nearby.

### Features

* Contactless operation
* Ultrasonic distance measurement
* Servo motor lid control
* LCD status display
* Automatic open/close mechanism

### Hardware Used

* PSoC 4100S Plus
* HC-SR04 Ultrasonic Sensor
* Servo Motor (SG90/MG995)
* 16x2 I2C LCD

---

## Platform

* Microcontroller: Infineon PSoC 4100S Plus
* Language: Embedded C
* Development Style: Bare-Metal Register Programming
* IDE: VS Code
* Debugging: OpenOCD + KitProg3
