// The following OLED setup taken from the ssd1306_128x64_i2c library example
// --------------------------------------------------------------------------
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// --------------------------------------------------------------------------

const int X_CENTER = SCREEN_WIDTH / 2;
const int Y_CENTER = SCREEN_HEIGHT / 2;
const int LEFT_EDGE = X_CENTER - SCREEN_HEIGHT / 2;
const int RIGHT_EDGE = X_CENTER + SCREEN_HEIGHT / 2;
const int CORNER_WARNING_LINE_LENGTH = 14;
const int EDGE_WARNING_LINE_LENGTH = 18;
const int WARNING_THICKNESS = 4;

const int ATTACK_COUNT = 4;
const int BLINK_END = 300;
const int PAUSE_END = 500;
const int ATTACK_END = 800;
const int ATTACK_EDGE_LENGTH = 30;

enum GAME_STATE {
  START,
  PLAY,
  HELP
};

GAME_STATE _curState = START;

/* This array tracks the states of each of the 4 possible line attacks
   Value: ATTACK_LENGTH+ inactive, 0 to BLINK_END blinking, BLINK_END to PAUSE_END pause, PAUSE_END to
      STRIKE_END striking
   Position: 0 horizontal, 1 vertical, 2 diagonal top-left to bottom-right, 3 diagonal bottom-left to 
      top-right
*/
int _attackStates[ATTACK_COUNT];

void setup() {
  // The following OLED setup taken from the ssd1306_128x64_i2c library example
  // --------------------------------------------------------------------------
  Serial.begin(9600);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // --------------------------------------------------------------------------

  for (int i = 0; i++; i<ATTACK_COUNT) {
    _attackStates[i] = 1000;
  }
  _attackStates[0] = 0;
  _attackStates[1] = 0;
  _attackStates[2] = 0;
  _attackStates[3] = 0;
  display.clearDisplay();
}

void loop() {
  display.clearDisplay();

  display.drawRect(LEFT_EDGE, 0, RIGHT_EDGE - LEFT_EDGE, SCREEN_HEIGHT, SSD1306_WHITE); // draw game border

/*
  drawUpLeftWarning();
  drawUpRightWarning();
  drawDownLeftWarning();
  drawDownRightWarning();

  if (_curState == PLAY) {
    drawUpWarning();
  }
  drawDownWarning();
  drawLeftWarning();
  drawRightWarning();
  */
  int i = 0;
  while (i <= 3) {
    if (_attackStates[i] < ATTACK_END) {
      updateAttack(i, 20);
      break;
    }
    i++;
  }
  if (i == 4) {
    for (int j = 0; j < ATTACK_COUNT; j++) {
      _attackStates[j] = 0;
    }
  }
  /*
  drawAttackPartial(0, 0);
  drawAttackPartial(0, 1);
  drawAttackPartial(1, 0);
  drawAttackPartial(1, 1);
  drawAttackPartial(2, 0);
  drawAttackPartial(2, 1);
  drawAttackPartial(3, 0);
  drawAttackPartial(3, 1);
  */
  display.display();
}

void updateAttack(int direction, int speed) {
  int pos = _attackStates[direction];
  if (pos < BLINK_END) {
    int blink_time = BLINK_END / 6;
    if ((pos < blink_time) || (pos > 2 * blink_time && pos < 3 * blink_time) || (pos > 4 * blink_time &&
        pos < 5 * blink_time)) {
      if (direction == 0) {
        drawLeftWarning();
        drawRightWarning();
      } else if (direction == 1) {
        drawUpWarning();
        drawDownWarning();
      } else if (direction == 2) {
        drawUpLeftWarning();
        drawDownRightWarning();
      } else if (direction == 3) {
        drawDownLeftWarning();
        drawUpRightWarning();
      }
    }
  } else if (pos < PAUSE_END) {
    // do nothing
  } else if (pos < ATTACK_END) {
    drawAttackPartial(direction, 0);
    if (pos > (PAUSE_END + ATTACK_EDGE_LENGTH) && pos < (ATTACK_END - ATTACK_EDGE_LENGTH)) {
      drawAttackPartial(direction, 1);
    }
  }
  _attackStates[direction] += speed;
}

