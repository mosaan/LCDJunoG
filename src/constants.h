#define ZOOM_X 2
#define ZOOM_Y 3
#define ORIGINAL_LCD_WIDTH 240
#define ORIGINAL_LCD_HEIGHT 96
#define Y_PACKED_BYTES_LENGTH (ORIGINAL_LCD_HEIGHT / 8) // Original LCD has 96 pixels in Y direction, but 8 pixels are packed into 1 byte.
#define ORIGINAL_LCD_PART_X_MAX_ADDRESS 123 // max X address for CS1 and CS2. Last 3 pixels are not used.
#define RAW_DATA_BUFFER_LENGTH (ORIGINAL_LCD_PART_X_MAX_ADDRESS * Y_PACKED_BYTES_LENGTH)
#define CS2_X_OFFSET 120 // X offset for CS2. CS2 starts at X address 120.

// R/S (INSTRUCTION/DATA REGISTER SELECTION) pin position in the buffer_cs1 and buffer_cs2.
// The position is relative to the D0 pin.
#define RS_PIN_RELATIVE_POSITION ((32 + JUNO_RS - JUNO_D0) % 32)