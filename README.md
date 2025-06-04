# Nano WS2812B Notifier 🟢

A visual Nano (XNO) transaction monitor based on Wemos D1 Mini with WS2812B LEDs that light up with colorful animations for every received transaction.

## 🌟 Features

- **Real-time monitoring** of Nano transactions
- **Colorful LED animations** for each received transaction
- **Watchdog system** with automatic restart in case of issues
- **Simple WiFi configuration** via captive portal
- **Status LEDs** to monitor system state
- **Automatic heartbeat** to keep connection alive
- **Robust error handling** with reconnection capabilities

## 🔧 Required Hardware

- **Wemos D1 Mini** (ESP8266-based board)
![s-l1200](https://github.com/user-attachments/assets/cc83ffb1-87a8-400f-afcb-9f04bf000b83)

- **4x WS2812B LEDs** (NeoPixel strip or individual LEDs)
![2846-2X2NEOPIXEL](https://github.com/user-attachments/assets/33eadb3e-05fe-455c-8c4a-d0853b1ce628)
- Breadboard and jumper wires

## 📋 Wiring Diagram

```
Wemos D1 Mini    →    WS2812B LED Strip
5V/3.3V          →    VCC
GND              →    GND
D3 (GPIO0)       →    DIN (Data In)

```
![Immagine 2025-06-04 102336](https://github.com/user-attachments/assets/e18dafc9-8c05-49c6-b87c-6ce89e3ec7fe)
## 📚 Required Libraries

Install these libraries through Arduino Library Manager:

```
- ESP8266WiFi (included in ESP8266 Core)
- WebSocketsClient by Markus Sattler
- ArduinoJson by Benoit Blanchon  
- WiFiManager by tzapu
- Adafruit NeoPixel
- Ticker (included in ESP8266 Core)
```

## 🚀 Installation

1. **Open the file** `nano_ws2812_monitor.ino` in Arduino IDE

2. **Install required libraries** listed above

3. **Configure the board**:
   - Select **Wemos D1 Mini** from Tools > Board
   - Select the correct COM port
   - Set upload speed to 115200

4. **Upload the code** to your Wemos D1 Mini

## ⚙️ Configuration

### Initial Setup

1. **Power up the Wemos D1 Mini** - LED 0 (portal) will turn green
2. **Connect to WiFi network** `NanoMonitor` from your device
3. **Open browser** and navigate to `192.168.4.1`
4. **Configure settings**:
   - Select your WiFi network
   - Enter WiFi password
   - **Important**: Enter your Nano address in the "Nano Address" field
5. **Save** - The device will restart automatically

### Status LEDs

The system uses 4 LEDs to indicate status:

| LED | Color | Meaning |
|-----|-------|---------|
| 0 | 🟢 Green | Configuration portal active |
| 1 | 🟢 Green | WiFi connected |
| 2 | 🟢 Green | WebSocket connected |
| 3 | 🟢 Green | Subscription active |
| All | 🔴 Red | Connection error |

Status LEDs automatically turn off 3 seconds after subscription to avoid interfering with transaction animations.

## 🎨 Animations

### Transaction Animation
When a transaction is received:

1. **White flash** (3 times)
2. **Colorful rotation** with gradient effects
3. **Final pulsation** with cycling colors

Animation colors:
- 🟢 Aqua green
- 🔵 Blue
- 🟣 Purple
- 🟠 Orange

## 🛡️ Security & Reliability

### Watchdog Timer
- **Timeout**: 5 minutes of inactivity
- **Warning**: Alert at 4 minutes
- **Automatic restart** on timeout
- **Heartbeat**: Ping every minute to maintain connection

### Error Handling
- Automatic WebSocket reconnection
- System restart on critical errors
- Error animation before restart
- Activity monitoring and logging

## 🔧 Customization

### Modify Colors
```cpp
// Transaction animation colors
uint32_t COLOR_TRANSACTION[] = {
  strip.Color(0, 255, 100),    // Aqua green
  strip.Color(100, 200, 255),  // Blue
  strip.Color(200, 100, 255),  // Purple
  strip.Color(255, 150, 0)     // Orange
};
```

### Modify Timeouts
```cpp
const unsigned long WATCHDOG_TIMEOUT = 300000; // 5 minutes
const unsigned long HEARTBEAT_INTERVAL = 60000; // 1 minute
const unsigned long LED_OFF_DELAY = 3000; // 3 seconds
```

### Change Number of LEDs
```cpp
#define LED_COUNT 4  // Modify this value
```

### Change Data Pin
```cpp
#define LED_PIN D3  // Use any available digital pin
```

## 🐛 Troubleshooting

### LEDs Don't Light Up
- Check wiring connections
- Verify power supply (5V for WS2812B)
- Check data line resistor
- Ensure proper ground connection

### WiFi Connection Issues
- Reset WiFi settings by holding RESET button
- Verify router supports 2.4GHz
- Check username/password
- Move closer to router

### No Transaction Notifications
- Verify Nano address is correct
- Check internet connection
- Monitor serial output for debugging
- Ensure WebSocket connection is active

### Device Keeps Restarting
- Check power supply stability
- Monitor serial output for error messages
- Verify all connections are secure

## 📊 Serial Monitor

Open Serial Monitor (115200 baud) to see:
- Connection status updates
- Received transactions with amounts
- Watchdog warnings
- Debug messages
- Error notifications

## 🔍 Technical Details

### WebSocket Connection
- **Server**: bitrequest.app:8010
- **Protocol**: WSS (Secure WebSocket)
- **Auto-reconnect**: 5 second interval
- **Subscription**: Real-time Nano confirmations

### Power Consumption
- **Idle**: ~80mA
- **WiFi active**: ~120mA
- **LEDs on**: +20mA per LED
- **Recommended supply**: 5V/1A

### Memory Usage
- **Flash**: ~400KB
- **RAM**: ~45KB
- **Free heap**: ~35KB

## 🤝 Contributing

Contributions are welcome!

## 📝 License

This project is distributed under the MIT License. See `LICENSE` file for more details.

## 🙏 Acknowledgments

- [Nano Foundation](https://nano.org/) for the Nano cryptocurrency
- [BitRequest](https://bitrequest.app/) for the WebSocket API
- Arduino and ESP8266 community
- Adafruit for the NeoPixel library

---

⭐ If you like this project, please star it on GitHub!

💡 **Tip**: You can use multiple devices with different Nano addresses to monitor multiple wallets simultaneously!
