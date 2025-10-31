#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// --- ACCESS POINT CREDENTIALS ---
const char* ap_ssid = "GeneratorAP";
const char* ap_password = "Ax151Lw!";

// --- PIN DEFINITIONS ---
#define BTN_PLUS 18
#define BTN_MINUS 19
#define BTN_GEN 26
#define BTN_SLEEP 25
#define WAKE_PIN GPIO_NUM_33

// --- EEPROM CONFIGURATION ---
#define EEPROM_SIZE 10

// --- WEB SERVER AND LOGGING SETUP ---
AsyncWebServer server(80);
#define LOG_BUFFER_SIZE 25
String logLines[LOG_BUFFER_SIZE];
int currentLogLine = 0;

// --- GLOBAL VARIABLES ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
int passwordLength = 16;
const int minLength = 8;
const int maxLength = 32;
bool menuMode = false;
int menuIndex = 0;
bool useUpper, useLower, useDigits, useSymbolsBase, useSymbolsSpecial;
bool settingsModified = false;
String generatedPassword = "";

// --- NON-BLOCKING BUTTON HANDLING ---
unsigned long lastActionTime = 0;
unsigned long actionCooldown = 250; // (Test) Increased cooldown for more stability

// Function to log messages to Serial and Web
void logMessage(String message, bool addToWebLog = true) {
  Serial.println(message);
  if (addToWebLog) {
    logLines[currentLogLine] = message;
    currentLogLine = (currentLogLine + 1) % LOG_BUFFER_SIZE;
  }
}

// Function to build the HTML for the web log
String getLogsAsHtml() {
  String htmlLogs = "";
  int start = currentLogLine;
  for (int i = 0; i < LOG_BUFFER_SIZE; i++) {
    int index = (start + i) % LOG_BUFFER_SIZE;
    if (logLines[index] != "") {
      htmlLogs += logLines[index] + "<br>";
    }
  }
  return htmlLogs;
}

void setup() {
  // Disable Brownout Detector for testing without decoupling capacitors
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  EEPROM.begin(EEPROM_SIZE);
  Serial.begin(115200);

  // --- NEW: I2C Bus Stabilization ---
  // Slow down the I2C clock to a more stable speed for the LCD
  Wire.begin();
  Wire.setClock(100000); // 100kHz

  lcd.init();
  lcd.backlight();
  lcd.print("Initializing...");
  delay(1000);

  // Wi-Fi Access Point and Web Server Initialization
  lcd.clear();
  lcd.print("Creating AP:");
  lcd.setCursor(0, 1);
  lcd.print(ap_ssid);
  
  WiFi.softAP(ap_ssid, ap_password);
  WiFi.setTxPower(WIFI_POWER_11dBm); // low power wifi avoids brownout even without decoupling capacitors as of tests
                                     // plus it's safer to have a lower range for the transmission of the passwords
  IPAddress apIP = WiFi.softAPIP();
  
  lcd.clear();
  lcd.print("AP Started!");
  lcd.setCursor(0, 1);
  lcd.print(apIP);
  logMessage("Access Point started. IP: " + apIP.toString());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<!DOCTYPE html><html><head><title>ESP32 Logs</title><meta http-equiv='refresh' content='5'><style>body{font-family:monospace; background-color:#1e1e1e; color:#d4d4d4;} h1{color:#569cd6;}</style></head><body><h1>Generator Log</h1><p>";
    html += getLogsAsHtml();
    html += "</p></body></html>";
    request->send(200, "text/html", html);
  });

  server.begin();
  logMessage("Web Server started.");
  delay(2000);

  // Load settings from EEPROM
  if (EEPROM.read(5) != 0xAB) {
    logMessage("First run. Loading default settings.");
    useUpper = true; useLower = true; useDigits = true; useSymbolsBase = true; useSymbolsSpecial = true;
    saveSettings(useUpper, useLower, useDigits, useSymbolsBase, useSymbolsSpecial);
  } else {
    logMessage("Loading settings from EEPROM.");
    loadSettings(useUpper, useLower, useDigits, useSymbolsBase, useSymbolsSpecial);
  }

  // Initialize pins
  pinMode(BTN_PLUS, INPUT_PULLUP);
  pinMode(BTN_MINUS, INPUT_PULLUP);
  pinMode(BTN_GEN, INPUT_PULLUP);
  pinMode(BTN_SLEEP, INPUT_PULLUP);
  pinMode((int)WAKE_PIN, INPUT_PULLUP);

  esp_sleep_enable_ext0_wakeup(WAKE_PIN, 0);

  lcd.clear();
  showLength();
}

void loop() {
  // The main loop is now clean and runs very fast.
  if (millis() - lastActionTime > actionCooldown) {
    if (menuMode) {
      handleMenu();
    } else {
      handleMain();
    }
  }
}

// --- NEW: Button check function with software debounce ---
bool is_pressed(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(20); // Wait a moment to see if it's just noise
    if (digitalRead(pin) == LOW) { // Check again
      lastActionTime = millis(); // Reset cooldown timer
      return true;
    }
  }
  return false;
}


void saveSettings(bool u, bool l, bool n, bool sb, bool ss) {
  logMessage("Saving settings to EEPROM...");
  EEPROM.write(0, u); EEPROM.write(1, l); EEPROM.write(2, n); EEPROM.write(3, sb); EEPROM.write(4, ss);
  EEPROM.write(5, 0xAB);
  EEPROM.commit();
  logMessage("Settings saved successfully.");
}

