/*   _________________________________________
 *  |                                         |_
 *  |         Fingerprint Padlock             | |
 *  |           Operating System              | |
 *  |-----------------------------------------| |
 *  |        Credit: @friansh 2020            | |
 *  |_________________________________________| |
 *    |_________________________________________|                                       
 */

#include <Adafruit_Fingerprint.h>
#include <Servo.h>

// pin #2 is IN from sensor     (GREEN wire)
// pin #3 is OUT from arduino   (WHITE wire)

#define pin_fingerprint_rx      2
#define pin_fingerprint_tx      3
#define pin_servo               5
#define pin_led_green           6
#define pin_led_red             7
#define pin_servo_power         10
#define pin_led                 13

SoftwareSerial fingerprintReader(pin_fingerprint_rx, pin_fingerprint_tx); //RX, TX
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerprintReader);
Servo lock;

// Declaring variable for general purpose
String  serialData;
uint8_t state       = 0;
uint8_t id          = 0;

/* This sourcw code state list
 *  
 *  State 0: ready to receive all kinds of commands
 *           and scanning for biometric
 *  State 1: ready to enroll user data
 *  State 2: enroll fingerprint
 *  State 3: scan fingerprint
 *  State 4: scan finngerprint debug
 *  State 5: ready to delete user data
 *  State 6: delete fingerprint
 *  STate 7: check for fingerprint device
 */

#define STATE_MAIN_MENU         0
#define STATE_PREPARE_ENROLL    1
#define STATE_ENROLL            2
#define STATE_SCAN              3
#define STATE_SCAN_DEBUG        4
#define STATE_PREPARE_DELETE    5
#define STATE_DELETE            6
#define STATE_CHECK_MODULE      7
#define STATE_PREPARE_CLEAN     8
#define STATE_CLEAN             9
 
void setup() {
  pinMode( pin_led, OUTPUT );
  pinMode( pin_led_green, OUTPUT );
  pinMode( pin_led_red, OUTPUT );
  
  digitalWrite ( pin_led_red, HIGH );
  
  Serial.begin( 9600 );
  finger.begin( 57600 );
  lock.attach( pin_servo );
  lock.write(0);
  if ( !finger.verifyPassword() ) {
//    while (1){
      digitalWrite( pin_led, 1 ); delay( 100 );
      digitalWrite( pin_led, 0 ); delay( 100 );
//    }
  }

//  Serial.print("Set password... ");
//  uint8_t p = finger.setPassword(140910);
//  if (p == FINGERPRINT_OK) {
//    Serial.println("OK"); // Password is set
//  } else {
//    Serial.println("ERROR"); // Failed to set password
//  }
}

void loop() {   
  if ( state == STATE_MAIN_MENU ) {
    if ( finger.verifyPassword() ) 
      if ( getFingerprintIDez() > 0 ) {
        digitalWrite( pin_led_green, HIGH ); digitalWrite( pin_led_red, LOW );
        sweep();
        digitalWrite( pin_led_green, LOW ); digitalWrite( pin_led_red, HIGH );
      }
      
    while( Serial.available() ){
      serialData = Serial.readString();
      serialData.trim();
      
      if ( serialData == "who are you?" ){
        Serial.println("aingmacan;)");
      } else if ( serialData == "register" ) {
        Serial.println("Biometric registration. Enter the ID you want to add:");
        state = STATE_PREPARE_ENROLL;
      } else if ( serialData == "scan" ) {
        Serial.println("Entering biometric scan debug mode.");
        state = STATE_SCAN_DEBUG;
      } else if ( serialData == "delete" ) {
        Serial.println("Biometric deletion. Enter the ID you want to delete:");
        state = STATE_PREPARE_DELETE;
      } else if ( serialData == "status" ) {
        state = STATE_CHECK_MODULE;
      } else if ( serialData == "clean" ) {
        Serial.println("Are you sure want to clean the database? (y/n)");
        state = STATE_PREPARE_CLEAN;
      } else {
        Serial.println("I don't understand what are you saying.");
      }
    }
    delay(50);
  } else if ( state == STATE_PREPARE_ENROLL ) {
    while( Serial.available() ){
      serialData = Serial.readString();
      serialData.trim();
      id = serialData.toInt();
      
      if ( id == 0 ){
        Serial.println("Id ngga boleh nol sayangg :*");
      } else {
        Serial.println("Registration request received.");
        state = STATE_ENROLL;
      }
    }
  } else if ( state == STATE_ENROLL ) {
    Serial.println("Registering ID #" + String(id) + "...");
    if ( !getFingerprintEnroll() ) {
      delay(1000);
      Serial.println("Done!");
    } else
      Serial.println("Failed, ready to try again.");
    id    = 0;
    state = STATE_MAIN_MENU;
  } else if ( state == STATE_SCAN_DEBUG ) {
    getFingerprintID();
    delay(50);
  } else if ( state == STATE_PREPARE_DELETE ) {
    while( Serial.available() ){
      serialData = Serial.readString();
      serialData.trim();
      id = serialData.toInt();
      
      if ( id == 0 ){
        Serial.println("Id ngga boleh nol sayangg :*");
      } else {
        Serial.println("Deletion request received.");
        state = 6;
      }
    }
  } else if ( state == STATE_DELETE ) {
    Serial.println("Deleting ID #" + String(id) + "...");
    if ( deleteFingerprint(id) == FINGERPRINT_OK )
          Serial.println("Done!");
    else
      Serial.println("Failed, ready to try again.");
    id    = 0;
    state = STATE_MAIN_MENU;
    Serial.println("Done!");
  } else if ( state == STATE_CHECK_MODULE ) {
    if (finger.verifyPassword()) {
      finger.getTemplateCount();
      Serial.println("Fingerprint sensor detected with " + String(finger.templateCount) + " fingerprint template(s).");
    } else {
      Serial.println("Did not find fingerprint sensor :(");
    }
    state = STATE_MAIN_MENU;
  } else if ( state == STATE_PREPARE_CLEAN ) {
    while( Serial.available() ){
      serialData = Serial.readString();
      serialData.trim();
      
      if ( serialData == "y" or serialData == "Y" ) {
        state = STATE_CLEAN;
      } else if ( serialData == "n" or serialData == "N" ) {
        Serial.println("Database cleaning canceled.");
        state = STATE_MAIN_MENU;
      } else {
        Serial.println("The input you have entered is invalid, please enter the valid response (y/n).");
      }
    }
  } else if ( state == STATE_CLEAN ) {
    finger.emptyDatabase();
    state = STATE_MAIN_MENU;
    Serial.println("Fingerprint database cleaned.");
  }
}

void sweep() {
  lock.write(180);
  delay(3000);
  lock.write(0);
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence); 

  return finger.fingerID;
}

uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;
  
  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID; 
}

// Fingerprint enrollment function
uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.println("Waiting for valid finger to enroll as #" + String(id));
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
//    case FINGERPRINT_NOFINGER:
//      Serial.println(".");
//      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
//    default:
//      Serial.println("Unknown error");
//      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
//    case FINGERPRINT_NOFINGER:
//      Serial.print(".");
//      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
//    default:
//      Serial.println("Unknown error");
//      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
}
