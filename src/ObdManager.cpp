#include "ObdManager.h"
#include "DisplayManager.h"
#include "WebConsole.h"
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// ============================================================
// UDS parsing helpers
// ============================================================

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

int parseUDSBits(const String &resp, const char *expectedPrefix, int startBit, int endBit) {
  String r = resp;
  r.trim();
  r.replace(" ", "");
  String prefix = String(expectedPrefix);
  prefix.replace(" ", "");
  int idx = r.indexOf(prefix);
  if (idx < 0)
    return -1;

  String fullHex = r.substring(idx);

  int startByte = startBit / 8;
  int endByte = endBit / 8;

  if (fullHex.length() < (unsigned int)(endByte + 1) * 2) {
    return -1;
  }

  uint64_t val = 0;
  for (int i = startByte; i <= endByte; i++) {
    String byteStr = fullHex.substring(i * 2, i * 2 + 2);
    val = (val << 8) | strtol(byteStr.c_str(), NULL, 16);
  }

  int bitsToExtract = endBit - startBit + 1;
  int shiftRight = (8 - ((endBit + 1) % 8)) % 8;

  val = val >> shiftRight;
  uint64_t mask = (1ULL << bitsToExtract) - 1;
  return (int)(val & mask);
}

bool ObdManager::manualMode = false;

// ============================================================
// BLE Notify callback – assembles ELM327 responses
// ============================================================

void ObdManager::onBLENotify(NimBLERemoteCharacteristic *pChar, uint8_t *pData, size_t length, bool isNotify) {
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

        // Skip echoed AT commands
        if (lineUpper.startsWith("AT") && lineUpper.indexOf(' ') < 0 &&
            lineUpper.length() <= 6)
          continue;

        // Skip very short noise
        if (lineUpper.length() <= 3 && lineUpper.indexOf(' ') < 0) {
          continue;
        }

        // Strip ISO-TP sequence counters (e.g. "0:" "1:" "21:")
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
        
        WebConsole::pushLog(lineUpper); // Send raw line to Web Console
      }

      obdBufIndex = 0;
      obdBuffer[0] = '\0';
      if (fullResponse.length() == 0)
        continue;

      lastOBDValue = fullResponse;
      Serial.printf("[OBD] Response: '%s'\n", lastOBDValue.c_str());
    } else if (c != '\n') {
      if (obdBufIndex < (int)sizeof(obdBuffer) - 1)
        obdBuffer[obdBufIndex++] = c;
    }
  }
}

// ============================================================
// Send a command string to the ELM327 via BLE
// ============================================================

void ObdManager::sendCommand(const char *cmd) {
  if (pTxChar != nullptr && isBluetoothConnected) {
    char fullCmd[64];
    snprintf(fullCmd, sizeof(fullCmd), "%s\r", cmd);
    pTxChar->writeValue((uint8_t *)fullCmd, strlen(fullCmd));
    Serial.printf("[OBD] Sent: %s\n", cmd);
  }
}

void ObdManager::sendManualCommand(const char *cmd) {
  sendCommand(cmd);
}

// ============================================================
// Blocking helper: send an AT command and wait for "OK"
// Returns true if "OK" was received within timeout.
// ============================================================

static bool sendATAndWait(const char *cmd, unsigned long timeoutMs = 800) {
  lastOBDValue = "";
  ObdManager::sendCommand(cmd);
  unsigned long t0 = millis();
  while (millis() - t0 < timeoutMs) {
    delay(30);
    if (lastOBDValue.length() > 0) {
      String r = lastOBDValue;
      r.toUpperCase();
      lastOBDValue = "";
      if (r.indexOf("OK") >= 0) {
        return true;
      }
      // If it's not "OK", might be leftover data – keep waiting
    }
  }
  Serial.printf("[OBD] AT cmd '%s' no OK within %lu ms\n", cmd, timeoutMs);
  return false;
}

// ============================================================
// Blocking helper: send a DATA command and wait for any response.
// Returns the response string, or "" on timeout.
// Used for HVAC multi-frame queries that need longer waits.
// ============================================================

static String sendAndWaitResponse(const char *cmd, unsigned long timeoutMs = 3000) {
  lastOBDValue = "";
  ObdManager::sendCommand(cmd);
  unsigned long t0 = millis();
  while (millis() - t0 < timeoutMs) {
    delay(30);
    if (lastOBDValue.length() > 0) {
      String resp = lastOBDValue;
      lastOBDValue = "";
      return resp;
    }
  }
  Serial.printf("[OBD] Data cmd '%s' no response within %lu ms\n", cmd, timeoutMs);
  return "";
}

// ============================================================
// Switch the ELM327 context to a specific ECU (blocking).
// This ensures ATSH + ATCRA + ATFCSH are all applied before
// any data request is sent, preventing desync.
// ============================================================

static bool switchToECU(const char *txId, const char *rxId) {
  char cmd[16];

  snprintf(cmd, sizeof(cmd), "ATSH%s", txId);
  if (!sendATAndWait(cmd)) return false;

  snprintf(cmd, sizeof(cmd), "ATCRA%s", rxId);
  if (!sendATAndWait(cmd)) return false;

  snprintf(cmd, sizeof(cmd), "ATFCSH%s", txId);
  if (!sendATAndWait(cmd)) return false;

  return true;
}

