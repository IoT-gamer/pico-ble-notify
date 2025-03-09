#include "BLENotify.h"

// Global packet handler callback registration
static btstack_packet_callback_registration_t hci_event_callback_registration;

// Forward declaration of the packet handler
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

// Global instance of the BLENotify class
BLENotifyClass BLENotify;

BLENotifyClass::BLENotifyClass() {
    _con_handle = HCI_CON_HANDLE_INVALID;
    _char_count = 0;
    _handler_registered = false;
    
    // Initialize subscriptions array
    for (uint8_t i = 0; i < MAX_NOTIFY_CHARACTERISTICS; i++) {
        _subscriptions[i] = false;
        _char_handles[i] = 0;
    }
    
    // Initialize notification queue
    for (uint8_t i = 0; i < 5; i++) {
        _notification_queue[i].in_use = false;
    }
}

void BLENotifyClass::begin() {
    if (!_handler_registered) {
        // Register for HCI events
        hci_event_callback_registration.callback = &packet_handler;
        hci_add_event_handler(&hci_event_callback_registration);
        _handler_registered = true;
    }
}

uint16_t BLENotifyClass::addNotifyCharacteristic(UUID* uuid, uint16_t properties) {
    if (_char_count >= MAX_NOTIFY_CHARACTERISTICS) {
        Serial.println("Error: Maximum number of notify characteristics reached");
        return 0;
    }
    
    // Add the characteristic using BTstack
    uint16_t char_handle = BTstack.addGATTCharacteristicDynamic(uuid, properties, 0);
    
    // Add to our tracking arrays
    if (char_handle != 0) {
        _char_handles[_char_count] = char_handle;
        _subscriptions[_char_count] = false;
        _char_count++;
    }
    
    return char_handle;
}

bool BLENotifyClass::notify(uint16_t char_handle, const void* data, uint8_t data_length) {
    // Check if the characteristic is in our list
    int8_t index = findCharacteristicIndex(char_handle);
    if (index < 0) {
        Serial.println("Error: Trying to notify for an unregistered characteristic");
        return false;
    }
    
    // Check if notifications are enabled for this characteristic
    if (!_subscriptions[index]) {
        return false;  // Not an error, just no subscribers
    }
    
    // Check if we can send a notification right now
    if (_con_handle != HCI_CON_HANDLE_INVALID && att_server_can_send_packet_now(_con_handle)) {
        // Send the notification directly
        if (att_server_notify(_con_handle, char_handle, (uint8_t*)data, data_length) == 0) {
            return true;
        }
    } else {
        // Queue the notification for later
        if (queueNotification(char_handle, data, data_length)) {
            // Request permission to send when possible
            if (_con_handle != HCI_CON_HANDLE_INVALID) {
                att_server_request_can_send_now_event(_con_handle);
            }
            return true;
        }
    }
    
    return false;
}

bool BLENotifyClass::isSubscribed(uint16_t char_handle) {
    int8_t index = findCharacteristicIndex(char_handle);
    if (index < 0) {
        return false;
    }
    
    return _subscriptions[index];
}

void BLENotifyClass::update() {
    // Process any queued notifications if we can send now
    if (_con_handle != HCI_CON_HANDLE_INVALID && att_server_can_send_packet_now(_con_handle)) {
        processNotificationQueue();
    }
}

void BLENotifyClass::handleConnection(hci_con_handle_t handle) {
    _con_handle = handle;
}

void BLENotifyClass::handleDisconnection() {
    _con_handle = HCI_CON_HANDLE_INVALID;
    
    // Clear all subscriptions on disconnect
    for (uint8_t i = 0; i < _char_count; i++) {
        _subscriptions[i] = false;
    }
    
    // Clear notification queue
    for (uint8_t i = 0; i < 5; i++) {
        _notification_queue[i].in_use = false;
    }
}

void BLENotifyClass::handleSubscriptionChange(uint16_t char_handle, bool subscribed) {
    int8_t index = findCharacteristicIndex(char_handle);
    if (index >= 0) {
        _subscriptions[index] = subscribed;
    }
}

void BLENotifyClass::handleCanSendNowEvent() {
    processNotificationQueue();
}

int8_t BLENotifyClass::findCharacteristicIndex(uint16_t char_handle) {
    for (uint8_t i = 0; i < _char_count; i++) {
        if (_char_handles[i] == char_handle) {
            return i;
        }
    }
    return -1;
}

bool BLENotifyClass::queueNotification(uint16_t char_handle, const void* data, uint8_t data_length) {
    // Find an empty slot in the queue
    for (uint8_t i = 0; i < 5; i++) {
        if (!_notification_queue[i].in_use) {
            _notification_queue[i].char_handle = char_handle;
            memcpy(_notification_queue[i].data, data, data_length);
            _notification_queue[i].data_length = data_length;
            _notification_queue[i].in_use = true;
            return true;
        }
    }
    
    // Queue is full
    Serial.println("Warning: Notification queue is full");
    return false;
}

void BLENotifyClass::processNotificationQueue() {
    for (uint8_t i = 0; i < 5; i++) {
        if (_notification_queue[i].in_use) {
            // Check if this characteristic is still subscribed
            int8_t index = findCharacteristicIndex(_notification_queue[i].char_handle);
            if (index >= 0 && _subscriptions[index]) {
                // Send the notification
                if (att_server_notify(_con_handle, _notification_queue[i].char_handle, 
                                    _notification_queue[i].data, _notification_queue[i].data_length) == 0) {
                    // Notification sent successfully, mark slot as free
                    _notification_queue[i].in_use = false;
                    
                    // Only send one notification per update to avoid flooding
                    break;
                }
            } else {
                // Characteristic is not subscribed anymore, drop the notification
                _notification_queue[i].in_use = false;
            }
        }
    }
}

// BTstack packet handler
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET) return;
    
    uint8_t event = packet[0];
    switch (event) {
        case ATT_EVENT_CAN_SEND_NOW:
            BLENotify.handleCanSendNowEvent();
            break;
        
        case HCI_EVENT_LE_META:
            if (packet[2] == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                hci_con_handle_t con_handle = little_endian_read_16(packet, 4);
                BLENotify.handleConnection(con_handle);
            }
            break;
            
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            BLENotify.handleDisconnection();
            break;
            
        default:
            break;
    }
}