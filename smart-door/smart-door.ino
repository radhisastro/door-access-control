#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <XBee.h>
#include <EEPROM.h>

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

XBee xbee = XBee();
XBeeAddress64 addr64 = XBeeAddress64(0x0013a200, 0x40c8ccaf);
ZBTxStatusResponse txStatus = ZBTxStatusResponse();

#define lockPin 4
#define redLed 6
#define greenLed 7
#define blueLed 8
#define memBase 0
#define eepromSize 64  // WARNING! Don't exceed EEPROM size of Arduino.

uint8_t masterMode = 0;  
uint32_t masterModeCounter = 0;
uint8_t masterCard[4] = {226, 234, 92, 116};  // HARDCODED
uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID, 7 bit max
uint8_t uidLength;      

uint8_t doorStatus[1];
uint8_t data[5];  // 4 bytes for UID, 1 byte for door status

uint8_t success;  // success reading tag
uint8_t value;  // door condition
uint8_t match = false; // initialize card match to false

/////////////////////////// Setup ///////////////////////////
void setup() {
  // Arduino Pin Configuration
  pinMode(lockPin,OUTPUT);
  pinMode(redLed,OUTPUT);
  pinMode(greenLed,OUTPUT);
  pinMode(blueLed,OUTPUT);
  digitalWrite(lockPin,LOW);  
  digitalWrite(redLed,LOW);
  digitalWrite(greenLed,LOW);
  digitalWrite(blueLed,LOW); 
 
  // Protocol Configuration
  Serial.begin(9600);
  xbee.setSerial(Serial);
  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    flashLed(redLed, 3, 100);
    while (1); // halt
  }
  
  nfc.SAMConfig();
  nfc.setPassiveActivationRetries(0xFF);

//  Uncomment initializeEeprom() below to erase all EEPROM
//  initializeEeprom();
  printEeprom();
  
  Serial.println("Setup begins, please wait...");
  delay(500);
}

/////////////////////////// Main Loop ///////////////////////////
void loop() {
  // If it's in NORMAL MODE and door is locked, blue LED is on.
  if((masterMode == 0) & (digitalRead(lockPin) == 0)) {  
    digitalWrite(blueLed,HIGH);
  }
  
  // If it's in NORMAL MODE and door is not locked, blue LED flashes.  
  if((masterMode == 0) & (digitalRead(lockPin) == 1)) {
    flashLed(blueLed, 2, 500);
  }

  //  If it's in MASTER MODE, start 5 times counter,
  //  otherwise go back to NORMAL MODE.
  if(masterMode == 1) {
    digitalWrite(blueLed,LOW);
    digitalWrite(redLed,LOW);
    digitalWrite(greenLed,LOW);
    flashLed(greenLed, 2, 400);
    masterModeCounter++;
    if(masterModeCounter >= 5) {
      masterMode = 0;
      masterModeCounter = 0;
    }
    Serial.println("MASTER MODE");
  }

  //  Waiting for a tag
  delay(1000);
  Serial.println("\nWaiting for a tag...");

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  //  A tag is detected
  if(success) {
    Serial.println("\nA tag is detected");
    Serial.print("UID : ");
    for(int i=0; i<uidLength; i++) {
    Serial.print(uid[i],HEX);
    Serial.print(" ");
    }
    
    Serial.println("");
    
    //  If tag is master card,
    //  enter MASTER MODE as masterMode=1 and
    //  start counter from beginning
    if(isMaster(uid)) {  
      masterMode = 1;
      masterModeCounter = 0;
     }
     
    else {
      int32_t findUid = findUidInEeprom(uidLength,uid);
      //  Enter MASTER MODE
      if(masterMode == 1) {
        //  If UID found, delete it
        if(findUid != -1) {  
          deleteUidfromEeprom(findUid, uidLength);
          flashLed(redLed,1,200);
          flashLed(greenLed,1,200);
          flashLed(blueLed,1,200);          
        }
        //  If UID not found, add it
        else if (findUid == -1) {  
          int32_t storageAddress = getEepromStorageAddress(uidLength);
          if(storageAddress >= 0) {
            writeUidToEeprom(storageAddress, uidLength, uid);
            flashLed(redLed,2,100);
            flashLed(greenLed,2,100);
            flashLed(blueLed,2,100);             
          }
          else {
            Serial.println("WARNING!!! memory full or error");
          }
        }
        masterMode = 0;  // Change to NORMAL MODE
      }
      
      //  Enter NORMAL MODE
      else {  
        //  If UID found, grant access
        if(findUid != -1) {  
          lockUnlock();
          Serial.println("Access granted...");
          flashLed(greenLed,2,200);        
          makeData();
          sendData();
          delay(200);
        }
        //  If UID not found, deny access
        else {  
          Serial.println("Access denied...");
          flashLed(redLed,2,200);
          Serial.println("");
          delay(500);          
        }
      }
    }
  }
}

