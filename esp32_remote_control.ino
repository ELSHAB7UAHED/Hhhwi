/*
  ================================================================================
  ESP32 Advanced Remote Control via Web & BLE
  ================================================================================
  - Creates a Wi-Fi Access Point for the controller device (e.g., Kali Linux).
  - Hosts a responsive web server with a command interface.
  - Acts as a Bluetooth (BLE) Human Interface Device (HID) keyboard.
  - Can send keystrokes to any paired host (PC, Mac, Linux, Android).
  - Author: Manus
  - Version: 1.0
  ================================================================================
*/

// ===============================================================================
// 1. LIBRARIES
// ===============================================================================
#include <WiFi.h>
#include <BleKeyboard.h>

// ===============================================================================
// 2. CONFIGURATION
// ===============================================================================
// --- Wi-Fi Access Point Settings ---
const char* WIFI_SSID = "ESP32_Control_Hub"; // The name of the Wi-Fi network
const char* WIFI_PASS = "secure12345";      // The password for the Wi-Fi network (min 8 chars)
WiFiServer server(80);                      // Web server runs on port 80

// --- Bluetooth HID Settings ---
// Device Name, Manufacturer, Initial Battery Level
BleKeyboard bleKeyboard("ESP32 Pro Keyboard", "Manus-Corp", 100);

// ===============================================================================
// 3. SETUP FUNCTION (runs once at startup)
// ===============================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n[INFO] Initializing ESP32 Advanced Remote Control...");

  // --- Start Bluetooth Keyboard ---
  bleKeyboard.begin();
  Serial.println("[OK] Bluetooth HID Service started.");
  Serial.print("[ACTION] Pair this device. Name: '");
  Serial.print(bleKeyboard.getDeviceName().c_str());
  Serial.println("'");

  // --- Start Wi-Fi Access Point ---
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  IPAddress apIP = WiFi.softAPIP();
  Serial.println("[OK] Wi-Fi Access Point is active.");
  Serial.print("[INFO] SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("[ACTION] Connect your controller and go to: http://");
  Serial.println(apIP);

  // --- Start Web Server ---
  server.begin();
  Serial.println("[OK] Web server is running.");
  Serial.println("\n[READY] System is fully operational.");
}

// ===============================================================================
// 4. HELPER FUNCTIONS
// ===============================================================================

/**
 * @brief Sends the HTML web page to the connected client.
 * @param client The WiFiClient object.
 */
void sendHtmlPage(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html><head><title>ESP32 Remote</title><meta name='viewport' content='width=device-width, initial-scale=1'>");
  client.println("<style>");
  client.println("html { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; }");
  client.println("body { background-color: #121212; color: #e0e0e0; margin: 0; padding: 20px; }");
  client.println(".container { max-width: 800px; margin: 0 auto; text-align: center; }");
  client.println("h1 { color: #bb86fc; border-bottom: 2px solid #bb86fc; padding-bottom: 10px; margin-bottom: 20px; }");
  client.println(".grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 15px; }");
  client.println(".btn { background-color: #03dac6; color: #000; padding: 15px 10px; text-decoration: none; border-radius: 8px; font-weight: bold; transition: transform 0.2s, background-color 0.2s; }");
  client.println(".btn:hover { background-color: #3700b3; color: #fff; transform: scale(1.05); }");
  client.println("</style></head><body><div class='container'><h1>ESP32 Remote Control</h1>");
  client.println("<div class='grid'>");
  // PC Commands
  client.println("<a class='btn' href='/cmd_terminal'>Open Terminal</a>");
  client.println("<a class='btn' href='/cmd_lock'>Lock Screen</a>");
  client.println("<a class='btn' href='/cmd_run'>Run Dialog (Win)</a>");
  // Android Commands
  client.println("<a class='btn' href='/cmd_home'>Android Home</a>");
  client.println("<a class='btn' href='/cmd_back'>Android Back</a>");
  client.println("<a class='btn' href='/cmd_apps'>Android Apps</a>");
  client.println("</div></div></body></html>");
}

/**
 * @brief Processes the command from the HTTP request and sends keystrokes.
 * @param requestLine The first line of the HTTP GET request.
 */
void processCommand(const String& requestLine) {
  if (requestLine.indexOf("GET /cmd_terminal") >= 0) {
    Serial.println("[CMD] Executing: Open Terminal (Ctrl+Alt+T)");
    bleKeyboard.press(KEY_LEFT_CTRL);
    bleKeyboard.press(KEY_LEFT_ALT);
    bleKeyboard.write('t');
    bleKeyboard.releaseAll();
  } else if (requestLine.indexOf("GET /cmd_lock") >= 0) {
    Serial.println("[CMD] Executing: Lock Screen (Win+L)");
    bleKeyboard.press(KEY_LEFT_GUI);
    bleKeyboard.write('l');
    bleKeyboard.releaseAll();
  } else if (requestLine.indexOf("GET /cmd_run") >= 0) {
    Serial.println("[CMD] Executing: Run Dialog (Win+R)");
    bleKeyboard.press(KEY_LEFT_GUI);
    bleKeyboard.write('r');
    bleKeyboard.releaseAll();
  } else if (requestLine.indexOf("GET /cmd_home") >= 0) {
    Serial.println("[CMD] Executing: Android Home");
    bleKeyboard.write(KEY_HOME);
  } else if (requestLine.indexOf("GET /cmd_back") >= 0) {
    Serial.println("[CMD] Executing: Android Back");
    bleKeyboard.write(KEY_ESC);
  } else if (requestLine.indexOf("GET /cmd_apps") >= 0) {
    Serial.println("[CMD] Executing: Android Recent Apps (Alt+Tab)");
    bleKeyboard.press(KEY_LEFT_ALT);
    bleKeyboard.write(KEY_TAB);
    bleKeyboard.releaseAll();
  }
}

// ===============================================================================
// 5. MAIN LOOP (runs repeatedly)
// ===============================================================================
void loop() {
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  Serial.println("\n[NET] New client connected.");
  unsigned long startTime = millis();
  String currentLine = "";

  while (client.connected() && (millis() - startTime < 2000)) { // Timeout to prevent hanging
    if (client.available()) {
      char c = client.read();
      if (c == '\n') {
        if (currentLine.length() == 0) {
          sendHtmlPage(client);
          break;
        } else {
          // First line of the request is what we care about
          processCommand(currentLine);
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }

  client.stop();
  Serial.println("[NET] Client disconnected.");
}
