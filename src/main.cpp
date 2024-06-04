/*
 * Copyright (c) 2022 Eddi De Pieri
 * Most code borrowed by Pico-DMX by Jostein Løwer 
 * Modified for performance by bjaan
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Description: 
 * Roland Juno G LCD Emulator
 */

// Define in setup to disable all #warnings in library (can be put in User_Setup_Select.h)
#define DISABLE_ALL_LIBRARY_WARNINGS

#include <Arduino.h>

#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#ifdef DRAW_SPLASH
#include <LCDJunoG_splash.h>
#endif
#include <LCDJunoG_extra.h>

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#include "LCDJunoG.h"
LCDJunoG lcdJunoG_cs1;
LCDJunoG lcdJunoG_cs2;

#define ZOOM_X 2
#define ZOOM_Y 3
#define ORIGINAL_LCD_WIDTH 240
#define ORIGINAL_LCD_HEIGHT 96
const uint Y_PACKED_BYTES_LENGTH = ORIGINAL_LCD_HEIGHT / 8; // Original LCD has 96 pixels in Y direction, but 8 pixels are packed into 1 byte.
#define ORIGINAL_LCD_PART_X_MAX_ADDRESS 123 // max X address for CS1 and CS2. Last 3 pixels are not used.
const uint RAW_DATA_BUFFER_LENGTH = ORIGINAL_LCD_PART_X_MAX_ADDRESS * Y_PACKED_BYTES_LENGTH;
#define CS2_X_OFFSET 120 // X offset for CS2. CS2 starts at X address 120.

uint tft_xoffset = 0;
uint tft_yoffset = 0;

// R/S (INSTRUCTION/DATA REGISTER SELECTION) pin position in the buffer_cs1 and buffer_cs2.
// The position is relative to the D0 pin.
const uint8_t RS_PIN_RELATIVE_POSITION = (32 + JUNO_RS - JUNO_D0) % 32;

/* 
  buffer_cs1[i]: raw data buffer sent from JUNO-G.

    bits:
    FEDCBA98 76543210 FEDCBA98 76543210
    ???????? ???????? ???????? vvvvvvvv
    s = R/S (INSTRUCTION/DATA REGISTER SELECTION) pin data (see JUNO_RS in platformio.ini and RS_PIN_RELATIVE_POSITION)
    v = value bits (8 bits: values 0 up to and including 255). y-axis values for 8 pixels packed into 1 byte.
*/
volatile uint32_t buffer_cs1[RAW_DATA_BUFFER_LENGTH];

/* 
  buffer_cs2[i]: raw data buffer sent from JUNO-G.

    bits:
    FEDCBA98 76543210 FEDCBA98 76543210
    ???????? ???????? ???????? vvvvvvvv
    s = R/S (INSTRUCTION/DATA REGISTER SELECTION) pin data (see JUNO_RS in platformio.ini and RS_PIN_RELATIVE_POSITION)
    v = value bits (8 bits: values 0 up to and including 255). y-axis values for 8 pixels packed into 1 byte.
*/
volatile uint32_t buffer_cs2[RAW_DATA_BUFFER_LENGTH];

// === buffer_cs1 and buffer_cs2 bit manipulation ===
// Extracts the value bits from the buffer_cs1 and buffer_cs2
#define VALUE_BITS(value) (value & 0xff)
// Extracts the RS bit from the buffer_cs1 and buffer_cs2
#define RS_BIT(value) ((value >> RS_PIN_RELATIVE_POSITION) & 1)

volatile uint8_t back_buffer[ORIGINAL_LCD_WIDTH][Y_PACKED_BYTES_LENGTH]; /* max X address register for CS1 and CS2 times max Y register */
volatile uint16_t pixel_x[ORIGINAL_LCD_WIDTH]; /* X location (pre-calculated) of actual pixel on new screen */
volatile uint16_t pixel_y[ORIGINAL_LCD_HEIGHT]; /* Y location (pre-calculated) of actual pixel on new screen */

uint32_t tft_bgcolor = TFT_ORANGE;
uint32_t tft_bgcolor_prev = TFT_BLACK;

