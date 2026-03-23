#include "BluetoothManager.h"
#include "DisplayManager.h"
#include "ObdManager.h"
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <WiFi.h>

void BluetoothManager::showList(bool fullRedraw) {
  if (fullRedraw) {
    gfx->fillScreen(BLACK);
    DisplayManager::drawTopBar();
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(YELLOW, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(100, 35);
    gfx->println("BT DEVICES");
    gfx->drawLine(0, 40, 320, 40, WHITE);
    gfx->fillTriangle(10, 70, 20, 62, 20, 78, CYAN);
    gfx->fillTriangle(310, 70, 300, 62, 300, 78, CYAN);
    gfx->fillRoundRect(20, 106, 130, 43, 8, GREEN);
    gfx->fillRoundRect(22, 108, 126, 39, 6, BLACK);
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(GREEN, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(42, 134);
    gfx->print("CONNECT");
    gfx->fillRoundRect(170, 106, 130, 43, 8, CYAN);
    gfx->fillRoundRect(172, 108, 126, 39, 6, BLACK);
    gfx->setTextColor(CYAN, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(195, 134);
    gfx->print("DETAILS");
  }
  gfx->fillRect(30, 45, 260, 45, BLACK);
  if (btTotalDevices == 0) {
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(RED, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(40, 75);
    gfx->println("No devices found.");
  } else {
    CachedDevice &dev = btDevices[btSelectedDeviceIndex];
    gfx->setTextColor(WHITE, BLACK);
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextSize(1);

    // Szöveg görgetése ha túl hosszú
    int charWidth = 12;
    String nameToDraw = dev.name;
    int maxChars = 18;
    if (nameToDraw.length() > maxChars) {
      int offset = (millis() / 400) % (nameToDraw.length() - maxChars + 6);
      if (offset > nameToDraw.length() - maxChars)
        offset = nameToDraw.length() - maxChars; // Pause at end
      nameToDraw = nameToDraw.substring(offset, offset + maxChars);
    }
    int textWidth = nameToDraw.length() * charWidth;
    int textX = (320 - textWidth) / 2;
    if (textX < 30)
      textX = 30;
    gfx->setCursor(textX, 75);
    gfx->print(nameToDraw.c_str());

    if (isBluetoothConnected && btSelectedDeviceIndex == 0) {
      gfx->fillRoundRect(22, 108, 126, 39, 6, GREEN);
      gfx->setFont(&FreeSans9pt7b);
      gfx->setTextColor(BLACK, GREEN);
      gfx->setTextSize(1);
      gfx->setCursor(35, 134);
      gfx->print("CONNECTED");
    } else {
      gfx->fillRoundRect(22, 108, 126, 39, 6, BLACK);
      gfx->setFont(&FreeSans9pt7b);
      gfx->setTextColor(GREEN, BLACK);
      gfx->setTextSize(1);
      gfx->setCursor(42, 134);
      gfx->print("CONNECT");
    }

    int scrollbarY = 165;
    int scrollbarX = 20;
    int scrollbarW = 280;
    int scrollbarH = 6;
    gfx->fillRect(scrollbarX, scrollbarY, scrollbarW, scrollbarH, 0x2104);
    if (btTotalDevices > 1) {
      int handleW = scrollbarW / btTotalDevices;
      if (handleW < 10)
        handleW = 10;
      int slideRange = scrollbarW - handleW;
      float scrollPct =
          (float)btSelectedDeviceIndex / (float)(btTotalDevices - 1);
      int handleX = scrollbarX + (int)(scrollPct * slideRange);
      gfx->fillRect(handleX, scrollbarY, handleW, scrollbarH, CYAN);
    } else {
      gfx->fillRect(scrollbarX, scrollbarY, scrollbarW, scrollbarH, CYAN);
    }
  }
}

void BluetoothManager::showDeviceInfo() {
  gfx->fillScreen(BLACK);
  DisplayManager::drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(76, 35);
  gfx->println("DEVICE DETAILS");
  gfx->drawLine(0, 40, 320, 40, WHITE);
  if (btSelectedDeviceIndex >= 0 && btSelectedDeviceIndex < btTotalDevices) {
    CachedDevice &dev = btDevices[btSelectedDeviceIndex];
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(WHITE, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(10, 65);
    gfx->printf("Name: %s", dev.name.c_str());
    gfx->setCursor(10, 95);
    gfx->printf("MAC : %s", dev.address.c_str());
    gfx->setCursor(10, 125);
    gfx->printf("RSSI: %d dBm", dev.rssi);
    gfx->setTextColor(CYAN, BLACK);
    gfx->setCursor(10, 165);
    gfx->println("Swipe Right -> Back");
  }
}

void BluetoothManager::runBLEScan() {
  gfx->fillScreen(BLACK);
  DisplayManager::drawTopBar();
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW);
  gfx->setTextSize(1);
  gfx->setCursor(112, 35);
  gfx->println("BLE SCAN");
  gfx->drawLine(0, 40, 320, 40, WHITE);
  gfx->setFont(&FreeSans18pt7b);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(75, 95);
  gfx->println("Scanning...");

  Serial.println("[BLE] Felkészülés a szkennelésre...");

  if (WiFi.getMode() != WIFI_OFF) {
    WiFi.scanDelete();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(300);
  }

  NimBLEDevice::deinit(true);
  delay(300);
  NimBLEDevice::init("ZoEyee-Scanner");
  NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC); // Fixált gyári MAC – Konnwei ne utasítsa el!

  pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  pBLEScan->setDuplicateFilter(false);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(60);

  pBLEScan->clearResults();

  Serial.printf("[BLE] Szkennelés indítása (scanTime=%d s)... \n", scanTime);
  Serial.flush();
  unsigned long startMillis = millis();

  if (pBLEScan->start(scanTime * 1000, false, true)) {
    Serial.println("[BLE] Rádió aktív, szkennelés folyamatban...");
    delay(200);
    while (pBLEScan->isScanning()) {
      delay(100);
      if (millis() - startMillis > (scanTime * 1000 + 4000))
        break;
    }
    Serial.printf("[BLE] Szkennelés vége %lu ms után.\n", millis() - startMillis);
    Serial.flush();
  } else {
    Serial.println("[BLE] HIBA: pBLEScan->start() sikertelen!");
  }

  NimBLEScanResults foundDevices = pBLEScan->getResults();
  btTotalDevices = foundDevices.getCount();

  Serial.printf("[BLE] Szkennelés befejeződött. Talált eszközök: %d\n", btTotalDevices);
  Serial.flush();

  if (btTotalDevices > MAX_BLE_DEVICES)
    btTotalDevices = MAX_BLE_DEVICES;

  for (int i = 0; i < btTotalDevices; i++) {
    const NimBLEAdvertisedDevice *device = foundDevices.getDevice(i);
    btDevices[i].name = device->getName().length() > 0 ? device->getName().c_str() : "Unknown Device";
    btDevices[i].address = device->getAddress().toString().c_str();
    btDevices[i].bleAddress = device->getAddress();
    btDevices[i].rssi = device->getRSSI();
    Serial.printf("[BLE] Látom: %s [%s] RSSI: %d\n", btDevices[i].name.c_str(), btDevices[i].address.c_str(), btDevices[i].rssi);
  }
  pBLEScan->clearResults();

  btSelectedDeviceIndex = 0;
  currentState = STATE_BT_LIST;
  showList();
}

class MyClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *client) {
    Serial.println("[BLE/NimBLE] Connected to server");
  }
  void onDisconnect(NimBLEClient *client, int reason) {
    Serial.printf("[BLE/NimBLE] Disconnected (reason=%d)\n", reason);
    isBluetoothConnected = false;
    pTxChar = nullptr;
    pRxChar = nullptr;
    bleDisconnectedFlag = true;
  }
};

bool BluetoothManager::connect(int deviceIndex) {
  if (deviceIndex < 0 || deviceIndex >= btTotalDevices)
    return false;
  if (bleConnecting)
    return false;

  bleConnecting = true;
  gfx->fillRect(30, 50, 260, 40, BLACK);
  gfx->setFont(&FreeSans12pt7b);
  gfx->setTextColor(YELLOW, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(85, 75);
  gfx->print("Connecting...");

  NimBLEAddress targetAddr = btDevices[deviceIndex].bleAddress;
  Serial.printf("[BLE] Kapcsolódás: %s [%s]\n", btDevices[deviceIndex].name.c_str(), targetAddr.toString().c_str());

  if (pClient != nullptr) {
    if (pClient->isConnected())
      pClient->disconnect();
    NimBLEDevice::deleteClient(pClient);
    pClient = nullptr;
    delay(200);
  }

  pClient = NimBLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallbacks());

  // KLÓN-KOMPATIBILIS PARAMÉTEREK (Konnwei/Vgate)
  pClient->setConnectionParams(32, 80, 0, 500);
  pClient->setConnectTimeout(5000);

  if (!pClient->connect(targetAddr)) {
    Serial.println("[BLE] Kapcsolódás sikertelen!");
    NimBLEDevice::deleteClient(pClient);
    pClient = nullptr;
    bleConnecting = false;
    return false;
  }

  Serial.println("[BLE] Fizikai kapcsolat sikeres! Várakozás a GATT felépülésre (1.5 mp)...");
  delay(1500);

  auto pServices = pClient->getServices(true);
  if (pServices.empty()) {
    Serial.println("[BLE] HIBA: Nem sikerültek a szolgáltatások lekérdezései!");
    pClient->disconnect();
    bleConnecting = false;
    return false;
  }

  pTxChar = nullptr;
  pRxChar = nullptr;
  NimBLERemoteService *pObdSvc = nullptr;

  for (auto pSvc : pServices) {
    String svcUUID = pSvc->getUUID().toString().c_str();
    svcUUID.toLowerCase();
    Serial.printf("[BLE] Látott szolgáltatás: %s\n", svcUUID.c_str());
    if (svcUUID.indexOf("fff0") >= 0 || svcUUID.indexOf("ffe0") >= 0) {
      pObdSvc = pSvc;
      break;
    }
  }

  if (pObdSvc == nullptr) {
    for (auto pSvc : pServices) {
      String svcUUID = pSvc->getUUID().toString().c_str();
      svcUUID.toLowerCase();
      if (svcUUID.indexOf("ae30") >= 0) {
        pObdSvc = pSvc;
        break;
      }
    }
  }

  if (pObdSvc != nullptr) {
    Serial.printf("[BLE]   >>> OBD/KONNWEI SZOLGÁLTATÁS MEGTALÁLVA (%s)! <<<\n", pObdSvc->getUUID().toString().c_str());
    auto pChars = pObdSvc->getCharacteristics(true);
    if (!pChars.empty()) {
      for (auto pChr : pChars) {
        String charUUID = pChr->getUUID().toString().c_str();
        charUUID.toLowerCase();
        bool writable = pChr->canWrite() || pChr->canWriteNoResponse();
        bool notifiable = pChr->canNotify() || pChr->canIndicate();
        Serial.printf("[BLE]   Karakterisztika: %s (Write=%d, Notify=%d)\n", charUUID.c_str(), writable, notifiable);
        if (notifiable && pRxChar == nullptr)
          pRxChar = pChr;
        else if (writable && pTxChar == nullptr)
          pTxChar = pChr;
      }
    }
  }

  if (pTxChar == nullptr || pRxChar == nullptr) {
    pTxChar = nullptr;
    pRxChar = nullptr;
    for (auto pSvc : pServices) {
      String svcUUID = pSvc->getUUID().toString().c_str();
      svcUUID.toLowerCase();
      if (svcUUID.indexOf("1800") >= 0 || svcUUID.indexOf("1801") >= 0)
        continue;
      auto pChars = pSvc->getCharacteristics(true);
      if (!pChars.empty()) {
        for (auto pChr : pChars) {
          if ((pChr->canWrite() || pChr->canWriteNoResponse()) && pTxChar == nullptr)
            pTxChar = pChr;
          else if (pChr->canNotify() && pRxChar == nullptr)
            pRxChar = pChr;
        }
      }
      if (pTxChar && pRxChar) {
        Serial.printf("[BLE] Fallback SPP megtalálva a %s szolgáltatásban!\n", pSvc->getUUID().toString().c_str());
        break;
      }
    }
  }

  if (pTxChar == nullptr || pRxChar == nullptr) {
    Serial.println("[BLE] HIBA: Nem találom a TX/RX csatornákat!");
    gfx->fillRect(30, 50, 260, 60, BLACK);
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(RED, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(40, 75);
    gfx->print("No OBD service found!");
    delay(2000);
    disconnect();
    return false;
  }

  if (pRxChar->canNotify()) {
    pRxChar->subscribe(true, ObdManager::onBLENotify);
    delay(200);
    NimBLERemoteDescriptor *p2902 = pRxChar->getDescriptor(NimBLEUUID((uint16_t)0x2902));
    if (p2902 != nullptr) {
      uint8_t notifyOn[] = {0x01, 0x00};
      p2902->writeValue(notifyOn, 2, true);
      Serial.println("[BLE]   >>> CCCD (2902) Descriptor manuálisan engedélyezve! <<<");
    } else {
      Serial.println("[BLE]   >>> FIGYELEM: Nincs 2902 CCCD Descriptor! <<<");
    }
  }

  pRxChar->subscribe(true, ObdManager::onBLENotify);
  isBluetoothConnected = true;
  bleConnecting = false;

  obdBufIndex = 0;
  obdBuffer[0] = '\0';
  lastOBDValue = "";

  gfx->fillRect(30, 50, 260, 40, BLACK);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(YELLOW, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(70, 75);
  gfx->print("Verifying OBD...");

  delay(500);
  
  if (!ObdManager::initOBD()) {
    disconnect();
    return false;
  }
  
  return true;
}

void BluetoothManager::disconnect() {
  Serial.println("[BLE] Leválasztás kezdeményezése...");
  if (pClient != nullptr && pClient->isConnected()) {
    if (pRxChar != nullptr) {
      pRxChar->unsubscribe();
      delay(100);
    }
    pClient->disconnect();
    delay(500);
  }
  isBluetoothConnected = false;
  pTxChar = nullptr;
  pRxChar = nullptr;
  lastOBDValue = "";
  obdBufIndex = 0;
  obdBuffer[0] = '\0';
  Serial.println("[BLE] Sikeresen leválasztva az OBD-ről.");
}
