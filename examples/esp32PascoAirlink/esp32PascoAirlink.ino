#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>
#include <BLERemoteDescriptor.h>

#include <map>

const char TARGET_NAME_SUBSTRING[] = "AirLink";
const char TARGET_MAC[] = "80:6f:b0:74:88:6f";
const uint32_t SCAN_SECONDS = 7;

BLEScan *pScan = nullptr;
BLEAdvertisedDevice *targetDevice = nullptr;
BLEClient *pClient = nullptr;

bool doConnect = false;
bool connected = false;
bool doScan = false;

BLERemoteCharacteristic *preferredWriteCharacteristic = nullptr;
BLERemoteCharacteristic *pascoWrite0 = nullptr;
BLERemoteCharacteristic *pascoWrite1 = nullptr;
BLERemoteCharacteristic *pascoNotify0 = nullptr;
BLERemoteCharacteristic *pascoNotify1 = nullptr;
BLERemoteCharacteristic *pascoReadNotify = nullptr;

const char PASCO_SERVICE_0_UUID[] = "4a5c0000-0000-0000-0000-5c1e741f1c00";
const char PASCO_SERVICE_1_UUID[] = "4a5c0001-0000-0000-0000-5c1e741f1c00";
const char PASCO_WRITE_CHAR_PRIMARY[] = "4a5c0001-0003-0000-0000-5c1e741f1c00";
const char PASCO_WRITE_CHAR_FALLBACK[] = "4a5c0000-0003-0000-0000-5c1e741f1c00";
const char PASCO_NOTIFY_CHAR_0[] = "4a5c0000-0002-0000-0000-5c1e741f1c00";
const char PASCO_NOTIFY_CHAR_1[] = "4a5c0001-0002-0000-0000-5c1e741f1c00";
const char PASCO_READ_NOTIFY_CHAR[] = "4a5c0001-0004-0000-0000-5c1e741f1c00";
const uint8_t PASCO_READ_ONE_SAMPLE_CMD = 0x05;
const uint8_t PASCO_SOIL_MOISTURE_PACKET_SIZE = 0x02;

String hexEncode(const uint8_t *data, size_t len) {
  String out;
  for (size_t i = 0; i < len; ++i) {
    if (data[i] < 0x10) {
      out += "0";
    }
    out += String(data[i], HEX);
    if (i + 1 < len) {
      out += " ";
    }
  }
  out.toUpperCase();
  return out;
}

String safeAscii(const uint8_t *data, size_t len) {
  String out;
  for (size_t i = 0; i < len; ++i) {
    char c = (char)data[i];
    if (c >= 32 && c <= 126) {
      out += c;
    } else {
      out += ".";
    }
  }
  return out;
}

String formatProperties(BLERemoteCharacteristic *chr) {
  String props;
  if (chr->canRead()) props += "R";
  if (chr->canWrite()) props += "W";
  if (chr->canWriteNoResponse()) props += "N";
  if (chr->canNotify()) props += "T";
  if (chr->canIndicate()) props += "I";
  if (chr->canBroadcast()) props += "B";
  if (props.length() == 0) props = "-";
  return props;
}

String channelName(BLERemoteCharacteristic *chr) {
  if (chr == nullptr) {
    return "(null)";
  }

  String uuid = chr->getUUID().toString().c_str();
  if (uuid.equalsIgnoreCase(PASCO_WRITE_CHAR_PRIMARY)) return "pasco-write-1";
  if (uuid.equalsIgnoreCase(PASCO_WRITE_CHAR_FALLBACK)) return "pasco-write-0";
  if (uuid.equalsIgnoreCase(PASCO_NOTIFY_CHAR_0)) return "pasco-notify-0";
  if (uuid.equalsIgnoreCase(PASCO_NOTIFY_CHAR_1)) return "pasco-notify-1";
  if (uuid.equalsIgnoreCase(PASCO_READ_NOTIFY_CHAR)) return "pasco-read-notify";
  return uuid;
}

