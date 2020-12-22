/*
 * RecordLight HUI DINUSB
 * ======================
 * Mixed version for Arduino LEONARDO USB / DIN-MIDI on rx/tx
 * Mackie HUI compatibel Record Light for Apple's Logic Pro
 * 
 * Expect Data on Channel 1 from Logic Mackie HUI Control
 * 
 * xelarep, 22.12.2020
 */

// Debuggug on serial  
//#define DEBUG

// build for USB port
#define USBACTIVE

#ifndef USBACTIVE
// Standard MIDI on serial RX/TX /DIN)
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

#else
// MIDI over USB
#include <USB-MIDI.h>
USBMIDI_CREATE_DEFAULT_INSTANCE();

#endif

#include <FastLED.h>

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    3           // WS2812 Data Pin
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    12
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  120

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

// State Machine
enum eStatus { Idle, Stopped, Play, Record };
eStatus curState = Idle;

// pending Update
bool updateState = false;
bool sendPing = false;     

// Filter for zone
byte HUIzone = 0;

byte transport = 0x00;
#define   RECDMODE  0x01
#define   PLAYMODE  0x02
#define   STOPMODE  0x04

// timeout for ping
int timeout = 0;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
#endif  
  MIDI.begin();

  MIDI.setHandleNoteOn(OnNoteOn);
  MIDI.setHandleNoteOff(OnNoteOff);
  MIDI.setHandleControlChange(OnControlChange);
 
  delay(2000);

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);  

  curState = Idle;
  updateState = true;   // initial update
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void loop()
{
  MIDI.read();

  if(updateState == true)
  {
    switch(transport)
    {
      case 0x07 :
      case 0x06 :
      case 0x05 :
      case 0x04 : curState = Stopped;
                  break;
      case 0x02 : curState = Play;
                  break;
      case 0x01 :
      case 0x03 : curState = Record;
                  break;
      default :   curState = Idle;
                  break;
    }
    switch(curState) {
      case Idle:
                    //SetRGBColor(0x04, 0x02, 0x2);
#ifdef DEBUG
                    Serial.print("Idle "); Serial.println(transport, BIN);
#endif
                    break;
      case Stopped:
                    SetRGBColor(0x00, 0x00, 0x05);    // Stop / Connected to Logic
#ifdef DEBUG
                    Serial.print("Stopped "); Serial.println(transport, BIN);
#endif
                    break;
      case Play:                                      // Playback
                    SetCRGBColor(CRGB::Green);
#ifdef DEBUG
                    Serial.print("Play "); Serial.println(transport, BIN);
#endif
                    break;
      case Record:                                    // Recording
                    SetCRGBColor(CRGB::Red);
#ifdef DEBUG
                    Serial.print("Record "); Serial.println(transport, BIN);
#endif
                    break;

      default:      break;
    }

    // send the 'leds' array out to the actual LED strip
    FastLED.show();  
    
    updateState = false;
  }

  if(sendPing == true)
  {
     MIDI.sendNoteOn (0, 127, 1);      // note 0, velocity 127 on channel 1  
     sendPing = false;
     timeout = 0;
  }

  EVERY_N_MILLISECONDS( 250 ) { timeout++; }

  if(timeout >= 15 && curState != Idle)
  {
#ifdef DEBUG
    Serial.println("Timeout!");
#endif
    curState = Idle;
    transport = 0;
    updateState = true;
  }

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow

  if(curState == Idle)
  {
     // FastLED's built-in rainbow generator
     fill_rainbow( leds, NUM_LEDS, gHue, 7);
     // send the 'leds' array out to the actual LED strip
     FastLED.show();
     FastLED.delay(1000/FRAMES_PER_SECOND);
  }
}  

// ====================================================================================
// Event handlers for incoming MIDI messages
// ====================================================================================

// -----------------------------------------------------------------------------
// Received note on
// -----------------------------------------------------------------------------
void OnNoteOn(byte channel, byte note, byte velocity) {
     // Note On /w velocity = 0 is received as note off! ==> Ping handled in OnNoteOff() 
}

// -----------------------------------------------------------------------------
// Received note off
// -----------------------------------------------------------------------------
void OnNoteOff(byte channel, byte note, byte velocity) {
  if(channel == 1)
  {
     if(note == 0)                        // Ping
     {
        if (curState == Idle)
        {
          curState = Stopped;
          transport = STOPMODE;
          updateState = true;
        }

        sendPing = true;
     }
  }  
}

// -----------------------------------------------------------------------------
// Continous Control
// -----------------------------------------------------------------------------
void OnControlChange(byte channel, byte number, byte value) {
   if(channel == 1)
   {
     if (number == 0x0c)    // get zone
       HUIzone = value;

     if(HUIzone == 0x0e && number == 0x2c)     // get zone ctrl and value
     {
        updateState = true;

        switch(value)                         // rebuild buttons in single value
        {
          case 0x45:        // record on
                            transport |= RECDMODE;
                            break;
          case 0x44:        // play on
                            transport |= PLAYMODE;
                            break;
          case 0x43:        // stop on
                            transport |= STOPMODE;
                            break;
          case 0x05:        // record off
                            transport &= ~RECDMODE;
                            break;
          case 0x04:        // play off
                            transport &= ~PLAYMODE;
                            break;
          case 0x03:        // stop off
                            transport &= ~STOPMODE;
                            break;
          default:          
                            updateState = false;
                            break;                         
        }
     }
   }  
}

// -----------------------------------------------------------------------------
// Color management

void SetCRGBColor(CRGB col) {
  for( int i = 0; i < NUM_LEDS; i++) 
     leds[i] = col;      
}
void SetRGBColor(byte r, byte g, byte b) {
  for( int i = 0; i < NUM_LEDS; i++) 
     leds[i].setRGB( r, g, b);
}
// -----------------------------------------------------------------------------
