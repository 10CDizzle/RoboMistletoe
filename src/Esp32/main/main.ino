/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"
   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.
   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Tone32.h>

#define BUZZER_PIN 27
#define BUZZER_CHANNEL 0

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
int BlinkCounter = 0;
char CounterBuffer[5];
char StrBuf[50];
float InputNums[2] = {0};

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void sendBLEString(std::string sender)
{
  uint8_t DecodedString[sender.length()];
  for(int i = 0; i<sender.length(); i++)
  {
   DecodedString[i]=(uint8_t)sender[i]; 
  }
  pTxCharacteristic->setValue(DecodedString, sender.length());
  pTxCharacteristic->notify();
}

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        uint8_t DecodedString[rxValue.length()];
        for (int i = 0; i < rxValue.length(); i++)
        {
        Serial.print(rxValue[i]);
        DecodedString[i]=(uint8_t)rxValue[i];
        }

        Serial.println();
        Serial.println("*********");
        pTxCharacteristic->setValue(DecodedString, rxValue.length());
        pTxCharacteristic->notify();
        sprintf(StrBuf, "%s", DecodedString);
        ParseNums(InputNums,StrBuf);
        //PlayTone(InputNums[0],InputNums[1]);
      }
    }
};


void setup() {
  Serial.begin(115200);
  pinMode(2,OUTPUT);

  // Create the BLE Device
  BLEDevice::init("MistletoeCtrl");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
    if (deviceConnected) {
     // bluetooth stack will go into congestion, if too many packets are sent
  }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
      BlinkCounter = 0;
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("Disconnected, Now Advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}

//Jogs a servo for a requisite amount of time, at a given speed.
void JogMotor(int duration, int spd)
{
  
}

//plays a tone through the ESP32
void PlayTone(int pitch, int duration)
{
  tone(BUZZER_PIN,pitch,duration,BUZZER_CHANNEL);
  noTone(BUZZER_PIN,BUZZER_CHANNEL);
}

//Handler for getting one-value commands from the client
void ParseNums(float (& val)[2], String input)
{
  int FirstSpace = input.indexOf(' ');
  int SecondSpace = input.indexOf(' ', FirstSpace+1);
  String FirstNumber = input.substring(0, FirstSpace);
  String SecondNumber = input.substring(FirstSpace, SecondSpace);
  val[0] = FirstNumber.toFloat();
  val[1] = SecondNumber.toFloat();
}