void drawAttackPartial(int direction, int offset) {
  if (direction == 0) {
    if (offset == 0) {
      display.drawFastHLine(LEFT_EDGE, Y_CENTER, RIGHT_EDGE - LEFT_EDGE, SSD1306_WHITE);
    } else {
      display.drawFastHLine(LEFT_EDGE, Y_CENTER + offset, RIGHT_EDGE - LEFT_EDGE, SSD1306_WHITE);
      display.drawFastHLine(LEFT_EDGE, Y_CENTER - offset, RIGHT_EDGE - LEFT_EDGE, SSD1306_WHITE);
    }
  } else if (direction == 1) {
    if (offset == 0) {
      display.drawFastVLine(X_CENTER, 0, SCREEN_HEIGHT, SSD1306_WHITE);
    } else {
      display.drawFastVLine(X_CENTER + offset, 0, SCREEN_HEIGHT, SSD1306_WHITE);
      display.drawFastVLine(X_CENTER - offset, 0, SCREEN_HEIGHT, SSD1306_WHITE);
    }
  } else if (direction == 2) {
    if (offset == 0) {
      display.drawLine(LEFT_EDGE, 0, RIGHT_EDGE, SCREEN_HEIGHT, SSD1306_WHITE);
    } else {
      display.drawLine(LEFT_EDGE + offset, 0, RIGHT_EDGE, SCREEN_HEIGHT - offset, SSD1306_WHITE);
      display.drawLine(LEFT_EDGE, offset, RIGHT_EDGE - offset, SCREEN_HEIGHT, SSD1306_WHITE);
    }
  } else if (direction == 3) {
    if (offset == 0) {
      display.drawLine(LEFT_EDGE, SCREEN_HEIGHT, RIGHT_EDGE, 0, SSD1306_WHITE);
    } else {
      display.drawLine(LEFT_EDGE + offset, SCREEN_HEIGHT, RIGHT_EDGE, offset, SSD1306_WHITE);
      display.drawLine(LEFT_EDGE, SCREEN_HEIGHT - offset, RIGHT_EDGE - offset, 0, SSD1306_WHITE);
    }
  }
}

void drawUpLeftWarning() {
  for (int i = 0; i < WARNING_THICKNESS; i++) {
    display.drawFastHLine(LEFT_EDGE+2+i, 2+i, CORNER_WARNING_LINE_LENGTH-2*i, SSD1306_WHITE);
    display.drawFastVLine(LEFT_EDGE+2+i, 2+i, CORNER_WARNING_LINE_LENGTH-2*i, SSD1306_WHITE);
  }
}

void drawUpRightWarning() {
  for (int i = 0; i < WARNING_THICKNESS; i++) {
    display.drawFastHLine(RIGHT_EDGE-2-CORNER_WARNING_LINE_LENGTH+i, 2+i, CORNER_WARNING_LINE_LENGTH-2*i, SSD1306_WHITE);
    display.drawFastVLine(RIGHT_EDGE-3-i, 2+i, CORNER_WARNING_LINE_LENGTH-2*i, SSD1306_WHITE);
  }
}

void drawDownLeftWarning() {
  for (int i = 0; i < WARNING_THICKNESS; i++) {
    display.drawFastHLine(LEFT_EDGE+2+i, SCREEN_HEIGHT-3-i, CORNER_WARNING_LINE_LENGTH-2*i, SSD1306_WHITE);
    display.drawFastVLine(LEFT_EDGE+2+i, SCREEN_HEIGHT-CORNER_WARNING_LINE_LENGTH-2+i, CORNER_WARNING_LINE_LENGTH-2*i, SSD1306_WHITE);
  }
}

void drawDownRightWarning() {
  for (int i = 0; i < WARNING_THICKNESS; i++) {
    display.drawFastHLine(RIGHT_EDGE-2-CORNER_WARNING_LINE_LENGTH+i, SCREEN_HEIGHT-3-i, CORNER_WARNING_LINE_LENGTH-2*i, SSD1306_WHITE);
    display.drawFastVLine(RIGHT_EDGE-3-i, SCREEN_HEIGHT-CORNER_WARNING_LINE_LENGTH-2+i, CORNER_WARNING_LINE_LENGTH-2*i, SSD1306_WHITE);
  }
}

void drawUpWarning() {
  for (int i = 0; i < WARNING_THICKNESS; i++) {
    display.drawFastHLine(X_CENTER-EDGE_WARNING_LINE_LENGTH/2+i, 2+i, EDGE_WARNING_LINE_LENGTH-2*i, SSD1306_WHITE);
  }
}

void drawDownWarning() {
  for (int i = 0; i < WARNING_THICKNESS; i++) {
    display.drawFastHLine(X_CENTER-EDGE_WARNING_LINE_LENGTH/2+i, SCREEN_HEIGHT-3-i, EDGE_WARNING_LINE_LENGTH-2*i, SSD1306_WHITE);
  }
}

void drawLeftWarning() {
  for (int i = 0; i < WARNING_THICKNESS; i++) {
    display.drawFastVLine(LEFT_EDGE+2+i, Y_CENTER-EDGE_WARNING_LINE_LENGTH/2+i, EDGE_WARNING_LINE_LENGTH-2*i, SSD1306_WHITE);
  }
}

void drawRightWarning() {
  for (int i = 0; i < WARNING_THICKNESS; i++) {
    display.drawFastVLine(RIGHT_EDGE-3-i, Y_CENTER-EDGE_WARNING_LINE_LENGTH/2+i, EDGE_WARNING_LINE_LENGTH-2*i, SSD1306_WHITE);
  }
}