// ============================================================
// OBD Initialization
// ============================================================

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

  // Echo off
  sendATAndWait("ATE0");

  Serial.println("[OBD] ZOE CAN protokoll beállítás...");
  gfx->fillRect(30, 50, 260, 40, BLACK);
  gfx->setFont(&FreeSans9pt7b);
  gfx->setTextColor(YELLOW, BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(50, 75);
  gfx->print("Setting up CAN...");

  // CAN protocol 6 (ISO 15765-4, 11-bit, 500 kbaud)
  sendATAndWait("ATSP6");

  // Flow Control settings for multi-frame responses
  sendATAndWait("ATFCSD300000");
  sendATAndWait("ATFCSM1");

  // Default to EVC (Battery Management)
  switchToECU("7E4", "7EC");

  // Open extended diagnostic session
  lastOBDValue = "";
  sendCommand("10C0");
  delay(500);
  lastOBDValue = "";

  obdZoeMode = true;
  obdPollIndex = 0;
  obdSOC = -1;
  obdSOH = -1;
  obdHVBatTemp = -99;
  obd12V = "";
  obdResponsePending = false;
  lastOBDSentTime = 0;
  lastOBDPollTime = 0;

  // Track which ECU the ELM327 is currently pointed at
  // 0 = EVC (7E4/7EC), 1 = HVAC (744/764)
  obdCurrentECU = 0;

  Serial.println("[BLE] OBD adapter felállt, ZOE mód aktív!");
  return true;
}

// ============================================================
// Blocking HVAC readout – runs as a single atomic operation.
// Switches to HVAC ECU, queries cabin & external temp, then
// switches back to EVC. No other command can interfere.
// ============================================================

void ObdManager::readHvacBlocking() {
  Serial.println("[OBD] === HVAC blocking read START ===");

  // 1. Switch to HVAC ECU
  switchToECU("744", "764");
  obdCurrentECU = 1;

  // 2. Open diagnostic session on HVAC (flush response)
  String sessResp = sendAndWaitResponse("1003", 1500);
  Serial.printf("[OBD] HVAC session resp: '%s'\n", sessResp.c_str());

  // 3. Query 2143 – Cabin Temp & AC Pressure (multi-frame, needs 3s)
  String resp2143 = sendAndWaitResponse("2143", 3000);
  Serial.printf("[OBD] 2143 resp: '%s'\n", resp2143.c_str());
  if (resp2143.indexOf("6143") >= 0 || resp2143.indexOf("61 43") >= 0) {
    // In-Car Temp: bytes 13 and 14 (bits 104 to 119)
    int rawCabin = parseUDSBits(resp2143, "6143", 104, 119);
    if (rawCabin >= 0) {
      obdCabinTemp = rawCabin / 10.0f;
      Serial.printf("[ZOE] Cabin Temp = %.1f°C\n", obdCabinTemp);
    }
    // AC Pressure: bytes 16 and 17 (bits 128 to 143)
    int rawPress = parseUDSBits(resp2143, "6143", 128, 143);
    if (rawPress >= 0) {
      obdACPressure = rawPress * 0.1f;
      Serial.printf("[ZOE] AC Press = %.1f bar\n", obdACPressure);
    }
  }

  // 4. Query 2144 – AC RPM
  String resp2144 = sendAndWaitResponse("2144", 3000);
  Serial.printf("[OBD] 2144 resp: '%s'\n", resp2144.c_str());
  if (resp2144.indexOf("6144") >= 0 || resp2144.indexOf("61 44") >= 0) {
    int raw = parseUDSBits(resp2144, "6144", 104, 119);
    if (raw >= 0) {
      obdACRpm = raw * 10;
      Serial.printf("[ZOE] AC RPM = %.0f rpm\n", obdACRpm);
    }
  }

  // 5. Query 2121 – External / Hot Source Temp
  String resp2121 = sendAndWaitResponse("2121", 3000);
  Serial.printf("[OBD] 2121 resp: '%s'\n", resp2121.c_str());
  if (resp2121.indexOf("6121") >= 0 || resp2121.indexOf("61 21") >= 0) {
    int rawHot = parseUDSBits(resp2121, "6121", 96, 111);
    if (rawHot >= 0) {
      float hotSourceTemp = rawHot / 100.0f;
      Serial.printf("[ZOE] Hot Source Temp = %.2f°C\n", hotSourceTemp);
    }
  }

  // 6. Switch back to EVC
  switchToECU("7E4", "7EC");
  obdCurrentECU = 0;

  // 7. Update display with new data
  lastOBDRxTime = millis();
  DisplayManager::updateHomeOBD();

  Serial.println("[OBD] === HVAC blocking read DONE ===");
}

// ============================================================
// Two-phase polling: EVC queries (async) → HVAC queries (blocking)
//
// EVC queries use the normal async mechanism.
// HVAC queries are done in a BLOCKING fashion to prevent
// command collisions on multi-frame ISO-TP responses.
// ============================================================

