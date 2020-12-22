# Recordlight-HUI-DINUSB
FastLED based Record Light for Apple's Logic Pro, using the Mackie HUI protocol. Runs on an Arduino Leonardo with the Arduino MIDI, USB_MIDI and FastLED libraries. 
Lights used are 12 LEDs on a WS2812 based LED strip.

## Light inidicators:

Idle/not connected to Logic / Mackie HUI:
- rainbow effect 

Stop Mode in Logic (once the first 'Ping' from Logic is received):
- dark blue

Play:
- green

Record:
- red

Build for USB _or_ DIN-MIDI. Refer to source code...
