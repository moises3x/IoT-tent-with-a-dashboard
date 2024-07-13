#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin Definitions
#define DHTPIN D6 // GPIO12
#define DHTTYPE DHT11
#define MQPIN A0
#define PIRPIN D5 // GPIO14
#define MOTION_BLUE_PIN D3 // GPIO0

// GPS Pin Definitions
#define RX_PIN D7 // GPIO13
#define TX_PIN D8 // GPIO15

// WiFi and Server Configuration
const char* ssid = "Visitors";
const char* password = "";
const char* serverName = "http://10.72.48.18:3000/api/sensor_data";

DHT dht(DHTPIN, DHTTYPE);
TinyGPSPlus gps;

const int motionDebounceDelay = 500; // milliseconds for debouncing
unsigned long lastMotionTime = 0;
bool motionDetected = false;

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600); // Initialize Serial1 for GPS module
    while (!Serial) { ; } // Wait for Serial to be ready

    Serial.println(F("Starting setup..."));

    dht.begin();
    pinMode(MQPIN, INPUT);
    pinMode(PIRPIN, INPUT);
    pinMode(MOTION_BLUE_PIN, OUTPUT);

    // Initialize I2C for OLED display
    Wire.begin(D2, D1); // SDA = D2 (GPIO4), SCL = D1 (GPIO5)

    Serial.println(F("Initializing OLED display..."));
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C is a common I2C address for OLED displays
        Serial.println(F("SSD1306 allocation failed"));
        while (1); // Stay here if there is a problem
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println(F("Setup Complete"));
    display.display();
    Serial.println(F("OLED display initialized"));

    Serial.println(F("Connecting to WiFi..."));
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println(F("Connecting to WiFi..."));
    }
    Serial.println(F("WiFi connected."));
    Serial.print(F("IP Address: "));
    Serial.println(WiFi.localIP());
}

void loop() {
    maintainWiFiConnection();
    bool currentMotionDetected = checkForMotion();

    if (currentMotionDetected != motionDetected) {
        if (millis() - lastMotionTime > motionDebounceDelay) {
            motionDetected = currentMotionDetected;
            lastMotionTime = millis();
            updateMotionBlueLed(motionDetected);
        }
    }

    readSensorsAndDisplay(motionDetected);
    delay(5000); // Increase the delay to 5 seconds
}

bool checkForMotion() {
    int sensorValue = digitalRead(PIRPIN);
    Serial.print(F("PIR Sensor Value: ")); // Debugging output for PIR sensor value
    Serial.println(sensorValue);
    return sensorValue == HIGH; // Adjusted to check for HIGH signal when motion is detected
}

void updateMotionBlueLed(bool motionDetected) {
    if (motionDetected) {
        Serial.println(F("Motion detected!"));
        digitalWrite(MOTION_BLUE_PIN, HIGH);
    } else {
        Serial.println(F("No motion detected."));
        digitalWrite(MOTION_BLUE_PIN, LOW);
    }
}

void maintainWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WiFi not connected, attempting to reconnect..."));
        WiFi.reconnect();
    }
}

void readSensorsAndDisplay(bool motionDetected) {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int airQuality = analogRead(MQPIN);

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
    }

    // Update the OLED display
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print(F("Temp: ")); display.print(temperature); display.println(F(" C"));
    display.setCursor(0, 10);
    display.print(F("Hum: ")); display.print(humidity); display.println(F(" %"));
    display.setCursor(0, 20);
    display.print(F("AirQ: ")); display.print(airQuality);
    display.display();
    delay(100); // Add a short delay after updating the display

    Serial.println(F("OLED display updated"));

    Serial.println(F("Reading Sensors:"));
    Serial.println(F("Temperature: ") + String(temperature) + F("C"));
    Serial.println(F("Humidity: ") + String(humidity) + F("%"));
    Serial.println(F("Air Quality: ") + String(airQuality));
    Serial.println(F("Motion Detected: ") + String(motionDetected));

    sendDataToServer(temperature, humidity, airQuality, motionDetected);
}

String getIPGeolocation() {
    String ipGeolocation = "N/A";
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;
        http.begin(client, "http://ip-api.com/json");
        int httpResponseCode = http.GET();
        if (httpResponseCode > 0) {
            String payload = http.getString();
            StaticJsonDocument<300> doc;
            deserializeJson(doc, payload);
            float lat = doc["lat"];
            float lon = doc["lon"];
            ipGeolocation = String(lat, 6) + "," + String(lon, 6);
        }
        http.end();
    }
    return ipGeolocation;
}

void sendDataToServer(float temp, float hum, int airQual, bool motionDetected) {
    if (WiFi.status() == WL_CONNECTED) {
        String ipGeolocation = getIPGeolocation();
        StaticJsonDocument<300> doc;
        doc["temperature"] = temp;
        doc["humidity"] = hum;
        doc["airQuality"] = airQual;
        doc["motionDetected"] = motionDetected;

        if (ipGeolocation != "N/A") {
            int commaIndex = ipGeolocation.indexOf(',');
            String lat = ipGeolocation.substring(0, commaIndex);
            String lon = ipGeolocation.substring(commaIndex + 1);
            JsonObject gps = doc.createNestedObject("gps");
            gps["latitude"] = lat;
            gps["longitude"] = lon;
        }

        WiFiClient client;
        HTTPClient http;
        http.begin(client, serverName);
        http.addHeader("Content-Type", "application/json");

        String requestBody;
        serializeJson(doc, requestBody);

        Serial.println(F("Sending data to server..."));
        int httpResponseCode = http.POST(requestBody);
        if (httpResponseCode > 0) {
            Serial.println(F("HTTP Response code: ") + String(httpResponseCode));
        } else {
            Serial.print(F("Error on sending POST: "));
            Serial.println(httpResponseCode);
        }
        http.end();
    } else {
        Serial.println(F("Error in WiFi connection"));
    }
}
