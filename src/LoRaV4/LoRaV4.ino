#include "LoRaWan_APP.h"
#include "Arduino.h"

static const uint32_t RF_FREQUENCY = 433775000; // Hz

// ===== LED SETUP =====
// Auf vielen CubeCell-Boards ist die LED über Vext/Pin anders.
// Standardmäßig nehmen wir LED_BUILTIN, wenn vorhanden.
// Falls deine LED nicht blinkt: setze LED_PIN auf den richtigen Pin.
#ifndef LED_BUILTIN
  #define LED_BUILTIN 0
#endif
static const int LED_PIN = LED_BUILTIN;

// ===== Radio callbacks =====
static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
static void OnRxTimeout(void);
static void OnRxError(void);

static RadioEvents_t RadioEvents;

// ===== RSSI smoothing =====
static const int RSSI_WIN = 10;
static int16_t rssiBuf[RSSI_WIN];
static int idx = 0;
static int filled = 0;
static float lastAvg = -200.0f;

static float rssiAvg() {
  if (filled == 0) return -200.0f;
  long sum = 0;
  for (int i = 0; i < filled; i++) sum += rssiBuf[i];
  return (float)sum / (float)filled;
}

static const char* trendWord(float avg, float prev) {
  if (filled < RSSI_WIN) return "WARMUP";
  float d = avg - prev; // weniger negativ = stärker => "HOT"
  if (d > 2.0f) return "HOT";
  if (d < -2.0f) return "COLD";
  return "STABLE";
}

// ===== LED blink mapping (RSSI -> Blink Interval) =====
// RSSI typischerweise: -120 (sehr schwach) bis -50 (sehr stark)
// Wir mappen auf Intervall: 1200ms (langsam) bis 80ms (schnell)
static uint32_t blinkIntervalMs = 1200;
static uint32_t lastBlinkMs = 0;
static bool ledState = false;

static uint32_t clampU32(uint32_t v, uint32_t lo, uint32_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static uint32_t mapRssiToBlink(int16_t rssi) {
  // Clamp RSSI in sinnvollen Bereich
  int16_t r = rssi;
  if (r < -120) r = -120;
  if (r > -50)  r = -50;

  // Linear mapping: -120 -> 1200ms, -50 -> 80ms
  // Je größer (weniger negativ) desto schneller
  float t = (float)(r - (-120)) / (float)((-50) - (-120)); // 0..1
  float ms = 1200.0f + t * (80.0f - 1200.0f);
  if (ms < 80.0f) ms = 80.0f;
  if (ms > 1200.0f) ms = 1200.0f;
  return (uint32_t)ms;
}

static void StartRx() {
  Radio.Rx(0); // continuous
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("=== CubeCell #5 RX Finder (Serial + LED Hot/Cold) ===");

  RadioEvents.RxDone    = OnRxDone;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError   = OnRxError;

  Radio.Init(&RadioEvents);
  Radio.SetPublicNetwork(false);
  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetRxConfig(
    MODEM_LORA,
    0,      // BW 125kHz
    7,      // SF7
    1,      // CR 4/5
    0,      // AFC (FSK)
    8,      // preamble
    0,      // symbTimeout
    false,  // fixLen
    0,      // payloadLen
    true,   // crcOn
    0,      // freqHopOn
    0,      // hopPeriod
    false,  // iqInverted
    true    // rxContinuous
  );

  StartRx();
}

void loop() {
  // Radio IRQ handling
  Radio.IrqProcess();

  // LED blinken (non-blocking)
  uint32_t now = millis();
  if (now - lastBlinkMs >= blinkIntervalMs) {
    lastBlinkMs = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  }
}

static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  char buf[128];
  uint16_t n = (size < (sizeof(buf) - 1)) ? size : (sizeof(buf) - 1);
  memcpy(buf, payload, n);
  buf[n] = '\0';

  // RSSI window
  rssiBuf[idx] = rssi;
  idx = (idx + 1) % RSSI_WIN;
  if (filled < RSSI_WIN) filled++;

  float avg = rssiAvg();
  const char* trend = trendWord(avg, lastAvg);
  lastAvg = avg;

  // Blink-Speed updaten: wir nehmen den aktuellen RSSI (oder avg)
  blinkIntervalMs = mapRssiToBlink((int16_t)avg);

  Serial.print("[RX] ");
  Serial.print(buf);
  Serial.print(" | RSSI=");
  Serial.print(rssi);
  Serial.print(" dBm | SNR=");
  Serial.print((int)snr);
  Serial.print(" dB | AVG10=");
  Serial.print(avg, 1);
  Serial.print(" | LED=");
  Serial.print(blinkIntervalMs);
  Serial.print("ms | ");
  Serial.println(trend);

  StartRx();
}

static void OnRxTimeout(void) {
  StartRx();
}

static void OnRxError(void) {
  Serial.println("[RX] error");
  StartRx();
}
