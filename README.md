# Flock Squawk - Enhanced Flock Safety Detector

A PlatformIO project for the Xiao ESP32 S3 that detects Flock Safety cameras via **multiple detection methodologies** including WiFi sniffing, BLE scanning, MAC address detection, and device name patterns.

## **Enhanced Detection Methods**

### **1. WiFi SSID Detection**
- **Promiscuous Mode**: Captures probe requests and beacon frames in real-time
- **Active Scanning**: Monitors all WiFi traffic on 2.4GHz channels (1-13)
- Detects SSIDs containing: "flock", "Flock", "FLOCK", "FS Ext Battery", "Penguin", "Pigvision"
- **Channel Hopping**: Automatically cycles through channels every 2.5 seconds
- **Probe Request Monitoring**: Captures device probe requests even for hidden networks

### **2. BLE Device Detection**
- **Passive BLE scanning** for nearby devices
- **MAC address prefix matching** from known Flock Safety device ranges
- **Device name pattern matching** in BLE advertisement data
- Detects devices broadcasting names like "FS Ext Battery", "Penguin", etc.

### **3. MAC Address Detection**
- **35+ known MAC address prefixes** from real Flock Safety devices
- Covers multiple device types: FS Ext Battery, Penguin, Flock WiFi, Pigvision
- Detects devices even when SSIDs are randomized or hidden

### **4. Device Name Detection**
- Pattern matching for device names in BLE advertisements
- Case-insensitive matching for maximum detection coverage

## Features

- **Multi-Method Detection**: WiFi + BLE + MAC + Device Names
- **Promiscuous WiFi Monitoring**: Captures probe requests and beacons in real-time
- **JSON Detection Output**: Structured data with timestamps, RSSI, MAC addresses, and device info
- **Audio Alerts**: Buzzer notifications with distinct sound patterns
- **Boot Sequence**: Two beeps (low, medium) on startup
- **Detection Alert**: Three consecutive high-pitch beeps when Flock device detected
- **Channel Hopping**: Automatically cycles through 2.4GHz WiFi channels
- **Passive Operation**: No signals transmitted, only receives and monitors

## Hardware Requirements

- **Xiao ESP32 S3** board
- **Buzzer** connected to GPIO3 (D2) and GND
- **USB-C cable** for programming and power

## Wiring

```
Xiao ESP32 S3    Buzzer
GPIO3 (D2)  ---> Positive (+)
GND         ---> Negative (-)
```

## Installation

1. **Install PlatformIO** (if not already installed):
   ```bash
   pip install platformio
   ```

2. **Clone or download this project**

3. **Build and upload**:
   ```bash
   cd flock-squawk
   pio run --target upload
   ```

4. **Monitor serial output**:
   ```bash
   pio device monitor
   ```

## Usage

1. **Power on** the device - you'll hear two beeps (low, then medium pitch)
2. **Wait** for the device to start scanning (WiFi + BLE simultaneously)
3. **When a Flock Safety device is detected**, you'll hear three consecutive high-pitch beeps
4. **Check the serial monitor** for JSON detection output including:
   - **Timestamp**: Milliseconds since boot
   - **Detection Type**: "wifi" or "ble"  
   - **Detection Method**: "probe_request", "beacon", "mac_prefix", "device_name"
   - **RSSI**: Signal strength in dBm
   - **MAC Address**: Full device MAC address
   - **SSID**: Network name (for WiFi detections)
   - **Device Name**: BLE advertised name (for BLE detections)
   - **Matched Pattern**: Specific pattern that triggered detection

### **Example JSON Output**

**WiFi Probe Request Detection:**
```json
{
  "timestamp": 45230,
  "type": "wifi",
  "detection_method": "probe_request",
  "ssid": "Flock-Cam-01",
  "rssi": -65,
  "mac": "70:c9:4e:12:34:56",
  "matched_pattern": "Flock"
}
```

**BLE Device Detection:**
```json
{
  "timestamp": 47890,
  "type": "ble", 
  "detection_method": "mac_prefix",
  "mac": "58:8e:81:ab:cd:ef",
  "rssi": -72,
  "device_name": "FS Ext Battery",
  "matched_pattern": "58:8e:81"
}
```

## Detection Coverage

### **WiFi Detection**
- **Channels**: 2.4GHz (1-13) with automatic channel hopping
- **Scan Method**: WiFi promiscuous mode - captures all frames in real-time
- **Frame Types**: Probe requests, beacon frames, and other management frames
- **SSID Patterns**: 6 different patterns covering all known Flock Safety naming conventions
- **Channel Hop Interval**: Every 2.5 seconds

### **BLE Detection**
- **Scan Type**: Passive scanning (no interference)
- **MAC Prefixes**: 35+ known Flock Safety device MAC address ranges
- **Device Names**: Pattern matching for 4 different device name types
- **Advertisement Parsing**: Complete and short local name detection

### **MAC Address Ranges Detected**
- **FS Ext Battery**: 58:8e:81, cc:cc:cc, ec:1b:bd, 90:35:ea, 04:0d:84, f0:82:c0, etc.
- **Flock WiFi**: 70:c9:4e, 3c:91:80, d8:f3:bc, 80:30:49, 14:5a:fc, etc.
- **Penguin**: cc:09:24, ed:c7:63, e8:ce:56, ea:0c:ea, d8:8f:14, etc.
- **Pigvision**: Various manufacturer-specific ranges

## Technical Details

- **Framework**: Arduino (with NimBLE for BLE)
- **Board**: Xiao ESP32 S3
- **Buzzer Pin**: GPIO3 (D2)
- **WiFi Channels**: 2.4GHz (1-13)
- **Channel Hop Interval**: 2.5 seconds per channel
- **JSON Output**: Structured detection data with full device information
- **BLE Scanning**: Continuous passive scanning
- **Detection Patterns**: 6 SSID + 35 MAC + 4 Device Name patterns

## Detection Accuracy

### **High Detection Rate**
- **Multiple detection vectors** ensure devices are caught even if one method fails
- **MAC address detection** works regardless of SSID naming
- **BLE scanning** detects devices that may not broadcast WiFi
- **Pattern matching** covers various naming conventions

### **False Positive Reduction**
- **Specific MAC address ranges** from real device databases
- **Multiple pattern confirmation** before alerting
- **Case-insensitive matching** with exact pattern requirements

## Limitations

- Requires devices to be within WiFi/BLE range (~100m)
- May miss devices using only cellular connectivity
- Some devices may use randomized MAC addresses
- Detection depends on devices actively broadcasting

## Real-World Effectiveness

This enhanced system significantly improves detection rates by:

1. **Covering multiple communication protocols** (WiFi + BLE)
2. **Using real device databases** for accurate MAC address ranges
3. **Implementing pattern matching** for various naming conventions
4. **Providing redundant detection methods** for maximum coverage

## Credits

Based on the original Flock Safety Trap Shooter Sniffer Alarm concept.
Enhanced with real-world device data from multiple Flock Safety deployments.

## **Real-World Device Data**

The detection patterns in this system are based on **real device databases** located in the `datasets/` folder:

- **FS+Ext+Battery**: 4,908 Flock Safety Extended Battery devices
- **Penguin**: Large dataset of Penguin surveillance devices  
- **Flock WiFi**: Standard Flock Safety WiFi network data
- **Pigvision**: Physical camera location data
- **Maximum Dots**: Multi-manufacturer surveillance system data

See `datasets/README.md` for detailed information about the data sources and analysis.

## License

Same as original project - see LICENSE file.
