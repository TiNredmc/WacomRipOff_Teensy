/* Wacom Ripoff project ported to Teensy++ 2.0 
 * Ported by TinLethax 2021/07/26 +7
 */

#include <Wire.h>
#define TWI_FREQ 400000L // Define speed used by twi.c, 400KHz i2c

#define WACOM_ADDR      0x09 // Wacom i2c address (7-bit)

#define WACOM_CMD_QUERY0  0x04
#define WACOM_CMD_QUERY1  0x00
#define WACOM_CMD_QUERY2  0x33
#define WACOM_CMD_QUERY3  0x02
#define WACOM_CMD_THROW0  0x05
#define WACOM_CMD_THROW1  0x00
#define WACOM_QUERY_SIZE  19

#define WACOM_RST 3 // PD3 as output
#define WACOM_INT 2 // PD2 as Input interrupt

static uint8_t dataQ[WACOM_QUERY_SIZE];// data array, store the data received from w9013
static uint16_t Yinvert;// For Y axis inverting


#define LED_stat 6  // LED status on PD6
uint8_t hid_report[8]={2,0,0,0,0,0,0,0};// store HID report for sending via usb

//#define DEBUG_SER

// IRQ handler for w9013 incoming interrupt
void w9013_send(uint8_t *buf, size_t len){
  Wire.beginTransmission(WACOM_ADDR);// Begin transmission
  while(len--){
    Wire.write(*buf++);
  }
 Wire.endTransmission();// End transmission 
}

void w9013_read(uint8_t *buf, size_t len){
 Wire.requestFrom(WACOM_ADDR, len);
 while(len--){
  *buf++ = Wire.read();
 }
}

void w9013_irq(){

  w9013_read(dataQ, WACOM_QUERY_SIZE);
    digitalWrite(LED_stat, HIGH);
    
  switch(dataQ[3]){
  case 0x20: // Pen is in range (Windows determine as Tip is in range)
    hid_report[1] = 0x20;
    break;

  case 0x21: // Pen tip is pressing on surface
    hid_report[1] = 0x21;
    break;

  case 0x28: // Eraser is in range
    hid_report[1] = 0x22;// Set in-range and invert bit
    break;

  case 0x2c: // Erase is pressing on surface
    hid_report[1] = 0x2A;// Set in-rage bit and also invert and eraser bit.
    break;

  default:
    break;
  }

  hid_report[1] = hid_report[1] | ((dataQ[3] & 0x02 ? 1 : 0) << 2) | ((dataQ[3] & 0x10 ? 1 : 0) << 1);

    // hid_report[2] and hid_report[3] is for X position
  hid_report[2] = dataQ[6];// Send lower byte first
  hid_report[3] = dataQ[7];// Then send higher byte

    // hid_report[4] and hid_report[5] is for Y position
  Yinvert = dataQ[5] << 8 | dataQ[4];
  Yinvert = 0x344E - Yinvert;// Invert Y axis
  hid_report[4] = Yinvert;// Send lower byte first
  hid_report[5] = Yinvert >> 8;// Then send higher byte

    // hid_report[6] and hid_report[7] is for pressure
  hid_report[6] = dataQ[8];// Send lower byte first
  hid_report[7] = dataQ[9];// Then send higher byte

#ifdef DEBUG_SER
    Serial.println("HID Data:");
  for(uint8_t i; i < 8; i++){
    Serial.print("0x");
    Serial.println(hid_report[i], HEX);
  }
#else
  // Send USB report
  RawHID.send(hid_report, sizeof(hid_report));
#endif
    digitalWrite(LED_stat, LOW);
}

// w9013 initialization
uint8_t w9013_query_device(){
  uint8_t cmd1[4] = { WACOM_CMD_QUERY0, WACOM_CMD_QUERY1,
      WACOM_CMD_QUERY2, WACOM_CMD_QUERY3 };
  uint8_t cmd2[2] = { WACOM_CMD_THROW0, WACOM_CMD_THROW1 };
  uint16_t fwVer = 0;
  
  // Send 2 query CMDs
  w9013_send(cmd1, 4);
  w9013_send(cmd2, 2);
  // Receive the respond
  w9013_read(dataQ, WACOM_QUERY_SIZE);

#ifdef DEBUG_SER
  Serial.println("Query Data:");
  for(uint8_t i; i < WACOM_QUERY_SIZE; i++){
    Serial.print("0x");
    Serial.println(dataQ[i], HEX);
  }
#endif

  fwVer = dataQ[13] | dataQ[14] << 8;// parsing firmware version 
  if(!fwVer){// If FWversion is 0
     return 1; // error happened.
  }
  return 0;// nomal 
}
void setup() {
  // put your setup code here, to run once:
#ifdef DEBUG_SER
  Serial.begin(9600);
#endif
  Wire.begin();// Start I2C bus
  pinMode(WACOM_RST, OUTPUT);
  pinMode(WACOM_INT, INPUT_PULLUP);
  pinMode(LED_stat, OUTPUT);

  //attachInterrupt(WACOM_INT, w9013_irq, FALLING);// Enable interrupt

  digitalWrite(LED_stat, HIGH);
  digitalWrite(WACOM_RST, LOW);
  delay(500);
  digitalWrite(LED_stat, LOW);
  digitalWrite(WACOM_RST, HIGH);
  
    while(w9013_query_device()){// fast blink while trying to probe the w9013
      digitalWrite(LED_stat, HIGH);
      delay(100);
      digitalWrite(LED_stat, LOW);
      delay(100);
      }

}

void loop() {
  // put your main code here, to run repeatedly:
  if(digitalRead(WACOM_INT) == LOW){

  do{
  w9013_read(dataQ, WACOM_QUERY_SIZE);
  }while(digitalRead(WACOM_INT) == LOW);
  
    digitalWrite(LED_stat, HIGH);
    
  switch(dataQ[3]){
  case 0x20: // Pen is in range (Windows determine as Tip is in range)
    hid_report[1] = 0x20;
    break;

  case 0x21: // Pen tip is pressing on surface
    hid_report[1] = 0x21;
    break;

  case 0x28: // Eraser is in range
    hid_report[1] = 0x22;// Set in-range and invert bit
    break;

  case 0x2c: // Erase is pressing on surface
    hid_report[1] = 0x2A;// Set in-rage bit and also invert and eraser bit.
    break;

  default:
    break;
  }

  hid_report[1] = hid_report[1] | ((dataQ[3] & 0x02 ? 1 : 0) << 2) | ((dataQ[3] & 0x10 ? 1 : 0) << 1);

    // hid_report[2] and hid_report[3] is for X position
  hid_report[2] = dataQ[6];// Send lower byte first
  hid_report[3] = dataQ[7];// Then send higher byte

    // hid_report[4] and hid_report[5] is for Y position
  Yinvert = dataQ[5] << 8 | dataQ[4];
  Yinvert = 0x344E - Yinvert;// Invert Y axis
  hid_report[4] = Yinvert;// Send lower byte first
  hid_report[5] = Yinvert >> 8;// Then send higher byte

    // hid_report[6] and hid_report[7] is for pressure
  hid_report[6] = dataQ[8];// Send lower byte first
  hid_report[7] = dataQ[9];// Then send higher byte

#ifdef DEBUG_SER
    Serial.println("HID Data:");
  for(uint8_t i; i < 8; i++){
    Serial.print("0x");
    Serial.println(hid_report[i], HEX);
  }
#else
  // Send USB report
  RawHID.send(&hid_report, 1);
#endif
    digitalWrite(LED_stat, LOW);
    
    
  }
}
