#if defined(__has_include)
#if __has_include(<LGATT.h>) && __has_include(<LGATTClient.h>)
#include <LGATT.h>
#include <LGATTClient.h>
#else
#error "This LinkIt ONE package is missing the BLE GATT libraries (LGATT/LGATTClient). Restore the original MediaTek LinkIt SDK libraries first."
#endif
#else
#include <LGATT.h>
#include <LGATTClient.h>
#endif

LGATTUUID appUUID("AFA5B1C5-2B7B-471B-BD4C-7A92DE9D6DBD");
LGATTClient client;

// Replace this after the first scan if you want auto-connect.
// Format: "AA:BB:CC:DD:EE:FF"
const char TARGET_ADDR[] = "";

bool connectAttempted = false;

void printHexByte(uint8_t value) {
  if (value < 0x10) {
    Serial.print("0");
  }
  Serial.print(value, HEX);
}

void printAddress(const LGATTAddress &addr) {
  for (int i = 5; i >= 0; --i) {
    printHexByte(addr.addr[i]);
    if (i > 0) {
      Serial.print(":");
    }
  }
}

bool parseAddress(const char *text, LGATTAddress &addr) {
  if (!text || !text[0]) {
    return false;
  }

  uint8_t bytes[6] = {0};
  int values[6] = {0};
  int matched = sscanf(
    text,
    "%02x:%02x:%02x:%02x:%02x:%02x",
    &values[0], &values[1], &values[2],
    &values[3], &values[4], &values[5]
  );

  if (matched != 6) {
    return false;
  }

  for (int i = 0; i < 6; ++i) {
    bytes[i] = (uint8_t)values[i];
  }

  // LinkIt stores the address bytes little-endian in bd_addr.addr[].
  addr.addr[0] = bytes[5];
  addr.addr[1] = bytes[4];
  addr.addr[2] = bytes[3];
  addr.addr[3] = bytes[2];
  addr.addr[4] = bytes[1];
  addr.addr[5] = bytes[0];
  return true;
}

void dumpServices() {
  int numberOfServices = client.getServiceCount();

  Serial.print("Service count: ");
  Serial.println(numberOfServices);

  for (int i = 0; i < numberOfServices; ++i) {
    LGATTUUID serviceUUID;
    boolean isPrimary = false;

    if (client.getServiceInfo(i, serviceUUID, isPrimary)) {
      Serial.print("Service[");
      Serial.print(i);
      Serial.print("] UUID=");
      Serial.print(serviceUUID);
      Serial.print(" primary=");
      Serial.println(isPrimary ? "true" : "false");
    } else {
      Serial.print("Service[");
      Serial.print(i);
      Serial.println("] info read failed");
    }
  }
}

void tryReadCharacteristic(const LGATTUUID &serviceUUID, boolean isPrimary, uint16_t characteristicUuid, const char *label) {
  LGATTAttributeValue value;
  memset(&value, 0, sizeof(value));

  if (client.readCharacteristic(serviceUUID, isPrimary, characteristicUuid, value)) {
    Serial.print(label);
    Serial.print(" len=");
    Serial.print(value.len);
    Serial.print(" data=");

    for (int i = 0; i < value.len; ++i) {
      printHexByte(value.value[i]);
      Serial.print(" ");
    }

    Serial.print(" ascii=\"");
    for (int i = 0; i < value.len; ++i) {
      char c = (char)value.value[i];
      if (c >= 32 && c <= 126) {
        Serial.print(c);
      } else {
        Serial.print(".");
      }
    }
    Serial.println("\"");
  } else {
    Serial.print(label);
    Serial.println(" not readable or not present");
  }
}

void tryCommonReads() {
  int numberOfServices = client.getServiceCount();

  for (int i = 0; i < numberOfServices; ++i) {
    LGATTUUID serviceUUID;
    boolean isPrimary = false;

    if (!client.getServiceInfo(i, serviceUUID, isPrimary)) {
      continue;
    }

    // Device Information service
    if (serviceUUID == LGATTUUID(0x180A)) {
      Serial.println("Reading Device Information service");
      tryReadCharacteristic(serviceUUID, isPrimary, 0x2A29, "  Manufacturer");
      tryReadCharacteristic(serviceUUID, isPrimary, 0x2A24, "  Model Number");
      tryReadCharacteristic(serviceUUID, isPrimary, 0x2A25, "  Serial Number");
      tryReadCharacteristic(serviceUUID, isPrimary, 0x2A26, "  Firmware Revision");
    }

    // Battery service
    if (serviceUUID == LGATTUUID(0x180F)) {
      Serial.println("Reading Battery service");
      tryReadCharacteristic(serviceUUID, isPrimary, 0x2A19, "  Battery Level");
    }
  }
}

void scanAndPrint() {
  Serial.println();
  Serial.println("Scanning for BLE devices for 5 seconds...");

  int numberOfDevices = client.scan(5);

  Serial.print("Devices found: ");
  Serial.println(numberOfDevices);

  for (int i = 0; i < numberOfDevices; ++i) {
    LGATTDeviceInfo info = {0};
    if (!client.getScanResult(i, info)) {
      Serial.print("Scan result ");
      Serial.print(i);
      Serial.println(" unavailable");
      continue;
    }

    Serial.print("Device[");
    Serial.print(i);
    Serial.print("] addr=");
    printAddress(info.bd_addr);
    Serial.print(" rssi=");
    Serial.println(info.rssi);
  }
}

void connectToTargetIfConfigured() {
  LGATTAddress targetAddr;

  if (!parseAddress(TARGET_ADDR, targetAddr)) {
    Serial.println("No TARGET_ADDR configured yet. Copy the PASCO AirLink MAC from the scan output.");
    return;
  }

  Serial.print("Connecting to ");
  Serial.println(TARGET_ADDR);

  if (!client.connect(targetAddr)) {
    Serial.println("Connect failed");
    return;
  }

  Serial.println("Connected");
  dumpServices();
  tryCommonReads();
  client.disconnect(targetAddr);
  Serial.println("Disconnected");
}

void setup() {
  Serial.begin(115200);
  delay(4000);

  Serial.println("LinkIt ONE PASCO AirLink BLE probe");
  Serial.println("Attach the Wi-Fi/Bluetooth antenna before scanning.");

  client.begin(appUUID);
  Serial.println("BLE client ready");
}

void loop() {
  scanAndPrint();

  if (!connectAttempted) {
    connectAttempted = true;
    connectToTargetIfConfigured();
  }

  delay(5000);
}
