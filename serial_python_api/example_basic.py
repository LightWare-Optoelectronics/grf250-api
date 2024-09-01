"""
example_basic.py

This example demonstrates how to communicate with the GRF-250 using the serial interface using
the built-in serial service handler.

Notes:
    Requires the pySerial module.

2024 LightWare Optoelectronics (Pty) Ltd.
https://www.lightwarelidar.com
"""

import time
import grf250_serial

# --------------------------------------------------------------------------------------------------
# Main application.
# --------------------------------------------------------------------------------------------------
print("Running GRF-250 sample.")

grf250 = grf250_serial.Grf250(grf250_serial.SerialServiceHandler("COM70", 115200))

# NOTE: Only needed if running over serial interface and mode is set to 'Wait for interface'.
grf250.initiate_serial()

product_info = grf250.get_product_information()

print("Product: " + product_info.product_name)
print("Hardware: " + str(product_info.hardware_version))
print("Firmware: " + product_info.firmware_version)
print("Serial: " + product_info.serial_number)

# Setup device
grf250.set_stream(grf250_serial.StreamId.NONE)
grf250.set_update_rate(5)
distance_config = grf250_serial.DistanceConfig.ALL
grf250.set_distance_config(distance_config)

# Poll for distance data
distance_data = grf250.get_distance_data(distance_config)
print("Polled distance: " + str(distance_data.first_return_raw_mm / 1000.0) + " m")

# Stream distance data: Blocking version.
grf250.set_stream(grf250_serial.StreamId.DISTANCE)

for i in range(10):
    distance_data = grf250.wait_for_streamed_distance(distance_config, 1)

    if distance_data is not None:
        print("Streamed distance: " + str(distance_data.first_return_raw_mm / 1000.0) + " m")
    else:
        print("Stream timeout")

# Stream distance data: Non-blocking version.
for i in range(10):
    print("Attempting to get response...")

    while True:
        # NOTE: The timeout is set to 0.
        distance_data = grf250.wait_for_streamed_distance(distance_config, 0)

        if distance_data is None:
            print("Full response not received yet, waiting/doing other work...")
            # NOTE: Simulate doing some work...
            time.sleep(0.05)
        else:
            print(
                "Non-blocking streamed distance: "
                + str(distance_data.first_return_raw_mm / 1000.0)
                + " m"
            )
            break
