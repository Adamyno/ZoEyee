#include "ObdManager.h"
#include "DisplayManager.h"
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// Külső UI frissítő hívás definiálása, hogy a parser elérje (ez később a DisplayManagerbe kerül)

int parseUDSHex(const String &resp, const char *expectedPrefix, int byteCount) {
  String r = resp;
  r.trim();
  r.replace(" ", "");
  String prefix = String(expectedPrefix);
  prefix.replace(" ", "");
  int idx = r.indexOf(prefix);
  if (idx < 0)
    return -1;
  String hexPart =
      r.substring(idx + prefix.length(), idx + prefix.length() + byteCount * 2);
  return (int)strtol(hexPart.c_str(), NULL, 16);
}

// Új segédfüggvény specifikus bitek kiolvasásához (CanZE bit indexek pl. startBit=52, endBit=61)
int parseUDSBits(const String &resp, const char *expectedPrefix, int startBit, int endBit) {
  String r = resp;
  r.trim();
  r.replace(" ", "");
  String prefix = String(expectedPrefix);
  prefix.replace(" ", "");
  int idx = r.indexOf(prefix);
  if (idx < 0)
    return -1;
  
  // A válasz hex stringje prefixszel együtt (a bit index a legelső bájttól indul a CanZE szerint)
  String fullHex = r.substring(idx);
  
  // startBit és endBit az üzenet elejétől számítva van (ahol byte 0 = 0..7 bit)
  int startByte = startBit / 8;
  int endByte = endBit / 8;
  
  if (fullHex.length() < (unsigned int)(endByte + 1) * 2) {
    return -1; // Nincs elég adat
  }
  
  uint64_t val = 0;
  for (int i = startByte; i <= endByte; i++) {
    String byteStr = fullHex.substring(i * 2, i * 2 + 2);
    val = (val << 8) | strtol(byteStr.c_str(), NULL, 16);
  }
  
  int bitsToExtract = endBit - startBit + 1;
  int shiftRight = (8 - ((endBit + 1) % 8)) % 8; // Hátralévő bitek a byte végéig
  
  val = val >> shiftRight;
  uint64_t mask = (1ULL << bitsToExtract) - 1;
  return (int)(val & mask);
}

void ObdManager::onBLENotify(NimBLERemoteCharacteristic *pChar, uint8_t *pData, size_t length, bool isNotify) {
  Serial.print("[OBD] RX RAW: ");
  for (size_t i = 0; i < length; i++)
    Serial.printf("%02X ", pData[i]);
  Serial.println();

  for (size_t i = 0; i < length; i++) {
    char c = (char)pData[i];
    if (c == '>') {
      obdBuffer[obdBufIndex] = '\0';

      String fullResponse = "";
      char *ptr = obdBuffer;
      while (*ptr) {
        char *lineStart = ptr;
        while (*ptr && *ptr != '\r')
          ptr++;
        if (*ptr == '\r') {
          *ptr = '\0';
          ptr++;
        }

        char *s = lineStart;
        while (*s == ' ')
          s++;
        int l = strlen(s);
        while (l > 0 && (s[l - 1] == ' ' || s[l - 1] == '\n'))
          s[--l] = '\0';

        if (strlen(s) == 0)
          continue;
        String line = String(s);
        line.trim();
        if (line.length() == 0)
          continue;

        String lineUpper = line;
        lineUpper.toUpperCase();

        if (lineUpper.startsWith("AT") && lineUpper.indexOf(' ') < 0 &&
            lineUpper.length() <= 6)
          continue;

        if (lineUpper.length() <= 3 && lineUpper.indexOf(' ') < 0) {
          continue;
        }

        if (lineUpper.length() >= 2 && lineUpper.charAt(1) == ':') {
          lineUpper = lineUpper.substring(2);
          lineUpper.trim();
        } else if (lineUpper.length() >= 3 && lineUpper.charAt(2) == ':') {
          lineUpper = lineUpper.substring(3);
          lineUpper.trim();
        }

        if (fullResponse.length() > 0)
          fullResponse += " ";
        fullResponse += lineUpper;
      }

      obdBufIndex = 0;
      obdBuffer[0] = '\0';
      if (fullResponse.length() == 0)
        continue;

      lastOBDValue = fullResponse;
      Serial.printf("[OBD] Response complete: '%s'\n", lastOBDValue.c_str());
    } else if (c != '\n') {
      if (obdBufIndex < (int)sizeof(obdBuffer) - 1)
        obdBuffer[obdBufIndex++] = c;
    }
  }
}

