// Board: STM32 MCU based boards > Discovery
// Board part number: STM32WB5MM-DK
// Upload method: SWD
// More Info: https://github.com/stm32duino/Arduino_Core_STM32/commit/bb11ba8bcfb606dda471c302896b280732c58d06

#include <Wire.h>
#include <U8g2lib.h>
#include "src/TLC59731.h"
#include "src/STTS22H/STTS22HSensor.h"
#include "src/ISM330DHCX/ISM330DHCXSensor.h"
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
  BLE.setDeviceName("STM32WB5MM-DK");
  // set the local name peripheral advertises
  BLE.setLocalName("WB55 Sensor Node");
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
TwoWire MyWire(PB11, PB13);
HardwareSerial STLink(PIN_SERIAL_RX, PIN_SERIAL_TX);

// On-board sensors
STTS22HSensor Temp(&MyWire, STTS22H_I2C_ADD_H);
ISM330DHCXSensor AccGyr(&MyWire);

// On-board display
# define RST_DISP PC8
# define DC_DISP PC9
# define CS_DISP PH0
# define SCK_DISP PA1
# define MOSI_DISP PA7
U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(
  U8G2_R0, /* clock=*/ SCK_DISP, /* data=*/ MOSI_DISP, /* cs=*/ CS_DISP, /* dc=*/ DC_DISP, /* reset=*/ RST_DISP
);

// External sensors
MAX30105 particleSensor;
// - PA_10 // D14 SDA
// - PB_8  // D15 SCL

void setup() {
  // Init communication
  STLink.begin(115200);
  MyWire.begin();
  MyWire.setClock(I2C_SPEED_FAST);

  // Init LED
  pinMode(LED_SELECT, OUTPUT);
  digitalWrite(LED_SELECT, HIGH);
  pinMode(RGB_LED, OUTPUT);
  TLC59731_setLed(RGB_LED, 8, 4, 0);
  TLC59731_gsDataLatchSequence();
  digitalWrite(LED_SELECT, LOW);

  // Init sensors
  Temp.begin();
  Temp.Enable();

  AccGyr.begin();
  AccGyr.ACC_Enable();  
  AccGyr.GYRO_Enable();

  // Init display
	u8g2.begin();
	u8g2.setFont(u8g2_font_6x13_tr);            // choose a suitable font
  u8g2.clear();


  // Init particleSensor
  STLink.println("Initializing...");
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    STLink.println("MAX30105 was not found. Please check wiring/power. ");
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
int32_t accelerometer[3];
int32_t gyroscope[3];
float temperature = 0.0;

void loop() {
  // Send raw data to serial plotter
  uint32_t ir_reading = particleSensor.getIR();
  STLink.println(ir_reading);

  // Send raw data to BLE plotter
  snprintf(txBuffer, 20, "%d\r\n", ir_reading);
  txChar.setValue((unsigned char *) txBuffer, 20);

  // Clear buffer and draw header
	u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_symbols);
	u8g2.drawGlyph(112, 56, 0x2603);

  u8g2.setFont(u8g2_font_6x13_tr);
	u8g2.drawStr(0, 12*1, "Elektor STM32WB");
	u8g2.drawStr(0, 12*2, "Wireless Design");

  // Read temperature
  Temp.GetTemperature(&temperature);

  snprintf(text_buffer, 100, "Temp: %02d.%02d C         ", (int)temperature, ((int)(temperature*100)%100));
  u8g2.drawStr(0, 12*3.5, text_buffer);
  
  // Read accelerometer and gyroscope
  AccGyr.ACC_GetAxes(accelerometer);  
  AccGyr.GYRO_GetAxes(gyroscope);

  // STLink.print("Acc[mg]:");
  // STLink.print(accelerometer[0]);
  // STLink.print(", ");
  // STLink.print(accelerometer[1]);
  // STLink.print(", ");
  // STLink.print(accelerometer[2]);
  // STLink.print(", Gyro[mdps]:");
  // STLink.print(gyroscope[0]);
  // STLink.print(", ");
  // STLink.print(gyroscope[1]);
  // STLink.print(", ");
  // STLink.println(gyroscope[2]);

  snprintf(text_buffer, 100, "AccX: %02d.%02d         ", (int)(accelerometer[0]), ((int)(accelerometer[0]*100)%100));
  u8g2.drawStr(0, 12*4.5, text_buffer);
	
  // Update buffer
  u8g2.sendBuffer();

  // BLE handler
  BLE.poll();
}