void loadSettings(bool &u, bool &l, bool &n, bool &sb, bool &ss) {
  u = EEPROM.read(0); l = EEPROM.read(1); n = EEPROM.read(2); sb = EEPROM.read(3); ss = EEPROM.read(4);
}

// --- MODIFIED: Uses the new is_pressed() function ---
void handleMain() {
  if (digitalRead(BTN_PLUS) == LOW && digitalRead(BTN_MINUS) == LOW) {
    lastActionTime = millis() + 300; // Longer cooldown for two buttons
    menuMode = true;
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("+-:Nav  G:Toggle");
    showMenu();
  }
  else if (is_pressed(BTN_PLUS)) {
    if (passwordLength < maxLength) {
      passwordLength++;
      showLength();
    }
  }
  else if (is_pressed(BTN_MINUS)) {
    if (passwordLength > minLength) {
      passwordLength--;
      showLength();
    }
  }
  else if (is_pressed(BTN_GEN)) {
    generatedPassword = generatePassword(passwordLength);
    if (generatedPassword.length() > 0) {
        showPassword();
        logMessage("Password generated: " + generatedPassword);
    }
  }
  else if (is_pressed(BTN_SLEEP)) {
    sleepy();
  }
}

// --- MODIFIED: Uses the new is_pressed() function ---
void handleMenu() {
  if (is_pressed(BTN_PLUS)) {
    menuIndex = (menuIndex + 1) % 5;
    showMenu();
  }
  else if (is_pressed(BTN_MINUS)) {
    menuIndex = (menuIndex + 4) % 5;
    showMenu();
  }
  else if (is_pressed(BTN_GEN)) {
    toggleOption(menuIndex);
    showMenu();
  }
  else if (is_pressed(BTN_SLEEP)) {
    menuMode = false;
    lcd.clear();
    showLength();
  }
}

void showMenu() {
  lcd.setCursor(0, 0);
  lcd.print("                "); // Clear the line with spaces
  lcd.setCursor(0, 0);
  
  switch (menuIndex) {
    case 0: lcd.print("Uppercase: "); lcd.print(useUpper ? "ON" : "OFF"); break;
    case 1: lcd.print("Lowercase: "); lcd.print(useLower ? "ON" : "OFF"); break;
    case 2: lcd.print("Numbers: "); lcd.print(useDigits ? "ON" : "OFF"); break;
    case 3: lcd.print("Base Sym: "); lcd.print(useSymbolsBase ? "ON" : "OFF"); break;
    case 4: lcd.print("Sim. ext: "); lcd.print(useSymbolsSpecial ? "ON" : "OFF"); break;
  }
}

void toggleOption(int index) {
  String optionName;
  switch (index) {
    case 0: useUpper = !useUpper; optionName = "Uppercase"; break;
    case 1: useLower = !useLower; optionName = "Lowercase"; break;
    case 2: useDigits = !useDigits; optionName = "Numbers"; break;
    case 3: useSymbolsBase = !useSymbolsBase; optionName = "Base Sym"; break;
    case 4: useSymbolsSpecial = !useSymbolsSpecial; optionName = "Ext. Sym"; break;
  }
  settingsModified = true;
  logMessage("Setting '" + optionName + "' changed.");
}

void sleepy() {
  lcd.clear();
  lcd.print("Sleep mode...");
  logMessage("Entering sleep mode...");

  if (settingsModified) {
    saveSettings(useUpper, useLower, useDigits, useSymbolsBase, useSymbolsSpecial);
    settingsModified = false;
  } else {
    logMessage("Settings not modified, skipping EEPROM write.");
  }

  delay(1000);
  lcd.clear();
  lcd.print("Sleep mode");
  lcd.setCursor(0, 1);
  delay(200);
  lcd.print(".");
  delay(200);
  lcd.setCursor(1, 1);
  lcd.print(".");
  delay(200);
  lcd.setCursor(2, 1);
  lcd.print(".");
  delay(1000);
  lcd.clear();
  lcd.noBacklight();
  esp_deep_sleep_start();
}

void showLength() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Length: ");
  lcd.print(passwordLength);
  lcd.print("  "); // Add spaces to clear old digits
}

void showPassword() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(generatedPassword.substring(0, 16));
  if (passwordLength > 16) {
    lcd.setCursor(0, 1);
    lcd.print(generatedPassword.substring(16, 32));
  }
}

String generatePassword(int length) {
  String charset = "";

  if (useUpper) charset += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  if (useLower) charset += "abcdefghijklmnopqrstuvwxyz";
  if (useDigits) charset += "0123456789";
  if (useSymbolsBase) charset += "!@#$%^?*()_+-=";
  if (useSymbolsSpecial) charset += "&[]{}|<>";

  if (charset.length() <= 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error:");
    lcd.setCursor(0, 1);
    lcd.print("charset empty.");
    logMessage("ERROR: charset is empty, cannot generate password.");
    delay(1500);
    showLength();
    return "";
  }

  String pwd = "";
  for (int i = 0; i < length; i++) {
    int idx = esp_random() % charset.length();
    pwd += charset.charAt(idx);
  }
  return pwd;
}