/*
  Draws a packed_pixels byte on the TFT screen at x,y_index

  arguments:
  - packed_pixels: 8 pixels packed into 1 byte. MSB is the top pixel, LSB is the bottom pixel.
  - y_index: the y index of the pixel. packed_pixels is drawn at y_index*8 up to and including y_index*8+7, thus 0 <= y_index < 12 (= Y_PACKED_BYTES_LENGTH).
  - x: the x address of the pixel
 */
void drawPixels(uint8_t packed_pixels, uint8_t x, uint8_t y_index) {
  int16_t real_x = pixel_x[x];
  int8_t y = y_index * 8;
  //for (int b = 0; b < 8; b++ ) {
  //  if (((packed_pixels >> b) & 0x1) == 1) tft.drawPixel(real_x, pixel_y[y+b], TFT_BLACK); else tft.drawPixel(real_x, pixel_y[y+b], TFT_WHITE);
  //}
  //unrolled loop for performance:
  if ((packed_pixels & 0x1) == 0x1)   tft.drawPixel(real_x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(real_x, pixel_y[y++], tft_bgcolor);
  if ((packed_pixels & 0x2) == 0x2)   tft.drawPixel(real_x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(real_x, pixel_y[y++], tft_bgcolor);
  if ((packed_pixels & 0x4) == 0x4)   tft.drawPixel(real_x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(real_x, pixel_y[y++], tft_bgcolor);
  if ((packed_pixels & 0x8) == 0x8)   tft.drawPixel(real_x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(real_x, pixel_y[y++], tft_bgcolor);
  if ((packed_pixels & 0x10) == 0x10) tft.drawPixel(real_x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(real_x, pixel_y[y++], tft_bgcolor);
  if ((packed_pixels & 0x20) == 0x20) tft.drawPixel(real_x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(real_x, pixel_y[y++], tft_bgcolor);
  if ((packed_pixels & 0x40) == 0x40) tft.drawPixel(real_x, pixel_y[y++], TFT_BLACK); else tft.drawPixel(real_x, pixel_y[y++], tft_bgcolor);
  if ((packed_pixels & 0x80) == 0x80) tft.drawPixel(real_x, pixel_y[y  ], TFT_BLACK); else tft.drawPixel(real_x, pixel_y[y  ], tft_bgcolor);
}

void fillscreenInterlaced(uint32_t bgcolor) {
  tft.startWrite();
  tft.fillScreen(TFT_BLACK);
  for (uint x = 0; x < ORIGINAL_LCD_WIDTH; x++) {
    for (uint y = 0; y < Y_PACKED_BYTES_LENGTH; y++) {
        drawPixels(back_buffer[x][y], x, y);
    }
  }
  tft.endWrite();
}

void tft_change_brightness(uint32_t analog_read) {
//42 54  =  50 255  0 -> 12 
  analogWrite( JUNO_BACKLIGHT, ((analog_read - 41) & 0xc) * 17 + 51 );
}

void tft_change_bgcolor(uint32_t analog_read) {
  //Serial.println("ANALOG"+ String(analog_read));
  //min = 42 
  //max = 54
  if (analog_read < 44) {
    tft_bgcolor = TFT_ORANGE;
  } else if ( analog_read >= 44 && analog_read < 46) {
    tft_bgcolor = TFT_SKYBLUE;
  } else if ( analog_read >= 46 && analog_read < 48) {
    tft_bgcolor = TFT_GREENYELLOW;
  } else if ( analog_read >= 48 && analog_read < 50) {
    tft_bgcolor = TFT_YELLOW;
  } else if ( analog_read >= 50 && analog_read < 52) {
    tft_bgcolor = TFT_WHITE;
  } else if ( analog_read >= 52 && analog_read < 54) {
    tft_bgcolor = TFT_CYAN;
  } else if ( analog_read >= 54) {
    tft_bgcolor = TFT_WHITE;
  } 

  if (tft_bgcolor != tft_bgcolor_prev) {
    fillscreenInterlaced(tft_bgcolor);
    tft_bgcolor_prev = tft_bgcolor;
  }
}

void setup()
{
  analogReadResolution(6);
  uint32_t analog_read = analogRead(JUNO_BRGT);


  //Serial.begin(115200);
  // delay(2000);
  //Serial.setTimeout(50);
  // Serial.println("juno g lcd emulator");
  tft.init();
  tft.setRotation(1);
  tft_xoffset = (tft.width() - ORIGINAL_LCD_WIDTH * ZOOM_X) / 2 - ((tft.width() - ORIGINAL_LCD_WIDTH * ZOOM_X) / 2 % ZOOM_X);
  tft_yoffset = (tft.height() - ORIGINAL_LCD_HEIGHT * ZOOM_Y) / 2 - ((tft.height() - ORIGINAL_LCD_HEIGHT * ZOOM_Y) / 2 % ZOOM_Y);

  pinMode( JUNO_BACKLIGHT, OUTPUT );
#ifdef MODE_BRIGHTNESS
  analogWriteResolution(8);
  analogWrite( JUNO_BACKLIGHT, int((analog_read - 2750) / 2.9) );
#else
  digitalWrite(JUNO_BACKLIGHT, HIGH);
#endif

#ifdef DRAW_SPLASH
  tft_change_bgcolor();
  drawBitmapZoom(0, 0, (const uint8_t *)junog, 240, 90, TFT_BLACK);
  delay(500);
#endif

  {  //precalculate new display pixel indices
    for (uint8_t x = 0; x < ORIGINAL_LCD_WIDTH; x++) {
      pixel_x[x] = tft_xoffset + x * ZOOM_X;
    }
    for (uint8_t y = 0; y < ORIGINAL_LCD_HEIGHT; y++) {
      pixel_y[y] = tft_yoffset + y * ZOOM_Y;
    }
  }
  { //initialize back buffer
    for (uint8_t x = 0; x < ORIGINAL_LCD_WIDTH; x++) {
      for (uint8_t y = 0; y < Y_PACKED_BYTES_LENGTH; y++) {
        back_buffer[x][y] = 0;
      }
    }
  }

#define DRAW_INFO
#ifdef DRAW_INFO
#ifdef MODE_BRIGHTNESS
  fillscreenInterlaced(tft_bgcolor);
  tft_change_brightness(analog_read);
#else
  tft_change_bgcolor(analog_read);
#endif
  tft.setTextColor(TFT_BLACK);
  tft.setCursor(0, tft.height()/2 -10  , 2);
  tft.setTextSize(2);
  
  tft.setTextDatum(TC_DATUM);
  tft.drawString("JunoG LCD replacement V2.1.3", tft.width() /2, tft.height() / 2 - 20 );
  //tft.drawString("CPU_FREQ:" + String(rp2040.f_cpu()), tft.width() /2, tft.height() / 2 + 10 );
  delay(500);
  fillscreenInterlaced(tft_bgcolor);
#endif

#ifdef DRAW_PINOUT  
  drawPinout(1000);
#endif
   
  tft.setCursor(0, 0, 2);
  tft.setTextSize(1);
  
  // Setup our DMX Input to read on GPIO 0, from channel 1 to 3
  // TFT_eSPI uses all of instruction memory of pio0 or pio1,
  // so we use pio1 for both(CS1/CS2) chip monitoring to keep enough memory for TFT_eSPI.
  // (TFT_eSPI consumes 32 PIO instructions. see ".pio/libdeps/pico/TFT_eSPI/Processors/pio_SPI.pio.h")
  lcdJunoG_cs1.begin(JUNO_D0, /* pio0 */ pio1, 1);
  lcdJunoG_cs2.begin(JUNO_D0, pio1, 2);
  lcdJunoG_cs1.read_async(buffer_cs1);
  lcdJunoG_cs2.read_async(buffer_cs2);

  // Setup the onboard LED so that we can blink when we receives packets
  pinMode(LED_BUILTIN, OUTPUT);
}

volatile uint8_t x_cs1 = 0; //X address counter for CS1: 7 bit: values 0 up and to including 127: actually max. 120
volatile uint8_t x_cs2 = CS2_X_OFFSET; //X address counter for CS2: 7 bit: values 0 up and to including 127: actually max. 120. Here it is set, and later reset to 120, to avoid having to add an offset of 120 all the time
volatile uint8_t y_cs1 = 0; //Y address register for CS1: 4 bit: values 0 up and to including 15: actually max. 12 as 12*8 = 96
volatile uint8_t y_cs2 = 0; //Y address register for CS2: 4 bit: values 0 up and to including 15: actually max. 12 as 12*8 = 96

bool led_on = false;
unsigned long latest_packet_timestamp_cs1 = 0;
unsigned long latest_packet_timestamp_cs2 = 0;

int period = 500;
unsigned long time_now = 0;
unsigned long my_millis = 0;

void loop()
{  
  my_millis = millis();

  if(my_millis >= time_now + period){
    time_now += period;
    uint32_t analog_read = analogRead(JUNO_BRGT);
#ifdef MODE_BRIGHTNESS
        tft_change_brightness(analog_read);
#else        
        tft_change_bgcolor(analog_read);
#endif
#ifdef DEBUG_READ
    tft.fillRect(tft.width() / 2 - 40, 0, 80, 14, TFT_BLACK);
    tft.setTextColor(tft_bgcolor);
    tft.setCursor(tft.width() /2 - 40 , 0, 2);
    tft.setTextSize(1);
  
    tft.setTextDatum(TC_DATUM);
    tft.drawString(String(analog_read), tft.width() /2, 0);
#endif
  }

  if(latest_packet_timestamp_cs1 == lcdJunoG_cs1.latest_packet_timestamp() && latest_packet_timestamp_cs2 == lcdJunoG_cs2.latest_packet_timestamp()) {
    return; // no packet received
  } 
  latest_packet_timestamp_cs1 = lcdJunoG_cs1.latest_packet_timestamp();
  latest_packet_timestamp_cs2 = lcdJunoG_cs2.latest_packet_timestamp();
  tft.startWrite();
  for (uint i = 0; i < RAW_DATA_BUFFER_LENGTH; i++)  
  {
    { //CS 1
      uint8_t val = VALUE_BITS(buffer_cs1[i]);
      uint8_t rs = RS_BIT(buffer_cs1[i]);
      if (rs == 0) { //INSTRUCTION REGISTER
#ifdef SHOWCMD
          showcmd( cs, val );
#endif
      // JUNO-G has KS0713 like LCD controller. It has 2 display data RAMs. One for the left hand part and one for the right hand part.
      // 
      if ((val & 0xf0 /*11110000*/) == 0xb0 /*1011000*/) { //Sets the Y address at the Y address register
          y_cs1 = val & 0x0f /*00001111*/;
          x_cs1 = 0;
        }
      } else if (rs == 1) { //DATA REGISTER
        //writes data on JUNO_D0 to JUNO_D7 pins into display data RAM.
        if (x_cs1 < CS2_X_OFFSET) {  // avoid overflow
          if (back_buffer[x_cs1][y_cs1] != val) { // only draw if value has changed for performance
            back_buffer[x_cs1][y_cs1] = val;
            drawPixels(val, x_cs1, y_cs1);
          }
          x_cs1++; //After the writing instruction, X address is increased by 1 automatically.
        }
      }    
    }
    { //CS 2
      uint8_t val = VALUE_BITS(buffer_cs2[i]);
      uint8_t rs = RS_BIT(buffer_cs2[i]);
      if (rs == 0) { //INSTRUCTION REGISTER
#ifdef SHOWCMD
        showcmd( cs, val );
#endif
        if ((val & 0xf0 /*11110000*/) == 0xb0 /*1011000*/) { //Sets the X address at the X address register
          y_cs2 = val & 0x0f /*00001111*/;
          x_cs2 = CS2_X_OFFSET; // 120 is left most pixel of the right hand part
        }
      } else if (rs == 1) { //DATA REGISTER
        //writes data on JUNO_D0 to JUNO_D7 pins into display data RAM.
        if (x_cs2 >= CS2_X_OFFSET && x_cs2 < ORIGINAL_LCD_WIDTH) {  // avoid overflow  
          if (back_buffer[x_cs2][y_cs2] != val) {
            back_buffer[x_cs2][y_cs2] = val;
            drawPixels(val, x_cs2, y_cs2);
          }
          x_cs2++; //After the writing instruction, Y address is increased by 1 automatically.
        }
      }
    }
  }
  tft.endWrite();
  // Blink the LED to indicate that a packet was received
  if (!led_on) digitalWrite(LED_BUILTIN, HIGH); else digitalWrite(LED_BUILTIN, LOW);
  led_on = !led_on;
}
