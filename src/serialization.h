// Arduino-side implementation (C++)
// Reference code for serializing and sending BleDetection and WifiDetection protobufs
// using nanopb with callback-based string encoding
//
// This implementation uses nanopb for lightweight protobuf encoding on ESP32

#ifndef SERIAL_PROTOBUF_H
#define SERIAL_PROTOBUF_H

#include <Arduino.h>
#include "pb_encode.h"  // nanopb library
#include "messages.pb.h" // Generated from messages.proto using nanopb

class SerialProtobuf {
private:
    uint32_t frameId;

    // Callback function for encoding strings
    // This is called by nanopb for each string field
    static bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
        const char *str = (const char *)(*arg);

        if (str == NULL || str[0] == '\0') {
            return true; // Empty string, nothing to encode
        }

        if (!pb_encode_tag_for_field(stream, field)) {
            return false;
        }

        return pb_encode_string(stream, (const uint8_t *)str, strlen(str));
    }

    // Callback function for encoding bytes (payload_data)
    static bool encode_bytes(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {
        const pb_bytes_array_t *bytes = (const pb_bytes_array_t *)(*arg);

        if (bytes == NULL || bytes->size == 0) {
            return true; // Empty bytes, nothing to encode
        }

        if (!pb_encode_tag_for_field(stream, field)) {
            return false;
        }

        return pb_encode_string(stream, bytes->bytes, bytes->size);
    }

    // Helper function to write varint-encoded length prefix
    void writeVarint(uint32_t value) {
        while (value >= 0x80) {
            Serial.write((uint8_t)(value | 0x80));
            value >>= 7;
        }
        Serial.write((uint8_t)value);
    }

    // Helper: Set up string field callback
    static void setStringField(pb_callback_t *field, const char *str) {
        field->funcs.encode = &encode_string;
        field->arg = (void *)str;
    }

    // Wrap payload in PayloadEncapsulation and send
    bool sendEncapsulatedPayload(Protocol payloadType, const uint8_t* payloadData, size_t payloadSize) {
        // Create a temporary bytes array for the payload
        struct {
            pb_size_t size;
            uint8_t bytes[512];
        } payload_bytes;

        payload_bytes.size = payloadSize;
        memcpy(payload_bytes.bytes, payloadData, payloadSize);

        // Create PayloadEncapsulation
        PayloadEncapsulation envelope = PayloadEncapsulation_init_zero;
        envelope.frame_id = frameId++;
        envelope.payload_type = payloadType;
        envelope.payload_data.funcs.encode = &encode_bytes;
        envelope.payload_data.arg = &payload_bytes;

        // Encode the envelope
        uint8_t buffer[600]; // Slightly larger than max payload
        pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

        if (!pb_encode(&stream, PayloadEncapsulation_fields, &envelope)) {
            Serial.printf("ERROR: Failed to encode PayloadEncapsulation: %s\n", PB_GET_ERROR(&stream));
            return false;
        }

        // Write varint length prefix
        writeVarint(stream.bytes_written);

        // Write the message
        Serial.write(buffer, stream.bytes_written);

        return true;
    }

public:
    SerialProtobuf() : frameId(0) {}

    void begin(unsigned long baudRate = 2000000) {
        Serial.begin(baudRate);
        while (!Serial) {
            ; // Wait for serial port to connect (mainly for native USB)
        }
    }

    // Send a BLE detection message
    bool sendBleDetection(BleDetection* detection) {
        // Encode the BleDetection payload
        uint8_t payloadBuffer[512];
        pb_ostream_t payloadStream = pb_ostream_from_buffer(payloadBuffer, sizeof(payloadBuffer));

        if (!pb_encode(&payloadStream, BleDetection_fields, detection)) {
            Serial.printf("ERROR: Failed to encode BleDetection: %s\n", PB_GET_ERROR(&payloadStream));
            return false;
        }

        // Wrap in PayloadEncapsulation and send
        return sendEncapsulatedPayload(Protocol_BLUETOOTH_LE,
                                       payloadBuffer,
                                       payloadStream.bytes_written);
    }

    // Send a WiFi detection message
    bool sendWifiDetection(WifiDetection* detection) {
        // Encode the WifiDetection payload
        uint8_t payloadBuffer[512];
        pb_ostream_t payloadStream = pb_ostream_from_buffer(payloadBuffer, sizeof(payloadBuffer));

        if (!pb_encode(&payloadStream, WifiDetection_fields, detection)) {
            Serial.printf("ERROR: Failed to encode WifiDetection: %s\n", PB_GET_ERROR(&payloadStream));
            return false;
        }

        // Wrap in PayloadEncapsulation and send
        return sendEncapsulatedPayload(Protocol_WIFI,
                                       payloadBuffer,
                                       payloadStream.bytes_written);
    }

    // Helper: Initialize a BLE detection with callbacks set up
    static void initBleDetection(BleDetection* ble,
                                  const char* macAddress,
                                  int32_t rssi,
                                  uint64_t timestamp) {
        *ble = BleDetection_init_zero;

        // Set scalar fields
        ble->timestamp = timestamp;
        ble->rssi = rssi;
        ble->protocol = Protocol_BLUETOOTH_LE;
        ble->alert_level = HighMedLow_LOW;
        ble->signal_strength = SignalStrength_WEAK;
        ble->threat_score = 0;
        ble->has_device_name = false;

        // Set string callbacks
        setStringField(&ble->mac_address, macAddress);
    }

    // Helper: Initialize a WiFi detection with callbacks set up
    static void initWifiDetection(WifiDetection* wifi,
                                   const char* ssid,
                                   const char* macAddress,
                                   int32_t rssi,
                                   uint32_t channel,
                                   uint64_t timestamp) {
        *wifi = WifiDetection_init_zero;

        // Set scalar fields
        wifi->timestamp = timestamp;
        wifi->rssi = rssi;
        wifi->channel = channel;
        wifi->protocol = Protocol_WIFI;
        wifi->alert_level = HighMedLow_LOW;
        wifi->signal_strength = SignalStrength_WEAK;
        wifi->threat_score = 0;
        wifi->frame_type = FrameType_BEACON;

        // Set string callbacks
        setStringField(&wifi->ssid, ssid);
        wifi->ssid_length = strlen(ssid);
        setStringField(&wifi->mac_address, macAddress);
    }

    // Helper: Set a string field on BLE detection
    static void setBleString(BleDetection* ble, const char* fieldName, const char* value) {
        if (strcmp(fieldName, "detection_time") == 0) {
            setStringField(&ble->detection_time, value);
        } else if (strcmp(fieldName, "detection_method") == 0) {
            setStringField(&ble->detection_method, value);
        } else if (strcmp(fieldName, "device_name") == 0) {
            setStringField(&ble->device_name, value);
            ble->has_device_name = (value != NULL && value[0] != '\0');
            ble->device_name_length = ble->has_device_name ? strlen(value) : 0;
        } else if (strcmp(fieldName, "mac_prefix") == 0) {
            setStringField(&ble->mac_prefix, value);
        } else if (strcmp(fieldName, "vendor_oui") == 0) {
            setStringField(&ble->vendor_oui, value);
        } else if (strcmp(fieldName, "matched_mac_pattern") == 0) {
            setStringField(&ble->matched_mac_pattern, value);
        } else if (strcmp(fieldName, "matched_name_pattern") == 0) {
            setStringField(&ble->matched_name_pattern, value);
        } else if (strcmp(fieldName, "advertisement_description") == 0) {
            setStringField(&ble->advertisement_description, value);
        } else if (strcmp(fieldName, "detection_reason") == 0) {
            setStringField(&ble->detection_reason, value);
        }
    }

    // Helper: Set a string field on WiFi detection
    static void setWifiString(WifiDetection* wifi, const char* fieldName, const char* value) {
        if (strcmp(fieldName, "detection_time") == 0) {
            setStringField(&wifi->detection_time, value);
        } else if (strcmp(fieldName, "detection_method") == 0) {
            setStringField(&wifi->detection_method, value);
        } else if (strcmp(fieldName, "mac_prefix") == 0) {
            setStringField(&wifi->mac_prefix, value);
        } else if (strcmp(fieldName, "vendor_oui") == 0) {
            setStringField(&wifi->vendor_oui, value);
        } else if (strcmp(fieldName, "matched_ssid_pattern") == 0) {
            setStringField(&wifi->matched_ssid_pattern, value);
        } else if (strcmp(fieldName, "matched_mac_pattern") == 0) {
            setStringField(&wifi->matched_mac_pattern, value);
        } else if (strcmp(fieldName, "frame_description") == 0) {
            setStringField(&wifi->frame_description, value);
        }
    }
};

