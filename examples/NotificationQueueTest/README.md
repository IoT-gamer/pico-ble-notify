# BLE Notification Queue Test

This example serves as both a stress test and demonstration of the BLE notification queueing system in the `pico-ble-notify` library.

## What This Test Does

The NotificationQueueTest deliberately attempts to send BLE notifications faster than they can be transmitted over the air to test how the library handles this common real-world scenario.

When run, it will:

1. Set up a simple Battery Service with a notification-enabled characteristic
2. Once a client connects and subscribes to notifications
3. Rapidly send as many notifications as possible
4. Print notification statistics every 2 seconds

## Expected Output

When running this test, you should expect to see:

1. **"Device connected!"** when a client connects
2. **"Notifications enabled by client"** when the client subscribes
3. **"Warning: Notification queue is full"** messages showing the queue reaching capacity
4. Periodic **"Notification count: X"** messages showing successful notifications

The warnings about the queue being full are expected and normal - they demonstrate the library correctly identifying when notifications are being generated faster than they can be sent. Despite the warnings, you should see the notification count consistently increasing, which shows that the queue is working correctly.

## Interpreting Results

- **Increasing notification count**: Confirms notifications are being processed and sent successfully
- **Queue full warnings**: Shows the library is correctly managing the flow of notifications
- **Stable performance**: The test should run indefinitely without crashing

The exact notification rate will vary depending on your specific hardware, the BLE connection parameters, and client device capabilities. In general, you can expect anywhere from 100-600 notifications per minute on a standard BLE connection.

## Adjusting Test Parameters

You can modify this test to match your application's needs:

1. Adjust `delayMicroseconds(1000)` in the loop to change the notification generation rate:
   - Lower values will create more queue pressure (more warnings)
   - Higher values will reduce queue pressure (fewer warnings)

2. Modify the data being sent (currently just a counter) to match your application's payload size

## Real-World Applications

This test represents a common scenario in BLE applications where data is generated faster than it can be sent. For example:

- Sensor data acquisition at high frequencies
- Streaming applications
- Bulk data transfer

The pico-ble-notify library handles these scenarios by:
1. Queuing notifications when they can't be sent immediately
2. Requesting permission to send when the client is ready
3. Processing the queue in order
4. Providing feedback when the queue reaches capacity

## Troubleshooting

If you don't see the notification count increasing:
- Ensure your BLE client has properly enabled notifications
- Check that the BLE connection is stable
- Verify you can read the characteristic value manually

If you experience frequent disconnections:
- The client may be overwhelmed by the notification rate
- Try increasing the delay between notification attempts