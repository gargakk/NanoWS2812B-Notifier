#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <Adafruit_NeoPixel.h>
#include <Ticker.h>

// Configurazione LED WS2812B
#define LED_PIN D3
#define LED_COUNT 4
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Configurazione WebSocket
WebSocketsClient webSocket;
bool connected = false;
bool subscribed = false;

// Watchdog Timer
Ticker watchdogTicker;
volatile bool watchdogFlag = false;
unsigned long lastActivity = 0;
const unsigned long WATCHDOG_TIMEOUT = 300000; // 5 minuti
const unsigned long HEARTBEAT_INTERVAL = 60000; // 1 minuto
unsigned long lastHeartbeat = 0;

// Timer per spegnere i LED di stato
unsigned long subscriptionTime = 0;
const unsigned long LED_OFF_DELAY = 3000; // 3 secondi dopo sottoscrizione

// Indirizzo Nano
String nanoAddress = "nano_3fdqzd9pe55kpozg534kk1xskhybxy8ecjbc63ha7dx3o9nap84irgaqxd79";

// LED States
#define LED_PORTAL 0
#define LED_WIFI 1
#define LED_WEBSOCKET 2
#define LED_SUBSCRIBED 3

// Colori LED
uint32_t COLOR_OFF = strip.Color(0, 0, 0);
uint32_t COLOR_ACTIVE = strip.Color(0, 255, 0);
uint32_t COLOR_ERROR = strip.Color(255, 0, 0);

// Colori per animazione transazione
uint32_t COLOR_TRANSACTION[] = {
  strip.Color(0, 255, 100),    // Verde acqua
  strip.Color(100, 200, 255),  // Azzurro
  strip.Color(200, 100, 255),  // Viola
  strip.Color(255, 150, 0)     // Arancione
};

void ICACHE_RAM_ATTR watchdogISR();

void setup() {
  Serial.begin(115200);
  
  // Inizializza i LED
  strip.begin();
  strip.show();
  turnOffAllLeds();
  
  // Inizializza il watchdog
  initWatchdog();
  
  // Configura WiFi
  setupWiFi();
  
  // Configura WebSocket
  setupWebSocket();
  
  Serial.println("Setup completato!");
  updateActivity();
}

void loop() {
  // Gestisci watchdog
  if (watchdogFlag) {
    handleWatchdog();
  }
  
  // Spegni i LED di stato dopo 3 secondi dalla sottoscrizione
  if (subscribed && subscriptionTime > 0 && 
      millis() - subscriptionTime > LED_OFF_DELAY) {
    turnOffAllLeds();
    subscriptionTime = 0; // Evita di chiamare ancora
    Serial.println("LED di stato spenti - Sistema pronto");
  }
  
  // Heartbeat
  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeat = millis();
  }
  
  // Controlla timeout attività
  checkActivityTimeout();
  
  webSocket.loop();
  delay(10);
}

void setupWiFi() {
  setLed(LED_PORTAL, COLOR_ACTIVE);
  
  WiFiManager wm;
  WiFiManagerParameter nano_param("nano", "Nano Address", nanoAddress.c_str(), 65);
  wm.addParameter(&nano_param);
  
  if (!wm.autoConnect("NanoMonitor")) {
    Serial.println("Connessione WiFi fallita");
    setLed(LED_PORTAL, COLOR_ERROR);
    delay(3000);
    ESP.restart();
  }
  
  setLed(LED_PORTAL, COLOR_OFF);
  setLed(LED_WIFI, COLOR_ACTIVE);
  
  nanoAddress = nano_param.getValue();
  Serial.println("WiFi connesso!");
  Serial.println("Indirizzo Nano: " + nanoAddress);
  updateActivity();
}

void setupWebSocket() {
  webSocket.beginSSL("bitrequest.app", 8010, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      connected = false;
      subscribed = false;
      subscriptionTime = 0;
      Serial.println("WebSocket disconnesso");
      setLed(LED_WEBSOCKET, COLOR_ERROR);
      setLed(LED_SUBSCRIBED, COLOR_ERROR);
      break;
      
    case WStype_CONNECTED:
      connected = true;
      Serial.println("WebSocket connesso");
      setLed(LED_WEBSOCKET, COLOR_ACTIVE);
      subscribeToTransactions();
      updateActivity();
      break;
      
    case WStype_TEXT:
      handleWebSocketMessage(payload);
      updateActivity();
      break;
      
    case WStype_ERROR:
      Serial.println("Errore WebSocket");
      connected = false;
      subscribed = false;
      subscriptionTime = 0;
      setLed(LED_WEBSOCKET, COLOR_ERROR);
      setLed(LED_SUBSCRIBED, COLOR_ERROR);
      break;
      
    default:
      break;
  }
}

void subscribeToTransactions() {
  if (!connected) return;
  
  DynamicJsonDocument doc(512);
  doc["action"] = "subscribe";
  doc["topic"] = "confirmation";
  
  JsonObject options = doc.createNestedObject("options");
  options["all_local_accounts"] = true;
  options["accounts"][0] = nanoAddress;
  
  String message;
  serializeJson(doc, message);
  webSocket.sendTXT(message);
  
  Serial.println("Sottoscrizione inviata");
}

