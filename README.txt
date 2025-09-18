# Tiny Instrument to Gather Radiation (TIGR) (MSP430)

## Overview
This project runs on the **MSP430FR5969 microcontroller** and is designed to detect muon events categorized into **four energy bands**. Each energy band is mapped to a GPIO input pin connected to a comparator. 
When a falling-edge interrupt occurs on one of these pins, the corresponding energy band is logged, along with a timestamp from the Real-Time Clock (RTC).

LEDs on the development board are used to visually display which energy band was triggered. Logged events are stored in memory, with the ability to later expand to external storage (e.g., SD card).

---

## Development & Deployment
- **Current Development Board:** [MSP430FR6989 LaunchPad Development Kit](https://www.ti.com/tool/MSP-EXP430FR6989)  
- **Target Deployment Board:** [MSP430FR5989-SP (Space-Grade MSP430FR5989)](https://www.ti.com/product/MSP430FR5989-SP)  

Development and testing are currently performed on the FR6989 prototyping board. The final deployment target is the FR5989-SP for space-qualified applications.

---

## Features
- Interrupt-driven detection of up to **4 energy bands** using GPIO falling edges.  
- Event logging with timestamp:
  - Muon event number  
  - Energy band (1–4)  
  - Year, month, day, hour, minute, second  
- On-board LED indicators for quick visual identification of triggered energy bands.  
- Low power mode support for energy-efficient operation.  
- Expandable storage: currently logs to a buffer in RAM, but designed to be extended to SD card storage when buffer is full.  

---

## Hardware Setup
**MCU:** TI MSP430 (with RTC support).  

**Inputs:**  
- P2.1 → Energy Band 1  
- P2.2 → Energy Band 2  
- P2.3 → Energy Band 3  
- P2.4 → Energy Band 4  

**Outputs:**  
- P1.0 (LED1)  
- P9.7 (LED2)  

Each input pin is configured with an internal pull-up resistor and triggers on a falling edge signal.  

---

## Data Structure
Events are stored in the `EnergyReading` struct:

```c
typedef struct {
    unsigned int muon_number;    // Muon event number
    unsigned char energy_band;   // Energy band (1-4)
    unsigned int year;           // Year
    unsigned char month;         // Month (1-12)
    unsigned char day;           // Day (1-31)
    unsigned char hour;          // Hour (0-23)
    unsigned char minute;        // Minute (0-59)
    unsigned char second;        // Second (0-59)
} EnergyReading;