void printPacket(const char *direction, BLERemoteCharacteristic *chr, const uint8_t *data, size_t length) {
  Serial.print("[");
  Serial.print(millis());
  Serial.println(" ms]");
  Serial.print(direction);
  Serial.print(" ");
  Serial.print(channelName(chr));
  Serial.print(" ");
  Serial.println(chr->getUUID().toString().c_str());
  Serial.print("HEX: ");
  Serial.println(hexEncode(data, length));
  Serial.print("ASCII: ");
  Serial.println(safeAscii(data, length));

  if (length == 6 && data[0] == 0x85) {
    uint16_t raw16 = (uint16_t)data[1] | ((uint16_t)data[2] << 8);
    float normalized = raw16 / 4095.0f;
    Serial.print("Decoded 0x85 frame: likelyRawADC=");
    Serial.print(raw16);
    Serial.print(" normalized=");
    Serial.print(normalized, 4);
    Serial.print(" stateByte=");
    Serial.print((int)data[3]);
    Serial.print(" flags=");
    Serial.print((int)data[4]);
    Serial.print(",");
    Serial.println((int)data[5]);
  }

  if (length == 3 && data[0] == 0x82) {
    Serial.print("Decoded 0x82 frame: a=");
    Serial.print((int)data[1]);
    Serial.print(" b=");
    Serial.println((int)data[2]);
  }

  if (length >= 5 && data[0] == 0xC0 && data[1] == 0x00 && data[2] == PASCO_READ_ONE_SAMPLE_CMD) {
    Serial.print("Decoded READ_ONE_SAMPLE response payload bytes=");
    Serial.println((int)(length - 3));

    if (length >= 5) {
      uint16_t raw16 = (uint16_t)data[3] | ((uint16_t)data[4] << 8);
      Serial.print("Decoded READ_ONE_SAMPLE raw16=");
      Serial.print(raw16);
      Serial.print(" normalized=");
      Serial.println(raw16 / 4095.0f, 4);
    }
  }
}

bool parseHexString(const String &text, uint8_t *buffer, size_t &outLen) {
  String cleaned;
  for (size_t i = 0; i < text.length(); ++i) {
    char c = text.charAt(i);
    if (c != ' ' && c != '\t') {
      cleaned += c;
    }
  }

  if (cleaned.length() == 0 || (cleaned.length() % 2) != 0) {
    return false;
  }

  outLen = 0;
  for (size_t i = 0; i < cleaned.length(); i += 2) {
    char hi = cleaned.charAt(i);
    char lo = cleaned.charAt(i + 1);
    int high = isxdigit(hi) ? strtol(String(hi).c_str(), nullptr, 16) : -1;
    int low = isxdigit(lo) ? strtol(String(lo).c_str(), nullptr, 16) : -1;
    if (high < 0 || low < 0) {
      return false;
    }
    buffer[outLen++] = (uint8_t)((high << 4) | low);
  }
  return true;
}

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  help");
  Serial.println("  send <text>     -> send ASCII to selected target");
  Serial.println("  send0 <text>    -> send ASCII to pasco-write-0");
  Serial.println("  send1 <text>    -> send ASCII to pasco-write-1");
  Serial.println("  hex <bytes>     -> send hex to selected target, example: hex 01 02 0A FF");
  Serial.println("  hex0 <bytes>    -> send hex to pasco-write-0");
  Serial.println("  hex1 <bytes>    -> send hex to pasco-write-1");
  Serial.println("  targets         -> print current PASCO channel assignments");
  Serial.println("  probe           -> run canned probes on both PASCO write channels");
  Serial.println("  probe0          -> run canned probes on pasco-write-0");
  Serial.println("  probe1          -> run canned probes on pasco-write-1");
  Serial.println("  pasco           -> run PASCO-style command probes on both channels");
  Serial.println("  pasco0          -> run PASCO-style command probes on pasco-write-0");
  Serial.println("  pasco1          -> run PASCO-style command probes on pasco-write-1");
  Serial.println("  sample0         -> send PASCO READ_ONE_SAMPLE [05 02] to pasco-write-0");
  Serial.println("  sample1         -> send PASCO READ_ONE_SAMPLE [05 02] to pasco-write-1");
}

