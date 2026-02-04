#include "main.h"

// ============================================================================
// PROTOBUF OUTPUT FUNCTIONS
// ============================================================================

// void output_wifi_detection_pb(const char* ssid, const uint8_t* mac, int rssi, const char* detection_type)
// {
//     WifiDetection doc = WifiDetection_init_zero;
//
//     // Core detection info
//     doc.timestamp = millis();
//     doc.detection_time = String(millis() / 1000.0, 3) + "s";
//     doc.protocol = Protocol_WIFI;
//     doc.detection_method = detection_type;
//     doc.alert_level = HighMedLow_HIGH;
//     doc.device_category = DeviceCategory_FLOCK_SAFETY;
//
//     // WiFi specific info
//     doc.ssid = ssid;
//     doc.ssid_length = strlen(ssid);
//     doc.rssi = rssi;
//     doc.signal_strength = rssi > -50 ? SignalStrength_STRONG : (rssi > -70 ? SignalStrength_MEDIUM : SignalStrength_WEAK);
//     doc.channel = current_channel;
//
//     // MAC address info
//     char mac_str[18];
//     snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
//              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//     doc.mac_address = mac_str;
//
//     char mac_prefix[9];
//     snprintf(mac_prefix, sizeof(mac_prefix), "%02x:%02x:%02x", mac[0], mac[1], mac[2]);
//     doc.mac_prefix = mac_prefix;
//     doc.vendor_oui = mac_prefix;
//
//     // Detection pattern matching
//     bool ssid_match = false;
//     bool mac_match = false;
//
//     for (int i = 0; i < sizeof(wifi_ssid_patterns)/sizeof(wifi_ssid_patterns[0]); i++) {
//         if (strcasestr(ssid, wifi_ssid_patterns[i])) {
//             doc.matched_ssid_pattern = wifi_ssid_patterns[i];
//             doc.ssid_match_confidence = HighMedLow_HIGH;
//             ssid_match = true;
//             break;
//         }
//     }
//
//     for (int i = 0; i < sizeof(mac_prefixes)/sizeof(mac_prefixes[0]); i++) {
//         if (strncasecmp(mac_prefix, mac_prefixes[i], 8) == 0) {
//             doc.matched_mac_pattern = mac_prefixes[i];
//             doc.mac_match_confidence = HighMedLow_HIGH;
//             mac_match = true;
//             break;
//         }
//     }
//
//     // Detection summary
//     doc.detection_criteria = ssid_match && mac_match ? DetectionCriteria_SSID_AND_MAC : (ssid_match ? DetectionCriteria_SSID_ONLY : DetectionCriteria_MAC_ONLY);
//     doc.threat_score = ssid_match && mac_match ? 100 : (ssid_match || mac_match ? 85 : 70);
//
//     // Frame type details
//     if (strcmp(detection_type, "probe_request") == 0 || strcmp(detection_type, "probe_request_mac") == 0) {
//         doc.frame_type = FrameType_PROBE_REQUEST;
//         doc.frame_description = "Device actively scanning for networks";
//     } else {
//         doc.frame_type = FrameType_BEACON;
//         doc.frame_description.funcs.encode = &pb_encode_string;
//         doc.frame_description = "Device advertising its network";
//     }
//
//
//
//     String json_output;
//     serializeJson(doc, json_output);
//     Serial.println(json_output);
// }

