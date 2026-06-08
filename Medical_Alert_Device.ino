#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

// Wi-Fi Settings
const char* ssid = "commune";
const char* password = "automatic";

#define TELEGRAM_BUTTON_PIN D7

// Telegram config
#define BOT_TOKEN "7069156211:AAGNESFQLadkxY_Y0RTJwrMJmbnEGPliOG4"
#define CHAT_ID "6974906607"

// SSL client
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

String ipAddress = "192.168.1.72";
volatile bool telegramButtonPressedFlag = false;

ICACHE_RAM_ATTR void telegramButtonPressed() {
  telegramButtonPressedFlag = true;
}

void setup() {
  Serial.begin(115200);

  // Initialize the button
  pinMode(TELEGRAM_BUTTON_PIN, INPUT_PULLUP);

  // Attach interrupt
  attachInterrupt(digitalPinToInterrupt(TELEGRAM_BUTTON_PIN), telegramButtonPressed, FALLING);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to WiFi network
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  ipAddress = ip.toString();

  // Ensure secure connection
  client.setInsecure(); // Use this line for quick testing without certificate validation
}

// Function to URL encode strings
String urlEncode(const String &str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
    yield();
  }
  return encodedString;
}

void sendTelegramMessage() {
  String message = "I've fallen and I can't get up!" + ipAddress;
  String encodedMessage = urlEncode(message);
  String url = "/bot" + String(BOT_TOKEN) + "/sendMessage?chat_id=" + String(CHAT_ID) + "&text=" + encodedMessage;

  Serial.println("Sending message to Telegram...");
  Serial.print("Requesting URL: ");
  Serial.println(url);

  if (client.connect("api.telegram.org", 443)) {
    Serial.println("Connection established.");
    
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: api.telegram.org\r\n" +
                 "User-Agent: ESP8266\r\n" +
                 "Connection: close\r\n\r\n");

    // Read the response from the server
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }

    String payload = client.readString();
    Serial.println("Response payload:");
    Serial.println(payload);

    // Check for success in the response
    if (payload.indexOf("\"ok\":true") != -1) {
      Serial.println("Telegram message successfully sent");
    } else {
      Serial.println("Failed to send message to Telegram");
    }
  } else {
    Serial.println("Connection failed.");
  }
  client.stop();
  telegramButtonPressedFlag = false;
}

void loop() {
  if (telegramButtonPressedFlag) {
    sendTelegramMessage();
  }
}
