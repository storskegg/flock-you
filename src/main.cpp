#include <Arduino.h>
#include <WiFi.h>
#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>
#include <ArduinoJson.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"

// ============================================================================
// CONFIGURATION
// ============================================================================

// Hardware Configuration
#define BUZZER_PIN 3  // GPIO3 (D2) - PWM capable pin on Xiao ESP32 S3

// Audio Configuration
#define LOW_FREQ 200      // Boot sequence - low pitch
#define MED_FREQ 400      // Boot sequence - medium pitch  
#define HIGH_FREQ 800     // Detection alert - high pitch
#define BEEP_DURATION 200 // milliseconds per beep

// WiFi Promiscuous Mode Configuration
#define MAX_CHANNEL 13
#define CHANNEL_HOP_INTERVAL 2500  // milliseconds

// Detection Pattern Limits
#define MAX_SSID_PATTERNS 10
#define MAX_MAC_PATTERNS 50
#define MAX_DEVICE_NAMES 20

// ============================================================================
// DETECTION PATTERNS (Extracted from Real Flock Safety Device Databases)
// ============================================================================

// WiFi SSID patterns to detect (case-insensitive)
static const char* wifi_ssid_patterns[] = {
    "flock",        // Standard Flock Safety naming
    "Flock",        // Capitalized variant
    "FLOCK",        // All caps variant
    "FS Ext Battery", // Flock Safety Extended Battery devices
    "Penguin",      // Penguin surveillance devices
    "Pigvision"     // Pigvision surveillance systems
};

// Known Flock Safety MAC address prefixes (from real device databases)
static const char* mac_prefixes[] = {
    // FS Ext Battery devices
    "58:8e:81", "cc:cc:cc", "ec:1b:bd", "90:35:ea", "04:0d:84", 
    "f0:82:c0", "1c:34:f1", "38:5b:44", "94:34:69", "b4:e3:f9",
    
    // Flock WiFi devices
    "70:c9:4e", "3c:91:80", "d8:f3:bc", "80:30:49", "14:5a:fc",
    "74:4c:a1", "08:3a:88", "9c:2f:9d",
    
    // Penguin devices
    "cc:09:24", "ed:c7:63", "e8:ce:56", "ea:0c:ea", "d8:8f:14",
    "f9:d9:c0", "f1:32:f9", "f6:a0:76", "e4:1c:9e", "e7:f2:43",
    "e2:71:33", "da:91:a9", "e1:0e:15", "c8:ae:87", "f4:ed:b2",
    "d8:bf:b5", "ee:8f:3c", "d7:2b:21", "ea:5a:98"
};

// Device name patterns for BLE advertisement detection
static const char* device_name_patterns[] = {
    "FS Ext Battery",  // Flock Safety Extended Battery
    "Penguin",         // Penguin surveillance devices
    "Flock",           // Standard Flock Safety devices
    "Pigvision"        // Pigvision surveillance systems
};

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

static uint8_t current_channel = 1;
static unsigned long last_channel_hop = 0;
static bool triggered = false;
static NimBLEScan* pBLEScan;

// ============================================================================
// SYSTEM BANNER & DISPLAY
// ============================================================================

static void display_banner(void)
{
    printf("\n"
           "╔══════════════════════════════════════════════════════════════╗\n"
           "║                                                              ║\n"
           "║    ███████╗██╗     ██╗      █████╗ ██╗  ██╗██╗  ██╗        ║\n"
           "║    ██╔════╝██║     ██║     ██╔══██╗██║ ██╔╝██║ ██╔╝        ║\n"
           "║    ███████╗██║     ██║     ███████║█████╔╝ █████╔╝         ║\n"
           "║    ╚════██║██║     ██║     ██╔══██║██╔═██╗ ██╔═██╗         ║\n"
           "║    ███████║███████╗███████╗██║  ██║██║  ██╗██║  ██╗        ║\n"
           "║    ╚══════╝╚══════╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝        ║\n"
           "║                                                              ║\n"
           "║              FLOCK SAFETY DETECTOR - ENHANCED                ║\n"
           "║                        SQUAWK v2.0                          ║\n"
           "║                                                              ║\n"
           "║    🦅 Multi-Method Detection System 🦅                      ║\n"
           "║    📡 WiFi + BLE + MAC + Device Name Detection              ║\n"
           "║    🔊 Audio Alerts with Distinct Sound Patterns             ║\n"
           "║    🎯 Real-World Device Database Integration                 ║\n"
           "║    📊 JSON Detection Output with Full Device Info           ║\n"
           "║                                                              ║\n"
           "╚══════════════════════════════════════════════════════════════╝\n\n");
}

