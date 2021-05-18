// The following OLED setup taken from the ssd1306_128x64_i2c library example
// --------------------------------------------------------------------------
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Shape.hpp>;
#include <ParallaxJoystick.hpp>;
#include <PushButton.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Frequencies of notes (in Hz)
// From: https://en.wikipedia.org/wiki/Piano_key_frequencies
#define KEY_C 262  // 261.6256 Hz (middle C)
#define KEY_D 294  // 293.6648 Hz
#define KEY_E 330  // 329.6276 Hz
#define KEY_F 350  // 349.2282 Hz
#define KEY_G 392  // 391.9954 Hz

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// --------------------------------------------------------------------------

// Speaker constants
const int OUTPUT_PIEZO_PIN = 12;

// Button constants
const int SHIELD_BUTTON_PIN = 10;

// Vibromotor constants
const int VIBROMOTOR_OUTPUT_PIN = 6;
const int MAX_ANALOG_OUT = 255;

// Earthquake constants
const int WARN_END = 500;
const int QUAKE_END = 800;

// Joystick constants
const int JOYSTICK_UPDOWN_PIN = A1;
const int JOYSTICK_LEFTRIGHT_PIN = A0;
const int MAX_ANALOG_VAL = 1023;
const enum JoystickYDirection JOYSTICK_Y_DIR = RIGHT;
const int UPPER_CUTOFF = 650;
const int LOWER_CUTOFF = 350;

// Screen and warning constants
const int X_CENTER = SCREEN_WIDTH / 2;
const int Y_CENTER = SCREEN_HEIGHT / 2;
const int LEFT_EDGE = X_CENTER - SCREEN_HEIGHT / 2;
const int RIGHT_EDGE = X_CENTER + SCREEN_HEIGHT / 2;
const int CORNER_WARNING_LINE_LENGTH = 14;
const int EDGE_WARNING_LINE_LENGTH = 18;
const int WARNING_THICKNESS = 4;

// Attack animation constants
const int ATTACK_COUNT = 4;
const int BLINK_END = 300;
const int PAUSE_END = 500;
const int ATTACK_END = 800;
const int ATTACK_EDGE_LENGTH = 30;

// Rogue position constants
const int R_OFFSET = SCREEN_HEIGHT / 4;
const int R_PARTIAL_OFFSET = (R_OFFSET * 7) / 10; // approximating 0.7x for sqrt 2 for pythagorean theorem

enum GAME_STATE {
  START,
  PLAYSTART,
  PLAY,
  GAMEOVER,
  HELP
};

ParallaxJoystick _analogJoystick(JOYSTICK_UPDOWN_PIN, JOYSTICK_LEFTRIGHT_PIN, MAX_ANALOG_VAL, JOYSTICK_Y_DIR);
PushButton _shieldButton(SHIELD_BUTTON_PIN);

GAME_STATE _curState = START;

/* This array tracks the states of each of the 4 possible line attacks
   Value: ATTACK_LENGTH+ inactive, 0 to BLINK_END blinking, BLINK_END to PAUSE_END pause, PAUSE_END to
      STRIKE_END striking
   Position: 0 horizontal, 1 vertical, 2 diagonal top-left to bottom-right, 3 diagonal bottom-left to 
      top-right
*/
int _attackStates[ATTACK_COUNT];
int _earthquakeState;
int _score;

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

  pinMode(VIBROMOTOR_OUTPUT_PIN, OUTPUT);
  pinMode(OUTPUT_PIEZO_PIN, OUTPUT);
  pinMode(SHIELD_BUTTON_PIN, INPUT_PULLUP);
  
  gameReset();
  display.clearDisplay();
}

void loop() {
  display.clearDisplay();

  if (_curState == PLAY) {
    playScreen();
  } else if (_curState == PLAYSTART) {
    playstartScreen();
  } else if (_curState == START) {
    startScreen();
  } else if (_curState == GAMEOVER) {
    gameoverScreen();
  } else if (_curState == HELP) {
    helpScreen();
  }
  
  display.display();
}