// Poll steps: 0=SOC, 1=SOH, 2=BatTemp, 3=HVAC(blocking), 4=12V
static const int POLL_STEPS = 5;

void ObdManager::processPolling() {
  if (manualMode) return; // SKIP automatic polling if user is debugging via Web Console
  
  bool shouldSendNext = false;
  unsigned long now = millis();

  if (!obdResponsePending) {
    shouldSendNext = true;
  } else if (lastOBDValue.length() > 0) {
    if (now - lastOBDPollTime >= OBD_NEXT_REQUEST_DELAY) {
      shouldSendNext = true;
    }
  } else if (now - lastOBDSentTime >= OBD_RESPONSE_TIMEOUT) {
    Serial.printf("[OBD] Timeout step %d, skipping...\n", obdPollIndex);
    shouldSendNext = true;
  }

  if (shouldSendNext) {
    // --- Process any pending response FIRST ---
    if (lastOBDValue.length() > 0) {
      lastOBDRxTime = millis();
      gfx->fillCircle(234, 10, 3, GREEN);
      obdHeartbeatLit = true;
      lastOBDPollTime = millis();

      String resp = lastOBDValue;
      lastOBDValue = "";
      Serial.printf("[ZOE] Parse: '%s'\n", resp.c_str());

      // === EVC responses (Service 22 → positive response 62) ===
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
      } else if (resp.indexOf("622001") >= 0 || resp.indexOf("62 20 01") >= 0) {
        int raw = parseUDSHex(resp, "622001", 1);
        if (raw >= 0) {
          obdHVBatTemp = raw - 40;
          Serial.printf("[ZOE] Bat Temp = %.0f°C\n", obdHVBatTemp);
        }
      }
      // === HVAC responses (Service 21 → positive response 61) ===
      else if (resp.indexOf("6144") >= 0 || resp.indexOf("61 44") >= 0) {
        // AC RPM in 2144: bytes 13 and 14 (bits 104 to 119)
        int raw = parseUDSBits(resp, "6144", 104, 119);
        if (raw >= 0) {
          obdACRpm = raw * 10;
          Serial.printf("[ZOE] AC RPM = %.0f rpm\n", obdACRpm);
        }
      } else if (resp.indexOf("6143") >= 0 || resp.indexOf("61 43") >= 0) {
        // In-Car Temp: bytes 13 and 14 (bits 104 to 119)
        int rawCabin = parseUDSBits(resp, "6143", 104, 119);
        if (rawCabin >= 0) {
          obdCabinTemp = rawCabin / 10.0f;
          Serial.printf("[ZOE] Cabin Temp = %.1f°C\n", obdCabinTemp);
        }
        // AC Pressure: bytes 16 and 17 (bits 128 to 143)
        int rawPress = parseUDSBits(resp, "6143", 128, 143);
        if (rawPress >= 0) {
          obdACPressure = rawPress * 0.1f;
          Serial.printf("[ZOE] AC Press = %.1f bar\n", obdACPressure);
        }
      } else if (resp.indexOf("6121") >= 0 || resp.indexOf("61 21") >= 0) {
        // Hot Source Temp: bytes 12 and 13 (bits 96 to 111)
        int rawHot = parseUDSBits(resp, "6121", 96, 111);
        if (rawHot >= 0) {
          float hotSourceTemp = rawHot / 100.0f;
          Serial.printf("[ZOE] Hot Source Temp = %.2f°C\n", hotSourceTemp);
        }
      }
      // === General ===
      else if (resp.endsWith("V")) {
        obd12V = resp;
        Serial.printf("[ZOE] 12V = %s\n", obd12V.c_str());
      } else if (resp.indexOf("NO DATA") >= 0 || resp.indexOf("ERROR") >= 0 || resp.indexOf("7F") >= 0) {
        Serial.printf("[ZOE] ECU error/no data: %s\n", resp.c_str());
      }
      // "OK" from AT commands silently ignored

      DisplayManager::updateHomeOBD();
    }

    // --- ECU context switching (blocking, guaranteed) ---
    if (obdZoeMode) {
      // Before EVC queries: ensure we're pointed at EVC
      if (obdPollIndex == 0 && obdCurrentECU != 0) {
        Serial.println("[OBD] Switching to EVC (7E4/7EC)...");
        switchToECU("7E4", "7EC");
        obdCurrentECU = 0;
      }
    }

    // --- Send the next data request ---
    lastOBDSentTime = millis();
    obdResponsePending = true;

    if (obdZoeMode) {
      switch (obdPollIndex) {
        // EVC (Service 22) – async
        case 0: sendCommand("222002"); break;   // SOC
        case 1: sendCommand("223206"); break;   // SOH
        case 2: sendCommand("222001"); break;   // Battery Rack Temp
        // HVAC – fully blocking, no collision possible
        case 3:
          readHvacBlocking();
          obdResponsePending = false; // Already processed
          break;
        // General
        case 4: sendCommand("ATRV"); break;     // 12V Battery
      }
      obdPollIndex = (obdPollIndex + 1) % POLL_STEPS;
    } else {
      sendCommand("ATRV");
    }
  }
}
