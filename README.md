# Pill Safety Device
Features
- Fingerprint reader to ensure no unauthorized access
- Timers and clocks prevent pills from being taken too early or too late
- Wifi connection to a custom website that allows managers to 
  a. Calibrate the device
  b. Manually override the fingerprint sensor and timing locks
  c. Reload the device with a new week of pills
  d. Reset both the parent and child fingerprints

# Application
This device could be used to allow older children to take their own pills without strict supervision since the device ensures they can not take too many pills or take them at the wrong time
Additionally instead of a parent and a child this device could be used by an elderly person and their caretaker.

# Design
## Materials
- esp32
- servo motors 2x
- 3d printed container and parts (found in cadfiles folder)
- arduino buzzer
- real time clock module
- push button and pull down resistor
- R30x finger print module

## Schematic
<img width="80px" src="./schematic.png" alt="schematic image" />

## Code 
The code can be found in main/main.ino