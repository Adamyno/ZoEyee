// ============================================================
// ZoEyee Language Strings
// ============================================================
//
// Ez a fajl tartalmazza az osszes felhasznaloi feluleti szoveget.
// Szerkeszd a masodik oszlopot (HU) a magyar forditashoz.
//
// FONTOS: A FreeSans fontok csak ASCII karaktereket tamogatnak
// (0x20-0x7E), ezert az ekezetes betuk helyett ekezet nelkuli
// valtozatot kell hasznalni:
//   a/e/i/o/u/o/u   (nem: á/é/í/ó/ú/ö/ü/ő/ű)
//
// Minden sor formatuma: { "English", "Magyar" }
// ============================================================

#include "Lang.h"

//                                         EN                        HU
// ── Menu & Navigation ──────────────────────────────────────
const char* const lang_MENU[]            = { "MENU",                 "MENU" };
const char* const lang_INFO[]            = { "INFO",                 "INFO" };
const char* const lang_WIFI[]            = { "WIFI",                 "WIFI" };
const char* const lang_BT_SCAN[]         = { "BT SCAN",              "BT KERESES" };
const char* const lang_BRIGHTNESS[]      = { "BRIGHTNESS",           "FENYERO" };
const char* const lang_SETTINGS[]        = { "SETTINGS",             "BEALLITASOK" };

// ── Settings ───────────────────────────────────────────────
const char* const lang_PAGES[]           = { "Pages",                "Oldalak" };
const char* const lang_AUTO_SCROLL[]     = { "Auto-Scroll",          "Auto-latozas" };
const char* const lang_CAR_TYPE[]        = { "CAR TYPE",             "AUTO TIPUS" };
const char* const lang_LANGUAGE[]        = { "LANGUAGE",             "NYELV" };
const char* const lang_ON[]              = { "ON ",                  "BE " };
const char* const lang_OFF[]             = { "OFF",                  "KI " };
const char* const lang_PAGES_DESC[]      = { "Number of dashboard pages", "Muszerfal oldalak szama" };
const char* const lang_TAP_ARROWS[]      = { "Tap arrows to adjust", "Nyilakkal allitsd" };
const char* const lang_TAP_TOGGLE[]      = { "(tap to toggle)",      "(erintsd a valtashoz)" };
const char* const lang_INTERVAL[]        = { "Interval:",            "Idokoz:" };

// ── Brightness ─────────────────────────────────────────────
const char* const lang_BRIGHT_1[]        = { "Swipe up/down",        "Huzd fel/le" };
const char* const lang_BRIGHT_2[]        = { "anywhere to",          "barhol a" };
const char* const lang_BRIGHT_3[]        = { "adjust",               "fenyero" };
const char* const lang_BRIGHT_4[]        = { "brightness.",          "allitasahoz." };

// ── Info Screen ────────────────────────────────────────────
const char* const lang_FMT_SW_VER[]      = { "SW Version: %s",       "Verzio: %s" };
const char* const lang_FMT_FLASH[]       = { "Free Flash: %u / %uKB","Flash: %u / %u KB" };
const char* const lang_FMT_RAM[]         = { "Free RAM: %u / %uKB",  "RAM: %u / %u KB" };
const char* const lang_FMT_UPTIME[]      = { "Uptime: %02d:%02d:%02d","Uzemido: %02d:%02d:%02d" };

// ── Bluetooth ──────────────────────────────────────────────
const char* const lang_BT_DEVICES[]      = { "BT DEVICES",           "BT ESZKOZOK" };
const char* const lang_CONNECT[]         = { "CONNECT",              "CSATLAKOZAS" };
const char* const lang_DETAILS[]         = { "DETAILS",              "RESZLETEK" };
const char* const lang_NO_DEVICES[]      = { "No devices found.",    "Nincs talalat." };
const char* const lang_CONNECTED[]       = { "CONNECTED",            "CSATLAKOZVA" };
const char* const lang_DEVICE_DETAILS[]  = { "DEVICE DETAILS",       "ESZKOZ ADATOK" };
const char* const lang_SWIPE_BACK[]      = { "Swipe Right -> Back",  "Jobbra huzas -> Vissza" };
const char* const lang_BT_STATUS[]       = { "BT STATUS",            "BT ALLAPOT" };
const char* const lang_DISCONNECT[]      = { "DISCONNECT",           "LEVALASZTAS" };
const char* const lang_BLE_SCAN[]        = { "BLE SCAN",             "BLE KERESES" };
const char* const lang_SCANNING[]        = { "Scanning...",          "Kereses..." };
const char* const lang_CONNECTING[]      = { "Connecting...",        "Csatlakozas..." };
const char* const lang_NO_OBD[]          = { "No OBD service found!","Nincs OBD szolgaltatas!" };
const char* const lang_VERIFY_OBD[]      = { "Verifying OBD...",     "OBD ellenorzes..." };
const char* const lang_FMT_NAME[]        = { "Name: %s",             "Nev: %s" };
const char* const lang_FMT_MAC[]         = { "MAC : %s",             "MAC : %s" };
const char* const lang_FMT_MAC2[]        = { "MAC: %s",              "MAC: %s" };
const char* const lang_FMT_RSSI[]        = { "RSSI: %d dBm",         "RSSI: %d dBm" };
const char* const lang_FMT_DEVICE[]      = { "Device: %s",           "Eszkoz: %s" };
const char* const lang_STATUS_CONNECTED[]    = { "Status: Connected",     "Allapot: Csatlakozva" };
const char* const lang_STATUS_RECONNECTING[] = { "Status: Reconnecting...","Allapot: Ujracsatl..." };

