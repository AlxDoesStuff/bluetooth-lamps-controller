#include <Arduino.h>
#include <WiFi.h>
#include <Credentials.h>
#include <WebServer.h>
#include <ArduinoBLE.h>

WebServer server(80);

String lampUUIDs[1] = {""};

void handlePeripheral(BLEDevice peripheral)
{
  if (peripheral.connect())
  {
    Serial.println("Successfully connected to peripheral!");
  }
  else
  {
    Serial.println("Failed to connect to peripheral!");
    return;
  }
  // discover peripheral attributes
  Serial.println("Discovering attributes...");
  if (peripheral.discoverAttributes())
  {
    Serial.println("Attributes discovered");
  }
  else
  {
    // If attribute discovery fails, just give up
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }
  if (peripheral.hasService("FFF0"))
  {
    if (peripheral.service("FFF0").hasCharacteristic("FFF3"))
    {
      //lights ON
      BLECharacteristic fff3 = peripheral.service("FFF0").characteristic("FFF3");
      byte value[] = {0x55, 0x01, 0xff, 0x06, 0x00, 0xa4};
      fff3.writeValue(value,sizeof(value),true);
      //lights OFF
      delay(1000);
      byte value2[] = {0x55, 0x01, 0xff, 0x06, 0x01, 0xa3};
      fff3.writeValue(value2,sizeof(value2),true);
    }
    else
    {
      // If important characteristic not found, disconnect
      Serial.println("Important Lamp characteristic not found, disconnecting!");
      peripheral.disconnect();
      return;
    }
  }
  else
  {
    // If important service not found, disconnect
    Serial.println("Important Lamp service not found, disconnecting!");
    peripheral.disconnect();
    return;
  }
}

void setup()
{
  Serial.begin(115200);

  // initialize the Bluetooth® Low Energy hardware
  BLE.begin();
  Serial.println("Beginning BLE Module and scanning for device with name Sunset lights");

  // start scanning for peripheral
  BLE.scanForName("Sunset lights");
}

void loop()
{
  // check if the peripheral has been discovered
  BLEDevice bleDevice = BLE.available();
  if (bleDevice)
  {
    // discovered a peripheral, print out address, local name, and advertised service
    Serial.print("Found ");
    Serial.print(bleDevice.address());
    Serial.print(" '");
    Serial.print(bleDevice.localName());
    Serial.print("', service: ");
    Serial.print(bleDevice.advertisedServiceUuid());
    Serial.print("', RSSI: ");
    Serial.print(bleDevice.rssi());
    Serial.println();
    // stop scanning
    BLE.stopScan();
    // Handle peripheral
    handlePeripheral(bleDevice);
    // start scanning if disconnected
    BLE.scanForName("Sunset lights");
  }
}
