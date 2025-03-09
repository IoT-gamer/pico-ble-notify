#ifndef BLE_NOTIFY_H
#define BLE_NOTIFY_H

#include <Arduino.h>
#include <BTstackLib.h>
#include "ble/att_server.h"
#include "bluetooth.h"
#include "btstack_event.h"

// Maximum number of characteristics that can have notifications enabled
#define MAX_NOTIFY_CHARACTERISTICS 10

// Notification queue entry
struct NotificationEntry {
    uint16_t char_handle;
    uint8_t data[20];  // Maximum BLE attribute value size is typically 20 bytes
    uint8_t data_length;
    bool in_use;
};

class BLENotifyClass {
public:
    BLENotifyClass();
    
    // Initialize the library
    void begin();
    
    // Create a characteristic with notification support
    uint16_t addNotifyCharacteristic(UUID* uuid, uint16_t properties = ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY);
    
    // Send a notification for a characteristic
    bool notify(uint16_t char_handle, const void* data, uint8_t data_length);
    
    // Check if notifications are enabled for a characteristic
    bool isSubscribed(uint16_t char_handle);
    
    // Process events (call this in your loop)
    void update();
    
    // Internal callbacks - don't call these directly
    void handleConnection(hci_con_handle_t handle);
    void handleDisconnection();
    void handleSubscriptionChange(uint16_t char_handle, bool subscribed);
    void handleCanSendNowEvent();
    
private:
    // Connection handle for the current client
    hci_con_handle_t _con_handle;
    
    // Subscription status for each characteristic
    bool _subscriptions[MAX_NOTIFY_CHARACTERISTICS];
    uint16_t _char_handles[MAX_NOTIFY_CHARACTERISTICS];
    uint8_t _char_count;
    
    // Notification queue
    NotificationEntry _notification_queue[5];  // Queue size of 5 entries
    
    // Check if a characteristic is in our tracking list
    int8_t findCharacteristicIndex(uint16_t char_handle);
    
    // Add a notification to the queue
    bool queueNotification(uint16_t char_handle, const void* data, uint8_t data_length);
    
    // Process the notification queue
    void processNotificationQueue();
    
    // Flag to track if the packet handler is registered
    bool _handler_registered;
};

extern BLENotifyClass BLENotify;

#endif // BLE_NOTIFY_H