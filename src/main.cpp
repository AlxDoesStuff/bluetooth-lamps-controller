#include <Arduino.h>
#include <WiFi.h>
#include <Credentials.h>
#include <WebServer.h>
#include <ArduinoBLE.h>
#include <BLECommands.h>

WebServer server(80);

// Boolean array to store if peripherals are connected
bool peripheralConnected[sizeof(Peripherals) / sizeof(Peripherals[0])] = {false, false};

//Variables to store current values of peripheral settings
//LEDs
int LEDOnCurrent = 0;
int LEDRCurrent = 0;
int LEDGCurrent = 0;
int LEDBCurrent = 0;
//Lamp
int lampOnCurrent = 0;
int lampRCurrent = 0;
int lampGCurrent = 0;
int lampBCurrent = 0;
int lampBrightnessCurrent = 0;

// TO DO: make this less shit
// Characteristics refernces
BLECharacteristic lampCharacteristics[2]; // Lamp characteristics 1: write, 2: notify
BLECharacteristic LEDCharacteristics[2];  // LED characteristics 1: write, 2: notify

// Returns if a BLE scan is already happening
bool isScanning = false;

// Basic Control Functions
//  Returns true if all peripherals are connected
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

// Specific light control functions
// Turns the peripheral either on/off, returns true if successful
bool peripheralOnOff(int index, bool on)
{
  // Check if peripheral is connected
  if (peripheralConnected[index])
  {
    switch (index)
    {
    case 0:
      if (on)
      {
        //.writeValue returns 1 upon success
        if (lampCharacteristics[0].writeValue(lampOn, sizeof lampOn, true) == 1)
        {
          lampOnCurrent = 1;
          return true;
        }
        else
        {
          return false;
        }
      }
      else
      {
        //.writeValue returns 1 upon success
        if (lampCharacteristics[0].writeValue(lampOff, sizeof lampOff, true) == 1)
        {
          lampOnCurrent = 0;
          return true;
        }
        else
        {
          return false;
        }
      }
    case 1:
      if (on)
      {
        if (LEDCharacteristics[0].writeValue(LEDOn, sizeof LEDOn, true) == 1)
        {
          LEDOnCurrent = 1;
          return true;
        }
        else
        {
          LEDOnCurrent = 0;
          return false;
        }
      }
      else
      {
        if (LEDCharacteristics[0].writeValue(LEDOff, sizeof LEDOff, true) == 1)
        {
          return true;
        }
        else
        {
          return false;
        }
      }
    default:
      return false;
    }
  }
  else
  {
    return false;
  }
}