// void output_ble_detection_pb(const char* mac, const char* name, int rssi, const char* detection_method)
// {
//     BleDetection doc = BleDetection_init_zero;
//
//     // Core detection info
//     doc.timestamp = millis();
//     doc.detection_time = String(millis() / 1000.0, 3) + "s";
//     doc.protocol = Protocol_BLUETOOTH_LE;
//     doc.detection_method = detection_method;
//     doc.alert_level = HighMedLow_HIGH;
//     doc.device_category = DeviceCategory_FLOCK_SAFETY;
//
//     // BLE specific info
//     doc.mac_address = mac;
//     doc.rssi = rssi;
//     doc.signal_strength = rssi > -50 ? SignalStrength_STRONG : (rssi > -70 ? SignalStrength_MEDIUM : SignalStrength_WEAK);
//
//     // Device name info
//     if (name && strlen(name) > 0) {
//         doc.device_name = name;
//         doc.device_name_length = strlen(name);
//         doc.has_device_name = true;
//     } else {
//         doc.device_name = "";
//         doc.device_name_length = 0;
//         doc.has_device_name = false;
//     }
//
//     // MAC address analysis
//     char mac_prefix[9];
//     strncpy(mac_prefix, mac, 8);
//     mac_prefix[8] = '\0';
//     doc.mac_prefix = mac_prefix;
//     doc.vendor_oui = mac_prefix;
//
//     // Detection pattern matching
//     bool name_match = false;
//     bool mac_match = false;
//
//     // Check MAC prefix patterns
//     for (int i = 0; i < sizeof(mac_prefixes)/sizeof(mac_prefixes[0]); i++) {
//         if (strncasecmp(mac, mac_prefixes[i], strlen(mac_prefixes[i])) == 0) {
//             doc.matched_mac_pattern = mac_prefixes[i];
//             doc.mac_match_confidence = HighMedLow_HIGH;
//             mac_match = true;
//             break;
//         }
//     }
//
//     // Check device name patterns
//     if (name && strlen(name) > 0) {
//         for (int i = 0; i < sizeof(device_name_patterns)/sizeof(device_name_patterns[0]); i++) {
//             if (strcasestr(name, device_name_patterns[i])) {
//                 doc.matched_name_pattern = device_name_patterns[i];
//                 doc.name_match_confidence = HighMedLow_HIGH;
//                 name_match = true;
//                 break;
//             }
//         }
//     }
//
//     // Detection summary
//     doc.detection_criteria = name_match && mac_match ? "NAME_AND_MAC" :
//                                (name_match ? "NAME_ONLY" : "MAC_ONLY");
//     doc.threat_score = name_match && mac_match ? 100 :
//                          (name_match || mac_match ? 85 : 70);
//
//     // BLE advertisement type analysis
//     doc.advertisement_type = AdvertisementType_BLE_ADVERTISEMENT;
//     doc.advertisement_description = "Bluetooth Low Energy device advertisement";
//
//     // Detection method details
//     if (strcmp(detection_method, "mac_prefix") == 0) {
//         doc.primary_indicator = PrimaryIndicator_MAC_ADDRESS;
//         doc.detection_reason = "MAC address matches known Flock Safety prefix";
//     } else if (strcmp(detection_method, "device_name") == 0) {
//         doc.primary_indicator = PrimaryIndicator_DEVICE_NAME;
//         doc.detection_reason = "Device name matches Flock Safety pattern";
//     }
//
//     String json_output;
//     serializeJson(doc, json_output);
//     Serial.println(json_output);
// }

// ============================================================================
// JSON OUTPUT FUNCTIONS
// ============================================================================

void output_wifi_detection_json(const char* ssid, const uint8_t* mac, int rssi, const char* detection_type)
{
    DynamicJsonDocument doc(2048);

    // Core detection info
    doc["timestamp"] = millis();
    doc["detection_time"] = String(millis() / 1000.0, 3) + "s";
    doc["protocol"] = "wifi";
    doc["detection_method"] = detection_type;
    doc["alert_level"] = "HIGH";
    doc["device_category"] = "FLOCK_SAFETY";

    // WiFi specific info
    doc["ssid"] = ssid;
    doc["ssid_length"] = strlen(ssid);
    doc["rssi"] = rssi;
    doc["signal_strength"] = rssi > -50 ? "STRONG" : (rssi > -70 ? "MEDIUM" : "WEAK");
    doc["channel"] = current_channel;

    // MAC address info
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    doc["mac_address"] = mac_str;

    char mac_prefix[9];
    snprintf(mac_prefix, sizeof(mac_prefix), "%02x:%02x:%02x", mac[0], mac[1], mac[2]);
    doc["mac_prefix"] = mac_prefix;
    doc["vendor_oui"] = mac_prefix;

    // Detection pattern matching
    bool ssid_match = false;
    bool mac_match = false;

    for (int i = 0; i < sizeof(wifi_ssid_patterns)/sizeof(wifi_ssid_patterns[0]); i++) {
        if (strcasestr(ssid, wifi_ssid_patterns[i])) {
            doc["matched_ssid_pattern"] = wifi_ssid_patterns[i];
            doc["ssid_match_confidence"] = "HIGH";
            ssid_match = true;
            break;
        }
    }

    for (int i = 0; i < sizeof(mac_prefixes)/sizeof(mac_prefixes[0]); i++) {
        if (strncasecmp(mac_prefix, mac_prefixes[i], 8) == 0) {
            doc["matched_mac_pattern"] = mac_prefixes[i];
            doc["mac_match_confidence"] = "HIGH";
            mac_match = true;
            break;
        }
    }

    // Detection summary
    doc["detection_criteria"] = ssid_match && mac_match ? "SSID_AND_MAC" : (ssid_match ? "SSID_ONLY" : "MAC_ONLY");
    doc["threat_score"] = ssid_match && mac_match ? 100 : (ssid_match || mac_match ? 85 : 70);

    // Frame type details
    if (strcmp(detection_type, "probe_request") == 0 || strcmp(detection_type, "probe_request_mac") == 0) {
        doc["frame_type"] = "PROBE_REQUEST";
        doc["frame_description"] = "Device actively scanning for networks";
    } else {
        doc["frame_type"] = "BEACON";
        doc["frame_description"] = "Device advertising its network";
    }

    String json_output;
    serializeJson(doc, json_output);
    Serial.println(json_output);
}