void printTargets() {
  Serial.println("Current channel assignments:");
  if (pascoWrite0) {
    Serial.print("  pasco-write-0: ");
    Serial.println(pascoWrite0->getUUID().toString().c_str());
  }
  if (pascoWrite1) {
    Serial.print("  pasco-write-1: ");
    Serial.println(pascoWrite1->getUUID().toString().c_str());
  }
  if (pascoNotify0) {
    Serial.print("  pasco-notify-0: ");
    Serial.println(pascoNotify0->getUUID().toString().c_str());
  }
  if (pascoNotify1) {
    Serial.print("  pasco-notify-1: ");
    Serial.println(pascoNotify1->getUUID().toString().c_str());
  }
  if (pascoReadNotify) {
    Serial.print("  pasco-read-notify: ");
    Serial.println(pascoReadNotify->getUUID().toString().c_str());
  }
  if (preferredWriteCharacteristic) {
    Serial.print("  selected-write: ");
    Serial.println(preferredWriteCharacteristic->getUUID().toString().c_str());
  }
}

bool writePacket(BLERemoteCharacteristic *target, const uint8_t *data, size_t len) {
  if (!connected || target == nullptr) {
    Serial.println("Write target unavailable");
    return false;
  }

  bool ok = target->writeValue((uint8_t *)data, len, target->canWrite());
  printPacket("TX", target, data, len);
  Serial.print("Write result: ");
  Serial.println(ok ? "true" : "false");
  return ok;
}

void delayWithBackground(unsigned long durationMs) {
  unsigned long started = millis();
  while (millis() - started < durationMs) {
    delay(20);
  }
}

void runProbeSequence(BLERemoteCharacteristic *target, const char *label) {
  if (target == nullptr) {
    Serial.print("Probe target unavailable: ");
    Serial.println(label);
    return;
  }

  Serial.print("Running probe sequence on ");
  Serial.println(label);

  const uint8_t probeA[] = {0x01};
  const uint8_t probeB[] = {0x00};
  const uint8_t probeC[] = {0x01, 0x00};
  const uint8_t probeD[] = {0x00, 0x01};
  const uint8_t probeE[] = {0x02};
  const uint8_t probeF[] = {0x7F};

  writePacket(target, probeA, sizeof(probeA));
  delayWithBackground(500);
  writePacket(target, probeB, sizeof(probeB));
  delayWithBackground(500);
  writePacket(target, probeC, sizeof(probeC));
  delayWithBackground(500);
  writePacket(target, probeD, sizeof(probeD));
  delayWithBackground(500);
  writePacket(target, probeE, sizeof(probeE));
  delayWithBackground(500);
  writePacket(target, probeF, sizeof(probeF));
  delayWithBackground(500);
}

void runPascoCommandSequence(BLERemoteCharacteristic *target, const char *label) {
  if (target == nullptr) {
    Serial.print("PASCO command target unavailable: ");
    Serial.println(label);
    return;
  }

  Serial.print("Running PASCO-style command sequence on ");
  Serial.println(label);

  const uint8_t keepAlive[] = {0x00};
  const uint8_t readOneSample[] = {0x05};
  const uint8_t readOneSampleSized[] = {PASCO_READ_ONE_SAMPLE_CMD, PASCO_SOIL_MOISTURE_PACKET_SIZE};
  const uint8_t burstTransfer[] = {0x0E};
  const uint8_t customDetect[] = {0x37, 0x08};
  const uint8_t customRead1[] = {0x37, 0x01, 0x00};
  const uint8_t customRead2[] = {0x37, 0x05};
  const uint8_t customBurst[] = {0x37, 0x0E};

  writePacket(target, keepAlive, sizeof(keepAlive));
  delayWithBackground(1200);
  writePacket(target, readOneSample, sizeof(readOneSample));
  delayWithBackground(1200);
  writePacket(target, readOneSampleSized, sizeof(readOneSampleSized));
  delayWithBackground(1200);
  writePacket(target, burstTransfer, sizeof(burstTransfer));
  delayWithBackground(1200);
  writePacket(target, customDetect, sizeof(customDetect));
  delayWithBackground(1200);
  writePacket(target, customRead1, sizeof(customRead1));
  delayWithBackground(1200);
  writePacket(target, customRead2, sizeof(customRead2));
  delayWithBackground(1200);
  writePacket(target, customBurst, sizeof(customBurst));
  delayWithBackground(1200);
}

