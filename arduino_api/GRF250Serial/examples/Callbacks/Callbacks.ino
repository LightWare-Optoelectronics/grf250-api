#include <GRF250Serial.h>

// Serial port used for the serial monitor output.
#define SERIAL_MONITOR Serial

// A helper function that checks if a function returns LW_RESULT_SUCCESS, otherwise
// prints an error message to the SERIAL_MONITOR interface and pauses the program.
void check_success(lw_result result, const char *error_message) {
  if (result != LW_RESULT_SUCCESS) {
    SERIAL_MONITOR.print(error_message);
    // Sit in a loop to prevent the program from going further.
    while (true) {}
  }
}

// ----------------------------------------------------------------------------
// Device service callbacks.
// ----------------------------------------------------------------------------
struct user_device_context {
  lw_callback_device device;
  Stream *port;
};

user_device_context grf250;

uint32_t custom_get_time_ms_callback(lw_callback_device *device) {
    return (uint32_t)millis();
}

void custom_sleep_callback(lw_callback_device *device, uint32_t time_ms) {
    delay(time_ms);
}

uint32_t custom_serial_send_callback(lw_callback_device *device, uint8_t *buffer, uint32_t size) {
    user_device_context *device_context = (user_device_context *)device->user_data;
    return device_context->port->write(buffer, size);
}

int32_t custom_serial_receive_callback(lw_callback_device *device, uint8_t *buffer, uint32_t size, uint32_t timeout_ms) {
    user_device_context *device_context = (user_device_context *)device->user_data;

    device_context->port->setTimeout(timeout_ms);
    return device_context->port->readBytes(buffer, size);
}

lw_grf_distance_config distance_config = LW_GRF250_DISTANCE_CONFIG_ALL;

void setup() {
  // Serial monitor serial port.
  SERIAL_MONITOR.begin(115200);

  // GRF250 serial port.
  Serial2.begin(115200);
  grf250.port = (Stream *)&Serial2;

  grf250.device = lw_create_callback_device(&grf250,
                                            &custom_sleep_callback,
                                            &custom_get_time_ms_callback,
                                            &custom_serial_send_callback,
                                            &custom_serial_receive_callback);

  check_success(lw_grf250_initiate_serial(&grf250.device), "Failed to initiate serial\n");

  // ----------------------------------------------------------------------------
  // Get device product info.
  // ----------------------------------------------------------------------------
  lw_grf250_product_info product_info;
  check_success(lw_grf250_get_product_info(&grf250.device, &product_info), "Failed to get product info\n");

  SERIAL_MONITOR.print("Product name: "); SERIAL_MONITOR.println(product_info.product_name);
  SERIAL_MONITOR.print("Hardware version: "); SERIAL_MONITOR.println(product_info.hardware_version);
  SERIAL_MONITOR.print("Firmware version: "); SERIAL_MONITOR.print(product_info.firmware_version.major); SERIAL_MONITOR.print(".");
                                              SERIAL_MONITOR.print(product_info.firmware_version.minor); SERIAL_MONITOR.print(".");
                                              SERIAL_MONITOR.println(product_info.firmware_version.patch);
  SERIAL_MONITOR.print("Serial number: "); SERIAL_MONITOR.println(product_info.serial_number);

  // ----------------------------------------------------------------------------
  // Set up the device.
  // ----------------------------------------------------------------------------
  check_success(lw_grf250_set_stream(&grf250.device, LW_GRF250_STREAM_NONE), "Failed to set stream: none\n");
  check_success(lw_grf250_set_update_rate(&grf250.device, 5), "Failed to set update rate\n");

  
  check_success(lw_grf250_set_distance_config(&grf250.device, distance_config), "Failed to set distance config\n");

  // ----------------------------------------------------------------------------
  // Poll for distance data.
  // ----------------------------------------------------------------------------
  lw_grf250_distance_data distance_data;
  check_success(lw_grf250_get_distance_data(&grf250.device, &distance_data, distance_config), "Failed to get distance data\n");

  SERIAL_MONITOR.print("Polled distance: "); SERIAL_MONITOR.print(distance_data.first_return_raw_mm); SERIAL_MONITOR.println(" mm");
}

void loop() {
  lw_grf250_distance_data distance_data;

  // ----------------------------------------------------------------------------
  // Stream distance data: Blocking version.
  // ----------------------------------------------------------------------------
  check_success(lw_grf250_set_stream(&grf250.device, LW_GRF250_STREAM_DISTANCE), "Failed to set stream: distance\n");

  for (int i = 0; i < 10; ++i) {
      lw_result result = lw_grf250_wait_for_streamed_distance(&grf250.device, distance_config, &distance_data, 1000);

      if (result == LW_RESULT_SUCCESS) {
          SERIAL_MONITOR.print("Streamed distance: "); SERIAL_MONITOR.print(distance_data.first_return_raw_mm); SERIAL_MONITOR.println(" mm");
      } else if (result == LW_RESULT_TIMEOUT) {
          // TODO: Could be timeout or communication error.
          SERIAL_MONITOR.println("Stream timeout");
      }
  }

  // ----------------------------------------------------------------------------
  // Stream distance data: Non-blocking version.
  // ----------------------------------------------------------------------------
  for (int i = 0; i < 10; ++i) {
      while (1) {
          SERIAL_MONITOR.println("Attempting to get response...");
          // NOTE: The timeout is set to 0.
          lw_result result = lw_grf250_wait_for_streamed_distance(&grf250.device, distance_config, &distance_data, 0);

          if (result == LW_RESULT_SUCCESS) {
              SERIAL_MONITOR.print("Streamed distance: "); SERIAL_MONITOR.print(distance_data.first_return_raw_mm); SERIAL_MONITOR.println(" mm");
              break;
          } else if (result == LW_RESULT_AGAIN) {
              SERIAL_MONITOR.println("Full response not received yet, waiting/doing other work...");
              delay(50);
              continue;
          }
      }
  }
}