// ============================================================================
// AUDIO SYSTEM
// ============================================================================

void beep(int frequency, int duration_ms)
{
    tone(BUZZER_PIN, frequency, duration_ms);
    delay(duration_ms + 50);
}

void boot_beep_sequence()
{
    printf("Initializing audio system...\n");
    printf("Playing boot sequence: Low -> Medium pitch\n");
    beep(LOW_FREQ, BEEP_DURATION);
    beep(MED_FREQ, BEEP_DURATION);
    printf("Audio system ready\n\n");
}

void flock_detected_beep_sequence()
{
    printf("FLOCK SAFETY DEVICE DETECTED!\n");
    printf("Playing alert sequence: 3 high-pitch beeps\n");
    for (int i = 0; i < 3; i++) {
        beep(HIGH_FREQ, BEEP_DURATION);
    }
    printf("Detection complete - device identified!\n\n");
}

// ============================================================================
// JSON OUTPUT FUNCTIONS
// ============================================================================

void output_wifi_detection_json(const char* ssid, const uint8_t* mac, int rssi, const char* detection_type)
{
    DynamicJsonDocument doc(1024);
    
    doc["timestamp"] = millis();
    doc["type"] = "wifi";
    doc["detection_method"] = detection_type;
    doc["ssid"] = ssid;
    doc["rssi"] = rssi;
    
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    doc["mac"] = mac_str;
    
    // Add matching pattern info
    for (int i = 0; i < sizeof(wifi_ssid_patterns)/sizeof(wifi_ssid_patterns[0]); i++) {
        if (strcasestr(ssid, wifi_ssid_patterns[i])) {
            doc["matched_pattern"] = wifi_ssid_patterns[i];
            break;
        }
    }
    
    String json_output;
    serializeJson(doc, json_output);
    Serial.println(json_output);
}

void output_ble_detection_json(const char* mac, const char* name, int rssi, const char* detection_method)
{
    DynamicJsonDocument doc(1024);
    
    doc["timestamp"] = millis();
    doc["type"] = "ble";
    doc["detection_method"] = detection_method;
    doc["mac"] = mac;
    doc["rssi"] = rssi;
    
    if (name && strlen(name) > 0) {
        doc["device_name"] = name;
    }
    
    // Add matching pattern info
    if (strcmp(detection_method, "mac_prefix") == 0) {
        for (int i = 0; i < sizeof(mac_prefixes)/sizeof(mac_prefixes[0]); i++) {
            if (strncasecmp(mac, mac_prefixes[i], strlen(mac_prefixes[i])) == 0) {
                doc["matched_pattern"] = mac_prefixes[i];
                break;
            }
        }
    } else if (strcmp(detection_method, "device_name") == 0) {
        for (int i = 0; i < sizeof(device_name_patterns)/sizeof(device_name_patterns[0]); i++) {
            if (name && strcasestr(name, device_name_patterns[i])) {
                doc["matched_pattern"] = device_name_patterns[i];
                break;
            }
        }
    }
    
    String json_output;
    serializeJson(doc, json_output);
    Serial.println(json_output);
}

// ============================================================================
// DETECTION HELPER FUNCTIONS
// ============================================================================

bool check_mac_prefix(const uint8_t* mac)
{
    char mac_str[9];  // Only need first 3 octets for prefix check
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x", mac[0], mac[1], mac[2]);
    
    for (int i = 0; i < sizeof(mac_prefixes)/sizeof(mac_prefixes[0]); i++) {
        if (strncasecmp(mac_str, mac_prefixes[i], 8) == 0) {
            return true;
        }
    }
    return false;
}

bool check_ssid_pattern(const char* ssid)
{
    if (!ssid) return false;
    
    for (int i = 0; i < sizeof(wifi_ssid_patterns)/sizeof(wifi_ssid_patterns[0]); i++) {
        if (strcasestr(ssid, wifi_ssid_patterns[i])) {
            return true;
        }
    }
    return false;
}

