#include <LBT.h>
#include <LBTClient.h>

// LinkIt ONE exposes classic Bluetooth SPP through LBT.
// PASCO AirLink appears to be BLE, so this sketch is mainly a host-side
// scanner/serial bridge for classic Bluetooth targets and for confirming
// what the board can actually see.

const char TARGET_NAME[] = "PASCO";
const char TARGET_MAC[] = "";
const char TARGET_PIN[] = "";
const unsigned long RESCAN_INTERVAL_MS = 10000UL;

LBTDeviceInfo targetInfo = {0};
bool hasTarget = false;
bool connectAttempted = false;
unsigned long lastScanAt = 0;

void printAddress(const LBTAddress &addr) {
  Serial.print(addr.nap[1], HEX);
  Serial.print(addr.nap[0], HEX);
  Serial.print(":");
  Serial.print(addr.uap, HEX);
  Serial.print(":");
  Serial.print(addr.lap[2], HEX);
  Serial.print(addr.lap[1], HEX);
  Serial.print(addr.lap[0], HEX);
}

bool matchesTarget(const LBTDeviceInfo &info) {
  if (TARGET_MAC[0] != '\0') {
    char mac[18];
    sprintf(
      mac,
      "%02X:%02X:%02X:%02X:%02X:%02X",
      info.address.nap[1],
      info.address.nap[0],
      info.address.uap,
      info.address.lap[2],
      info.address.lap[1],
      info.address.lap[0]
    );
    if (strcmp(mac, TARGET_MAC) == 0) {
      return true;
    }
  }

  if (TARGET_NAME[0] != '\0' && strstr(info.name, TARGET_NAME) != NULL) {
    return true;
  }

  return false;
}

void scanForDevices() {
  Serial.println();
  Serial.println("Scanning for classic Bluetooth SPP devices...");

  hasTarget = false;
  int deviceCount = LBTClient.scan(15);

  Serial.print("Devices found: ");
  Serial.println(deviceCount);

  for (int i = 0; i < deviceCount; ++i) {
    LBTDeviceInfo info = {0};
    if (!LBTClient.getDeviceInfo(i, &info)) {
      Serial.print("Device[");
      Serial.print(i);
      Serial.println("] info unavailable");
      continue;
    }

    Serial.print("Device[");
    Serial.print(i);
    Serial.print("] addr=");
    printAddress(info.address);
    Serial.print(" name=");
    Serial.println(info.name);

    if (!hasTarget && matchesTarget(info)) {
      memcpy(&targetInfo, &info, sizeof(targetInfo));
      hasTarget = true;
      connectAttempted = false;
      Serial.println("Target match found");
    }
  }

  if (!hasTarget) {
    Serial.println("No matching classic Bluetooth target found");
  }
}

void connectToTarget() {
  if (!hasTarget || connectAttempted || LBTClient.connected()) {
    return;
  }

  connectAttempted = true;

  Serial.print("Connecting to ");
  printAddress(targetInfo.address);
  Serial.print(" name=");
  Serial.println(targetInfo.name);

  bool ok;
  if (TARGET_PIN[0] != '\0') {
    ok = LBTClient.connect(targetInfo.address, TARGET_PIN);
  } else {
    ok = LBTClient.connect(targetInfo.address);
  }

  if (ok) {
    Serial.println("Connected");
  } else {
    Serial.println("Connect failed");
  }
}

void bridgeBluetoothToSerial() {
  while (LBTClient.available()) {
    int c = LBTClient.read();
    if (c < 0) {
      break;
    }

    Serial.print("BT->SER ");
    if (c >= 32 && c <= 126) {
      Serial.print("'");
      Serial.print((char)c);
      Serial.print("'");
    } else {
      Serial.print("0x");
      if (c < 16) {
        Serial.print("0");
      }
      Serial.print(c, HEX);
    }
    Serial.println();
  }
}

void bridgeSerialToBluetooth() {
  while (Serial.available() && LBTClient.connected()) {
    char c = (char)Serial.read();
    size_t written = LBTClient.write((uint8_t)c);

    Serial.print("SER->BT ");
    Serial.print("'");
    if (c >= 32 && c <= 126) {
      Serial.print(c);
    } else {
      Serial.print(".");
    }
    Serial.print("' bytes=");
    Serial.println((int)written);
  }
}

void setup() {
  Serial.begin(115200);
  delay(4000);

  Serial.println("LinkIt ONE classic Bluetooth host probe");
  Serial.println("This sketch uses LBT (classic SPP), not BLE GATT.");
  Serial.println("If PASCO AirLink is BLE-only, scan/connect will likely fail.");

  if (!LBTClient.begin()) {
    Serial.println("LBTClient.begin() failed");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("Bluetooth client ready");
  scanForDevices();
  lastScanAt = millis();
}

void loop() {
  if (!LBTClient.connected()) {
    connectToTarget();

    if (!hasTarget && millis() - lastScanAt >= RESCAN_INTERVAL_MS) {
      scanForDevices();
      lastScanAt = millis();
    }
  }

  bridgeBluetoothToSerial();
  bridgeSerialToBluetooth();
  delay(20);
}