void startScreen() {
  int16_t x1, y1, x2, y2;
  uint16_t textWidth1, textHeight1, textWidth2, textHeight2;
  char title1[] = "ROGUE";
  char title2[] = "& SEER";

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.getTextBounds(title1, 0, 0, &x1, &y1, &textWidth1, &textHeight1);
  display.getTextBounds(title2, 0, 0, &x2, &y2, &textWidth2, &textHeight2);

  display.setCursor(SCREEN_WIDTH / 2 - textWidth2 / 2, SCREEN_HEIGHT / 2 - textHeight2 / 2);
  display.print(title2);
  display.setCursor(SCREEN_WIDTH / 2 - textWidth1 / 2, SCREEN_HEIGHT / 2 - textHeight2 / 2 - textHeight1);
  display.println(title1);

  drawBottomRightText("Start");

  if (!_shieldButton.isActive()) {
    _curState = PLAYSTART;
  }
}

void playstartScreen() {
  _score = 0;
  display.drawRect(LEFT_EDGE, 0, RIGHT_EDGE - LEFT_EDGE, SCREEN_HEIGHT, SSD1306_WHITE); // draw game border
  display.display();
  delay(500);
  for (int i = 0; i < 3; i++) {
    display.clearDisplay();
    drawRogue(X_CENTER, Y_CENTER, true);
    display.drawRect(LEFT_EDGE, 0, RIGHT_EDGE - LEFT_EDGE, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    tone(OUTPUT_PIEZO_PIN, KEY_E);
    delay(500);
    display.clearDisplay();
    display.drawRect(LEFT_EDGE, 0, RIGHT_EDGE - LEFT_EDGE, SCREEN_HEIGHT, SSD1306_WHITE);
    display.display();
    noTone(OUTPUT_PIEZO_PIN);
    delay(500);
  }
  display.clearDisplay();
  drawRogue(X_CENTER, Y_CENTER, true);
  display.drawRect(LEFT_EDGE, 0, RIGHT_EDGE - LEFT_EDGE, SCREEN_HEIGHT, SSD1306_WHITE);
  drawScore();
  display.display();
  delay(1000);
  _curState = PLAY;
}

void playScreen() {
  display.drawRect(LEFT_EDGE, 0, RIGHT_EDGE - LEFT_EDGE, SCREEN_HEIGHT, SSD1306_WHITE); // draw game border
  
  int i = 0;
  while (i <= 3) {
    if (_attackStates[i] < ATTACK_END) {
      updateAttack(i, 10);
      break;
    }
    i++;
  }
  if (_earthquakeState > QUAKE_END) {
    i++;
  }
  if (i == 5) { // no attacks active
    int j = random(0, 5);
    if (j == 4) {
      _earthquakeState = 0;
    } else {
      _attackStates[j] = 0;
    }
  }

  _analogJoystick.read();
  int rogueUpDown = 0;
  int rogueLeftRight = 0;
  int upDownVal = _analogJoystick.getUpDownVal();
  int leftRightVal = _analogJoystick.getLeftRightVal();
  if (upDownVal > UPPER_CUTOFF) {
    rogueUpDown = 1;
  } else if (upDownVal < LOWER_CUTOFF) {
    rogueUpDown = -1;
  }
  if (leftRightVal > UPPER_CUTOFF) {
    rogueLeftRight = 1;
  } else if (leftRightVal < LOWER_CUTOFF) {
    rogueLeftRight = -1;
  }
  updateEarthquake(10);
  if (!_shieldButton.isActive()) {
    drawShield();
  }
  _score++;
  drawScore();
  updateRogue(rogueUpDown, rogueLeftRight);
}

void helpScreen() {
  
}

void gameoverScreen() {
  int16_t x1, y1, x2, y2;
  uint16_t textWidth1, textHeight1, textWidth2, textHeight2;
  char title1[] = "GAME";
  char title2[] = "OVER";

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.getTextBounds(title1, 0, 0, &x1, &y1, &textWidth1, &textHeight1);
  display.getTextBounds(title2, 0, 0, &x2, &y2, &textWidth2, &textHeight2);

  display.setCursor(SCREEN_WIDTH / 2 - textWidth2 / 2, SCREEN_HEIGHT / 2 - textHeight2 / 2);
  display.print(title2);
  display.setCursor(SCREEN_WIDTH / 2 - textWidth1 / 2, SCREEN_HEIGHT / 2 - textHeight2 / 2 - textHeight1);
  display.println(title1);

  drawScore();
  drawBottomRightText("Return to Menu");
  
  if (!_shieldButton.isActive()) {
    _curState = START;
    delay(500);
  }
}

void updateAttack(int direction, int speed) {
  int pos = _attackStates[direction];
  _attackStates[direction] += speed;
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
}

void updateEarthquake(int speed) {
  int vibrationLevel = 0;
  if (_earthquakeState < WARN_END) {
    vibrationLevel = map(_earthquakeState, 0, WARN_END, MAX_ANALOG_OUT / 2, (MAX_ANALOG_OUT * 3) / 4);
  } else if ( _earthquakeState < QUAKE_END) {
    vibrationLevel = MAX_ANALOG_OUT;
  }
  Serial.print(_earthquakeState);
  Serial.print(" ");
  Serial.println(vibrationLevel);
  analogWrite(VIBROMOTOR_OUTPUT_PIN, vibrationLevel);
  _earthquakeState += speed;
}

void updateRogue(int upDown, int leftRight) {
  int xPos = X_CENTER;
  int yPos = Y_CENTER;
  bool alive = true;
  
  // Calculate rogue x position
  if (leftRight == 1) { // go right
    if (upDown == 0) {
      xPos = X_CENTER + R_OFFSET; 
    } else {
      xPos = X_CENTER + R_PARTIAL_OFFSET;
    }
  } else if (leftRight == -1) { // go left
    if (upDown == 0) {
      xPos = X_CENTER - R_OFFSET; 
    } else {
      xPos = X_CENTER - R_PARTIAL_OFFSET;
    }
  }

  // Calculate rogue y position
  if (upDown == 1) { // go up
    if (leftRight == 0) {
      yPos = Y_CENTER - R_OFFSET; 
    } else {
      yPos = Y_CENTER - R_PARTIAL_OFFSET;
    }
  } else if (upDown == -1) { // go down
    if (leftRight == 0) {
      yPos = Y_CENTER + R_OFFSET; 
    } else {
      yPos = Y_CENTER + R_PARTIAL_OFFSET;
    }
  }

  // Check whether rogue has been killed
  int attackActive[ATTACK_COUNT];
  for (int i = 0; i < ATTACK_COUNT; i++) {
    attackActive[i] = (_attackStates[i] > PAUSE_END + ATTACK_EDGE_LENGTH) && (_attackStates[i] < ATTACK_END - ATTACK_EDGE_LENGTH);
  }
  if ((upDown == 0 && attackActive[0]) || (leftRight == 0 && attackActive[1]) ||
      (leftRight == -upDown && attackActive[2]) || (leftRight == upDown && attackActive[3])) {
        alive = false;
  }
  if (_earthquakeState > WARN_END && _earthquakeState < QUAKE_END) {
    if (_shieldButton.isActive() || upDown != 0 || leftRight != 0) {
      alive = false;
    }
  }

  drawRogue(xPos, yPos, alive);
  if (!alive) {
    _curState = GAMEOVER;
    display.display();
    tone(OUTPUT_PIEZO_PIN, KEY_E);
    delay(500);
    tone(OUTPUT_PIEZO_PIN, KEY_D);
    delay(500);
    tone(OUTPUT_PIEZO_PIN, KEY_C);
    delay(1000);
    gameReset();
  }
}

void gameReset() {
  for (int i = 0; i<ATTACK_COUNT; i++) {
    _attackStates[i] = 1000;
  }
  _earthquakeState = 1000;
  analogWrite(VIBROMOTOR_OUTPUT_PIN, 0);
  noTone(OUTPUT_PIEZO_PIN);
}

void drawScore() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.setCursor(0,0);
  display.println("Score");
  display.println((_score / 10) * 100);
}

void drawBottomRightText(char text[]) {
  int16_t x, y;
  uint16_t textWidth, textHeight;

  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.getTextBounds(text, 0, 0, &x, &y, &textWidth, &textHeight);

  display.setCursor(SCREEN_WIDTH - textWidth, SCREEN_HEIGHT - textHeight);
  display.print(text);
}

void drawShield() {
  display.drawCircle(X_CENTER, Y_CENTER, (R_OFFSET * 3) / 4, SSD1306_WHITE);
}

void drawRogue(int x, int y, bool alive) {
  if (alive) {
    display.fillRect(x - 5, y - 5, 10, 10, SSD1306_WHITE);
  } else {
    display.drawRect(x - 5, y - 5, 10, 10, SSD1306_WHITE);
  }
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
