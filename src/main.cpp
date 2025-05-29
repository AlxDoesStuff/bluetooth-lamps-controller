#include <Arduino.h>
#include <WiFi.h>
#include <Credentials.h>
#include <WebServer.h>
#include <ArduinoBLE.h>

WebServer server(80);

// Boolean array to store if peripherals are connected
bool peripheralConnected[sizeof(Peripherals) / sizeof(Peripherals[0])] = {false, false};

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
    peripheralConnected[peripheralIndex] = false;
    peripheral.disconnect();
    return;
  }

  // if (peripheral.hasService("FFF0"))
  // {
  //   if (peripheral.service("FFF0").hasCharacteristic("FFF3"))
  //   {
  //     // lights ON
  //     BLECharacteristic fff3 = peripheral.service("FFF0").characteristic("FFF3");
  //     byte value[] = {0x55, 0x01, 0xff, 0x06, 0x00, 0xa4};
  //     fff3.writeValue(value, sizeof(value), true);
  //     // lights OFF
  //     delay(1000);
  //     byte value2[] = {0x55, 0x01, 0xff, 0x06, 0x01, 0xa3};
  //     fff3.writeValue(value2, sizeof(value2), true);
  //   }
  //   else
  //   {
  //     // If important characteristic not found, disconnect
  //     Serial.println("Important Lamp characteristic not found, disconnecting!");
  //     peripheral.disconnect();
  //     return;
  //   }
  // }
  // else
  // {
  //   // If important service not found, disconnect
  //   Serial.println("Important Lamp service not found, disconnecting!");
  //   peripheral.disconnect();
  //   return;
  // }
}

void setup()
{
  // Initialize Serial
  Serial.begin(115200);
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
