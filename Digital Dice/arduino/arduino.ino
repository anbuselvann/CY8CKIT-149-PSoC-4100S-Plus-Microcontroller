#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>

const char* ssid     = "ASN";
const char* password = "anbu1234";

WebSocketsServer webSocket(81);

String serialBuffer = "";   // accumulate incoming bytes

void setup() {
  Serial.begin(9600);
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket started on port 81");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.printf("Client [%u] connected\n", num);
  } else if (type == WStype_DISCONNECTED) {
    Serial.printf("Client [%u] disconnected\n", num);
  }
}

void loop() {
  webSocket.loop();

  // Read all available bytes into buffer
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r') {
      // Line complete — process it
      serialBuffer.trim();   // remove any \r or spaces

      Serial.print("Line received: [");
      Serial.print(serialBuffer);
      Serial.println("]");

      // Expected format: "R3" where 3 is dice value 1-6
      if (serialBuffer.length() == 2 &&
          serialBuffer.charAt(0) == 'R') {

        char digit = serialBuffer.charAt(1);

        if (digit >= '1' && digit <= '6') {
          String msg = String(digit);
          Serial.print("Sending to WebSocket: ");
          Serial.println(msg);
          webSocket.broadcastTXT(msg);
        } else {
          Serial.println("Invalid dice value");
        }
      } else if (serialBuffer == "READY") {
        Serial.println("PSoC is READY");
      }

      serialBuffer = "";   // clear for next line

    } else {
      serialBuffer += c;   // keep accumulating

      // Safety: prevent buffer overflow
      if (serialBuffer.length() > 32) {
        serialBuffer = "";
        Serial.println("Buffer overflow cleared");
      }
    }
  }
}