void ObdManager::sendCommand(const char *cmd) {
  if (pTxChar != nullptr && isBluetoothConnected) {
    char fullCmd[64];
    snprintf(fullCmd, sizeof(fullCmd), "%s\r", cmd);
    pTxChar->writeValue((uint8_t *)fullCmd, strlen(fullCmd));
    Serial.printf("[OBD] Sent: %s\n", cmd);
  }
}

bool ObdManager::initOBD() {
  bool isELM = false;
  const char *atzCommands[] = {"ATZ", "ATZ\n"};

  for (int cmdIdx = 0; cmdIdx < 2 && !isELM; cmdIdx++) {
    sendCommand(atzCommands[cmdIdx]);
    unsigned long verifyStart = millis();
    while (millis() - verifyStart < 2500) {
      delay(50);
      if (lastOBDValue.length() > 0) {
        Serial.printf("[OBD] ATZ Válasz: '%s'\n", lastOBDValue.c_str());
        String resp = lastOBDValue;
        resp.toUpperCase();
        if (resp.indexOf("ELM") >= 0 || resp.indexOf("KONNWEI") >= 0 ||
            resp.indexOf("OBD") >= 0 || resp.indexOf("KW") >= 0 ||
            resp.indexOf("V1.5") >= 0 || resp.indexOf("OK") >= 0) {
          isELM = true;
          break;
        }
        lastOBDValue = "";
      }
    }
    if (!isELM)
      Serial.printf("[BLE] Nincs válasz a(z) %s parancsra, újrapróbálkozás...\n", atzCommands[cmdIdx]);
  }

  if (!isELM) {
    Serial.println("[BLE] Nem válaszolt mint ELM327. Lecsatlakozás.");
    gfx->fillRect(30, 50, 260, 60, BLACK);
    gfx->setFont(&FreeSans12pt7b);
    gfx->setTextColor(RED, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(40, 75);
    gfx->print("Not an OBD device");
    delay(2000);
    return false;
  }

  Serial.printf("[BLE] ELM327 hitelesítve: %s\n", lastOBDValue.c_str());
  sendCommand("ATE0");
  delay(500);

  Serial.println("[OBD] ZOE CAN protokoll beállítás...");
  gfx->fillRect(30, 50, 260, 40, BLACK);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(YELLOW, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(50, 75);
  gfx->print("Setting up CAN...");

  sendCommand("ATSP6"); delay(500); lastOBDValue = "";
  
  // ========================================================
  // FLOW CONTROL ALAPBEÁLLÍTÁSOK A MULTI-FRAME ÜZENETEKHEZ
  // ========================================================
  sendCommand("ATFCSD300000"); delay(300); lastOBDValue = ""; // FC Data
  sendCommand("ATFCSM1"); delay(300); lastOBDValue = "";      // FC Mode 1
  
  // Alapértelmezett ECU (EVC) megcélzása
  sendCommand("ATSH7E4"); delay(300); lastOBDValue = "";
  sendCommand("ATCRA7EC"); delay(300); lastOBDValue = "";
  sendCommand("ATFCSH7E4"); delay(300); lastOBDValue = "";    // FC Header az EVC-hez
  
  sendCommand("10C0"); delay(500); lastOBDValue = "";

  obdZoeMode = true;
  obdPollIndex = 0;
  obdSOC = -1;
  obdSOH = -1;
  obdHVBatTemp = -99;
  obd12V = "";
  obdResponsePending = false;
  lastOBDSentTime = 0;
  lastOBDPollTime = 0;
  Serial.println("[BLE] OBD adapter felállt, ZOE mód aktív!");
  return true;
}

void ObdManager::processPolling() {
  bool shouldSendNext = false;
  unsigned long now = millis();
  if (!obdResponsePending) {
    shouldSendNext = true;
  } else if (lastOBDValue.length() > 0) {
    if (now - lastOBDPollTime >= OBD_NEXT_REQUEST_DELAY) {
      shouldSendNext = true;
    }
  } else if (now - lastOBDSentTime >= OBD_RESPONSE_TIMEOUT) {
    Serial.printf("[OBD] Timeout a %lu ms-os kérésnél, következő PID...\n", OBD_RESPONSE_TIMEOUT);
    shouldSendNext = true;
  }

  if (shouldSendNext) {
    lastOBDSentTime = now;
    obdResponsePending = true;
    if (obdZoeMode) {
      // 12 lépéses állapotgép a megfelelő Flow Control kezelés miatt
      switch(obdPollIndex) {
        // --- EVC (Battery) ECU lekérdezések ---
        case 0: sendCommand("ATSH7E4"); break;
        case 1: sendCommand("ATCRA7EC"); break;
        case 2: sendCommand("ATFCSH7E4"); break; // FC fejléc
        case 3: sendCommand("222002"); break;    // SOC
        case 4: sendCommand("223206"); break;    // SOH
        case 5: sendCommand("223204"); break;    // HV Bat Temp (CanZE alapértelmezett PID!)
        // --- HVAC (Climate) ECU (Service 21) ---
        case 6: sendCommand("ATSH744"); break;
        case 7: sendCommand("ATCRA764"); break;
        case 8: sendCommand("ATFCSH744"); break; // FC fejléc a klímához!
        case 9: sendCommand("2144"); break;      // AC RPM (Service 21)
        case 10: sendCommand("2143"); break;     // AC Pressure (Service 21)
        // --- Általános ---
        case 11: sendCommand("ATRV"); break;     // 12V Battery
      }
      obdPollIndex = (obdPollIndex + 1) % 12;
    } else {
      sendCommand("ATRV");
    }
  }

  if (lastOBDValue.length() > 0) {
    lastOBDRxTime = millis();
    gfx->fillCircle(234, 10, 3, GREEN);
    obdHeartbeatLit = true;

    lastOBDPollTime = millis();
    obdResponsePending = false;

    String resp = lastOBDValue;
    Serial.printf("[ZOE] Feldolgozás: '%s'\n", resp.c_str());

    if (resp.indexOf("622002") >= 0 || resp.indexOf("62 20 02") >= 0) {
      int raw = parseUDSHex(resp, "622002", 2);
      if (raw >= 0) {
        obdSOC = raw * 0.02;
        Serial.printf("[ZOE] SOC = %.1f%%\n", obdSOC);
      }
    } else if (resp.indexOf("623206") >= 0 || resp.indexOf("62 32 06") >= 0) {
      int raw = parseUDSHex(resp, "623206", 1);
      if (raw >= 0) {
        obdSOH = raw;
        Serial.printf("[ZOE] SOH = %d%%\n", obdSOH);
      }
    } else if (resp.indexOf("623204") >= 0 || resp.indexOf("62 32 04") >= 0) {
      int raw = parseUDSHex(resp, "623204", 1);
      if (raw >= 0) {
        obdHVBatTemp = raw - 40;
        Serial.printf("[ZOE] Bat Temp = %.0f°C\n", obdHVBatTemp);
      }
    } else if (resp.indexOf("6144") >= 0 || resp.indexOf("61 44") >= 0) {
      // AC RPM is bits 107 to 116 (Status) (val * 10)
      int raw = parseUDSBits(resp, "6144", 107, 116);
      if (raw >= 0) {
        obdACRpm = raw * 10;
        Serial.printf("[ZOE] AC RPM = %.0f rpm\n", obdACRpm);
      }
    } else if (resp.indexOf("6143") >= 0 || resp.indexOf("61 43") >= 0) {
      // AC Pressure is bits 134 to 142 (val * 0.1)
      int raw = parseUDSBits(resp, "6143", 134, 142);
      if (raw >= 0) {
        obdACPressure = raw * 0.1f;
        Serial.printf("[ZOE] AC Press = %.1f bar\n", obdACPressure);
      }
    } else if (resp.endsWith("V")) {
      obd12V = resp;
      Serial.printf("[ZOE] 12V = %s\n", obd12V.c_str());
    } else if (resp.indexOf("NO DATA") >= 0 || resp.indexOf("ERROR") >= 0) {
      Serial.printf("[ZOE] ECU nem válaszolt: %s\n", resp.c_str());
    } else if (resp == "OK") {
      // Ignoráljuk az AT parancsok nyugtázását
    }
    lastOBDValue = "";
    DisplayManager::updateHomeOBD();
  }
}
