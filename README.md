# Tiny Instrument to Gather Radiation (TIGR)

[![Platform](https://img.shields.io/badge/Platform-MSP430-blue.svg)](https://www.ti.com/microcontrollers-mcus-processors/msp430-microcontrollers/overview.html)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)](#)

## Overview

TIGR is an embedded system designed to detect and log muon events across **four distinct energy bands**. Built for the **MSP430FR2355 microcontroller**, this instrument provides interrupt-driven detection with precision timestamping using the integrated Real-Time Clock (RTC).

The system is optimized for low-power operation and designed with space-grade deployment in mind, making it suitable for satellite and high-altitude applications.

## Features

- **Multi-band Detection**: Simultaneous monitoring of 4 energy bands via GPIO interrupt pins
- **Precision Timestamping**: MSP430FR2355 does not include a hardware RTC, so TIGR implements a full BCD   timestamping system using Timer_B0
- **Visual Feedback**: On-board LED indicators for real-time energy band identification  
- **Low Power Design**: Optimized for energy-efficient operation in space environments
- **Expandable Storage**: RAM buffer with SD card expansion capability
- **Interrupt-Driven**: Efficient event capture using falling-edge GPIO interrupts
- **Space-Qualified**: Designed for deployment on MSP430FR5989-SP hardware

## Hardware Requirements

### Development Platform
- **MCU**: [MSP430FR2355 LaunchPad Development Kit](https://www.ti.com/tool/MSP-EXP430FR2355)
- **Comparators**: 4x external comparator circuits (one per energy band)
- **Power Supply**: 3.3V regulated supply

### Production Platform  
- **MCU**: [MSP430FR5989-SP (Space-Grade)](https://www.ti.com/product/MSP430FR5989-SP)
- **Operating Temperature**: -55°C to +125°C
- **Radiation Tolerance**: Space-qualified components

### Pin Configuration

| Function | Pin | Description |
|----------|-----|-------------|
| Energy Band 4 | P2.1 | GPIO input with pull-up, falling edge trigger |
| Energy Band 3 | P2.2 | GPIO input with pull-up, falling edge trigger |
| Energy Band 2 | P2.3 | GPIO input with pull-up, falling edge trigger |
| Energy Band 1 | P2.4 | GPIO input with pull-up, falling edge trigger |
| Status LED 1 | P1.0 | Visual indicator for energy bands 1-2 |
| Status LED 2 | P6.6 | Visual indicator for energy bands 3-4 |

## Installation

### Prerequisites
- **Code Composer Studio (CCS)** v12.0 or later
- **MSP430 GCC Toolchain**
- **MSP430FR2355 LaunchPad** (for development)
- **SD Card Breakout Board**

## SD Card Pin Configuration
| Function        | Pin      | Notes                     |
| --------------- | -------- | ------------------------- |
| SCLK       | P1.1 | UCB0CLK                   |
| MOSI       | P1.2 | UCB0SIMO D1         |
| MISO        | P1.3 | UCB0SOMI D0                 |
| CS         | P1.0 | Dedicated chip-select     |
| Card Detect | P3.7 | Pull-up enabled, optional |


## Data Structure

Events are stored using the following structure:

```c
typedef struct {
    unsigned int muon_number;    // Muon event number
    unsigned char energy_band;   // Energy band (1-4)
    unsigned int year;           // Year
    unsigned char month;         // Month (1-12)
    unsigned char day;           // Day (1-31)
    unsigned char hour;          // Hour (0-23)
    unsigned char minute;        // Minute (0-59)
    unsigned int second;         // Second (0-59)
    int temperature;             // Temperature in Celsius
} EnergyReading;

```

### Data Output Format

```
Muon#,Band,Date,Time,TempC
0,3,2025-10-14,12:00:10,27
1,3,2025-10-14,12:00:11,24
2,3,2025-10-14,12:00:16,22
3,3,2025-10-14,12:00:16,28
4,3,2025-10-14,12:00:16,23
5,3,2025-10-14,12:00:16,23
```
### SD Write Strategy

Data is accumulated in sd_buffer
When full or when MAX_READINGS reached: A 512-byte sector is written via: 

```
mmc_write_sector(current_sector, sd_buffer);
```
## Low Power Mode

The system automatically enters low power mode between events to conserve energy:

```c
__bis_SR_register(LPM3_bits + GIE);  // Enter LPM3 with interrupts enabled
```