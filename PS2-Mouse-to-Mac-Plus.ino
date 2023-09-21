/* PS2 Mouse to Mac Plus
 * 
 * This is code for an adapter that converts PS2 mouse signals to Mac Plus serial mouse.
 * 
 * This is a fork of glitterkitty's PS2 to Amiga project which can be found at at 
 *   https://github.com/glitterkitty/Arduino-PS2-Mouse-to-Amiga-Adapter
 *   
 * Which uses code from Spark cssvb9'4s USB mouse to Amiga adapter which can be found at
 *   https://github.com/glitterkitty/Arduino-PS2-Mouse-to-Amiga-Adapter
 * 
 * This project requires the PS2Mouse library available at
 *   http://github.com/kristopher/PS2-Mouse-Arduino/
 * 
 * This project is designed to be used with the ATtiny806. To compile and use this code 
 * with the Arduino IDE for the ATtiny806 you will need to install megaTinyCore from SpenceKonde:
 *   https://github.com/SpenceKonde/megaTinyCore
 * 
 * The PCB this project was designed for, along with a BOM for the PCB, is available through OSHPark at:
 *   https://oshpark.com/shared_projects/cSaM9jY9
 */

#include <PS2Mouse.h>

//#define SERIAL_DEBUG

// pins from mac-port to arduino 
// https://deskthority.net/wiki/Bus_mouse
//
#define V_PULSE     PIN_PC3    // A:1  M:9 (Y1)       18
#define VQ_PLSE     PIN_PC1    // A:3  M:8 (Y2)       16
#define H_PULSE     PIN_PC2    // A:2  M:4 (X1)       17
#define HQ_PLSE     PIN_PA1    // A:4  M:5 (X2)       20

#define LMB         PIN_PC0    // A:6  M:7 (SW-)      15
#define RMB         PIN_PB0    // A:9  M:7 (unused)   14
#define MMB         PIN_PB1    // A:5  M:7 (unused)   13

#define MOUSE_DATA  PIN_PB5
#define MOUSE_CLOCK PIN_PB4

#define ADELAY      150

PS2Mouse mouse(MOUSE_CLOCK, MOUSE_DATA, STREAM);

// quadrature encoding used by the amiga mouse protocol
//
uint8_t H[4]  = { LOW, LOW, HIGH, HIGH};
uint8_t HQ[4] = { LOW, HIGH, HIGH, LOW};

uint8_t QX = 3;
uint8_t QY = 3;

uint8_t XSTEPS;
uint8_t YSTEPS;
uint8_t XSIGN;
uint8_t YSIGN;

// MB handling
//
void LeftButtonUp() {
  #ifdef SERIAL_DEBUG
    Serial.println("LeftButtonUp()");
  #endif
  digitalWrite(LMB, HIGH);
}
void LeftButtonDown() {
  #ifdef SERIAL_DEBUG
    Serial.println("LeftButtonDown()");
  #endif
  digitalWrite(LMB, LOW);
}
void RightButtonUp() {
  #ifdef SERIAL_DEBUG
    Serial.println("RightButtonUp()");
  #endif
  digitalWrite(RMB, HIGH);
}
void RightButtonDown() {
  #ifdef SERIAL_DEBUG
    Serial.println("RightButtonDown()");
  #endif
  digitalWrite(RMB, LOW);
}
void MiddleButtonUp() {
  #ifdef SERIAL_DEBUG
    Serial.println("MiddleButtonUp()");
  #endif
  digitalWrite(MMB, HIGH);
}
void MiddleButtonDown() {
  #ifdef SERIAL_DEBUG
    Serial.println("MiddleButtonDown()");
  #endif
  digitalWrite(MMB, LOW);
}

// x/y handling
//
void MOUSEHorizontalMove() {
  // set bits acc. to curr. position in quadr. sequence
  digitalWrite(H_PULSE, H[QX]);
  digitalWrite(HQ_PLSE, HQ[QX]);

  delayMicroseconds(ADELAY);
}