bool check_device_name_pattern(const char* name)
{
    if (!name) return false;
    
    for (int i = 0; i < sizeof(device_name_patterns)/sizeof(device_name_patterns[0]); i++) {
        if (strcasestr(name, device_name_patterns[i])) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// WIFI PROMISCUOUS MODE HANDLER
// ============================================================================

typedef struct {
    unsigned frame_ctrl:16;
    unsigned duration_id:16;
    uint8_t addr1[6]; /* receiver address */
    uint8_t addr2[6]; /* sender address */
    uint8_t addr3[6]; /* filtering address */
    unsigned sequence_ctrl:16;
    uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
    if (triggered) return;
    
    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
    const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
    
    // Check for probe requests (subtype 0x04) and beacons (subtype 0x08)
    uint8_t frame_type = (hdr->frame_ctrl & 0xFF) >> 2;
    if (frame_type != 0x20 && frame_type != 0x80) { // Probe request or beacon
        return;
    }
    
    // Extract SSID from probe request or beacon
    char ssid[33] = {0};
    uint8_t *payload = (uint8_t *)ipkt + 24; // Skip MAC header
    
    if (frame_type == 0x20) { // Probe request
        payload += 0; // Probe requests start with SSID immediately
    } else { // Beacon frame
        payload += 12; // Skip fixed parameters in beacon
    }
    
    // Parse SSID element (tag 0, length, data)
    if (payload[0] == 0 && payload[1] <= 32) {
        memcpy(ssid, &payload[2], payload[1]);
        ssid[payload[1]] = '\0';
    }
    
    // Check if SSID matches our patterns
    if (strlen(ssid) > 0 && check_ssid_pattern(ssid)) {
        const char* detection_type = (frame_type == 0x20) ? "probe_request" : "beacon";
        output_wifi_detection_json(ssid, hdr->addr2, ppkt->rx_ctrl.rssi, detection_type);
        
        triggered = true;
        flock_detected_beep_sequence();
        return;
    }
    
    // Check MAC address
    if (check_mac_prefix(hdr->addr2)) {
        const char* detection_type = (frame_type == 0x20) ? "probe_request_mac" : "beacon_mac";
        output_wifi_detection_json(ssid[0] ? ssid : "hidden", hdr->addr2, ppkt->rx_ctrl.rssi, detection_type);
        
        triggered = true;
        flock_detected_beep_sequence();
        return;
    }
}

// ============================================================================
// BLE SCANNING
// ============================================================================

class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        if (triggered) return;
        
        NimBLEAddress addr = advertisedDevice->getAddress();
        std::string addrStr = addr.toString();
        uint8_t mac[6];
        sscanf(addrStr.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x", 
               &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        
        int rssi = advertisedDevice->getRSSI();
        std::string name = "";
        if (advertisedDevice->haveName()) {
            name = advertisedDevice->getName();
        }
        
        // Check MAC prefix
        if (check_mac_prefix(mac)) {
            output_ble_detection_json(addrStr.c_str(), name.c_str(), rssi, "mac_prefix");
            triggered = true;
            flock_detected_beep_sequence();
            return;
        }
        
        // Check device name
        if (!name.empty() && check_device_name_pattern(name.c_str())) {
            output_ble_detection_json(addrStr.c_str(), name.c_str(), rssi, "device_name");
            triggered = true;
            flock_detected_beep_sequence();
            return;
        }
    }
};

// ============================================================================
// CHANNEL HOPPING
// ============================================================================

void hop_channel()
{
    unsigned long now = millis();
    if (now - last_channel_hop > CHANNEL_HOP_INTERVAL) {
        current_channel++;
        if (current_channel > MAX_CHANNEL) {
            current_channel = 1;
        }
        esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
        last_channel_hop = now;
        printf("Hopped to channel %d\n", current_channel);
    }
}

// ============================================================================
// MAIN FUNCTIONS
// ============================================================================

void setup()
{
    Serial.begin(115200);
    delay(1000);
    
    display_banner();
    
    // Initialize buzzer
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    boot_beep_sequence();
    
    printf("Starting Flock Squawk Enhanced Detection System...\n\n");
    
    // Initialize WiFi in promiscuous mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
    esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
    
    printf("WiFi promiscuous mode enabled on channel %d\n", current_channel);
    printf("Monitoring probe requests and beacons...\n");
    
    // Initialize BLE
    printf("Initializing BLE scanner...\n");
    NimBLEDevice::init("");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    
    printf("BLE scanner initialized\n");
    printf("System ready - hunting for Flock Safety devices...\n\n");
    
    last_channel_hop = millis();
}

void loop()
{
    // Handle channel hopping for WiFi promiscuous mode
    hop_channel();
    
    // Run BLE scan
    if (!triggered) {
        pBLEScan->start(1, false); // Scan for 1 second, don't clear results
        pBLEScan->clearResults();
    }
    
    delay(100);
}