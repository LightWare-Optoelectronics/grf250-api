#include <stdio.h>
#include <stdlib.h>

#include "lw_serial_api_grf250.h"

#ifdef _WIN32
#include "lw_platform_win_serial.h"
#elif __linux__
#include "lw_platform_linux_serial.h"
#endif

void lw_debug_print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void check_success(lw_result result, const char *error_message) {
    if (result != LW_RESULT_SUCCESS) {
        printf("%s\n", error_message);
        exit(1);
    }
}

int main(void) {
    // ----------------------------------------------------------------------------
    // Platform related setup.
    // ----------------------------------------------------------------------------
    lw_platform_serial_device grf250;
    // Example Windows COM port: "\\\\.\\COM70"
    // Example Linux device serial port: "/dev/ttyACM0"
    check_success(lw_platform_create_serial_device("\\\\.\\COM70", 115200, &grf250), "Failed to create serial device");

    check_success(lw_grf250_initiate_serial(&grf250.device), "Failed to initiate serial\n");

    // ----------------------------------------------------------------------------
    // Get device product info.
    // ----------------------------------------------------------------------------
    lw_grf250_product_info product_info;
    check_success(lw_grf250_get_product_info(&grf250.device, &product_info), "Failed to get product info\n");

    printf("Product name: %s\n", product_info.product_name);
    printf("Hardware version: %d\n", product_info.hardware_version);
    printf("Firmware version: %d.%d.%d\n", product_info.firmware_version.major, product_info.firmware_version.minor, product_info.firmware_version.patch);
    printf("Serial number: %s\n", product_info.serial_number);

    // ----------------------------------------------------------------------------
    // Set up the device.
    // ----------------------------------------------------------------------------
    check_success(lw_grf250_set_stream(&grf250.device, LW_GRF250_STREAM_NONE), "Failed to set stream: none\n");
    check_success(lw_grf250_set_update_rate(&grf250.device, 5), "Failed to set update rate\n");

    lw_grf_distance_config distance_config = LW_GRF250_DISTANCE_CONFIG_ALL;
    check_success(lw_grf250_set_distance_config(&grf250.device, distance_config), "Failed to set distance config\n");

    // ----------------------------------------------------------------------------
    // Poll for distance data.
    // ----------------------------------------------------------------------------
    lw_grf250_distance_data distance_data;
    check_success(lw_grf250_get_distance_data(&grf250.device, &distance_data, distance_config), "Failed to get distance data\n");

    printf("Polled distance: %d mm\n", distance_data.first_return_raw_mm);

    // ----------------------------------------------------------------------------
    // Stream distance data: Blocking version.
    // ----------------------------------------------------------------------------
    check_success(lw_grf250_set_stream(&grf250.device, LW_GRF250_STREAM_DISTANCE), "Failed to set stream: distance\n");

    for (int i = 0; i < 10; ++i) {
        lw_result result = lw_grf250_wait_for_streamed_distance(&grf250.device, distance_config, &distance_data, 1000);

        if (result == LW_RESULT_SUCCESS) {
            printf("Streamed distance: %d mm\n", distance_data.first_return_raw_mm);
        } else if (result == LW_RESULT_TIMEOUT) {
            // TODO: Could be timeout or communication error.
            printf("Stream timeout\n");
        } else if (result == LW_RESULT_ERROR) {
            printf("Communication error\n");
            return 1;
        }
    }

    // ----------------------------------------------------------------------------
    // Stream distance data: Non-blocking version.
    // ----------------------------------------------------------------------------
    for (int i = 0; i < 10; ++i) {
        while (1) {
            printf("Attempting to get response...\n");
            // NOTE: The timeout is set to 0.
            lw_result result = lw_grf250_wait_for_streamed_distance(&grf250.device, distance_config, &distance_data, 0);

            if (result == LW_RESULT_SUCCESS) {
                printf("Non blocking streamed distance: %d mm\n", distance_data.first_return_raw_mm);
                break;
            } else if (result == LW_RESULT_AGAIN) {
                printf("Full response not received yet, waiting/doing other work...\n");
                lw_platform_sleep(50);
                continue;
            } else {
                printf("Communication error\n");
                return 1;
            }
        }
    }

    // ----------------------------------------------------------------------------
    // Closing down.
    // ----------------------------------------------------------------------------
    check_success(lw_grf250_set_stream(&grf250.device, LW_GRF250_STREAM_NONE), "Failed to set stream: none\n");

    printf("Sample completed\n");

    return 0;
}
