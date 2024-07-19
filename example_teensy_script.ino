#include <SPI.h>
#include <SD.h>
#include <FlexCAN_T4.h>
#include <ArduinoJson.h>

// Initialize CAN1 for Teensy 4.1
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1;
StaticJsonDocument<4096> doc;

void setup() {
    Serial.begin(115200);
    while (!Serial);

    // Initialize CAN1 at 500kbps
    can1.begin();
    can1.setBaudRate(500000);

    // Initialize the SD card
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("Initialization of SD card failed!");
        return;
    }

    // Open the JSON file from SD card
    File file = SD.open("can_database.json");
    if (!file) {
        Serial.println("Failed to open file!");
        return;
    }

    // Parse the JSON file
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        return;
    }

    file.close();
    Serial.println("CAN ready");
}

void loop() {
    CAN_message_t msg;
    if (can1.read(msg)) {
        decodeCANMessage(msg.id, msg.len, msg.buf);
    }
}

uint64_t extractBits(uint8_t* data, int startBit, int length) {
    uint64_t value = 0;
    for (int i = 0; i < length; ++i) {
        int byteIndex = (startBit + i) / 8;
        int bitIndex = (startBit + i) % 8;
        value |= ((data[byteIndex] >> bitIndex) & 0x01) << i;
    }
    return value;
}

float extractSignal(uint8_t* data, int startBit, int length, float factor, float offset) {
    uint64_t raw_value = extractBits(data, startBit, length);
    return raw_value * factor + offset;
}

void decodeBitFields(uint8_t value, JsonArray bitFields) {
    for (JsonObject bitField : bitFields) {
        if (bitField.containsKey("bit_range")) {
            // Extract and decode bit range
            const char* bit_range = bitField["bit_range"];
            int start_bit, end_bit;
            sscanf(bit_range, "%d-%d", &start_bit, &end_bit);
            int bit_length = end_bit - start_bit + 1;
            int bit_value = (value >> start_bit) & ((1 << bit_length) - 1);

            Serial.print(bitField["description"].as<String>());
            Serial.print(": ");
            Serial.println(bit_value);
        } else {
            // Extract and decode single bit
            int bit = bitField["bit"];
            const char* description = bitField["description"];
            bool bitValue = value & (1 << bit);

            Serial.print(description);
            Serial.print(": ");
            Serial.println(bitValue ? "1" : "0");
        }
    }
}

void decodeCANMessage(uint32_t id, int length, uint8_t* data) {
    for (JsonObject message : doc["messages"].as<JsonArray>()) {
        if (strtol(message["id"], NULL, 16) == id) {
            for (JsonObject signal : message["signals"].as<JsonArray>()) {
                int start_bit = signal["start_bit"];
                int length = signal["length"];
                float factor = signal["factor"];
                float offset = signal["offset"];

                float value = extractSignal(data, start_bit, length, factor, offset);
                Serial.print(signal["name"].as<String>());
                Serial.print(": ");
                Serial.print(value);
                Serial.print(" ");
                Serial.println(signal["unit"].as<String>());

                if (signal.containsKey("bit_fields")) {
                    uint8_t raw_value = static_cast<uint8_t>(extractBits(data, start_bit, length));
                    decodeBitFields(raw_value, signal["bit_fields"].as<JsonArray>());
                }
            }
        }
    }
}