// ── WiFi ───────────────────────────────────────────────────
const char* const lang_AP_MODE_ON[]      = { "AP MODE: ON",          "AP MOD: BE" };
const char* const lang_AP_MODE_OFF[]     = { "AP MODE: OFF",         "AP MOD: KI" };
const char* const lang_CLIENT_MENU[]     = { "CLIENT MENU",          "KLIENS MENU" };
const char* const lang_STATUS[]          = { "STATUS",               "ALLAPOT" };
const char* const lang_SCAN_NETWORKS[]   = { "SCAN NETWORKS",        "HALOZAT KERESES" };
const char* const lang_SAVE_AUTO[]       = { "Save & Auto-Conn",     "Mentes & Auto-Csatl." };
const char* const lang_DEL[]             = { "DEL",                  "TORL" };
const char* const lang_WIFI_LIST[]       = { "WIFI LIST",            "WIFI LISTA" };
const char* const lang_NO_NETWORKS[]     = { "No networks found",    "Nincs halozat" };
const char* const lang_SPACE_KEY[]       = { "SPACE",                "SZOKOZ" };
const char* const lang_BACK[]            = { "BACK",                 "VISSZA" };
const char* const lang_CONNECTED_OK[]    = { "Connected!",           "Csatlakozva!" };
const char* const lang_CONN_FAILED[]     = { "Connection failed!",   "Csatl. sikertelen!" };
const char* const lang_WIFI_STATUS[]     = { "WIFI STATUS",          "WIFI ALLAPOT" };
const char* const lang_MODE_AP[]         = { "Mode: Access Point",   "Mod: Hotspot (AP)" };
const char* const lang_MODE_CLIENT[]     = { "Mode: Client (Connected)","Mod: Kliens (Csatl.)" };
const char* const lang_NOT_CONNECTED[]   = { "Not Connected",        "Nincs kapcsolat" };

// ── Dashboard ──────────────────────────────────────────────
const char* const lang_EMPTY[]           = { "EMPTY",                "URES" };

// Dashboard parameter full names (indexed by param number 0-18)
// Must match the order in dashParams[] in DisplayManager.cpp
const char* const lang_paramNames[][LANG_COUNT] = {
  { "Battery SOH",    "Akku SOH" },       // 0
  { "Battery SOC",    "Akku SOC" },       // 1
  { "Cabin Temp",     "Belso hom." },   // 2
  { "Battery Temp",   "Akku hom." },      // 3
  { "AC Compressor",  "Klima kompr." },   // 4
  { "AC Pressure",    "Klima nyomas" },   // 5
  { "12V Battery",    "12V Akku" },       // 6
  { "Ext. Temp",      "Kulso hom." },     // 7
  { "Cell Delta V",   "Cella Delta V" },  // 8
  { "Cell V Max",     "Cella V Max" },    // 9
  { "Cell V Min",     "Cella V Min" },    // 10
  { "Engine Fan",     "Motor vent." },    // 11
  { "Climate Mode",   "Klima mod" },      // 12
  { "Max Charge",     "Max toltes" },     // 13
  { "DC Power",       "DC telj." },       // 14
  { "Avail Energy",   "Elerheto en." },   // 15
  { "HV Voltage",     "HV feszultseg" }, // 16
  { "AC Phase",       "AC fazis" },       // 17
  { "Insulation",     "Szigeteles" },     // 18
};