/////////////////////////// Check if UID is masterCard ///////////////////////////
boolean isMaster(byte test[]) {
  if(checkTwo(test, masterCard)) {
    return true;
  }
  else {
    return false;
  }
}

/////////////////////////// Check bytes ///////////////////////////
boolean checkTwo (byte a[], byte b[]) {
  if (a[0] != 0) // Make sure there is something in the array first
    match = true; // Assume they match at first
  for (int k=0; k<sizeof(masterCard); k++) { // Loop 4 times
    if (a[k] != b[k]) // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if (match) { // Check to see if if match is still true
    return true; // Return true
  }
  else  {
    return false; // Return false
  }
}

/////////////////////////// Lock or Unlock Door ///////////////////////////
void lockUnlock(){
  value = digitalRead(lockPin);
  value =! value; 
  if(value == 1) {
    doorStatus[0] = {0xAA};  // door is open
  }
  else if(value == 0) {
    doorStatus[0] = {0xBB};  // door is closed
  }  
  digitalWrite(lockPin,value);
  delay(500);
}

/////////////////////////// Make Data ///////////////////////////
void makeData() {
  for(int i=0; i<uidLength; i++) {
    data[i] = uid[i];
  }  
  data[uidLength] = doorStatus[0];
  Serial.print("\nData Payload:");
  for(int i=0; i<=uidLength; i++) {
    Serial.print(data[i],HEX);
    Serial.print(" ");
  }
  Serial.println("");
}

/////////////////////////// Send Data ///////////////////////////
void sendData() {
  ZBTxRequest zbTx = ZBTxRequest(addr64, data, sizeof(data));
  xbee.send(zbTx);
  Serial.println("");

  // after sending a tx request, we expect a status response
  // wait up to half second for the status response
  if (xbee.readPacket(500)) {
    // got a response!
    Serial.println("Packet received");
    if (xbee.getResponse().isAvailable()) {
      Serial.print("xbee.getResponse().isAvailable(): ");
      Serial.println(xbee.getResponse().isAvailable());
    }
     
     // got something
     // should be a znet tx status             
    if (xbee.getResponse().getApiId() == ZB_TX_STATUS_RESPONSE) {
      xbee.getResponse().getZBTxStatusResponse(txStatus);
      Serial.print("xbee.getResponse().getApiId(): ");
      Serial.println(xbee.getResponse().getApiId());
      Serial.print("ZB_TX_STATUS_RESPONSE: ");
      Serial.println(ZB_TX_STATUS_RESPONSE);
          
      if (txStatus.getDeliveryStatus() == SUCCESS) {
        Serial.println("\nSuccess. Time to celebrate");     
      } 
      else {
        Serial.println("\nThe remote XBee did not receive our packet. Maybe it powered off"); 
      }
    }
  }
  
  else if (xbee.getResponse().isError()) {
    Serial.print("\nError reading packet.  Error code: ");  
    Serial.println(xbee.getResponse().getErrorCode());
  }
  
  else {
    // local XBee did not provide a timely TX Status Response -- should not happen
    Serial.println("\nLocal XBee did not provide a timely TX Status Response"); 
  }
}

