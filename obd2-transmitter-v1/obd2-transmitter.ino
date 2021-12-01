
#include <SPI.h>
#include <Wire.h>
#include <SSD1306.h>
#include <LoRa.h>
#include "BluetoothSerial.h"
#include "ELMduino.h"

#define ELM_PORT   SerialBT
#define DEBUG_PORT Serial
#define LORA_BAND 915
#define OLED_SDA  4
#define OLED_SCL  15
#define OLED_RST  16
#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

BluetoothSerial SerialBT;

// String MACadd = "8C:DE:00:01:0A:F6";
uint8_t address[6]  = {0x8C, 0xDE, 0x00, 0x01, 0x0A, 0xF6};

struct CarPerformance {
  uint32_t rpm;
  uint32_t kmh;
  double throttle_position;
  uint32_t intake_air_temperature;
  uint32_t engine_oil_temperature;
  uint32_t engine_coolant_temperature;
  uint32_t engine_runtime;
};

SSD1306 display(0x3c, OLED_SDA, OLED_SCL);
ELM327 myELM327;
CarPerformance carPerformance = {
  0, 0, 0.0, 0, 0, 0, 0
};

void sendLoRaData(CarPerformance carPerformance);
void displayLoraData(CarPerformance carPerformance);

void setup()
{  
#if LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

  DEBUG_PORT.begin(9600);
  ELM_PORT.begin("ArduHUD", true);
  DEBUG_PORT.println("Start");

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, HIGH);
  delay(100);
  digitalWrite(OLED_RST, LOW);
  delay(100);
  digitalWrite(OLED_RST, HIGH);

  display.init();
  display.flipScreenVertically();
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(display.getWidth() / 2, display.getHeight() / 2, "OBD2 LoRa");
  display.display();
  delay(2000);

  // Configure the LoRA radio
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(LORA_BAND * 1E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("Initializaition OK");

  if (!ELM_PORT.connect(address))
  {
    DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1");
    while (1);
  }

  if (!myELM327.begin(ELM_PORT, true, 2000))
  {
    Serial.println("Couldn't connect to OBD scanner - Phase 2");
    while (1);
  }
}


void loop()
{
  
  // RPM
  float tempRPM = myELM327.rpm();

  if (myELM327.status == ELM_SUCCESS)
  {
    carPerformance.rpm = (uint32_t)tempRPM;
    Serial.print("RPM: "); Serial.println(carPerformance.rpm);
  }
  else {
    myELM327.printError();
  }

  // KMH
  float tempKMH = myELM327.kph();

  if (myELM327.status == ELM_SUCCESS)
  {
    carPerformance.kmh = (uint32_t)tempKMH;
    Serial.print("KMH: "); Serial.println(carPerformance.kmh);
  }
  else {
    myELM327.printError();
  }

  // THROTTLE POSITION
  float tempThrottlePosition = myELM327.throttle();

  if (myELM327.status == ELM_SUCCESS)
  {
    carPerformance.throttle_position = (double)tempThrottlePosition;
    Serial.print("THROTTLE POSITION: "); Serial.println(carPerformance.throttle_position);
  }
  else {
    myELM327.printError();
  }

  // INTAKE AIR TEMP
  float tempIntakeAirTemp = myELM327.intakeAirTemp();

  if (myELM327.status == ELM_SUCCESS)
  {
    carPerformance.intake_air_temperature = (uint32_t)tempIntakeAirTemp;
    Serial.print("INTAKTE AIR TEMP: "); Serial.println(carPerformance.intake_air_temperature);
  }
  else {
    myELM327.printError();
  }

  // ENGINE OIL TEMP
  float tempEngineOilTemp = myELM327.oilTemp();

  if (myELM327.status == ELM_SUCCESS)
  {
    carPerformance.engine_oil_temperature = (uint32_t)tempEngineOilTemp;
    Serial.print("OIL TEMP: "); Serial.println(carPerformance.engine_oil_temperature);
  }
  else {
    myELM327.printError();
  }

  // ENGINE COOLANT TEMP
  float tempEngineCoolantTemp = myELM327.engineCoolantTemp();

  if (myELM327.status == ELM_SUCCESS)
  {
    carPerformance.engine_coolant_temperature = (uint32_t)tempEngineCoolantTemp;
    Serial.print("ENGINE COOLANT TEMP: "); Serial.println(carPerformance.engine_coolant_temperature);
  }
  else {
    myELM327.printError();
  }

  // ENGINE RUNTIME
  float tempEngineRuntime = myELM327.runTime();

  if (myELM327.status == ELM_SUCCESS)
  {
    carPerformance.engine_runtime = (uint32_t)tempEngineRuntime;
    Serial.print("ENGINE RUNTIME: "); Serial.println(carPerformance.engine_runtime);
  }
  else {
    myELM327.printError();
  }
  
  displayLoraData(carPerformance);
  sendLoRaData(carPerformance);
}

void sendLoRaData(CarPerformance carPerformance) {
  char carData [1000];
  sprintf(carData, "%d,%d,%d,%d,%d,%d,%d", carPerformance.rpm, carPerformance.kmh, carPerformance.throttle_position, carPerformance.intake_air_temperature, carPerformance.engine_oil_temperature, carPerformance.engine_coolant_temperature, carPerformance.engine_runtime);
  // char carData [1000];
  // sprintf(carData, "carPerformance=%d,%d,%f,%d,%d,%d,%d",222, 222, 222.66, 333, 333, 333, 333);
 
  LoRa.beginPacket();
  LoRa.print(carData);
  LoRa.endPacket();
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
}

void displayLoraData(CarPerformance carPerformance) {
  String carPerformanceString = String(carPerformance.rpm);
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Car performance (RPM): ");
  display.drawString(120, 0, carPerformanceString);
  display.display();
}
