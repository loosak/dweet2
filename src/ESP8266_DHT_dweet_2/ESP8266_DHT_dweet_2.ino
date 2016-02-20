#include <ESP8266WiFi.h>
#include <Ticker.h>

#include "DHT.h"      //Adafruit DHT-sensor-library
#define DHTPIN 5      // GPIO05 == D1
#define DHTTYPE DHT22 // DHT11, DHT21, DHT22
// ESP8266 initialize DHT as follows: DHT dht(DHTPIN, DHTTYPE, 15);
DHT dht(DHTPIN, DHTTYPE, 15);

// WiFi parameters
// const char *ssid = ".........";       // WiFi.SSID()
// const char *pass = ".........";       // WiFi.psk()

// Host
const char *host = "dweet.io";

Ticker ticker;
// flag changed in the ticker function every 1 minutes
bool readyForUpdate = true;

void setReadyForUpdate() {
  Serial.printf("Setting readyForUpdate: %u\n", millis());
  readyForUpdate = true;
}

void update();

void setup() {
  Serial.begin(115200);

  // We start by connecting to a WiFi network
  Serial.printf("\n\nConnecting to %s\n", WiFi.SSID().c_str());
  // WiFi.begin(ssid, pass);
  WiFi.begin();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWiFi connected\nIP address: %s\n",
                WiFi.localIP().toString().c_str());
  Serial.printf("Channel: %i\n", WiFi.channel());

  dht.begin();

  // flag changed in the ticker function every 1 minutes
  ticker.attach(1 * 60, setReadyForUpdate);
}

void loop() {
  if (readyForUpdate) {
    readyForUpdate = false;
    update();
  }
}

void update() {
  float h = dht.readHumidity();    // Luftfeuchte auslesen
  float t = dht.readTemperature(); // Temperatur auslesen

  if (isnan(t) || isnan(h)) {
    Serial.println("DHT22 konnte nicht ausgelesen werden");
    return;
  }

  String humm = String(h);
  String temp = String(t);

  Serial.printf("--------------------------------------------------------------"
                "-------\n");
  Serial.printf("\nConnecting to %s\n", host);
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  // This will send the request to the server
  client.print(String("GET /dweet/for/kuchyn?temperature=") + temp +
               "&humidity=" + humm + " HTTP/1.1\r\n" + "Host: " + host +
               "\r\n" + "Connection: close\r\n\r\n");
  delay(10);
  // Read all the lines of the reply from server and print them to Serial
  int i = 0;
  int size = 0;
  int parsedPayload = 0;

  while ((size = client.available()) > 0) {
    String line = client.readStringUntil('\n');
    i++;
    // Serial.printf("line no: %i %s \n", i, line.c_str());
    if (line.startsWith("Content-Length: ")) {
      // Serial.println( line.substring(15, line.length()) );
      parsedPayload = line.substring(15, line.length()).toInt();
    }

    if (line.startsWith("Date: ")) { // Date: Sun, 13 Dec 2015 14:00:22 GMT
      // Serial.println(line.substring(23, 25) + ":" + line.substring(26, 28) +
      // ":" + line.substring(29, 31) + " " + line.substring(18, 22) + " " +
      // line.substring(11, 18) );
      int parsedHours = (line.substring(23, 25).toInt()) + 1; // CET DST
      int parsedMinutes = line.substring(26, 28).toInt();
      int parsedSeconds = line.substring(29, 31).toInt();
      int parsedYear = line.substring(18, 22).toInt();
      int parsedDay = line.substring(11, 13).toInt();
      String parsedMonth = line.substring(14, 17);
      Serial.printf("Date %02i:%02i:%02i %02i %s %i \t Luftfeuchte: %s "
                    "Temperatur: %s °C\n",
                    parsedHours, parsedMinutes, parsedSeconds, parsedDay,
                    parsedMonth.c_str(), parsedYear, humm.c_str(),
                    temp.c_str());
    }

    if (i == 8) {
      Serial.printf("line #%i: len: %i pay: %i  %s \n", i, line.length(),
                    parsedPayload, line.c_str());
    }
  }
  Serial.printf("\nClosing connection to %s\n", host);
}