void output_ble_detection_json(const char* mac, const char* name, int rssi, const char* detection_method)
{
    DynamicJsonDocument doc(2048);

    // Core detection info
    doc["timestamp"] = millis();
    doc["detection_time"] = String(millis() / 1000.0, 3) + "s";
    doc["protocol"] = "bluetooth_le";
    doc["detection_method"] = detection_method;
    doc["alert_level"] = "HIGH";
    doc["device_category"] = "FLOCK_SAFETY";

    // BLE specific info
    doc["mac_address"] = mac;
    doc["rssi"] = rssi;
    doc["signal_strength"] = rssi > -50 ? "STRONG" : (rssi > -70 ? "MEDIUM" : "WEAK");

    // Device name info
    if (name && strlen(name) > 0) {
        doc["device_name"] = name;
        doc["device_name_length"] = strlen(name);
        doc["has_device_name"] = true;
    } else {
        doc["device_name"] = "";
        doc["device_name_length"] = 0;
        doc["has_device_name"] = false;
    }

    // MAC address analysis
    char mac_prefix[9];
    strncpy(mac_prefix, mac, 8);
    mac_prefix[8] = '\0';
    doc["mac_prefix"] = mac_prefix;
    doc["vendor_oui"] = mac_prefix;

    // Detection pattern matching
    bool name_match = false;
    bool mac_match = false;

    // Check MAC prefix patterns
    for (int i = 0; i < sizeof(mac_prefixes)/sizeof(mac_prefixes[0]); i++) {
        if (strncasecmp(mac, mac_prefixes[i], strlen(mac_prefixes[i])) == 0) {
            doc["matched_mac_pattern"] = mac_prefixes[i];
            doc["mac_match_confidence"] = "HIGH";
            mac_match = true;
            break;
        }
    }

    // Check device name patterns
    if (name && strlen(name) > 0) {
        for (int i = 0; i < sizeof(device_name_patterns)/sizeof(device_name_patterns[0]); i++) {
            if (strcasestr(name, device_name_patterns[i])) {
                doc["matched_name_pattern"] = device_name_patterns[i];
                doc["name_match_confidence"] = "HIGH";
                name_match = true;
                break;
            }
        }
    }

    // Detection summary
    doc["detection_criteria"] = name_match && mac_match ? "NAME_AND_MAC" :
                               (name_match ? "NAME_ONLY" : "MAC_ONLY");
    doc["threat_score"] = name_match && mac_match ? 100 :
                         (name_match || mac_match ? 85 : 70);

    // BLE advertisement type analysis
    doc["advertisement_type"] = "BLE_ADVERTISEMENT";
    doc["advertisement_description"] = "Bluetooth Low Energy device advertisement";

    // Detection method details
    if (strcmp(detection_method, "mac_prefix") == 0) {
        doc["primary_indicator"] = "MAC_ADDRESS";
        doc["detection_reason"] = "MAC address matches known Flock Safety prefix";
    } else if (strcmp(detection_method, "device_name") == 0) {
        doc["primary_indicator"] = "DEVICE_NAME";
        doc["detection_reason"] = "Device name matches Flock Safety pattern";
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
// RAVEN UUID DETECTION
// ============================================================================

// Check if a BLE device advertises any Raven surveillance service UUIDs
bool check_raven_service_uuid(NimBLEAdvertisedDevice* device, char* detected_service_out = nullptr)
{
    if (!device) return false;

    // Check if device has service UUIDs
    if (!device->haveServiceUUID()) return false;

    // Get the number of service UUIDs
    int serviceCount = device->getServiceUUIDCount();
    if (serviceCount == 0) return false;

    // Check each advertised service UUID against known Raven UUIDs
    for (int i = 0; i < serviceCount; i++) {
        NimBLEUUID serviceUUID = device->getServiceUUID(i);
        std::string uuidStr = serviceUUID.toString();

        // Compare against each known Raven service UUID
        for (int j = 0; j < sizeof(raven_service_uuids)/sizeof(raven_service_uuids[0]); j++) {
            if (strcasecmp(uuidStr.c_str(), raven_service_uuids[j]) == 0) {
                // Match found! Store the detected service UUID if requested
                if (detected_service_out != nullptr) {
                    strncpy(detected_service_out, uuidStr.c_str(), 40);
                }
                return true;
            }
        }
    }

    return false;
}

// Get a human-readable description of the Raven service
const char* get_raven_service_description(const char* uuid)
{
    if (!uuid) return "Unknown Service";

    if (strcasecmp(uuid, RAVEN_DEVICE_INFO_SERVICE) == 0)
        return "Device Information (Serial, Model, Firmware)";
    if (strcasecmp(uuid, RAVEN_GPS_SERVICE) == 0)
        return "GPS Location Service (Lat/Lon/Alt)";
    if (strcasecmp(uuid, RAVEN_POWER_SERVICE) == 0)
        return "Power Management (Battery/Solar)";
    if (strcasecmp(uuid, RAVEN_NETWORK_SERVICE) == 0)
        return "Network Status (LTE/WiFi)";
    if (strcasecmp(uuid, RAVEN_UPLOAD_SERVICE) == 0)
        return "Upload Statistics Service";
    if (strcasecmp(uuid, RAVEN_ERROR_SERVICE) == 0)
        return "Error/Failure Tracking Service";
    if (strcasecmp(uuid, RAVEN_OLD_HEALTH_SERVICE) == 0)
        return "Health/Temperature Service (Legacy)";
    if (strcasecmp(uuid, RAVEN_OLD_LOCATION_SERVICE) == 0)
        return "Location Service (Legacy)";

    return "Unknown Raven Service";
}

// Estimate firmware version based on detected service UUIDs
const char* estimate_raven_firmware_version(NimBLEAdvertisedDevice* device)
{
    if (!device || !device->haveServiceUUID()) return "Unknown";

    bool has_new_gps = false;
    bool has_old_location = false;
    bool has_power_service = false;

    int serviceCount = device->getServiceUUIDCount();
    for (int i = 0; i < serviceCount; i++) {
        NimBLEUUID serviceUUID = device->getServiceUUID(i);
        std::string uuidStr = serviceUUID.toString();

        if (strcasecmp(uuidStr.c_str(), RAVEN_GPS_SERVICE) == 0)
            has_new_gps = true;
        if (strcasecmp(uuidStr.c_str(), RAVEN_OLD_LOCATION_SERVICE) == 0)
            has_old_location = true;
        if (strcasecmp(uuidStr.c_str(), RAVEN_POWER_SERVICE) == 0)
            has_power_service = true;
    }

    // Firmware version heuristics based on service presence
    if (has_old_location && !has_new_gps)
        return "1.1.x (Legacy)";
    if (has_new_gps && !has_power_service)
        return "1.2.x";
    if (has_new_gps && has_power_service)
        return "1.3.x (Latest)";

    return "Unknown Version";
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

        if (!triggered) {
            triggered = true;
            flock_detected_beep_sequence(&set_detection);
        }
        // Always update detection time for heartbeat tracking
        last_detection_time = millis();
        return;
    }

    // Check MAC address
    if (check_mac_prefix(hdr->addr2)) {
        const char* detection_type = (frame_type == 0x20) ? "probe_request_mac" : "beacon_mac";
        output_wifi_detection_json(ssid[0] ? ssid : "hidden", hdr->addr2, ppkt->rx_ctrl.rssi, detection_type);

        if (!triggered) {
            triggered = true;
            flock_detected_beep_sequence(&set_detection);
        }
        // Always update detection time for heartbeat tracking
        last_detection_time = millis();
        return;
    }
}

// ============================================================================
// BLE SCANNING
// ============================================================================

class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {

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
            if (!triggered) {
                triggered = true;
                flock_detected_beep_sequence(&set_detection);
            }
            // Always update detection time for heartbeat tracking
            last_detection_time = millis();
            return;
        }

        // Check device name
        if (!name.empty() && check_device_name_pattern(name.c_str())) {
            output_ble_detection_json(addrStr.c_str(), name.c_str(), rssi, "device_name");
            if (!triggered) {
                triggered = true;
                flock_detected_beep_sequence(&set_detection);
            }
            // Always update detection time for heartbeat tracking
            last_detection_time = millis();
            return;
        }

        // Check for Raven surveillance device service UUIDs
        char detected_service_uuid[41] = {0};
        if (check_raven_service_uuid(advertisedDevice, detected_service_uuid)) {
            // Raven device detected! Get firmware version estimate
            const char* fw_version = estimate_raven_firmware_version(advertisedDevice);
            const char* service_desc = get_raven_service_description(detected_service_uuid);

            // Create enhanced JSON output with Raven-specific data
            StaticJsonDocument<1024> doc;
            doc["protocol"] = "bluetooth_le";
            doc["detection_method"] = "raven_service_uuid";
            doc["device_type"] = "RAVEN_GUNSHOT_DETECTOR";
            doc["manufacturer"] = "SoundThinking/ShotSpotter";
            doc["mac_address"] = addrStr.c_str();
            doc["rssi"] = rssi;
            doc["signal_strength"] = rssi > -50 ? "STRONG" : (rssi > -70 ? "MEDIUM" : "WEAK");

            if (!name.empty()) {
                doc["device_name"] = name.c_str();
            }

            // Raven-specific information
            doc["raven_service_uuid"] = detected_service_uuid;
            doc["raven_service_description"] = service_desc;
            doc["raven_firmware_version"] = fw_version;
            doc["threat_level"] = "CRITICAL";
            doc["threat_score"] = 100;

            // List all detected service UUIDs
            if (advertisedDevice->haveServiceUUID()) {
                JsonArray services = doc.createNestedArray("service_uuids");
                int serviceCount = advertisedDevice->getServiceUUIDCount();
                for (int i = 0; i < serviceCount; i++) {
                    NimBLEUUID serviceUUID = advertisedDevice->getServiceUUID(i);
                    services.add(serviceUUID.toString().c_str());
                }
            }

            // Output the detection
            serializeJson(doc, Serial);
            Serial.println();

            if (!triggered) {
                triggered = true;
                flock_detected_beep_sequence(&set_detection);
            }
            // Always update detection time for heartbeat tracking
            last_detection_time = millis();
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
         printf("[WiFi] Hopped to channel %d\n", current_channel);
    }
}

