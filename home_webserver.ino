#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>

// Replace with your network credentials
const char* ssid = "****";
const char* password = "****";

WiFiServer server(80);                              // Web Server
Preferences preferences;                            // Non-volatile storage (NVS)

unsigned long startMillis;
unsigned long elapsedMillis;
unsigned long savedCounter;

void setup() {
  Serial.begin(115200);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Initialize Preferences
  preferences.begin("counter", false);
  savedCounter = preferences.getULong("counter", 0);

  // Retrieve the saved counter value
  startMillis = millis() - savedCounter;

  // Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Start the server
  server.begin();                                       // Start the server
}

void loop() {
  // Calculate elapsed time
  elapsedMillis = millis() - startMillis;

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("New Client.");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Serve the HTML file
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
  while (file.available()) {
    s += String((char)file.read());
  }
  file.close();

  // Replace placeholders with actual values
  String counterHTML = String((elapsedMillis / 60000) % 60) + ":" + String((elapsedMillis / 1000) % 60);
  s.replace("{{savedCounter}}", counterHTML);

  // Send the response to the client
  client.print(s);
  delay(1);

  // Close the connection
  Serial.println("Client disconnected.");
  client.stop();

  // Save the current counter value
  preferences.putULong("counter", elapsedMillis);
}
