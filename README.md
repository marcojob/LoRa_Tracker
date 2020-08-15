# LoRa_Tracker
## Description:
This device aims to be a GPS tracker which connects over LoRa to the [Things Network](https://www.thethingsnetwork.org/), which then in turns makes the GPS position available to the internet. The PCB roughly has the footprint of a 18650 Lithium-ion rechargable cell, which is widely used and available.

### Hardware:
- Battery charging (using BQ24040)
- Battery protection (using BQ2970)
- LoRa module (RFM 95)
- GPS (ublox SAM M8Q)

## Progress so far:
- PCB design done
- PCB design verified (first prototype assembled)
![PCB front](/hardware/screenshots/front.png?raw=true "")
![PCB back](/hardware/screenshots/back.png?raw=true "")
- Ported IBM LMIC to STM32F103

## TODO and next steps
- Backend
- Implement GPS drivers
- Implement persistent memory writing for TTN keys
- Add Gerber files to repository
- Add STL files to repository

## Known issues
- ADC pin for battery voltage measurements wired wrong