// ============================================================================
// MAIN FUNCTIONS
// ============================================================================

void setup()
{
    Serial.begin(115200);
    delay(1000);

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

    // Handle heartbeat pulse if device is in range
    if (device_in_range) {
        unsigned long now = millis();

        // Check if 10 seconds have passed since last heartbeat
        if (now - last_heartbeat >= 10000) {
            heartbeat_pulse();
            last_heartbeat = now;
        }

        // Check if device has gone out of range (no detection for 30 seconds)
        if (now - last_detection_time >= 30000) {
            printf("Device out of range - stopping heartbeat\n");
            device_in_range = false;
            triggered = false; // Allow new detections
        }
    }

    if (millis() - last_ble_scan >= BLE_SCAN_INTERVAL && !pBLEScan->isScanning()) {
        printf("[BLE] scan...\n");
        pBLEScan->start(BLE_SCAN_DURATION, false);
        last_ble_scan = millis();
    }

    if (pBLEScan->isScanning() == false && millis() - last_ble_scan > BLE_SCAN_DURATION * 1000) {
        pBLEScan->clearResults();
    }

    delay(100);
}

// Mark device as in range and start heartbeat tracking
void set_detection() {
    device_in_range = true;
    last_detection_time = millis();
    last_heartbeat = millis();
}