/////////////////////////// Flash the LEDs ///////////////////////////
void flashLed(int pin, int times, int wait) {
  digitalWrite(blueLed, LOW);
  for (int i=0; i<times; i++) {
    digitalWrite(pin, HIGH);
    delay(wait);
    digitalWrite(pin, LOW);
    if (i + 1 < times) {
      delay(wait);
    }
  }
}

//-------------------------------------------------------------------------------------------------------
// EEPROM Functions
//-------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------
// erase EEPROM
//---------------------------------------------------------------
void initializeEeprom() { 
//  Serial.println("------------------------------------------------------");     
//  Serial.println("Initializing EEPROM by erasing all RFIDs              ");
//  Serial.println("Setting values of EEPROM addresses to 0               "); 
//  Serial.println("EEPROM max memory size:                               ");
//  Serial.println(eepromSize);  
//  Serial.println("------------------------------------------------------");    
  byte zero  = 0;
  for(uint32_t adr = memBase; adr <= eepromSize; adr++) {
    EEPROM.write(adr, zero);
  }
}

//---------------------------------------------------------------
// printEeprom()
// Print the EEPROM to serial output
// for debugging
//---------------------------------------------------------------
void printEeprom(){
  uint32_t ads = memBase;
  while(ads <= eepromSize) {   
    byte output = EEPROM.read(ads);
    if((ads % 10) == 0  ) Serial.println(" "); 
    Serial.print(ads);Serial.print(" => ");Serial.print(output,HEX); Serial.print("   "); 
    ads++;
  }
  Serial.println(" ");
}

//---------------------------------------------------------------
// findUidInEeprom(uidLength, uid)
// looks for matching Uid
// returns the address of the first byte of Uid (length indication) if found
// returns -1 if NOT found
// returns -2 if error
//---------------------------------------------------------------
int32_t findUidInEeprom(uint8_t uidLength, uint8_t uid[]) {
  byte key = memBase;
  byte val = EEPROM.read(key);
  boolean match = false; 
  
  if(val == 0 && key == 0) {
    Serial.println("EEPROM is empty ");
  }
  else {
    while(val != 0)
    {
      if(key >= eepromSize) 
      {
        Serial.println("ERROR: EEPROM STORAGE MESSED UP! Return -2");//this should not happen! If so initialize EEPROM
        return -2; 
      }
      if(val == uidLength) {
        // check if uid match the uid in EEPROM		
        byte uidAddress = key +1;
        match = true;     
        //compare uid bytes  
        for(byte i = 0; i < uidLength; i++) {
          byte uidVal = EEPROM.read(( uidAddress + i));
          //the first byte of uidVal is the next address
          if(uidVal != uid[i]) {
            //got to next key
            match = false;
	    // in case no break => all bytes same
            break;
          }
        }
      
        if(match) {
          Serial.println("\nFound UID in EEPROM");
          Serial.print("UID matching in Address : ");
          Serial.println(key);
          return key;
        }
      }
      
      key = key + val +1; 
      val = EEPROM.read(key);
    }
  }  
  Serial.println("\nNo UID match in EEPROM");
  return -1;
}

//---------------------------------------------------------------
// deleteUidfromEeprom(address, uidLength)
// delete the UID from EEPROM
// sets the UID values to 0
// in the EEPROM structure there will be a "hole" with zeroes
// we are not shifting the addresses to avoid unnecessary writes
// to the EEPROM. The 'hole' will be filled with next RFID 
// storage that has the same uidLength
//
//
//  BUG if uid was last uid in eeprom, length is not set to 0 => not needed because by formatting eeprom all is set to 0
//---------------------------------------------------------------
void deleteUidfromEeprom(uint32_t address, uint8_t uidLength) {
  byte zero = 0; 
  Serial.println("Erasing UID");
  for(uint8_t m = 1; m <= uidLength; m++) {
    uint32_t adr = address + m;
    Serial.print("Address: ");
    Serial.print(adr);
    Serial.println(" ");
    EEPROM.write(adr, zero);
  }
  printEeprom();
}

