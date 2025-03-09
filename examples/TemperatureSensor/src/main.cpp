#include <Arduino.h>
#include <BTstackLib.h>
#include <hardware/adc.h>
#include "BLENotify.h"

// Temperature service UUID (using standard Environmental Sensing service)
const char* TEMP_SERVICE_UUID = "181A";  // Environmental Sensing service
const char* TEMP_CHAR_UUID = "2A6E";     // Temperature characteristic

static uint16_t temp_char_handle = 0;
static float last_temp = 0.0f;
static uint32_t last_notify_time = 0;
const uint32_t NOTIFY_INTERVAL = 1000;  // Notify every 1 second

// Read temperature from Pico-W internal temperature sensor
float readTemperature() {
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);  // Temperature sensor is on input 4
    
    uint16_t raw = adc_read();
    float voltage = (raw * 3.3f) / 4096.0f;
    float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;
    
    return temperature;
}

// Callback for BLE device connections
void deviceConnectedCallback(BLEStatus status, BLEDevice *device) {
    switch (status) {
        case BLE_STATUS_OK:
            Serial.println("Device connected!");
            break;
        default:
            break;
    }
}

// Callback for BLE device disconnections
void deviceDisconnectedCallback(BLEDevice *device) {
    Serial.println("Device disconnected!");
}

// Callback for characteristic reads
uint16_t gattReadCallback(uint16_t value_handle, uint8_t* buffer, uint16_t buffer_size) {
    if (value_handle == temp_char_handle) {
        if (buffer) {
            float temp = readTemperature();
            // Convert temperature to 16-bit integer (multiply by 100 to preserve 2 decimal places)
            int16_t temp_int = (int16_t)(temp * 100);
            memcpy(buffer, &temp_int, sizeof(int16_t));
            Serial.printf("Temperature read: %.2f°C\n", temp);
        }
        return sizeof(int16_t);
    }
    return 0;
}

// Callback for characteristic writes (including CCC descriptor writes)
int gattWriteCallback(uint16_t value_handle, uint8_t *buffer, uint16_t buffer_size) {
    // Check if this is a write to the CCC descriptor (notifications enable/disable)
    if (buffer_size == 2) {
        uint16_t value = (buffer[1] << 8) | buffer[0];
        if (value == 0x0001) {
            // Find the corresponding characteristic handle from the descriptor handle
            uint16_t char_handle = value_handle - 1; // Usually CCC is right after the characteristic
            BLENotify.handleSubscriptionChange(char_handle, true);
            Serial.println("Notifications enabled by client");
        } else if (value == 0x0000) {
            uint16_t char_handle = value_handle - 1;
            BLENotify.handleSubscriptionChange(char_handle, false);
            Serial.println("Notifications disabled by client");
        }
    }
    return 0;
}

void setup() {
    Serial.begin(115200);
    delay(2000); // Give time for serial connection to establish
    
    Serial.println("Pico-W BLE Temperature Sensor with BLENotify Library");
    
    // Initialize the BLENotify library
    BLENotify.begin();
    
    // Set up BLE callbacks
    BTstack.setBLEDeviceConnectedCallback(deviceConnectedCallback);
    BTstack.setBLEDeviceDisconnectedCallback(deviceDisconnectedCallback);
    BTstack.setGATTCharacteristicRead(gattReadCallback);
    BTstack.setGATTCharacteristicWrite(gattWriteCallback);
    
    // Add Environmental Sensing service
    BTstack.addGATTService(new UUID(TEMP_SERVICE_UUID));
    
    // Add temperature characteristic with notification support
    temp_char_handle = BLENotify.addNotifyCharacteristic(
        new UUID(TEMP_CHAR_UUID),
        ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY
    );
    
    // Start BLE
    BTstack.setup("Pico-W Temp");
    BTstack.startAdvertising();
    
    Serial.println("BLE Temperature Sensor started");
    Serial.printf("Temperature characteristic handle: 0x%04X\n", temp_char_handle);
}

void loop() {
    BTstack.loop();
    BLENotify.update();  // Process any pending notifications
    
    // Read and update temperature periodically
    uint32_t current_time = millis();
    if (current_time - last_notify_time >= NOTIFY_INTERVAL) {
        float temp = readTemperature();
        
        // Only notify if temperature has changed and client is subscribed
        if (abs(temp - last_temp) >= 0.1f) {  // 0.1°C threshold
            if (BLENotify.isSubscribed(temp_char_handle)) {
                // Convert temperature to 16-bit integer (multiply by 100 to preserve 2 decimal places)
                int16_t temp_int = (int16_t)(temp * 100);
                
                // Send notification
                if (BLENotify.notify(temp_char_handle, &temp_int, sizeof(temp_int))) {
                    Serial.printf("Notification sent: %.2f°C\n", temp);
                    last_temp = temp;
                }
            }
        }
        
        last_notify_time = current_time;
    }
}