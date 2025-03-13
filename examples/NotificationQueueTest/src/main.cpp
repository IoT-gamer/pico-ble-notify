#include <Arduino.h>
#include <BTstackLib.h>
#include "BLENotify.h"

// Define a test service and characteristic
const char* TEST_SERVICE_UUID = "180F";  // Battery Service
const char* TEST_CHAR_UUID = "2A19";     // Battery Level Characteristic

static uint16_t test_char_handle = 0;
static int test_counter = 0;
static bool rapid_notification_test_active = false;
static uint32_t last_status_print = 0;
static const uint32_t STATUS_INTERVAL = 2000;  // Print status every 2 seconds

// Callback for BLE device connections
void deviceConnectedCallback(BLEStatus status, BLEDevice *device) {
    if (status == BLE_STATUS_OK) {
        Serial.println("Device connected!");
    }
}

// Callback for BLE device disconnections
void deviceDisconnectedCallback(BLEDevice *device) {
    Serial.println("Device disconnected!");
    rapid_notification_test_active = false;
}

// Callback for characteristic reads
uint16_t gattReadCallback(uint16_t value_handle, uint8_t* buffer, uint16_t buffer_size) {
    if (value_handle == test_char_handle) {
        if (buffer) {
            buffer[0] = test_counter;
            Serial.printf("Value read: %d\n", test_counter);
        }
        return 1;
    }
    return 0;
}

// Callback for characteristic writes (including CCC descriptor writes)
int gattWriteCallback(uint16_t value_handle, uint8_t *buffer, uint16_t buffer_size) {
    // Check if this is a write to the CCC descriptor
    if (buffer_size == 2) {
        uint16_t value = (buffer[1] << 8) | buffer[0];
        if (value == 0x0001) {
            uint16_t char_handle = value_handle - 1;
            BLENotify.handleSubscriptionChange(char_handle, true);
            Serial.println("Notifications enabled by client");
            
            // Start rapid notification test when notifications are enabled
            rapid_notification_test_active = true;
            test_counter = 0;
        } else if (value == 0x0000) {
            uint16_t char_handle = value_handle - 1;
            BLENotify.handleSubscriptionChange(char_handle, false);
            Serial.println("Notifications disabled by client");
            rapid_notification_test_active = false;
        }
    }
    return 0;
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("BLENotify Notification Queue Test");
    
    // Initialize the BLENotify library
    BLENotify.begin();
    
    // Set up BLE callbacks
    BTstack.setBLEDeviceConnectedCallback(deviceConnectedCallback);
    BTstack.setBLEDeviceDisconnectedCallback(deviceDisconnectedCallback);
    BTstack.setGATTCharacteristicRead(gattReadCallback);
    BTstack.setGATTCharacteristicWrite(gattWriteCallback);
    
    // Add Battery service and characteristic
    BTstack.addGATTService(new UUID(TEST_SERVICE_UUID));
    test_char_handle = BLENotify.addNotifyCharacteristic(
        new UUID(TEST_CHAR_UUID),
        ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY
    );
    
    // Start BLE
    BTstack.setup("NotifyQueueTest");
    BTstack.startAdvertising();
    
    Serial.println("Test device started");
}

void loop() {
    BTstack.loop();
    BLENotify.update();
    
    // Rapid notification test
    if (rapid_notification_test_active) {
        // Send notifications as fast as possible to test queue
        uint8_t value = test_counter % 255;
        if (BLENotify.notify(test_char_handle, &value, sizeof(value))) {
            test_counter++;
        }
        delayMicroseconds(1000); // Add a small delay to avoid overwhelming the queue
        
        // Print status periodically
        uint32_t current_time = millis();
        if (current_time - last_status_print >= STATUS_INTERVAL) {
            Serial.printf("Notification count: %d\n", test_counter);
            last_status_print = current_time;
        }
    }
}