// Set peripheral (LED) color and brightness, returns true if successful
bool LEDSetColor(int r, int g, int b)
{
  // Check if connected
  if (peripheralConnected[1])
  {
    // Check if inputs are within allowed color range
    if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255)
    {
      byte value[7] = {0x56, r, g, b, 0x01, 0xf0, 0xaa}; // bytes 2, 3 and 4 represent the color
      //.writeValue returns 1 upon success
      if (LEDCharacteristics[0].writeValue(value, sizeof value, true) == 1)
      {
        LEDRCurrent = r;
        LEDGCurrent = g;
        LEDBCurrent = b;
        return true;
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

// Set peripheral (lamp) brightness, returns true if successful
bool LampSetBrightness(int brightness)
{
  // Check if connected
  if (peripheralConnected[0])
  {
    // Check if inputs are within allowed color range
    if (brightness >= 0 && brightness <= 25700)
    {
      byte value[6] = {0x55, 0x05, 0xff, 0x06, highByte(brightness), lowByte(brightness)};
      if (lampCharacteristics[0].writeValue(value, sizeof value, true) == 1)
      {
        lampBrightnessCurrent = brightness;
        return true;
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

// Set peripheral (lamp) color, returns true if successful
bool LampSetColor(int r, int g, int b)
{
  // Check if connected
  if (peripheralConnected[1])
  {
    // Check if inputs are within allowed color range
    if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255)
    {
      // Goofy final byte shit. Idfk who came up with this on the other end, but I don't like it
      int finalByte = 160;
      if (r > 0)
      {
        finalByte++;
      }
      if (g > 0)
      {
        finalByte++;
      }
      if (b > 0)
      {
        finalByte++;
      }
      if (r == 0, b == 0, g == 0)
      {
        finalByte = 161;
      }

      byte value[8] = {0x55, 0x03, 0xff, 0x08, r, g, b, finalByte}; // bytes 5, 6 and 7 represent the color

      if (lampCharacteristics[0].writeValue(value, sizeof value, true) == 1)
      {
        lampRCurrent = r;
        lampGCurrent = g;
        lampBCurrent = b;
        return true;
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

// Connect Functions
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
  default:
    return false;
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
    Serial.print("[INFO] Connecting to peripheral with index ");
    Serial.println(peripheralIndex);
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
  Serial.print("[INFO] Successfully connected to peripheral with index ");
  Serial.println(peripheralIndex);
  peripheralConnected[peripheralIndex] = true;
  delay(1500);
  peripheralOnOff(0, true);
  LampSetBrightness(25700);
  LampSetColor(255, 255, 255);
}

// Handles http control for the lamp peripheral, returns HTTP response as int
int lampPeripheralHTTP(int brightness, int on, int r, int g, int b)
{
  bool noSuccess = true; // Variable to track if at least one of the functions succeeded
  // Attempt setting brightness
  if (brightness != -1)
  {
    if (LampSetBrightness(brightness))
    {
      noSuccess = false;
    }
  }
  // Attempt setting on/off
  if (on != -1)
  {
    if (on == 0)
    {
      if (peripheralOnOff(0, false))
      {
        noSuccess = false;
      }
    }
    else if (on == 1)
    {
      if (peripheralOnOff(0, true))
      {
        noSuccess = false;
      }
    }
  }
  // Attempt setting rgb
  if (r != -1 && g != -1 && b != -1)
  {
    if (LampSetColor(r, g, b))
    {
      noSuccess = false;
    }
  }
  // If success, return OK, otherwise bad request
  if (noSuccess)
  {
    return 400;
  }
  else
  {
    return 200;
  }
}

//Handle http control for the LED peripheral, returns HTTP repsonse as int
int LEDPeripheralHTTP(int on, int r, int g, int b){
   bool noSuccess = true; // Variable to track if at least one of the functions succeeded
  // Attempt setting on/off
  if (on != -1)
  {
    if (on == 0)
    {
      if (peripheralOnOff(1, false))
      {
        noSuccess = false;
      }
    }
    else if (on == 1)
    {
      if (peripheralOnOff(1, true))
      {
        noSuccess = false;
      }
    }
  }
  // Attempt setting rgb
  if (r != -1 && g != -1 && b != -1)
  {
    if (LEDSetColor(r, g, b))
    {
      noSuccess = false;
    }
  }
  // If success, return OK, otherwise bad request
  if (noSuccess)
  {
    return 400;
  }
  else
  {
    return 200;
  }
}

// Handles http requests with below arguments, returns HTTP response as int
int handleSetHTTP(String targetPeripheralIndex, String brightnessArg, String onArg, String redArg, String greenArg, String blueArg)
{
  bool tValidIndex = false; // temperorary storage for if the index is valid
  // Check if target Peripheral arg is one of the peripherals
  for (int i = 0; i < sizeof(Peripherals) / sizeof(Peripherals[0]); i++)
  {
    if (server.arg("targetPeripheralIndex").toInt() == i)
    {
      tValidIndex = true;
    }
  }
  if (!tValidIndex)
  {
    return 400; // Bad Request
  }
  else
  {
    // All args should be integers
    int periphIndex = targetPeripheralIndex.toInt();
    int brightness = brightnessArg.toInt();
    int on = onArg.toInt();
    int red = redArg.toInt();
    int green = greenArg.toInt();
    int blue = blueArg.toInt();
    switch (periphIndex)
    {
    case 0:
      return lampPeripheralHTTP(brightness, on, red, green, blue);
    case 1:
      return LEDPeripheralHTTP(on,red,green,blue);
    default:
      return 400;
    }
  }
}

//Gets the current settings of the perihperals as a CSV string
String getPeriphValuesAsCSV(){
  String table = "LED On, LED R, LED G, LED B, Lamp On, Lamp R, Lamp G, Lamp B\n";
  String seperator = ", ";
  String csv = table + LEDOnCurrent + seperator + LEDRCurrent +  seperator + LEDGCurrent + seperator + LEDBCurrent + seperator +  + lampOnCurrent + seperator + lampRCurrent  + seperator +  lampGCurrent  + seperator +  lampBCurrent;  //Very cursed
  return csv;
}

void setup()
{
  // Initialize Serial
  Serial.begin(115200);
  // Begin WiFi
  WiFi.setHostname("ESP32-BLE-Lamp-Controller");
  WiFi.mode(WIFI_STA);
  // Connect
  Serial.print("[INFO] Connecting to WiFi network: ");
  Serial.println(SSID);
  WiFi.begin(SSID, Pass);
  while (!WiFi.isConnected())
  {
    delay(10); // Wait until WiFi is on
  };
  Serial.print("[INFO] Successfully connected to WiFi. Local IP: ");
  Serial.println(WiFi.localIP().toString());
  // Initialize BLE
  BLE.begin();
  Serial.println("[INFO] Beginning BLE Module");
  // Start HTTP server
  server.begin();
  Serial.println("Starting HTTP Server");
  // HTTP Request structure: URL/controlDevice?targetPeripheral=(peripheral Index)&on=(value)&brightness=(value)&r=(value)&green=(value)&blue=(value)
  // All values except targetPeripheral can be set to NULL to not be changed, but must be included in the request.
  // HTTP handling
  server.on("/controlDevice", HTTP_POST, []()
            {
    //Check if all needed arguments are present
    if(server.hasArg("targetPeripheralIndex") && server.hasArg("brightness" )&& server.hasArg("on")  && server.hasArg("red") && server.hasArg("green") && server.hasArg("blue")){
      server.send(handleSetHTTP(server.arg("targetPeripheralIndex"), server.arg("brightness"), server.arg("on"), server.arg("red"), server.arg("green"), server.arg("blue")));
    }else{
      server.send(400); //If needed arguments missing, send bad request 
    } });

  server.on("/getPeripheralValues", HTTP_GET, [](){
    server.send(200, "text/csv",getPeriphValuesAsCSV());
  });
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
  // Check if WiFi is still connected
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi not connected!");
    WiFi.disconnect();
    WiFi.reconnect();
    // Wait for reconnection
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Connecting...");
      delay(100);
    }
  }
  server.handleClient();
  delay(100); // update tick
}