void runOneSampleRequest(BLERemoteCharacteristic *target, const char *label) {
  if (target == nullptr) {
    Serial.print("READ_ONE_SAMPLE target unavailable: ");
    Serial.println(label);
    return;
  }

  Serial.print("Sending PASCO READ_ONE_SAMPLE to ");
  Serial.println(label);

  const uint8_t readOneSampleSized[] = {PASCO_READ_ONE_SAMPLE_CMD, PASCO_SOIL_MOISTURE_PACKET_SIZE};
  writePacket(target, readOneSampleSized, sizeof(readOneSampleSized));
}

int characteristicPriority(BLERemoteService *service, BLERemoteCharacteristic *chr) {
  String serviceUuid = service->getUUID().toString().c_str();
  String charUuid = chr->getUUID().toString().c_str();

  if (charUuid.equalsIgnoreCase(PASCO_WRITE_CHAR_PRIMARY)) {
    return 100;
  }

  if (charUuid.equalsIgnoreCase(PASCO_WRITE_CHAR_FALLBACK)) {
    return 90;
  }

  if (serviceUuid.equalsIgnoreCase(PASCO_SERVICE_1_UUID) && (chr->canWrite() || chr->canWriteNoResponse())) {
    return 80;
  }

  if (serviceUuid.equalsIgnoreCase(PASCO_SERVICE_0_UUID) && (chr->canWrite() || chr->canWriteNoResponse())) {
    return 70;
  }

  if (chr->canWrite()) {
    return 60;
  }

  if (chr->canWriteNoResponse()) {
    return 50;
  }

  return 0;
}

void printAdvertisedDevice(BLEAdvertisedDevice advertisedDevice) {
  Serial.println("---- Device ----");
  Serial.print("Address: ");
  Serial.println(advertisedDevice.getAddress().toString().c_str());

  Serial.print("RSSI: ");
  Serial.println(advertisedDevice.getRSSI());

  Serial.print("Connectable: ");
  Serial.println(advertisedDevice.isConnectable() ? "yes" : "no");

  if (advertisedDevice.haveName()) {
    Serial.print("Name: ");
    Serial.println(advertisedDevice.getName().c_str());
  }

  if (advertisedDevice.haveManufacturerData()) {
    String md = advertisedDevice.getManufacturerData();
    Serial.print("Manufacturer data: ");
    Serial.println(hexEncode((const uint8_t *)md.c_str(), md.length()));
  }

  if (advertisedDevice.haveServiceUUID()) {
    int count = advertisedDevice.getServiceUUIDCount();
    for (int i = 0; i < count; ++i) {
      Serial.print("Service UUID[");
      Serial.print(i);
      Serial.print("]: ");
      Serial.println(advertisedDevice.getServiceUUID(i).toString().c_str());
    }
  }

  if (advertisedDevice.haveServiceData()) {
    int count = advertisedDevice.getServiceDataCount();
    for (int i = 0; i < count; ++i) {
      String data = advertisedDevice.getServiceData(i);
      Serial.print("Service data[");
      Serial.print(i);
      Serial.print("] uuid=");
      Serial.print(advertisedDevice.getServiceDataUUID(i).toString().c_str());
      Serial.print(" data=");
      Serial.println(hexEncode((const uint8_t *)data.c_str(), data.length()));
    }
  }
}

bool matchesTarget(BLEAdvertisedDevice advertisedDevice) {
  if (TARGET_MAC[0] != '\0') {
    if (String(advertisedDevice.getAddress().toString().c_str()).equalsIgnoreCase(TARGET_MAC)) {
      return true;
    }
  }

  if (TARGET_NAME_SUBSTRING[0] != '\0' && advertisedDevice.haveName()) {
    String name = advertisedDevice.getName().c_str();
    if (name.indexOf(TARGET_NAME_SUBSTRING) >= 0) {
      return true;
    }
  }

  return false;
}

static void notifyCallback(BLERemoteCharacteristic *chr, uint8_t *data, size_t length, bool isNotify) {
  Serial.print(isNotify ? "NOTIFY " : "INDICATE ");
  printPacket("RX", chr, data, length);
}

class MyClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient *client) {
    Serial.print("Connected to ");
    Serial.println(client->getPeerAddress().toString().c_str());
  }

  void onDisconnect(BLEClient *client) {
    connected = false;
    preferredWriteCharacteristic = nullptr;
    pascoWrite0 = nullptr;
    pascoWrite1 = nullptr;
    pascoNotify0 = nullptr;
    pascoNotify1 = nullptr;
    pascoReadNotify = nullptr;
    Serial.print("Disconnected from ");
    Serial.println(client->getPeerAddress().toString().c_str());
  }
};

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    printAdvertisedDevice(advertisedDevice);

    if (matchesTarget(advertisedDevice)) {
      Serial.println("Target match found");
      BLEDevice::getScan()->stop();

      if (targetDevice != nullptr) {
        delete targetDevice;
      }
      targetDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void readIfPossible(BLERemoteCharacteristic *chr) {
  if (!chr->canRead()) {
    return;
  }

  String value = chr->readValue();
  Serial.print("  Initial read HEX: ");
  Serial.println(hexEncode((const uint8_t *)value.c_str(), value.length()));
  Serial.print("  Initial read ASCII: ");
  Serial.println(safeAscii((const uint8_t *)value.c_str(), value.length()));
}

void registerNotificationsIfPossible(BLERemoteCharacteristic *chr) {
  if (chr->canNotify() || chr->canIndicate()) {
    chr->registerForNotify(notifyCallback, chr->canNotify());
    Serial.println("  Subscribed for updates");
  }
}

void inspectRemoteDatabase() {
  std::map<std::string, BLERemoteService *> *services = pClient->getServices();
  if (services == nullptr) {
    Serial.println("No remote services map available");
    return;
  }

  Serial.print("Service count: ");
  Serial.println((int)services->size());

  int bestWritePriority = -1;

  for (std::map<std::string, BLERemoteService *>::iterator it = services->begin(); it != services->end(); ++it) {
    BLERemoteService *service = it->second;
    Serial.println("==== Service ====");
    Serial.print("UUID: ");
    Serial.println(service->getUUID().toString().c_str());

    std::map<std::string, BLERemoteCharacteristic *> *chars = service->getCharacteristics();
    Serial.print("Characteristic count: ");
    Serial.println((int)chars->size());

    for (std::map<std::string, BLERemoteCharacteristic *>::iterator cit = chars->begin(); cit != chars->end(); ++cit) {
      BLERemoteCharacteristic *chr = cit->second;
      Serial.print("Characteristic: ");
      Serial.println(chr->getUUID().toString().c_str());
      Serial.print("  Props: ");
      Serial.println(formatProperties(chr));

      String chrUuid = chr->getUUID().toString().c_str();
      if (chrUuid.equalsIgnoreCase(PASCO_WRITE_CHAR_FALLBACK)) {
        pascoWrite0 = chr;
      }
      if (chrUuid.equalsIgnoreCase(PASCO_WRITE_CHAR_PRIMARY)) {
        pascoWrite1 = chr;
      }
      if (chrUuid.equalsIgnoreCase(PASCO_NOTIFY_CHAR_0)) {
        pascoNotify0 = chr;
      }
      if (chrUuid.equalsIgnoreCase(PASCO_NOTIFY_CHAR_1)) {
        pascoNotify1 = chr;
      }
      if (chrUuid.equalsIgnoreCase(PASCO_READ_NOTIFY_CHAR)) {
        pascoReadNotify = chr;
      }

      readIfPossible(chr);
      registerNotificationsIfPossible(chr);

      int priority = characteristicPriority(service, chr);
      if (priority > bestWritePriority) {
        bestWritePriority = priority;
        preferredWriteCharacteristic = chr;
        Serial.print("  Candidate Serial->BLE write target priority=");
        Serial.println(priority);
      }
    }
  }

  if (preferredWriteCharacteristic != nullptr) {
    Serial.print("Selected final Serial->BLE write target: ");
    Serial.println(preferredWriteCharacteristic->getUUID().toString().c_str());
  }
  printTargets();
}

bool connectToServer() {
  if (targetDevice == nullptr) {
    Serial.println("No target device captured from scan");
    return false;
  }

  Serial.print("Connecting to ");
  Serial.println(targetDevice->getAddress().toString().c_str());

  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallbacks());

  if (!pClient->connect(targetDevice)) {
    Serial.println("Connect failed");
    return false;
  }

  pClient->setMTU(517);
  Serial.print("MTU requested, current value: ");
  Serial.println(pClient->getMTU());

  inspectRemoteDatabase();
  connected = true;
  return true;
}