void handleWebSocketMessage(uint8_t * payload) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    Serial.print("Errore parsing JSON: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Conferma sottoscrizione
  if (!subscribed) {
    String ack = doc["ack"];
    if (ack == "subscribe") {
      subscribed = true;
      Serial.println("Sottoscrizione confermata");
      setLed(LED_SUBSCRIBED, COLOR_ACTIVE);
      subscriptionTime = millis(); // Avvia timer per spegnere i LED
      return;
    }
  }
  
  // Gestione transazioni
  if (subscribed && doc.containsKey("message")) {
    if (doc["message"].containsKey("block") && doc["message"].containsKey("amount")) {
      String subtype = doc["message"]["block"]["subtype"];
      String amount = doc["message"]["amount"];
      
      if (subtype == "send" && amount.length() > 0) {
        if (amount.length() >= 24) {
          String integerPart = amount.substring(0, amount.length() - 24);
          if (integerPart.length() == 0) integerPart = "0";
          
          double nanoAmount = integerPart.toDouble() / 1000000.0;
          
          Serial.print("Transazione ricevuta: ");
          Serial.print(nanoAmount, 6);
          Serial.println(" NANO");
          
          animateTransaction();
        }
      }
    }
  }
}

void setLed(int ledIndex, uint32_t color) {
  if (ledIndex >= 0 && ledIndex < LED_COUNT) {
    strip.setPixelColor(ledIndex, color);
    strip.show();
  }
}

void setAllLeds(uint32_t color) {
  for(int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void turnOffAllLeds() {
  for(int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, COLOR_OFF);
  }
  strip.show();
}

void animateTransaction() {
  Serial.println("Animazione transazione iniziata");
  
  // Flash iniziale
  for(int flash = 0; flash < 3; flash++) {
    setAllLeds(strip.Color(255, 255, 255));
    delay(50);
    turnOffAllLeds();
    delay(100);
  }
  
  // Rotazione colorata
  for(int cycle = 0; cycle < 8; cycle++) {
    for(int pos = 0; pos < LED_COUNT; pos++) {
      turnOffAllLeds();
      strip.setPixelColor(pos, COLOR_TRANSACTION[cycle % 4]);
      
      // Sfumatura sui LED adiacenti
      int prevPos = (pos - 1 + LED_COUNT) % LED_COUNT;
      strip.setPixelColor(prevPos, dimColor(COLOR_TRANSACTION[cycle % 4], 2));
      
      strip.show();
      delay(60);
    }
  }
  
  // Pulsazione finale
  for(int pulse = 0; pulse < 5; pulse++) {
    uint32_t pulseColor = COLOR_TRANSACTION[pulse % 4];
    
    for(int brightness = 0; brightness <= 255; brightness += 15) {
      uint32_t fadedColor = fadeColor(pulseColor, brightness);
      setAllLeds(fadedColor);
      delay(5);
    }
    
    for(int brightness = 255; brightness >= 0; brightness -= 15) {
      uint32_t fadedColor = fadeColor(pulseColor, brightness);
      setAllLeds(fadedColor);
      delay(5);
    }
  }
  
  turnOffAllLeds();
  Serial.println("Animazione transazione completata");
}

// Funzioni helper per i colori
uint32_t dimColor(uint32_t color, int factor) {
  return strip.Color(
    ((color >> 16) & 0xFF) / factor,
    ((color >> 8) & 0xFF) / factor,
    (color & 0xFF) / factor
  );
}

uint32_t fadeColor(uint32_t color, int brightness) {
  return strip.Color(
    (((color >> 16) & 0xFF) * brightness) / 255,
    (((color >> 8) & 0xFF) * brightness) / 255,
    ((color & 0xFF) * brightness) / 255
  );
}

// Funzioni Watchdog
void initWatchdog() {
  watchdogTicker.attach(1.0, watchdogISR);
  updateActivity();
  Serial.println("Watchdog inizializzato (timeout: 5 minuti)");
}

void ICACHE_RAM_ATTR watchdogISR() {
  watchdogFlag = true;
}

void updateActivity() {
  lastActivity = millis();
}

void checkActivityTimeout() {
  if (millis() - lastActivity > WATCHDOG_TIMEOUT) {
    Serial.println("WATCHDOG: Timeout raggiunto - Riavvio sistema");
    
    // Animazione errore
    for(int i = 0; i < 5; i++) {
      setAllLeds(COLOR_ERROR);
      delay(200);
      turnOffAllLeds();
      delay(200);
    }
    
    ESP.restart();
  }
}

void handleWatchdog() {
  watchdogFlag = false;
  
  // Warning a 4 minuti
  if (millis() - lastActivity > 240000) {
    Serial.print("WATCHDOG WARNING: Ultima attività ");
    Serial.print((millis() - lastActivity) / 1000);
    Serial.println(" secondi fa");
  }
}

void sendHeartbeat() {
  if (!connected) return;
  
  DynamicJsonDocument doc(128);
  doc["action"] = "ping";
  doc["timestamp"] = millis();
  
  String message;
  serializeJson(doc, message);
  webSocket.sendTXT(message);
  
  Serial.println("Heartbeat inviato");
  updateActivity();
}
