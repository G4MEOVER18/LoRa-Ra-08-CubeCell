#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RadioLib.h>

// =================== CONFIG ===================
const char* WIFI_SSID = "DEIN_WLAN";
const char* WIFI_PASS = "DEIN_PASS";

const char* MQTT_HOST = "192.168.1.10";   // BreachGateway IP
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER = "";              // falls nötig
const char* MQTT_PASS = "";              // falls nötig

// Node identity
String nodeId = String(ESP.getChipId(), HEX);

// Topics
String baseTopic   = "breach/ra08/" + nodeId;
String tStatus     = baseTopic + "/status";
String tLoraRx     = baseTopic + "/lora/rx";
String tLoraTx     = baseTopic + "/lora/tx";
String tLoraDown   = baseTopic + "/lora/down";

// LoRa config (für 433 MHz Bereich)
float LORA_FREQ_MHZ = 433.175;
int   LORA_SF = 9;         // spreading factor 7-12
long  LORA_BW = 125000;    // 125 kHz
int   LORA_CR = 7;         // coding rate denominator (5-8). In RadioLib: 5..8
int   LORA_TX_DBM = 17;    // vorsichtig (legal/region beachten)

// =================== PINMAPPING ===================
// >>> Diese 3 Werte sind der Knackpunkt <<<
// Wenn es nicht läuft: Pins prüfen/anpassen.
#define LORA_CS   15   // Beispiel: GPIO15
#define LORA_DIO0  4   // Beispiel: GPIO4
#define LORA_RST  5   // Beispiel: GPIO5

// SPI Pins beim ESP8266 sind typischerweise:
// SCK=GPIO14, MISO=GPIO12, MOSI=GPIO13 (Hardware SPI)
// ================================================

// RadioLib Module (SX1278)
SX1278 lora = new Module(LORA_CS, LORA_DIO0, LORA_RST, RADIOLIB_NC);

// WiFi/MQTT
WiFiClient espClient;
PubSubClient mqtt(espClient);

unsigned long lastStatusMs = 0;

// --------- helpers ----------
String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 10);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '\"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

void publishStatus(const char* state) {
  String payload = "{";
  payload += "\"state\":\"" + String(state) + "\"";
  payload += ",\"uptime_s\":" + String(millis()/1000);
  payload += ",\"wifi_rssi\":" + String(WiFi.RSSI());
  payload += ",\"heap\":" + String(ESP.getFreeHeap());
  payload += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  payload += "}";
  mqtt.publish(tStatus.c_str(), payload.c_str(), true); // retained
}

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    yield();
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  String t = topic;
  String msg;
  msg.reserve(len);
  for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];

  // Downlink: MQTT -> LoRa
  if (t == tLoraDown) {
    // sende exakt den MQTT-Payload als LoRa-Payload (raw)
    int st = lora.transmit(msg);
    String out = "{";
    out += "\"dir\":\"mqtt_to_lora\"";
    out += ",\"ok\":" + String(st == RADIOLIB_ERR_NONE ? "true" : "false");
    out += ",\"status\":" + String(st);
    out += ",\"payload\":\"" + jsonEscape(msg) + "\"";
    out += "}";
    mqtt.publish(tLoraTx.c_str(), out.c_str(), false);
  }
}

void ensureMqtt() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  if (mqtt.connected()) return;

  // Last Will: offline, retained
  String willPayload = "{\"state\":\"offline\"}";
  String clientId = "ra08-" + nodeId;

  bool ok;
  if (String(MQTT_USER).length() > 0) {
    ok = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS,
                      tStatus.c_str(), 1, true, willPayload.c_str());
  } else {
    ok = mqtt.connect(clientId.c_str(), tStatus.c_str(), 1, true, willPayload.c_str());
  }

  if (ok) {
    mqtt.subscribe(tLoraDown.c_str());
    publishStatus("online");
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  ensureWifi();

  // Init LoRa
  // optional: SPI.begin(); // ESP8266 uses default hardware SPI pins
  int st = lora.begin(LORA_FREQ_MHZ);
  if (st != RADIOLIB_ERR_NONE) {
    Serial.printf("LoRa init failed: %d\n", st);
  } else {
    lora.setSpreadingFactor(LORA_SF);
    lora.setBandwidth(LORA_BW);
    lora.setCodingRate(LORA_CR);
    lora.setOutputPower(LORA_TX_DBM);
    Serial.println("LoRa OK");
  }

  ensureMqtt();
}

void loop() {
  ensureWifi();
  ensureMqtt();
  mqtt.loop();

  // Periodischer Status
  if (millis() - lastStatusMs > 10000) {
    lastStatusMs = millis();
    if (mqtt.connected()) publishStatus("online");
  }

  // LoRa RX Polling (einfach & robust)
  String rx;
  int st = lora.receive(rx);
  if (st == RADIOLIB_ERR_NONE) {
    // Metadaten
    float rssi = lora.getRSSI();
    float snr  = lora.getSNR();

    String out = "{";
    out += "\"dir\":\"lora_to_mqtt\"";
    out += ",\"payload\":\"" + jsonEscape(rx) + "\"";
    out += ",\"rssi\":" + String(rssi, 1);
    out += ",\"snr\":" + String(snr, 1);
    out += ",\"freq_mhz\":" + String(LORA_FREQ_MHZ, 3);
    out += ",\"sf\":" + String(LORA_SF);
    out += ",\"bw\":" + String(LORA_BW);
    out += "}";
    mqtt.publish(tLoraRx.c_str(), out.c_str(), false);

  } else if (st != RADIOLIB_ERR_RX_TIMEOUT && st != RADIOLIB_ERR_NONE) {
    // optional: Fehlerreporting
  }

  delay(10);
}
