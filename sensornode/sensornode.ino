// Board: Generic STM32WB series
// Board part number: Generic WB5MMGHx
// USB support: CDC Serial supersede USART
// Upload method: DFU

#include <Wire.h>
#include "src/SparkFunHTU21D.h"
#include "src/SparkFun_VEML7700_Arduino_Library.h"
#include "src/MAX3010x/MAX30105.h"
#include <STM32duinoBLE.h>

// STM32WB5MM BLE
HCISharedMemTransportClass HCISharedMemTransport;
#if !defined(FAKE_BLELOCALDEVICE)
BLELocalDevice BLEObj(&HCISharedMemTransport);
BLELocalDevice& BLE = BLEObj;
#endif

// Nordic UART service
BLEService mainService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
BLECharacteristic rxChar("6E400002-B5A3-F393-E0A9-E50E24DCCA9E", BLEWrite, 20);
BLECharacteristic txChar("6E400003-B5A3-F393-E0A9-E50E24DCCA9E", BLERead | BLENotify, 20);
char txBuffer[20];

void MyBLE_Init() {
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }

  // set the device name peripheral advertises
  BLE.setDeviceName("Custom STM32WB5MMG");
  // set the local name peripheral advertises
  BLE.setLocalName("WB55 Sensor Node Mini");
  // set the UUID for the service this peripheral advertises
  BLE.setAdvertisedService(mainService);
  // add the characteristic to the service
  mainService.addCharacteristic(rxChar);
  mainService.addCharacteristic(txChar);
  // add service
  BLE.addService(mainService);
  // start advertising
  BLE.advertise();
}

// Communication
TwoWire MyWire(PB7, PB6);

// Sensors
MAX30105 particleSensor;

void setup() {
  // Init communication
  Serial.begin(115200);
  MyWire.begin();

  // Init particleSensor
  Serial.println("Initializing...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

  // Setup to sense a nice looking saw tooth on the plotter
  byte ledBrightness = 0x4F; //Options: 0=Off to 255=50mA
  byte sampleAverage = 8; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 3; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 1600; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 16384; //Options: 2048, 4096, 8192, 16384

  // Configure sensor with these settings
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

  // Configure BLE
  MyBLE_Init();
}

// the loop function runs over and over again forever
char text_buffer[100];

void loop() {
  // Send raw data to serial plotter
  uint32_t ir_reading = particleSensor.getIR();
  Serial.println(ir_reading);

  // Send raw data to BLE plotter
  snprintf(txBuffer, 20, "%d\r\n", ir_reading);
  txChar.setValue((unsigned char *) txBuffer, 20);

  // BLE handler
  BLE.poll();
}
