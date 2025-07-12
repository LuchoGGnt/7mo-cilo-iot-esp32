#include <WiFi.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHTesp.h"
#include <time.h>  // Para NTP

// Pines
const int DHT_PIN = 19;
const int MQ135_PIN = 33;

// Objetos
DHTesp dhtSensor;
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// Tópico MQTT para publicación
#define AWS_IOT_PUBLISH_TOPIC "iot/ambiente/data"

// Variables
String temperature;
String humidity;
int airQuality;

// Sincronización NTP
void syncTime() {
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Sincronizando con NTP");
  while (time(nullptr) < 8 * 3600 * 2) {
    Serial.print(".");
    delay(500);
  }

  Serial.println(" ¡Hora sincronizada!");
  time_t now = time(nullptr);
  Serial.println(ctime(&now));
}

// Conexión WiFi + AWS IoT Core
void connectAWS() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ¡Conectado!");

  syncTime();  // Sincroniza con NTP antes de TLS

  // Cargar certificados
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Conectar a AWS
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  Serial.print("Conectando a AWS IoT Core");
  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected()) {
    Serial.println(" ¡Error al conectar a AWS!");
    return;
  }

  Serial.println(" ¡Conectado a AWS IoT Core!");
}

// Publicar datos al tópico MQTT
void publishMessage() {
  StaticJsonDocument<256> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["air_quality"] = airQuality;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void setup() {
  Serial.begin(115200);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT11);
  connectAWS();
}

void loop() {
  // Leer sensores
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  temperature = String(data.temperature, 2);
  humidity = String(data.humidity, 1);
  airQuality = analogRead(MQ135_PIN);

  // Interpretar calidad del aire con rangos detallados
  String estadoAire;
  if (airQuality <= 1500) {
    estadoAire = "Excelente";
  } else if (airQuality <= 2500) {
    estadoAire = "Buena";
  } else if (airQuality <= 3200) {
    estadoAire = "Moderada";
  } else if (airQuality <= 3800) {
    estadoAire = "Pobre";
  } else {
    estadoAire = "Mala";
  }

  // Obtener hora actual (ya sincronizada por NTP)
  time_t now = time(nullptr);
  String timestamp = String(ctime(&now));  // Ej: "Tue Jul 09 19:45:10 2025\n"

  // Mostrar datos en consola
  Serial.println("Temperatura: " + temperature + " °C");
  Serial.println("Humedad: " + humidity + " %");
  Serial.println("Calidad del aire (ADC): " + String(airQuality));
  Serial.println("Estado del aire: " + estadoAire);
  Serial.println("Hora actual: " + timestamp);
  Serial.println("---");

  // Publicar datos a AWS IoT Core
  StaticJsonDocument<300> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["air_quality"] = airQuality;
  doc["estado_aire"] = estadoAire;
  doc["timestamp"] = timestamp;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);

  client.loop();
  delay(5000);
}