void processCommand(String line) {
  line.trim();
  if (line.length() == 0) {
    return;
  }

  if (line.equalsIgnoreCase("help")) {
    printHelp();
    return;
  }

  if (line.equalsIgnoreCase("targets")) {
    printTargets();
    return;
  }

  if (line.equalsIgnoreCase("probe")) {
    runProbeSequence(pascoWrite1 != nullptr ? pascoWrite1 : preferredWriteCharacteristic, "selected/pasco-write-1");
    runProbeSequence(pascoWrite0, "pasco-write-0");
    return;
  }

  if (line.equalsIgnoreCase("probe0")) {
    runProbeSequence(pascoWrite0, "pasco-write-0");
    return;
  }

  if (line.equalsIgnoreCase("probe1")) {
    runProbeSequence(pascoWrite1 != nullptr ? pascoWrite1 : preferredWriteCharacteristic, "pasco-write-1");
    return;
  }

  if (line.equalsIgnoreCase("pasco")) {
    runPascoCommandSequence(pascoWrite1 != nullptr ? pascoWrite1 : preferredWriteCharacteristic, "selected/pasco-write-1");
    runPascoCommandSequence(pascoWrite0, "pasco-write-0");
    return;
  }

  if (line.equalsIgnoreCase("pasco0")) {
    runPascoCommandSequence(pascoWrite0, "pasco-write-0");
    return;
  }

  if (line.equalsIgnoreCase("pasco1")) {
    runPascoCommandSequence(pascoWrite1 != nullptr ? pascoWrite1 : preferredWriteCharacteristic, "pasco-write-1");
    return;
  }

  if (line.equalsIgnoreCase("sample0")) {
    runOneSampleRequest(pascoWrite0, "pasco-write-0");
    return;
  }

  if (line.equalsIgnoreCase("sample1")) {
    runOneSampleRequest(pascoWrite1 != nullptr ? pascoWrite1 : preferredWriteCharacteristic, "pasco-write-1");
    return;
  }

  BLERemoteCharacteristic *target = preferredWriteCharacteristic;
  String payload = line;
  bool hexMode = false;

  if (line.startsWith("send0 ")) {
    target = pascoWrite0;
    payload = line.substring(6);
  } else if (line.startsWith("send1 ")) {
    target = pascoWrite1;
    payload = line.substring(6);
  } else if (line.startsWith("send ")) {
    payload = line.substring(5);
  } else if (line.startsWith("hex0 ")) {
    target = pascoWrite0;
    payload = line.substring(5);
    hexMode = true;
  } else if (line.startsWith("hex1 ")) {
    target = pascoWrite1;
    payload = line.substring(5);
    hexMode = true;
  } else if (line.startsWith("hex ")) {
    payload = line.substring(4);
    hexMode = true;
  }

  if (hexMode) {
    uint8_t buffer[128];
    size_t len = 0;
    if (!parseHexString(payload, buffer, len)) {
      Serial.println("Invalid hex payload");
      return;
    }
    writePacket(target, buffer, len);
    return;
  }

  writePacket(target, (const uint8_t *)payload.c_str(), payload.length());
}

void forwardSerialToBle() {
  static String line;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      if (line.length() > 0) {
        processCommand(line);
        line = "";
      }
    } else {
      line += c;
    }
  }
}

void startScan() {
  Serial.println();
  Serial.println("Starting BLE scan...");
  pScan->start(SCAN_SECONDS, false);
  Serial.println("Scan done");
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("ESP32 PASCO AirLink BLE probe");
  Serial.println("Scanning for PASCO-like BLE devices and dumping GATT data.");
  Serial.println("Use Serial commands after connect. Type 'help' for options.");

  BLEDevice::init("");
  pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pScan->setActiveScan(true);
  pScan->setInterval(100);
  pScan->setWindow(99);

  startScan();
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Connected and inspected remote GATT database.");
    } else {
      Serial.println("Connection failed.");
    }
    doConnect = false;
  }

  if (!connected && doScan) {
    delay(1000);
    startScan();
  }

  forwardSerialToBle();
  delay(20);
}
