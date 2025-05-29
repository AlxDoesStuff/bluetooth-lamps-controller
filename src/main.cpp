#include <Arduino.h>
#include <WiFi.h>
#include <Credentials.h>
#include <WebServer.h>
#include <ArduinoBLE.h>

WebServer server(80);

// Boolean array to store if peripherals are connected
bool peripheralConnected[sizeof(Peripherals) / sizeof(Peripherals[0])] = {false, false};

// TO DO: make this less shit
// Characteristics refernces
BLECharacteristic lampCharacteristics[2]; // Lamp characteristics 1: write, 2: notify
BLECharacteristic LEDCharacteristics[2];  // LED characteristics 1: write, 2: notify

// Returns if a BLE scan is already happening
bool isScanning = false;

// Returns true if all peripherals are connected
bool allPeriphsConnected()
{
  for (bool connected : peripheralConnected)
  {
    if (!connected)
    {
      return false;
    }
  }
  return true;
}

// Scans for a periphal by its peripheral index
void scanPeripheralIndex(int index)
{
  // Check if a scan is already happening
  if (!isScanning)
  {
    // Check if periph is already connected
    if (!peripheralConnected[index])
    {
      // Scan for periph
      Serial.print("[INFO] Scanning for BLE peripheral with index ");
      Serial.println(index);
      isScanning = true;
      BLE.scanForName(Peripherals[index]);
    }
  }
}

// Simple disconnect
void dcPeripheral(BLEDevice peripheral, int index)
{
  peripheralConnected[index] = false;
  Serial.print("[INFO] Disconnecting peripheral with index ");
  Serial.println(index);
  peripheral.disconnect();
}

// Check if connected peripheral has the required services and characteristics
// TO DO: put the services to check in a variable or something like that
bool checkAndSaveCharacteristics(BLEDevice peripheral, int index)
{
  switch (index)
  {
  case 0: // Lamp checks
    if (peripheral.hasService("FFF0") && peripheral.service("FFF0").hasCharacteristic("FFF3") && peripheral.service("FFF0").hasCharacteristic("FFF4"))
    {
      lampCharacteristics[0] = peripheral.service("FFF0").characteristic("FFF3");
      lampCharacteristics[1] = peripheral.service("FFF0").characteristic("FFF4");
      return true;
    }
  case 1: // LEDs checks
    if (peripheral.hasService("FFD5") && peripheral.service("FFD5").hasCharacteristic("FFD9") && peripheral.hasService("FFD0") && peripheral.service("FFD0").hasCharacteristic("FFD4"))
    {
      LEDCharacteristics[0] = peripheral.service("FFD5").characteristic("FFD9");
      LEDCharacteristics[1] = peripheral.service("FFD0").characteristic("FFD4");
      return true;
    }
  }
}

// Handling connecting to peripherals
void connectPeripheral(BLEDevice peripheral)
{
  int peripheralIndex = -1; // Variable for storing the peripheral's index

  // Find the peripheral's index
  for (int i = 0; i < sizeof(Peripherals) / sizeof(Peripherals[0]); i++)
  {
    if (Peripherals[i] == peripheral.localName())
    {
      peripheralIndex = i;
    }
  }

  // If the peripheral index wasn't found, don't attempt connecting (not part of the needed peripherals)
  if (peripheralIndex == -1)
  {
    Serial.print("[INFO] Peripheral wasn't found in peripherals");
    return;
  }

  // Check if peripheral is already connected
  if (peripheralConnected[peripheral])
  {
    return;
  }

  // Attempt connecting
  if (peripheral.connect())
  {
    Serial.print("[INFO] Successfully connected to peripheral with index ");
    Serial.println(peripheralIndex);
    peripheralConnected[peripheralIndex] = true;
  }
  else
  {
    Serial.print("[WARN] Failed to connect to peripheral with index ");
    Serial.println(peripheralIndex);
    peripheralConnected[peripheralIndex] = false;
    return;
  }

  // discover peripheral attributes
  Serial.print("[INFO] Discovering BLE attributes for device with index ");
  Serial.println(peripheralIndex);
  if (peripheral.discoverAttributes())
  {
    Serial.print("[INFO] BLE Attributes discovered for device with index ");
    Serial.println(peripheralIndex);
  }
  else
  {
    // If attribute discovery fails, disconnect
    Serial.print("[WARN] BLE Attribute discovery failed on device with index ");
    Serial.println(peripheralIndex);
    dcPeripheral(peripheral, peripheralIndex);
    return;
  }

  // Check if all the needed services and attributes are present, also save them in the reference
  if (!checkAndSaveCharacteristics(peripheral, peripheralIndex))
  {
    // If attribute discovery fails, disconnect
    Serial.print("[WARN] BLE Peripheral with index ");
    Serial.println(peripheralIndex);
    Serial.print(" is missing important characteristics or services!");
    dcPeripheral(peripheral, peripheralIndex);
    return;
  }
  //test thingy boing: turn one lamp on, then off and the other one on
  if (allPeriphsConnected){
    byte lampOn[] = {0x55, 0x01, 0xff, 0x06, 0x01, 0xa3};
    byte lampOff[] = {0x55, 0x01, 0xff, 0x06, 0x00, 0xa4};
    byte LEDOn[] = {0xcc, 0x23, 0x33};
    byte LEDOff[] = {0xcc, 0x24, 0x33};
    lampCharacteristics[0].writeValue(lampOn, sizeof(lampOn));
    delay(1000);
    lampCharacteristics[0].writeValue(lampOff, sizeof(lampOff));
    LEDCharacteristics[0].writeValue(LEDOn, sizeof(LEDOn));
    delay(1000);
    LEDCharacteristics[0].writeValue(LEDOff, sizeof(LEDOff));
  }
}

void setup()
{
  // Initialize Serial
  Serial.begin(115200);
  // Begin WiFi
  WiFi.setHostname("ESP32-BLE-Lamp-Controller");
  WiFi.mode(WIFI_STA);
  // Connect
  Serial.print("Connecting to WiFi network ");
  Serial.println(SSID);
  WiFi.begin(SSID, Pass);
  // Initialize BLE
  BLE.begin();
  Serial.println("Beginning BLE Module");
}

void loop()
{
  if (!allPeriphsConnected()) // Only scan if at least one of the devices is not connected
  {
    // Loop through each peripheral and scan for each one invdividually (prevents goofy weird stuff)
    for (int i = 0; i < sizeof(Peripherals) / sizeof(Peripherals[0]); i++)
    {
      scanPeripheralIndex(i);
      // check if a peripheral has been discovered
      BLEDevice bleDevice = BLE.available();
      if (bleDevice)
      {
        // When a BLE peripheral has been discovered, print out index, address, local name, and RSSI
        Serial.print("[INFO] Found Peripheral with index ");
        Serial.print(i);
        Serial.print(" at ");
        Serial.print(bleDevice.address());
        Serial.print(", '");
        Serial.print(bleDevice.localName());
        Serial.print("', RSSI: ");
        Serial.print(bleDevice.rssi());
        Serial.println();

        // stop scanning
        BLE.stopScan();
        isScanning = false;
        // Handle peripheral connection
        connectPeripheral(bleDevice);
      }
    }
  }
}
