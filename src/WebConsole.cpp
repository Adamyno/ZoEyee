#include "WebConsole.h"
#include "ObdManager.h"
#include <WiFi.h>

WebServer WebConsole::server(80);
String WebConsole::logBuffer = "";

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ZoEyee OBD Console</title>
<style>
body { background:#1e1e1e; color:#bbb; font-family:monospace; padding:10px; margin:0; }
h2 { color:#0f0; margin-top:0; }
#terminal { width:100%; height:60vh; background:#000; color:#0f0; overflow-y:scroll; padding:10px; border:1px solid #444; box-sizing:border-box; margin-bottom:10px; }
.input-row { display:flex; gap:10px; margin-bottom:10px; }
input[type="text"] { flex:1; padding:10px; background:#333; color:#fff; border:1px solid #555; }
button { padding:10px 20px; background:#080; color:#fff; border:none; cursor:pointer; font-weight:bold; }
button:hover { background:#0a0; }
.manual-toggle { padding:10px; background:#555; color:#fff; cursor:pointer; display:inline-block; border-radius:4px; font-weight:bold; }
.manual-on { background:#d00 !important; }
</style>
</head>
<body>
<h2>ZoEyee OBD Diagnostic Console</h2>
<div id="manualBtn" class="manual-toggle" onclick="toggleManual()">Manual Mode: OFF</div>
<br><br>

<div id="terminal"></div>

<form class="input-row" onsubmit="sendCmd(event)">
  <input type="text" id="cmdInput" placeholder="Enter AT or UDS command..." autocomplete="off">
  <button type="submit">SEND</button>
</form>

<script>
let terminal = document.getElementById("terminal");
let manualMode = false;

function printLog(msg, color="#0f0") {
  terminal.innerHTML += `<span style="color:${color}">${msg}</span><br>`;
  terminal.scrollTop = terminal.scrollHeight;
}

function pollLogs() {
  fetch('/poll')
    .then(res => res.text())
    .then(text => {
      if(text.length > 0) {
        text.split('\n').forEach(line => {
          if(line.trim()) printLog("RX: " + line);
        });
      }
    }).finally(() => {
      setTimeout(pollLogs, 300);
    });
}

function sendCmd(e) {
  e.preventDefault();
  if(!manualMode) {
    printLog("ERROR: You must turn ON Manual Mode first!", "#f00");
    return;
  }
  let inp = document.getElementById("cmdInput");
  let cmd = inp.value.trim();
  if(!cmd) return;
  
  printLog("TX: " + cmd, "#0ff");
  fetch('/cmd?q=' + encodeURIComponent(cmd));
  inp.value = '';
}

function toggleManual() {
  fetch('/toggleManual')
    .then(res => res.text())
    .then(state => {
      manualMode = (state === "1");
      let btn = document.getElementById("manualBtn");
      if(manualMode) {
        btn.className = "manual-toggle manual-on";
        btn.innerHTML = "Manual Mode: ON (Polling Suspended)";
        printLog("--- MANUAL MODE ENABLED ---", "#ff0");
      } else {
        btn.className = "manual-toggle";
        btn.innerHTML = "Manual Mode: OFF (Auto Polling Active)";
        printLog("--- MANUAL MODE DISABLED ---", "#ff0");
      }
    });
}

// Start polling
setTimeout(pollLogs, 500);
</script>
</body>
</html>
)rawliteral";

void WebConsole::begin() {
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", htmlPage);
    });

    server.on("/poll", HTTP_GET, []() {
        server.send(200, "text/plain", logBuffer);
        logBuffer = ""; // CLEAR buffer after sending
    });

    server.on("/cmd", HTTP_GET, []() {
        if(server.hasArg("q")) {
            String cmd = server.arg("q");
            ObdManager::sendManualCommand(cmd.c_str());
        }
        server.send(200, "text/plain", "OK");
    });

    server.on("/toggleManual", HTTP_GET, []() {
        ObdManager::manualMode = !ObdManager::manualMode;
        server.send(200, "text/plain", ObdManager::manualMode ? "1" : "0");
    });

    server.begin();
    Serial.println("[WebConsole] Server started on port 80");
}

void WebConsole::handleClient() {
    server.handleClient();
}

void WebConsole::pushLog(const String& line) {
    logBuffer += line + "\n";
    // Prevent giant buffer leaks if web interface isn't open
    if(logBuffer.length() > 4096) {
        logBuffer = logBuffer.substring(logBuffer.length() - 2048);
    }
}
