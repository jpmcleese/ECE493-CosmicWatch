# Tiny Instrument to Gather Radiation (TIGR)

[![Platform](https://img.shields.io/badge/Platform-MSP430-blue.svg)](https://www.ti.com/microcontrollers-mcus-processors/msp430-microcontrollers/overview.html)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)](#)

## Overview

TIGR is an embedded system designed to detect and log muon events across **four distinct energy bands**. Built for the **MSP430FR5969 microcontroller**, this instrument provides interrupt-driven detection with precision timestamping using the integrated Real-Time Clock (RTC).

The system is optimized for low-power operation and designed with space-grade deployment in mind, making it suitable for satellite and high-altitude applications.

## Features

- **Multi-band Detection**: Simultaneous monitoring of 4 energy bands via GPIO interrupt pins
- **Precision Timestamping**: RTC-based event logging with full date/time resolution
- **Visual Feedback**: On-board LED indicators for real-time energy band identification  
- **Low Power Design**: Optimized for energy-efficient operation in space environments
- **Expandable Storage**: RAM buffer with SD card expansion capability
- **Interrupt-Driven**: Efficient event capture using falling-edge GPIO interrupts
- **Space-Qualified**: Designed for deployment on MSP430FR5989-SP hardware

## Hardware Requirements

### Development Platform
- **MCU**: [MSP430FR6989 LaunchPad Development Kit](https://www.ti.com/tool/MSP-EXP430FR6989)
- **Comparators**: 4x external comparator circuits (one per energy band)
- **Power Supply**: 3.3V regulated supply

### Production Platform  
- **MCU**: [MSP430FR5989-SP (Space-Grade)](https://www.ti.com/product/MSP430FR5989-SP)
- **Operating Temperature**: -55°C to +125°C
- **Radiation Tolerance**: Space-qualified components

### Pin Configuration

| Function | Pin | Description |
|----------|-----|-------------|
| Energy Band 1 | P2.1 | GPIO input with pull-up, falling edge trigger |
| Energy Band 2 | P2.2 | GPIO input with pull-up, falling edge trigger |
| Energy Band 3 | P2.3 | GPIO input with pull-up, falling edge trigger |
| Energy Band 4 | P2.4 | GPIO input with pull-up, falling edge trigger |
| Status LED 1 | P1.0 | Visual indicator for energy bands 1-2 |
| Status LED 2 | P9.7 | Visual indicator for energy bands 3-4 |

## Installation

### Prerequisites
- **Code Composer Studio (CCS)** v12.0 or later
- **MSP430 GCC Toolchain**
- **MSP430FR6989 LaunchPad** (for development)
- **SD Card Breakout Board**

## SD Card Pin Configuration
| Function | Pin | Description |
|----------|-----|-------------|
| SCLK | P1.4 | System clock connection |
| MOSI | P1.6 | Master Out Slave In connection |
| MISO | P1.7 | Master In Slave Out connection |
| CS | P1.3 | Chip Select |
| Card Detect | P1.5 | Determine if card is inserted |
| 3.3V | 3.3V Source | MSP430 3.3V source |
| GND | Ground | MSP430 0V pin |

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
Event#, Band, Year, Month, Day, Hour, Minute, Second, Temperature (Celsius)
00001, 2, 2025, 03, 15, 14, 30, 45, 25
00002, 1, 2025, 03, 15, 14, 30, 47, 25
00003, 4, 2025, 03, 15, 14, 30, 52, 25
```

## Low Power Mode

The system automatically enters low power mode between events to conserve energy:

```c
__bis_SR_register(LPM3_bits + GIE);  // Enter LPM3 with interrupts enabled
```