#endif // SERIAL_PROTOBUF_H

/* ============================================================================
   EXAMPLE USAGE
   ============================================================================

#include "SerialProtobuf.h"

SerialProtobuf serialProto;

void setup() {
    serialProto.begin(2000000); // 2M baud

    delay(2000); // Give serial time to stabilize
    Serial.println("Serial protobuf streaming started");
}

void loop() {
    // Example 1: Simple BLE detection
    {
        BleDetection ble;
        SerialProtobuf::initBleDetection(&ble,
                                          "AA:BB:CC:DD:EE:FF",
                                          -67,
                                          millis());

        // Add optional string fields using the helper
        SerialProtobuf::setBleString(&ble, "device_name", "MyDevice");
        SerialProtobuf::setBleString(&ble, "vendor_oui", "Apple");
        SerialProtobuf::setBleString(&ble, "detection_reason", "MAC pattern match");

        // Set other optional fields
        ble.alert_level = HighMedLow_MED;
        ble.threat_score = 50;
        ble.advertisement_type = AdvertisementType_BLE_ADVERTISEMENT;

        serialProto.sendBleDetection(&ble);
    }

    delay(100);

    // Example 2: Simple WiFi detection
    {
        WifiDetection wifi;
        SerialProtobuf::initWifiDetection(&wifi,
                                           "TestNetwork",
                                           "11:22:33:44:55:66",
                                           -45,
                                           6,
                                           millis());

        // Add optional string fields
        SerialProtobuf::setWifiString(&wifi, "vendor_oui", "Apple");
        SerialProtobuf::setWifiString(&wifi, "detection_method", "SSID pattern");
        SerialProtobuf::setWifiString(&wifi, "detection_reason", "Known threat network");

        // Set other optional fields
        wifi.alert_level = HighMedLow_HIGH;
        wifi.threat_score = 75;

        serialProto.sendWifiDetection(&wifi);
    }

    delay(100);

    // Example 3: Full BLE detection with all fields
    {
        BleDetection ble;
        SerialProtobuf::initBleDetection(&ble,
                                          "DE:AD:BE:EF:CA:FE",
                                          -72,
                                          millis());

        // Set all scalar/enum fields
        ble.threat_score = 85;
        ble.advertisement_type = AdvertisementType_BLE_ADVERTISEMENT;
        ble.alert_level = HighMedLow_HIGH;
        ble.mac_match_confidence = HighMedLow_HIGH;
        ble.name_match_confidence = HighMedLow_MED;
        ble.detection_criteria = DetectionCriteria_MAC_ONLY;
        ble.device_category = DeviceCategory_FLOCK_SAFETY;
        ble.primary_indicator = PrimaryIndicator_MAC_ADDRESS;
        ble.signal_strength = SignalStrength_STRONG;

        // Set all string fields
        SerialProtobuf::setBleString(&ble, "detection_time", "2024-02-01T12:34:56Z");
        SerialProtobuf::setBleString(&ble, "detection_method", "MAC Pattern Match");
        SerialProtobuf::setBleString(&ble, "device_name", "SuspiciousDevice");
        SerialProtobuf::setBleString(&ble, "mac_prefix", "DE:AD:BE");
        SerialProtobuf::setBleString(&ble, "vendor_oui", "Unknown");
        SerialProtobuf::setBleString(&ble, "matched_mac_pattern", "DE:AD:BE:*");
        SerialProtobuf::setBleString(&ble, "matched_name_pattern", "");
        SerialProtobuf::setBleString(&ble, "advertisement_description", "Standard BLE advertisement");
        SerialProtobuf::setBleString(&ble, "detection_reason", "MAC prefix matches known threat pattern");

        serialProto.sendBleDetection(&ble);
    }

    delay(1000);
}

   ============================================================================
   IMPORTANT NOTES ON NANOPB CALLBACK STRINGS
   ============================================================================

1. String Lifetime:
   The strings you pass to setStringField() must remain valid until pb_encode()
   completes. This means:

   GOOD:
   const char* myString = "Hello";  // String literal (always valid)
   setBleString(&ble, "device_name", myString);
   sendBleDetection(&ble);

   GOOD:
   char buffer[64];
   snprintf(buffer, sizeof(buffer), "Device-%d", deviceId);
   setBleString(&ble, "device_name", buffer);
   sendBleDetection(&ble);  // buffer still in scope

   BAD:
   {
       char buffer[64] = "TempString";
       setBleString(&ble, "device_name", buffer);
   }  // buffer goes out of scope
   sendBleDetection(&ble);  // CRASH: buffer is invalid!

2. Memory Efficiency:
   Callback strings are more memory-efficient than fixed buffers because:
   - No memory is allocated until encoding
   - String data isn't copied multiple times
   - Perfect for ESP32 with limited RAM

3. Performance:
   Callback encoding is slightly slower than fixed buffers but:
   - Difference is negligible at 2Mbps serial
   - Memory savings far outweigh minor speed difference
   - Typical encode time: ~50-100μs per message

4. Debugging Callback Errors:
   If encoding fails with callback errors:
   - Check that all strings are NULL-terminated
   - Verify strings are in valid memory when encode() is called
   - Ensure total message size < 512 bytes

   ============================================================================
*/
