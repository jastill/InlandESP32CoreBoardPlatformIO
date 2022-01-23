/**
 * @file main.cpp
 * @author John Astill
 * @brief Inland ESP32 setup and testing for Servos and BLE. Used as a template for other projects.
 * @version 0.1
 * @date 2022-01-23
 *
 * @copyright Copyright (c) 2022
 *
 * https://www.microcenter.com/product/613822/inland-esp32-wroom-32d-module?storeid=061
 */
#include <Arduino.h>
#include <ESP32Servo.h>

#include <BLEDevice.h> 
#include <BLEUtils.h>  
#include <BLEServer.h> 
#include <BLE2902.h>

Servo servo;
int pos = 0; 
int servoPin = 33;


/////////////////////////////////////////////////////
// BLE Code
/////////////////////////////////////////////////////

// Add some technical debt and allow for more than one command to be tracked...
uint8_t commandsReceived = 0;
int16_t command = 0;

// For Bluetooth Low Energy, give the device a name
#define DEVICENAME  "ROBOBOB"

// Give the service and characterists UUIDs generated with uuidgen
#define SERVO_SERVICE "7a2d44ce-a094-46f2-9988-2f8d606e4764"
#define SERVO_CHARACTERISTIC "bb9919db-e7d9-4602-8546-709bd3f8e78a"

/**
 * @brief The following code is for the BLE callbacks. These
 *        are accessed when a BLE event occurs such as receiving
 *        a message.
 */
class BleCallbacks : public BLECharacteristicCallbacks
{
  /**
   * @brief This is a command received via a write.
   * 
   * @param pCharacteristic 
   */
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    // Read the value received in the characteristic
    std::string value = pCharacteristic->getValue();
    
    // For debug purposes print the received value
    Serial.println("write Bluetooth characteristic called");

    // Covert the received value into a char.
    const char *commandChar = value.c_str();

    Serial.println("Command Received");
    Serial.println(commandChar);
    command = atoi(commandChar);

    // For now simply assume the command is a direction and step
    commandsReceived++;
  }

  /**
   * @brief Callback for a read
   * 
   */
  void onRead(BLECharacteristic *pCharacteristic) {
    Serial.println("Read received");
  }
};

/**
 * @brief Setup the BLE service for both moving the servo
 * 
 */
void setupBle() {
  BLEDevice::init(DEVICENAME);
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pServoService = pServer->createService(SERVO_SERVICE);

  // We will only setup write
  BLECharacteristic *pCharacteristic = pServoService->createCharacteristic(
      SERVO_CHARACTERISTIC,
      BLECharacteristic::PROPERTY_WRITE|BLECharacteristic::PROPERTY_READ);

  pCharacteristic->setCallbacks(new BleCallbacks());

  // Create the BLE Service
  // Start the BLE service
  pServoService->start();

  // Set the advertizing power and start advertizing
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVO_SERVICE);
  pAdvertising->start();  
}

/////////////////////////////////////////////////////
// Distance Sensor code HC-SR04
/////////////////////////////////////////////////////
#define echoPin 16
#define trigPin 17

long duration = 0; // Variable for the duration of sound wave travel
int distance = 0;  // Distance Calculation
long lastDistanceReadTimestamp = 0;
uint8_t distanceCommandReceived = false;
#define TRIGGER_DISTANCE 7

/**
 * @brief Setup the pins for the distance sensor
 * 
 */
void setupDistanceSensor() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
}

/**
 * @brief 
 * 
 * @return int 
 */
int measureDistance() {
  // Clear the trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Send a 10uS pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the pulse echo response time
  duration = pulseIn(echoPin, HIGH);

  // Calculating the distance in CM
  distance = duration / 58;

  lastDistanceReadTimestamp = millis();

  return distance;
}

/**
 * @brief Setup the Serial Port and Servo
 *
 */
void setup()
{
  Serial.begin(115200);

  Serial.println("Inland ESP32 Testing");

  setupBle();

  setupDistanceSensor();

  servo.attach(servoPin); 
}

/**
 * @brief Main loop, no surprise here
 *
 */
void loop()
{
  // Only measure the distance every 200mS
  if ((millis() - lastDistanceReadTimestamp) > 200) {
    Serial.println(measureDistance());
  }

  if (distance <= TRIGGER_DISTANCE) {
    commandsReceived++;
    command = 90;
    distanceCommandReceived = true;
  } else if (distanceCommandReceived == true) {
    command = 0;
    commandsReceived++;
  }

  if (commandsReceived > 0) {
    if (commandsReceived > 1) {
      Serial.println("Commands being dropped, coming too quickly");
    }
    servo.write(command); 

    // Delay to allow servo move to complete
    delay(1000);

    command = 0;
    commandsReceived = 0;
  }
}