# ESP8266 WiFi Oscilloscope - WebSocket Implementation

## Overview

This project has been converted from HTTP request-based communication to real-time WebSocket communication for improved performance and responsiveness.

## Changes Made

### 1. WebSocket Server Setup

- Added WebSocket server on port 81
- Integrated WebSocketsServer library
- WebSocket server runs alongside HTTP server

### 2. Communication Protocol

- **WebSocket Port**: 81
- **HTTP Port**: 80 (for web interface serving only)

#### WebSocket Message Format

**Client to Server (JSON):**

```json
{
  "cmd": "getData",
  "mode": 0, // 0=analog, 1=digital
  "speed": 0, // 0=70ksps, 1=15ksps, 2=300sps (analog) / 3msps, 100ksps, 3ksps (digital)
  "trigger": 0 // 0=none, 1=rising, 2=falling
}
```

**Server to Client:**

- Binary data: Raw oscilloscope samples (Uint16Array)
- JSON status messages for connection confirmation

### 3. Web Interface Changes

- Replaced XMLHttpRequest with WebSocket
- Added connection status indicator
- Real-time data streaming instead of polling
- Automatic reconnection on disconnect

### 4. ESP8266 Code Changes

- Added `userMainWebSocket()` function parallel to original `userMain()`
- WebSocket event handling for incoming data requests
- Binary data transmission via WebSocket instead of HTTP response
- Maintained original HTTP interface for backward compatibility

## Benefits

1. **Real-time performance**: No HTTP request overhead
2. **Lower latency**: Direct binary data streaming
3. **Better reliability**: Automatic reconnection
4. **Reduced server load**: No HTTP polling
5. **Bidirectional communication**: Ready for future control features

## Usage

1. Connect to WiFi AP (default: WiCardOSC / 12345678)
2. Navigate to 192.168.4.1 (or 192.168.5.1 if odd IP configured)
3. WebSocket connection establishes automatically
4. Use oscilloscope controls as before - now with WebSocket backend

## Installation Requirements

- ESP8266 Arduino Core
- WebSocketsServer library (install via Arduino Library Manager)
- Standard ESP8266 libraries (WiFi, WebServer, etc.)

## Network Configuration

- Device operates as WiFi Access Point only
- No external WiFi connections attempted
- WebSocket server: ws://[device-ip]:81/
- Web interface: http://[device-ip]/

## Testing the WebSocket Implementation

### 1. Upload to ESP8266

1. Install the WebSocketsServer library in Arduino IDE:

   - Go to Sketch > Include Library > Manage Libraries
   - Search for "WebSocketsServer" by Markus Sattler
   - Install the "WebSockets" library

2. Upload the firmware to your ESP8266

### 2. Testing Steps

1. Connect to the WiFi network "WiCardOSC" (password: 12345678)
2. Open browser and navigate to http://192.168.4.1
3. The main oscilloscope interface should show "WebSocket: Connected" status
4. For detailed testing, save the websocket_test.html file and open it in browser

### 3. Expected Behavior

- WebSocket should connect automatically on port 81
- Real-time data streaming without page refresh
- Green "WS✓" indicator in the status bar
- Immediate response to control changes

### 4. Troubleshooting

- If WebSocket fails to connect, check ESP8266 serial output
- Ensure port 81 is not blocked by firewall
- Verify WebSocketsServer library is properly installed
- Check browser console for JavaScript errors

### 5. Performance Benefits

- Reduced latency compared to HTTP polling
- Real-time data streaming
- Lower CPU usage on ESP8266
- Better responsiveness to control changes
