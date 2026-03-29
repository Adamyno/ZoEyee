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
  if (idx < 0) {
    return -1;
  }

  int startPos = idx + prefix.length();
  if (r.length() < (unsigned int)(startPos + byteCount * 2)) {
    return -1;
  }

  char buf[33];
  int len = byteCount * 2;
  if (len > 32) {
    len = 32;
  }
  memcpy(buf, r.c_str() + startPos, len);
  buf[len] = '\0';

  return (int)strtol(buf, NULL, 16);
}

int parseUDSBits(const String &resp, const char *expectedPrefix, int startBit,
                 int endBit) {
  String r = resp;
  r.trim();
  r.replace(" ", "");
  String prefix = String(expectedPrefix);
  prefix.replace(" ", "");
  int idx = r.indexOf(prefix);
  if (idx < 0) {
    return -1;
  }

  String fullHex = r.substring(idx);

  int startByte = startBit / 8;
  int endByte = endBit / 8;

  if (fullHex.length() < (unsigned int)(endByte + 1) * 2) {
    return -1;
  }

  uint64_t val = 0;
  char byteBuf[3] = {0};
  const char *s = fullHex.c_str();
  for (int i = startByte; i <= endByte; i++) {
    byteBuf[0] = s[i * 2];
    byteBuf[1] = s[i * 2 + 1];
    val = (val << 8) | strtol(byteBuf, NULL, 16);
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

void ObdManager::onBLENotify(NimBLERemoteCharacteristic *pChar, uint8_t *pData,
                             size_t length, bool isNotify) {
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
        if (lineUpper.length() <= 3 && lineUpper.indexOf(' ') < 0 &&
            lineUpper != "OK") {
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

void ObdManager::sendManualCommand(const char *cmd) { sendCommand(cmd); }

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

static String sendAndWaitResponse(const char *cmd,
                                  unsigned long timeoutMs = 3000) {
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
  Serial.printf("[OBD] Data cmd '%s' no response within %lu ms\n", cmd,
                timeoutMs);
  return "";
}

// ============================================================
// ISO-TP request with manual multi-frame assembly.
// Uses ATCAF0 + ATS0 (like CanZE) to get raw frames, then
// reassembles SINGLE (0x) / FIRST (1xxx) + CONSECUTIVE (2x)
// frames into a contiguous hex string.
// ============================================================

static String sendIsoTpRequest(const char *serviceCmd,
                               unsigned long timeoutMs = 5000) {
  // Build the ISO-TP single frame: PCI byte + service data
  // serviceCmd is like "2143" (2 bytes) -> PCI = "02" + "2143"
  int dataLen = strlen(serviceCmd) / 2; // hex nibbles to bytes
  char isotpCmd[32];
  snprintf(isotpCmd, sizeof(isotpCmd), "0%d%s", dataLen, serviceCmd);

  lastOBDValue = "";
  ObdManager::sendCommand(isotpCmd);

  // Wait for first response line
  unsigned long t0 = millis();
  while (millis() - t0 < timeoutMs) {
    delay(30);
    if (lastOBDValue.length() > 0)
      break;
  }
  if (lastOBDValue.length() == 0) {
    Serial.printf("[OBD] IsoTP '%s' no response within %lu ms\n", serviceCmd,
                  timeoutMs);
    return "";
  }

  String resp = lastOBDValue;
  lastOBDValue = "";
  resp.trim();
  resp.replace(" ", ""); // remove any spaces
  resp.toUpperCase();

  Serial.printf("[OBD] IsoTP raw: '%s'\n", resp.c_str());

  if (resp.length() < 2)
    return "";

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
    Serial.printf("[OBD] IsoTP SINGLE: len=%d data='%s'\n", len,
                  hexData.c_str());
    return hexData;
  }

  if (frameType == '1') {
    // FIRST frame detected. The ELM327 BLE clone returns ALL CAN frames
    // (First + Consecutive) concatenated in one response string.
    // Each CAN frame is 16 hex chars (8 bytes). We must strip PCI bytes:
    //   First Frame: 4 PCI nibbles (1LLL), 12 data nibbles (6 bytes)
    //   Consecutive:  2 PCI nibbles (2X),  14 data nibbles (7 bytes)
    if (resp.length() < 16)
      return "";

    // Parse total length from nibbles 1-3
    String lenHex = resp.substring(1, 4);
    int totalLen = (int)strtol(lenHex.c_str(), NULL, 16);

    // First Frame data: skip 4 PCI nibbles, take 12 data nibbles (6 bytes)
    String hexData = resp.substring(4, 16);
    Serial.printf("[OBD] IsoTP FIRST: totalLen=%d, FF data='%s'\n", totalLen,
                  hexData.c_str());

    // Parse inline Consecutive Frames (each 16 hex chars)
    int pos = 16; // start of next CAN frame
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

    Serial.printf("[OBD] IsoTP assembled: %d bytes (%d CFs), '%s'\n", totalLen,
                  cfCount, hexData.c_str());
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
  if (!sendATAndWait(cmd))
    return false;

  snprintf(cmd, sizeof(cmd), "ATCRA%s", rxId);
  if (!sendATAndWait(cmd))
    return false;

  snprintf(cmd, sizeof(cmd), "ATFCSH%s", txId);
  if (!sendATAndWait(cmd))
    return false;

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
      Serial.printf(
          "[BLE] Nincs válasz a(z) %s parancsra, újrapróbálkozás...\n",
          atzCommands[cmdIdx]);
  }

  if (!isELM) {
    Serial.println("[BLE] Nem válaszolt mint ELM327. Lecsatlakozás.");
    if (currentState == STATE_BT_LIST || currentState == STATE_BT_STATUS || currentState == STATE_BT_DEVICE_INFO) {
      gfx->fillRect(30, 50, 260, 60, BLACK);
      gfx->setFont(&FreeSans12pt7b);
      gfx->setTextColor(RED, BLACK);
      gfx->setTextSize(1);
      gfx->setCursor(40, 75);
      gfx->print("Not an OBD device");
      delay(2000);
    }
    return false;
  }

  Serial.printf("[BLE] ELM327 hitelesítve: %s\n", lastOBDValue.c_str());

  // Echo off
  sendATAndWait("ATE0");

  Serial.println("[OBD] ZOE CAN protokoll beállítás...");
  if (currentState == STATE_BT_LIST || currentState == STATE_BT_STATUS || currentState == STATE_BT_DEVICE_INFO) {
    gfx->fillRect(30, 50, 260, 40, BLACK);
    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextColor(YELLOW, BLACK);
    gfx->setTextSize(1);
    gfx->setCursor(50, 75);
    gfx->print("Setting up CAN...");
  }

  // CAN protocol 6 (ISO 15765-4, 11-bit, 500 kbaud)
  sendATAndWait("ATSP6");

  // Flow Control settings for multi-frame responses
  sendATAndWait("ATFCSD300000");
  sendATAndWait("ATFCSM1");

  // Default to EVC (Battery Management)
  switchToECU("7E4", "7EC");

  // Open extended diagnostic session
  sendAndWaitResponse("10C0", 500);

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
// Non-blocking HVAC state machine.
// Each call processes ONE step: either sends a command or
// checks for a response. The main loop remains responsive.
// ============================================================

// Helper: parse IsoTP response from assembled hex string (reused from old code)
static String parseIsoTpResponse(const String &resp) {
  String r = resp;
  r.trim();
  r.replace(" ", "");
  r.toUpperCase();

  if (r.length() < 2) return "";

  char frameType = r.charAt(0);

  if (frameType == '0') {
    // SINGLE frame
    int len = 0;
    if (r.charAt(1) >= '0' && r.charAt(1) <= '9')
      len = r.charAt(1) - '0';
    else if (r.charAt(1) >= 'A' && r.charAt(1) <= 'F')
      len = r.charAt(1) - 'A' + 10;
    String hexData = r.substring(2);
    if ((int)hexData.length() > len * 2)
      hexData = hexData.substring(0, len * 2);
    return hexData;
  }

  if (frameType == '1') {
    // FIRST frame + inline Consecutive Frames
    if (r.length() < 16) return "";
    String lenHex = r.substring(1, 4);
    int totalLen = (int)strtol(lenHex.c_str(), NULL, 16);
    String hexData = r.substring(4, 16);

    int pos = 16;
    while (pos + 16 <= (int)r.length()) {
      String frame = r.substring(pos, pos + 16);
      if (frame.charAt(0) == '2') {
        hexData += frame.substring(2);
      }
      pos += 16;
    }

    if ((int)hexData.length() > totalLen * 2)
      hexData = hexData.substring(0, totalLen * 2);
    return hexData;
  }

  return "";
}

void ObdManager::processHvacStep() {
  // If IDLE, start the HVAC sequence
  if (hvacState == HVAC_IDLE) {
    Serial.println("[OBD] HVAC state machine START");
    hvacState = HVAC_SWITCH_SH;
    lastOBDValue = "";
    // Fall through to send the first command
  }

  // Check for DONE
  if (hvacState == HVAC_DONE) {
    Serial.println("[OBD] HVAC state machine DONE");
    obdCurrentECU = 0;
    hvacState = HVAC_IDLE;
    lastOBDRxTime = millis();
    DisplayManager::updateHomeOBD();
    return;
  }

  unsigned long now = millis();

  // Determine the timeout for the current state
  unsigned long timeout;
  if (hvacState >= HVAC_QUERY_2121 && hvacState <= HVAC_QUERY_2144) {
    timeout = HVAC_ISOTP_TIMEOUT;
  } else if (hvacState == HVAC_SESSION) {
    timeout = 2000;
  } else {
    timeout = HVAC_AT_TIMEOUT;
  }

  // If we already sent a command (hvacCmdSentTime > 0), wait for response
  if (hvacCmdSentTime > 0) {
    if (lastOBDValue.length() > 0) {
      // Got a response — process it
      String resp = lastOBDValue;
      lastOBDValue = "";
      hvacCmdSentTime = 0;

      // Process response based on current state
      switch (hvacState) {
        case HVAC_QUERY_2121: {
          String isotp = parseIsoTpResponse(resp);
          Serial.printf("[OBD] HVAC 2121: '%s'\n", isotp.c_str());
          if (isotp.indexOf("6121") >= 0) {
            int rawCabin = parseUDSBits(isotp, "6121", 26, 35);
            if (rawCabin >= 0) {
              obdCabinTemp = (rawCabin * 0.1f) - 40.0f;
              Serial.printf("[ZOE] Cabin Temp = %.1f C (raw=%d)\n", obdCabinTemp, rawCabin);
            }
          }
          break;
        }
        case HVAC_QUERY_2143: {
          String isotp = parseIsoTpResponse(resp);
          Serial.printf("[OBD] HVAC 2143: '%s'\n", isotp.c_str());
          if (isotp.indexOf("6143") >= 0) {
            int rawExt = parseUDSBits(isotp, "6143", 110, 117);
            if (rawExt >= 0) {
              obdExtTemp = rawExt - 40.0f;
              Serial.printf("[ZOE] External Temp = %.0f C\n", obdExtTemp);
            }
            int rawPress = parseUDSBits(isotp, "6143", 134, 142);
            if (rawPress >= 0) {
              obdACPressure = rawPress * 0.1f;
              Serial.printf("[ZOE] AC Pressure = %.1f bar\n", obdACPressure);
            }
          }
          break;
        }
        case HVAC_QUERY_2144: {
          String isotp = parseIsoTpResponse(resp);
          Serial.printf("[OBD] HVAC 2144: '%s'\n", isotp.c_str());
          if (isotp.indexOf("6144") >= 0) {
            int raw = parseUDSBits(isotp, "6144", 107, 116);
            if (raw >= 0) {
              obdACRpm = raw * 10;
              Serial.printf("[ZOE] AC RPM = %.0f rpm\n", obdACRpm);
            }
          }
          break;
        }
        case HVAC_QUERY_2167: {
          String isotp = parseIsoTpResponse(resp);
          Serial.printf("[OBD] HVAC 2167: '%s'\n", isotp.c_str());
          if (isotp.indexOf("6167") >= 0) {
            int raw = parseUDSBits(isotp, "6167", 21, 23);
            if (raw >= 0) {
              obdClimateLoopMode = raw;
              Serial.printf("[ZOE] Climate Loop Mode = %d\n", obdClimateLoopMode);
            }
          }
          break;
        }
        default:
          // AT command responses (OK) — just consume and move on
          break;
      }

      // Advance to next state
      hvacState = (HvacPollState)(hvacState + 1);
      // Will send the next command on the NEXT loop() call
      return;

    } else if (now - hvacCmdSentTime >= timeout) {
      // Timeout — skip this step and advance
      Serial.printf("[OBD] HVAC state %d timeout, advancing\n", hvacState);
      hvacCmdSentTime = 0;
      hvacState = (HvacPollState)(hvacState + 1);
      return;
    }
    // Still waiting — return and let loop() continue (touch, display, etc.)
    return;
  }

  // No command pending — send the command for the current state
  lastOBDValue = "";
  hvacCmdSentTime = now;

  switch (hvacState) {
    // Switch to HVAC ECU
    case HVAC_SWITCH_SH:   sendCommand("ATSH744");      obdCurrentECU = 1; break;
    case HVAC_SWITCH_CRA:  sendCommand("ATCRA764");     break;
    case HVAC_SWITCH_FCSH: sendCommand("ATFCSH744");    break;
    // Set raw mode
    case HVAC_SET_ATS0:    sendCommand("ATS0");         break;
    case HVAC_SET_ATCAF0:  sendCommand("ATCAF0");       break;
    case HVAC_SET_ATAL:    sendCommand("ATAL");          break;
    case HVAC_SET_FCSD:    sendCommand("ATFCSD300000"); break;
    case HVAC_SET_FCSM:    sendCommand("ATFCSM1");      break;
    case HVAC_SET_STFF:    sendCommand("ATSTFF");        break;
    // Diagnostic session
    case HVAC_SESSION: {
      // Build ISO-TP single frame for 10C0
      sendCommand("0210C0");
      break;
    }
    // Data queries — send as ISO-TP single frames
    case HVAC_QUERY_2121:  sendCommand("022121"); break;
    case HVAC_QUERY_2143:  sendCommand("022143"); break;
    case HVAC_QUERY_2144:  sendCommand("022144"); break;
    case HVAC_QUERY_2167:  sendCommand("022167"); break;
    // Restore normal mode
    case HVAC_RESTORE_ATS1:   sendCommand("ATS1");   break;
    case HVAC_RESTORE_ATCAF1: sendCommand("ATCAF1"); break;
    case HVAC_RESTORE_ATST32: sendCommand("ATST32"); break;
    // Switch back to EVC
    case HVAC_BACK_SH:   sendCommand("ATSH7E4");   break;
    case HVAC_BACK_CRA:  sendCommand("ATCRA7EC");  break;
    case HVAC_BACK_FCSH: sendCommand("ATFCSH7E4"); break;
    default:
      hvacState = HVAC_DONE;
      hvacCmdSentTime = 0;
      break;
  }
}

// ============================================================
// Non-blocking LBC state machine.
// Each call processes ONE step: either sends a command or
// checks for a response. Cell voltages via 2103 ISO-TP.
// ============================================================

void ObdManager::processLbcStep() {
  if (lbcState == LBC_IDLE) {
    Serial.println("[OBD] LBC state machine START");
    lbcState = LBC_SWITCH_SH;
    lastOBDValue = "";
  }

  if (lbcState == LBC_DONE) {
    Serial.println("[OBD] LBC state machine DONE");
    obdCurrentECU = -1; // Force re-switch next time
    lbcState = LBC_IDLE;
    lastOBDRxTime = millis();
    DisplayManager::updateHomeOBD();
    return;
  }

  unsigned long now = millis();

  // Determine timeout for the current state
  unsigned long timeout;
  if (lbcState == LBC_QUERY_2103 || lbcState == LBC_QUERY_2101) {
    timeout = HVAC_ISOTP_TIMEOUT;  // 5s for multi-frame data
  } else if (lbcState == LBC_SESSION) {
    timeout = 2000;
  } else {
    timeout = HVAC_AT_TIMEOUT;  // 1s for AT commands
  }

  // If we already sent a command, wait for response
  if (lbcCmdSentTime > 0) {
    if (lastOBDValue.length() > 0) {
      // Got a response
      String resp = lastOBDValue;
      lastOBDValue = "";
      lbcCmdSentTime = 0;

      // Process response based on current state
      if (lbcState == LBC_QUERY_2101) {
        String isotp = parseIsoTpResponse(resp);
        Serial.printf("[OBD] LBC 2101: '%s'\n", isotp.c_str());
        if (isotp.indexOf("6101") >= 0) {
          // Max Charge Power: bits 336-351, multiplier 0.01
          int rawMaxChg = parseUDSBits(isotp, "6101", 336, 351);
          if (rawMaxChg >= 0) {
            obdMaxChargePower = rawMaxChg * 0.01f;
            Serial.printf("[ZOE] Max Charge Power = %.2f kW\n", obdMaxChargePower);
          }
          lastOBDRxTime = millis();
          lastOBDPollTime = millis();
        } else {
          Serial.println("[OBD] LBC 2101 failed.");
        }
      } else if (lbcState == LBC_QUERY_2103) {
        String isotp = parseIsoTpResponse(resp);
        Serial.printf("[OBD] LBC 2103: '%s'\n", isotp.c_str());
        if (isotp.indexOf("6103") >= 0) {
          int rawMax = parseUDSBits(isotp, "6103", 96, 111);
          if (rawMax >= 0) {
            obdCellVoltageMax = rawMax * 0.01f;
            Serial.printf("[ZOE] Cell V Max = %.3f V\n", obdCellVoltageMax);
          }
          int rawMin = parseUDSBits(isotp, "6103", 112, 127);
          if (rawMin >= 0) {
            obdCellVoltageMin = rawMin * 0.01f;
            Serial.printf("[ZOE] Cell V Min = %.3f V\n", obdCellVoltageMin);
          }
          lastOBDRxTime = millis();
          lastOBDPollTime = millis();
        } else {
          Serial.println("[OBD] LBC 2103 failed.");
        }
      }

      // Advance to next state
      lbcState = (LbcPollState)(lbcState + 1);
      return;

    } else if (now - lbcCmdSentTime >= timeout) {
      Serial.printf("[OBD] LBC state %d timeout, advancing\n", lbcState);
      lbcCmdSentTime = 0;
      lbcState = (LbcPollState)(lbcState + 1);
      return;
    }
    // Still waiting
    return;
  }

  // No command pending — send the command for the current state
  lastOBDValue = "";
  lbcCmdSentTime = now;

  switch (lbcState) {
    // Switch to LBC ECU
    case LBC_SWITCH_SH:      sendCommand("ATSH79B");     obdCurrentECU = 2; break;
    case LBC_SWITCH_CRA:     sendCommand("ATCRA7BB");    break;
    case LBC_SWITCH_FCSH:    sendCommand("ATFCSH79B");   break;
    // Extended diagnostic session
    case LBC_SESSION:        sendCommand("0210C0");      break;
    // Set raw mode for multi-frame
    case LBC_SET_ATS0:       sendCommand("ATS0");        break;
    case LBC_SET_ATCAF0:     sendCommand("ATCAF0");      break;
    case LBC_SET_ATAL:       sendCommand("ATAL");         break;
    // Data queries
    case LBC_QUERY_2101:     sendCommand("022101");      break;
    case LBC_QUERY_2103:     sendCommand("022103");      break;
    // Restore normal mode
    case LBC_RESTORE_ATS1:   sendCommand("ATS1");        break;
    case LBC_RESTORE_ATCAF1: sendCommand("ATCAF1");      break;
    case LBC_RESTORE_ATST32: sendCommand("ATST32");      break;
    default:
      lbcState = LBC_DONE;
      lbcCmdSentTime = 0;
      break;
  }
}

// ============================================================
// Page-aware polling: only query parameters on the current page.
// Poll list is built dynamically from dashPages[currentPage].
// ============================================================

// Poll list: unique param indices on the current page
static int pagePollList[6] = {-1, -1, -1, -1, -1, -1};
static int pagePollCount = 0;
static bool pageNeedsHVAC = false;  // true if any HVAC param is on page
static bool pageNeedsEVC = false;   // true if any EVC param is on page
static bool pageNeedsLBC = false;   // true if any LBC param (cell voltages) is on page

// Which HVAC queries are needed on this page
static bool needHvac2121 = false; // Cabin temp (param 2)
static bool needHvac2143 = false; // AC Pressure (param 5)
static bool needHvac2144 = false; // AC RPM (param 4)
static bool needHvac2167 = false; // Climate Loop Mode (param 12)

void ObdManager::resetPollIndex() {
  obdPollIndex = 0;
  obdResponsePending = false;
  lastOBDValue = "";
  buildPollList();
}

void ObdManager::buildPollList() {
  pagePollCount = 0;
  pageNeedsHVAC = false;
  pageNeedsEVC = false;
  pageNeedsLBC = false;
  needHvac2121 = false;
  needHvac2143 = false;
  needHvac2144 = false;
  needHvac2167 = false;

  for (int i = 0; i < 6; i++) {
    int paramIdx = dashPages[currentPage][i].paramIndex;
    if (paramIdx < 0 || paramIdx >= MAX_DASH_PARAMS) continue;

    // Check for duplicates
    bool dup = false;
    for (int j = 0; j < pagePollCount; j++) {
      if (pagePollList[j] == paramIdx) { dup = true; break; }
    }
    if (dup) continue;

    pagePollList[pagePollCount++] = paramIdx;

    // Track ECU needs
    // ECU mapping: params 0,1,3,11 = EVC; params 2,4,5,7 = HVAC; param 6 = ATRV (general)
    if (paramIdx == 0 || paramIdx == 1 || paramIdx == 3 || paramIdx == 11) pageNeedsEVC = true;
    if (paramIdx == 14) pageNeedsEVC = true; // DC Power needs EVC (voltage + current)
    if (paramIdx == 8 || paramIdx == 9 || paramIdx == 10 || paramIdx == 13) pageNeedsLBC = true;
    if (paramIdx == 2) { pageNeedsHVAC = true; needHvac2121 = true; }
    if (paramIdx == 4) { pageNeedsHVAC = true; needHvac2144 = true; }
    if (paramIdx == 5 || paramIdx == 7) { pageNeedsHVAC = true; needHvac2143 = true; }
    if (paramIdx == 12) { pageNeedsHVAC = true; needHvac2167 = true; }
  }

  obdPollIndex = 0;
  Serial.printf("[OBD] Poll list built: %d params, EVC=%d, HVAC=%d\n",
                pagePollCount, pageNeedsEVC, pageNeedsHVAC);
}

// EVC poll steps: SOC(0), SOH(1), BatTemp(3)
// Returns the OBD command for an EVC param index, or nullptr
static const char* getEvcCommand(int paramIdx) {
  switch (paramIdx) {
    case 0: return "223206"; // SOH
    case 1: return "222002"; // SOC
    case 3: return "222001"; // Battery Temp
    case 11: return "223471"; // Fan Speed
    case 14: return "223203"; // HV Voltage (first of 2-step: voltage then current)
    case 99: return "223204"; // HV Current (shadow step for DC Power calc)
    default: return nullptr;
  }
}

// Poll steps for page-aware polling:
// We iterate pagePollList for EVC params, then trigger HVAC if needed, then ATRV

void ObdManager::processPolling() {
  if (manualMode) return;
  // Nothing to poll on this page
  if (pagePollCount == 0 && !pageNeedsHVAC) return;

  // --- If HVAC state machine is active, it owns the OBD bus ---
  if (hvacState != HVAC_IDLE) {
    processHvacStep();
    if (hvacState == HVAC_IDLE) {
      obdResponsePending = false;
    }
    return;
  }

  // --- If LBC state machine is active, it owns the OBD bus ---
  if (lbcState != LBC_IDLE) {
    processLbcStep();
    if (lbcState == LBC_IDLE) {
      obdResponsePending = false;
    }
    return;
  }

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
      lastOBDPollTime = millis();

      String resp = lastOBDValue;
      lastOBDValue = "";
      Serial.printf("[ZOE] Parse: '%s'\n", resp.c_str());

      // === EVC responses ===
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
          Serial.printf("[ZOE] Bat Temp = %.0f\u00b0C\n", obdHVBatTemp);
        }
      } else if (resp.indexOf("623471") >= 0 || resp.indexOf("62 34 71") >= 0) {
        int raw = parseUDSHex(resp, "623471", 1);
        if (raw >= 0) {
          obdFanSpeed = raw * 5;
          Serial.printf("[ZOE] Engine Fan = %d%%\n", obdFanSpeed);
        }
      } else if (resp.indexOf("623203") >= 0 || resp.indexOf("62 32 03") >= 0) {
        int raw = parseUDSHex(resp, "623203", 2);
        if (raw >= 0) {
          obdHVBatVoltage = raw * 0.5f;
          Serial.printf("[ZOE] HV Voltage = %.1f V\n", obdHVBatVoltage);
          // Recalculate DC Power if current is known
          if (obdHVBatCurrent > -900) {
            obdDCPower = obdHVBatVoltage * obdHVBatCurrent / 1000.0f;
            Serial.printf("[ZOE] DC Power = %.2f kW\n", obdDCPower);
          }
        }
      } else if (resp.indexOf("623204") >= 0 || resp.indexOf("62 32 04") >= 0) {
        int raw = parseUDSHex(resp, "623204", 2);
        if (raw >= 0) {
          obdHVBatCurrent = (raw - 32768) * 0.25f;
          Serial.printf("[ZOE] HV Current = %.2f A\n", obdHVBatCurrent);
          // Recalculate DC Power if voltage is known
          if (obdHVBatVoltage > 0) {
            obdDCPower = obdHVBatVoltage * obdHVBatCurrent / 1000.0f;
            Serial.printf("[ZOE] DC Power = %.2f kW\n", obdDCPower);
          }
        }
      } else if (resp.indexOf("621417") >= 0 || resp.indexOf("62 14 17") >= 0) {
        int raw = parseUDSHex(resp, "621417", 2);
        if (raw >= 0) {
          obdCellVoltageMax = raw * 0.001f;
          Serial.printf("[ZOE] Cell V Max = %.3f V\n", obdCellVoltageMax);
        }
      } else if (resp.indexOf("621419") >= 0 || resp.indexOf("62 14 19") >= 0) {
        int raw = parseUDSHex(resp, "621419", 2);
        if (raw >= 0) {
          obdCellVoltageMin = raw * 0.001f;
          Serial.printf("[ZOE] Cell V Min = %.3f V\n", obdCellVoltageMin);
        }
      } else if (resp.indexOf("V") >= 0 && resp.indexOf("62") < 0 && resp.indexOf("7F") < 0) {
        // ATRV response: "12.4V" or "13.3V OK"
        String numStr = resp;
        int vIdx = numStr.indexOf('V');
        if (vIdx > 0) numStr = numStr.substring(0, vIdx);
        numStr.trim();
        obd12VFloat = numStr.toFloat();
        obd12V = numStr + "V";
        Serial.printf("[ZOE] 12V = %s (%.1f)\n", obd12V.c_str(), obd12VFloat);
      } else if (resp.indexOf("NO DATA") >= 0 || resp.indexOf("ERROR") >= 0 ||
                 resp.indexOf("7F") >= 0) {
        Serial.printf("[ZOE] ECU error/no data: %s\n", resp.c_str());
      }

      DisplayManager::updateHomeOBD();
    }

    // --- Minimum 1-second poll cycle interval ---
    // When starting a new cycle (pollIndex == 0), ensure at least 1s since last cycle start
    if (obdPollIndex == 0 && pollCycleStartTime > 0) {
      if (millis() - pollCycleStartTime < 1000) {
        return;  // Wait until 1 second has passed
      }
    }
    // Record cycle start time when beginning a new cycle
    if (obdPollIndex == 0) {
      pollCycleStartTime = millis();
    }

    // --- Build the dynamic poll sequence ---
    // Phase 1: EVC params from pagePollList
    // Phase 2: HVAC (if needed)
    // Phase 3: ATRV (always)

    // Count EVC params in poll list
    int evcCount = 0;
    for (int i = 0; i < pagePollCount; i++) {
      int p = pagePollList[i];
      if (p == 0 || p == 1 || p == 3 || p == 11 || p == 14) evcCount++;
      if (p == 14) evcCount++; // param 14 needs 2 EVC steps (voltage + current)
    }
    
    // Count LBC params
    int lbcCount = 0;
    if (pageNeedsLBC) {
      lbcCount = 1; // Always poll both Max and Min together via 2103
    }
    
    int hvacStep = evcCount + lbcCount; // poll index where HVAC starts
    int atrvStep = hvacStep + (pageNeedsHVAC ? 1 : 0); // ATRV after HVAC
    int totalSteps = atrvStep + 1;

    lastOBDSentTime = millis();
    obdResponsePending = true;

    if (obdZoeMode) {
      if (obdPollIndex < evcCount) {
        // --- EVC Phase ---
        if (obdCurrentECU != 0) {
          Serial.println("[OBD] Switching to EVC (7E4/7EC)...");
          switchToECU("7E4", "7EC");
          sendAndWaitResponse("10C0", 500);
          obdCurrentECU = 0;
        }
        
        // Build expanded EVC command list (param 14 expands to 2 steps: voltage + current)
        int evcCmds[32];
        int evcCmdCount = 0;
        for (int i = 0; i < pagePollCount && evcCmdCount < 30; i++) {
          int p = pagePollList[i];
          if (p == 0 || p == 1 || p == 3 || p == 11 || p == 14) {
            evcCmds[evcCmdCount++] = p;
            if (p == 14) evcCmds[evcCmdCount++] = 99; // shadow: HV Current
          }
        }
        if (obdPollIndex < evcCmdCount) {
          const char *cmd = getEvcCommand(evcCmds[obdPollIndex]);
          if (cmd) sendCommand(cmd);
        }
      } else if (obdPollIndex < evcCount + lbcCount) {
        // --- LBC Phase (Non-blocking state machine) ---
        lbcState = LBC_SWITCH_SH;
        lbcCmdSentTime = 0;
        lastOBDValue = "";
        processLbcStep();
        obdPollIndex++;
        return;
      } else if (obdPollIndex == hvacStep && pageNeedsHVAC) {
        // Start HVAC state machine
        hvacState = HVAC_SWITCH_SH;
        hvacCmdSentTime = 0;
        lastOBDValue = "";
        processHvacStep();
        obdPollIndex++;
        return;
      } else {
        // ATRV
        sendCommand("ATRV");
      }
      obdPollIndex = (obdPollIndex + 1) % totalSteps;
    } else {
      sendCommand("ATRV");
    }
  }
}
