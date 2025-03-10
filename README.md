# BLENotify Library for Raspberry Pi Pico
A library that adds BLE notification support to the standard BTstackLib in Arduino-Pico for Raspberry Pi Pico.

## Overview
[The standard BTstackLib for Arduino-Pico doesn't provide an easy way to send BLE notifications](https://github.com/bluekitchen/btstack/issues/551#issuecomment-1827805004). This library fills that gap by providing a simple wrapper around the lower-level BTstack API for notifications.

## Features
- Easy API for sending BLE notifications
- Automatic handling of client subscription state
- Notification queueing for when the client is busy
- Simple integration with existing BTstackLib code

## PlatformIO Installation
- Open the PlatformIO project configuration file (`platformio.ini`) and add the following:
```ini
[env:rpipicow]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = rpipicow
framework = arduino
board_build.core = earlephilhower
board_build.filesystem_size = 0.5m
build_flags = 
    -DPIO_FRAMEWORK_ARDUINO_ENABLE_BLUETOOTH
    -DPIO_FRAMEWORK_ARDUINO_ENABLE_IPV4
lib_deps =
    pico-ble-notify
```

## Arduino IDE Installation
1. Create a folder named BLENotify in your Arduino libraries folder
2. Copy the `BLENotify.h` and `BLENotify.cpp` files into this folder
3. Restart the Arduino IDE

## Usage

### Step 1: Include the library
```cpp
#include <BTstackLib.h>
#include "BLENotify.h"
```

### Step 2: Initialize the library
```cpp
void setup() {
    // Initialize the BLENotify library
    BLENotify.begin();
    
    // Set up your BLE service and other callbacks as usual
    // ...
}
```
### Step 3: Create a characteristic with notification support
```cpp
// Add a characteristic with notification support
uint16_t my_char_handle = BLENotify.addNotifyCharacteristic(
    new UUID("YOUR_CHARACTERISTIC_UUID"),
    ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY
);
```
### Step 4: Handle subscription changes
In your gattWriteCallback function, handle subscription change events:
```cpp
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
```
### Step 5: Call update in your loop
```cpp
void loop() {
    BTstack.loop();
    BLENotify.update();  // Process any pending notifications
    
    // Your other code...
}
```
### Step 6: Send notifications
```cpp
// Check if client is subscribed
if (BLENotify.isSubscribed(my_char_handle)) {
    // Prepare your data
    uint8_t data[] = {0x01, 0x02, 0x03};
    
    // Send notification
    BLENotify.notify(my_char_handle, data, sizeof(data));
}
```

## Example
An example sketch demonstrating how to use the library to send temperature notifications is included. It reads the Pico's internal temperature sensor and sends notifications when the temperature changes.

## Limitations
- Currently supports tracking up to 10 characteristics (configurable in `BLENotify.h`)
- Notification queue limited to 5 entries per default (configurable in `BLENotify.h`)
- Maximum payload size is 20 bytes per notification (standard BLE limitation using default MTU)

## License
This library is licensed under the MIT license. See the LICENSE file for more details.

## Acknowledgements/Credits
- This library was inspired by the need for a simpler way to implement BLE notifications on the Raspberry Pi Pico platform with Arduino-Pico core.
- [@mringwal](https://github.com/mringwal) [work-around suggestion](https://github.com/bluekitchen/btstack/issues/551#issuecomment-1827805004)
- [@mglazzari-qinmotion](https://github.com/mglazzari-qinmotion) [code snippet and suggestion](https://github.com/bluekitchen/btstack/issues/551#issuecomment-2257178367)
- [Arduino-Pico](https://github.com/earlephilhower/arduino-pico)
- [BTstack](https://github.com/bluekitchen/btstack)