void MOUSEVerticalMove() {
  digitalWrite(V_PULSE, H[QY]);
  digitalWrite(VQ_PLSE, HQ[QY]);

  delayMicroseconds(ADELAY);
}

void MOUSE_Left() {
  #ifdef SERIAL_DEBUG
    Serial.print("<");
  #endif

  // do a move by setting the port
  MOUSEHorizontalMove();

  // advance in the quadr. sequence
  QX = (QX >= 3) ? 0 : QX + 1;
}

void MOUSE_Right() {
  #ifdef SERIAL_DEBUG
    Serial.print(">");
  #endif
  MOUSEHorizontalMove();
  QX = (QX <= 0) ? 3 : QX - 1;
}

void MOUSE_Down() {
  #ifdef SERIAL_DEBUG
    Serial.print("_");
  #endif
  MOUSEVerticalMove();
  QY = QY <= 0 ? 3 : QY - 1;
}

void MOUSE_Up() {
  #ifdef SERIAL_DEBUG
    Serial.print("^");
  #endif
  MOUSEVerticalMove();
  QY = QY >= 3 ? 0 : QY + 1;
}

void setup() {

  #ifdef SERIAL_DEBUG
    // give me some time to attach the pogo adapter before you start
    delay(2000);

    // initialize serial debug
    Serial.begin(115200);
    Serial.println("Hi, I'm a PS2 mouse adapter! \nConnect");
  #endif

  // i have found through trial-and-error that a delay is needed prior to mouse initialization
  delay(1000);

  // init ps2 connection
  mouse.initialize();

  // Set button and quadrature output pins to output
  pinMode(V_PULSE, OUTPUT);  // V-Pulse
  pinMode(H_PULSE, OUTPUT);  // H-Pulse
  pinMode(VQ_PLSE, OUTPUT);  // VQ-pulse
  pinMode(HQ_PLSE, OUTPUT);  // HQ-pulse
  pinMode(LMB, OUTPUT);  // LMB
  pinMode(RMB, OUTPUT);  // RMB
  pinMode(MMB, OUTPUT);  // MMB

  // Set quadrature output pins to zero
  digitalWrite(V_PULSE, LOW);
  digitalWrite(H_PULSE, LOW);
  digitalWrite(VQ_PLSE, LOW);
  digitalWrite(HQ_PLSE, LOW);

  // Set mouse button output pins to on, coz they are inverted
  digitalWrite(LMB, HIGH);
  digitalWrite(RMB, HIGH);
  digitalWrite(MMB, HIGH);
  
  delay(500);
}

void loop() {

  static int16_t data[3] = {0, 0, 0};
  static int16_t last_data[3] = {0, 0, 0};

  // get data from mouse
  mouse.report(data);

  // handle buttons
  if ((data[0] & 1) != (last_data[0] & 1)) {
    if (data[0] & 1) 
      LeftButtonDown();
    else
      LeftButtonUp();
  }

  if ((data[0] & 2) != (last_data[0] & 2)) {
    if (data[0] & 2) 
      RightButtonDown();
    else
      RightButtonUp();
  }

  if ((data[0] & 4) != (last_data[0] & 4)) {
    if (data[0] & 4) 
      MiddleButtonDown();
    else
      MiddleButtonUp();
  }

  // calc x/y movement
  XSTEPS = abs(data[1]);
  YSTEPS = abs(data[2]);
  XSIGN = (data[1] > 0 ? 1 : 0) ;
  YSIGN = (data[2] > 0 ? 1 : 0) ;

  // handle x/y movement 
  while ((XSTEPS | YSTEPS) != 0) {

    // steps left?
    if (XSTEPS != 0) {
    
      // direction
      if (XSIGN)
        MOUSE_Right();
      else
        MOUSE_Left();

      // decrease steps    
      XSTEPS--;
    }

    if (YSTEPS != 0) {

      if (YSIGN)
        MOUSE_Up(); 
      else
        MOUSE_Down();
      YSTEPS--;
    }
  }

  last_data[0] = data[0];
  last_data[1] = data[1];
  last_data[2] = data[2];
}