//---------------------------------------------------------------
// getEndOfUidsChainInEeprom(uidLength)
// returns the address of the end of the Rfids chain stored in the EEPROM
// returns -1 if no space left
// returns -2 if unexpected error
//---------------------------------------------------------------
int32_t getEndOfUidsChainInEeprom(uint8_t uidLength) {
  byte key = memBase;
  byte val = EEPROM.read(key);
  
  if(val == 0 && key == 0) {
    Serial.println("EEPROM is empty ");
    return key;
  }
  else {
    // if length byte indicator is 0 it means it is the end of the RFIDs stored, last RFID stored
    while(val != 0) {
      //this should not happen! If so initialize EEPROM
      if(key > eepromSize) {
        Serial.println("ERROR: EEPROM STORAGE MESSED UP! EXITING STORAGE READ");
        return -2;
      }
      key = key + val +1; 
      val = EEPROM.read(key);
    }
    if((key + uidLength) > eepromSize) {
      Serial.println("Not enough space left in EEPROM to store UID");//the RFID to be appended at the end of the storage chain exeeds the EEPROM length
      return -1;
    }
    else return key; 
  }  
}


//---------------------------------------------------------------
// getFreeEepromStorageFragment(uint8_t uidLength)
// return the address where to store the RFID 
// with the rfidLength specified.
// Instead of just appending the RFID to the end of 
// the storage we look for an erased RFID space and 
// fill this
// return address
// return -2 if error
// return -1 if no free storage address found
//---------------------------------------------------------------
int32_t getFreeEepromStorageFragment(uint8_t uidLength) {
  uint32_t key = memBase;
  byte val = EEPROM.read(key); //holds the uidLength stored in EEPROM
  boolean free = false; 

  if(val == 0 && key == 0) {
    // EEPROM empty, use the address at key = memBase
    return key;
  }
  else {
    //loop till the end of storage chain indicated by a zero value in the key position
    while(val != 0) {
      //this should not happen! If so initialize EEPROM
      if(key > eepromSize) {
        Serial.println("ERROR: EEPROM STORAGE MESSED UP! EXITING STORAGE READ");
        return -2; 
      }
      // check if uidLength  match the uidLength in EEPROM
      if(val == uidLength) {     
        uint32_t uidAddress = key +1;
        free = true;
        //check if uid bytes are all zero => free storage fragment
        for(uint8_t i = 0; i < uidLength; i++) {
          byte uidVal = EEPROM.read(( uidAddress + i));         
          if(uidVal != 0) {
            //got to next key
            free = false;
            break;
          }
        }
        // in case no break => all bytes have zero value => free fragment
        if(free) {
          return key;
        }
      }    
      key = key + val +1; 
      val = EEPROM.read(key);
    } 
    return -1;    
  }
}

//---------------------------------------------------------------
// getEepromStorageAddress(uint8_t uidLength)
// combination of getFreeEepromStorageFragment
// and getEndOfRfidsChainInEeprom
// return address
// return -1 if no free storage address found
// return -2 if error
//---------------------------------------------------------------
int32_t getEepromStorageAddress(uint8_t uidLength) {
  int32_t fragment = getFreeEepromStorageFragment(uidLength);
  // free fragment found
  if(fragment >= 0) {
    return fragment;
  }
  // error returned
  else if (fragment == -2) {
    return fragment;
  }
  // no free fragment available
  // check if space available at end of rfid storage chain
  else if (fragment == -1) {
    int32_t append = getEndOfUidsChainInEeprom(uidLength);    
    return append;
  }
  // should never occur, return error
  else {
    return -2;
  }  
}

//---------------------------------------------------------------
// writeUidToEeprom(addrees,uidlength,uid)
// write UID to EEPROM
//---------------------------------------------------------------
void writeUidToEeprom(uint32_t StoragePositionAddress, uint8_t uidLength, uint8_t uid[]) {
  // Writing into first free address the length of the RFID uid
  EEPROM.write(StoragePositionAddress, uidLength); 
  // Writing into the following addresses the RFID uid values (byte per byte)
  uint32_t uidBytePosition = StoragePositionAddress +1; //next position after addressByte which contains the uidLength
  for(uint8_t r=0; r < uidLength; r++) {
    EEPROM.write(uidBytePosition, uid[r]);   
    uidBytePosition++;
  } 
  printEeprom();
}
