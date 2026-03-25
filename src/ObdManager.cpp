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
      if (obdBufIndex < (int)sizeof(obdBuffer)) {
        obdBuffer[obdBufIndex] = '\0';
      } else {
        obdBuffer[sizeof(obdBuffer) - 1] = '\0';
      }

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

        // Skip very short noise (but NOT "OK" which AT commands return)
        if (lineUpper.length() <= 3 && lineUpper.indexOf(' ') < 0 && lineUpper != "OK") {
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
      if (obdBufIndex < (int)sizeof(obdBuffer) - 1) {
        obdBuffer[obdBufIndex++] = c;
      }
    }
  }
}

// ============================================================
// Send a command string to the ELM327 via BLE
// ============================================================

void ObdManager::sendCommand(const char *cmd) {
  if (pTxChar != nullptr && isBluetoothConnected) {
    if (cmd == nullptr) {
      return;
    }

    char fullCmd[64];
    if (strlen(cmd) + 2 > sizeof(fullCmd)) {
      Serial.println("[OBD] Error: Command too long, dropping.");
      return;
    }

    snprintf(fullCmd, sizeof(fullCmd), "%s\r", cmd);
    pTxChar->writeValue((uint8_t *)fullCmd, strlen(fullCmd));
    Serial.printf("[OBD] Sent: %s\n", cmd);
    // TX logging to web console
    String txLog = "TX: ";
    txLog += cmd;
    WebConsole::pushLog(txLog);
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
// ISO-TP request with manual multi-frame assembly.
// Uses ATCAF0 + ATS0 (like CanZE) to get raw frames, then
// reassembles SINGLE (0x) / FIRST (1xxx) + CONSECUTIVE (2x)
// frames into a contiguous hex string.
// ============================================================

static String sendIsoTpRequest(const char *serviceCmd, unsigned long timeoutMs = 5000) {
  // Build the ISO-TP single frame: PCI byte + service data
  // serviceCmd is like "2143" (2 bytes) -> PCI = "02" + "2143"
  int dataLen = strlen(serviceCmd) / 2;  // hex nibbles to bytes
  char isotpCmd[32];
  snprintf(isotpCmd, sizeof(isotpCmd), "0%d%s", dataLen, serviceCmd);

  lastOBDValue = "";
  ObdManager::sendCommand(isotpCmd);

  // Wait for first response line
  unsigned long t0 = millis();
  while (millis() - t0 < timeoutMs) {
    delay(30);
    if (lastOBDValue.length() > 0) break;
  }
  if (lastOBDValue.length() == 0) {
    Serial.printf("[OBD] IsoTP '%s' no response within %lu ms\n", serviceCmd, timeoutMs);
    return "";
  }

  String resp = lastOBDValue;
  lastOBDValue = "";
  resp.trim();
  resp.replace(" ", "");  // remove any spaces
  resp.toUpperCase();

  Serial.printf("[OBD] IsoTP raw: '%s'\n", resp.c_str());

  if (resp.length() < 2) return "";

  // Check first nibble for frame type
  char frameType = resp.charAt(0);

  if (frameType == '0') {
    // SINGLE frame: 0L DDDDDD...
    // L = length, data follows
    int len = 0;
    if (resp.charAt(1) >= '0' && resp.charAt(1) <= '9')
      len = resp.charAt(1) - '0';
    else if (resp.charAt(1) >= 'A' && resp.charAt(1) <= 'F')
      len = resp.charAt(1) - 'A' + 10;
    String hexData = resp.substring(2);
    // Trim to actual data length
    if ((int)hexData.length() > len * 2)
      hexData = hexData.substring(0, len * 2);
    Serial.printf("[OBD] IsoTP SINGLE: len=%d data='%s'\n", len, hexData.c_str());
    return hexData;
  }

  if (frameType == '1') {
    // FIRST frame detected. The ELM327 BLE clone returns ALL CAN frames
    // (First + Consecutive) concatenated in one response string.
    // Each CAN frame is 16 hex chars (8 bytes). We must strip PCI bytes:
    //   First Frame: 4 PCI nibbles (1LLL), 12 data nibbles (6 bytes)
    //   Consecutive:  2 PCI nibbles (2X),  14 data nibbles (7 bytes)
    if (resp.length() < 16) return "";

    // Parse total length from nibbles 1-3
    String lenHex = resp.substring(1, 4);
    int totalLen = (int)strtol(lenHex.c_str(), NULL, 16);

    // First Frame data: skip 4 PCI nibbles, take 12 data nibbles (6 bytes)
    String hexData = resp.substring(4, 16);
    Serial.printf("[OBD] IsoTP FIRST: totalLen=%d, FF data='%s'\n", totalLen, hexData.c_str());

    // Parse inline Consecutive Frames (each 16 hex chars)
    int pos = 16;  // start of next CAN frame
    int cfCount = 0;
    while (pos + 16 <= (int)resp.length()) {
      String frame = resp.substring(pos, pos + 16);
      // Verify it's a consecutive frame (starts with '2')
      if (frame.charAt(0) == '2') {
        // Skip 2 PCI nibbles (2X), take 14 data nibbles (7 bytes)
        String cfData = frame.substring(2);
        hexData += cfData;
        cfCount++;
        Serial.printf("[OBD] IsoTP CF#%d: '%s'\n", cfCount, cfData.c_str());
      }
      pos += 16;
    }

    // Trim to exact total length
    if ((int)hexData.length() > totalLen * 2)
      hexData = hexData.substring(0, totalLen * 2);

    Serial.printf("[OBD] IsoTP assembled: %d bytes (%d CFs), '%s'\n", totalLen, cfCount, hexData.c_str());
    return hexData;
  }

  // Unexpected frame type (NO DATA, error, etc.)
  Serial.printf("[OBD] IsoTP unexpected: '%s'\n", resp.c_str());
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

  // 1. Switch to HVAC ECU (ATSH, ATCRA, ATFCSH)
  switchToECU("744", "764");
  obdCurrentECU = 1;

  // 2. Switch to CanZE-compatible mode for multi-frame
  sendATAndWait("ATS0");          // No spaces (raw hex concat)
  sendATAndWait("ATCAF0");        // CAN Auto Formatting OFF
  sendATAndWait("ATAL");           // Allow Long messages
  sendATAndWait("ATFCSD300000");  // FC: ContinueToSend, BS=0, STmin=0
  sendATAndWait("ATFCSM1");       // User-defined FC mode
  sendATAndWait("ATSTFF");        // Max ELM327 timeout

  // 3. Open vendor diagnostic session on HVAC
  String sessResp = sendIsoTpRequest("10C0", 2000);
  Serial.printf("[OBD] HVAC session: '%s'\n", sessResp.c_str());

  // =================================================================
  // 4. Query 2121 – CABIN TEMP (CanZE: IH_InCarTemp)
  //    Bits 26-35, res 0.1, offset 400 → (raw * 0.1) - 40.0
  // =================================================================
  String resp2121 = sendIsoTpRequest("2121", 5000);
  Serial.printf("[OBD] 2121 IsoTP (%d chars): '%s'\n", resp2121.length(), resp2121.c_str());
  if (resp2121.indexOf("6121") >= 0) {
    int rawCabin = parseUDSBits(resp2121, "6121", 26, 35);
    if (rawCabin >= 0) {
      obdCabinTemp = (rawCabin * 0.1f) - 40.0f;
      Serial.printf("[ZOE] Cabin Temp = %.1f C (raw=%d)\n", obdCabinTemp, rawCabin);
    }
  }

  // =================================================================
  // 5. Query 2143 – ExternalTemp + ACPressure (multi-frame!)
  //    IH_ExternalTemp: bits 110-117, res 1, offset 40
  //    IH_ACHighPressureSensor: bits 134-142, res 0.1
  // =================================================================
  String resp2143 = sendIsoTpRequest("2143", 5000);
  Serial.printf("[OBD] 2143 IsoTP (%d chars): '%s'\n", resp2143.length(), resp2143.c_str());
  if (resp2143.indexOf("6143") >= 0) {
    int rawExt = parseUDSBits(resp2143, "6143", 110, 117);
    if (rawExt >= 0) {
      float extTemp = rawExt - 40.0f;
      Serial.printf("[ZOE] External Temp = %.0f C (raw=%d)\n", extTemp, rawExt);
    }
    int rawPress = parseUDSBits(resp2143, "6143", 134, 142);
    if (rawPress >= 0) {
      obdACPressure = rawPress * 0.1f;
      Serial.printf("[ZOE] AC Pressure = %.1f bar (raw=%d)\n", obdACPressure, rawPress);
    }
  }

  // =================================================================
  // 6. Query 2144 – AC Compressor RPM (multi-frame!)
  //    IH_ClimCompRPMStatus: bits 107-116, res 10
  // =================================================================
  String resp2144 = sendIsoTpRequest("2144", 5000);
  Serial.printf("[OBD] 2144 IsoTP (%d chars): '%s'\n", resp2144.length(), resp2144.c_str());
  if (resp2144.indexOf("6144") >= 0) {
    int raw = parseUDSBits(resp2144, "6144", 107, 116);
    if (raw >= 0) {
      obdACRpm = raw * 10;
      Serial.printf("[ZOE] AC RPM = %.0f rpm (raw=%d)\n", obdACRpm, raw);
    }
  }

  // 7. Restore normal ELM327 mode for EVC queries
  sendATAndWait("ATS1");     // Spaces back on
  sendATAndWait("ATCAF1");   // CAN Auto Formatting ON
  sendATAndWait("ATST32");   // Normal timeout

  // 8. Switch back to EVC
  switchToECU("7E4", "7EC");
  obdCurrentECU = 0;

  // 9. Update display with new data
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
