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

// Non blocking.
lw_result get_next_response(lw_platform_serial_port *serial_port, lw_response *response) {
    while (1) {
        uint8_t byte = 0;
        int32_t bytes_read = lw_platform_serial_read(serial_port, &byte, 1);

        if (bytes_read == -1) {
            return LW_RESULT_ERROR;
        } else if (bytes_read > 0) {
            if (lw_feed_response(response, byte) == LW_RESULT_SUCCESS) {
                return LW_RESULT_SUCCESS;
            }
        } else {
            return LW_RESULT_AGAIN;
        }
    }
}

// Blocking.
lw_result wait_for_next_response(lw_platform_serial_port *serial_port, lw_response *response, uint32_t command_id, uint32_t timeout_ms) {
    uint32_t timeout_time = lw_platform_get_time_ms() + timeout_ms;

    lw_init_response(response);

    while (1) {
        uint32_t current_time = lw_platform_get_time_ms();

        if (current_time >= timeout_time) {
            return LW_RESULT_TIMEOUT;
        }

        uint8_t byte = 0;
        int32_t bytes_read = lw_platform_serial_read(serial_port, &byte, 1);

        if (bytes_read == -1) {
            return LW_RESULT_ERROR;
        } else if (bytes_read > 0) {
            if (lw_feed_response(response, byte) == LW_RESULT_SUCCESS) {
                if (command_id == LW_ANY_COMMAND || response->command_id == command_id) {
                    return LW_RESULT_SUCCESS;
                }
            }
        }
    }
}

lw_result send_request_get_response(lw_platform_serial_port *serial_port, lw_request *request, lw_response *response, uint32_t timeout_ms) {
    // Write request packet to serial port.
    if (lw_platform_serial_write(serial_port, request->data, request->data_size) == 0) {
        printf("Failed to send request\n");
        return LW_RESULT_ERROR;
    }

    // Get response packet from serial port.
    if (wait_for_next_response(serial_port, response, request->command_id, timeout_ms) != 0) {
        printf("Failed to get response\n");
        return LW_RESULT_ERROR;
    }

    return LW_RESULT_SUCCESS;
}

int main(void) {
    // ----------------------------------------------------------------------------
    // Platform related setup.
    // This can be replaced with a different platform implementation.
    // ----------------------------------------------------------------------------
    check_success(lw_platform_init(), "Failed to initialize platform");

    lw_platform_serial_port grf250 = lw_platform_create_serial_port();
    // Example Windows COM port: "\\\\.\\COM70"
    // Example Linux device serial port: "/dev/ttyACM0"
    check_success(lw_platform_serial_connect("\\\\.\\COM70", 115200, &grf250), "Failed to connect to serial port");

    // Make sure the GRF-250 is in Serial mode.
    if (lw_platform_serial_write(&grf250, (uint8_t *)"UUU", 3) == 0) {
        printf("Failed to send UUU\n");
        return 1;
    }

    lw_request request;
    lw_response response;

    // ----------------------------------------------------------------------------
    // Get product name.
    // ----------------------------------------------------------------------------
    char product_name[16];
    check_success(lw_grf250_create_request_read_product_name(&request), "Failed to create request");
    check_success(send_request_get_response(&grf250, &request, &response, 1000), "Failed to run request");
    check_success(lw_grf250_parse_response_product_name(&response, product_name), "Failed to parse response");

    printf("Hardware name: %s\n", product_name);

    // ----------------------------------------------------------------------------
    // Get firmware version.
    // ----------------------------------------------------------------------------
    uint32_t firmware_version_int;
    check_success(lw_grf250_create_request_read_firmware_version(&request), "Failed to create request");
    check_success(send_request_get_response(&grf250, &request, &response, 1000), "Failed to run request");
    check_success(lw_grf250_parse_response_firmware_version(&response, &firmware_version_int), "Failed to parse response");

    lw_firmware_version firmware_version = lw_expand_firmware_version(firmware_version_int);
    printf("Firmware version: %d.%d.%d\n", firmware_version.major, firmware_version.minor, firmware_version.patch);

    // ----------------------------------------------------------------------------
    // Set streaming to none.
    // ----------------------------------------------------------------------------
    check_success(lw_grf250_create_request_write_stream(&request, LW_GRF250_STREAM_NONE), "Failed to create request");
    check_success(send_request_get_response(&grf250, &request, &response, 1000), "Failed to run request");

    // ----------------------------------------------------------------------------
    // Set update rate.
    // ----------------------------------------------------------------------------
    check_success(lw_grf250_create_request_write_update_rate(&request, 5), "Failed to create request");
    check_success(send_request_get_response(&grf250, &request, &response, 1000), "Failed to run request");

    // ----------------------------------------------------------------------------
    // Set distance configuration.
    // ----------------------------------------------------------------------------
    lw_grf_distance_config distance_config = LW_GRF250_DISTANCE_CONFIG_ALL;
    check_success(lw_grf250_create_request_write_distance_config(&request, distance_config), "Failed to create request");
    check_success(send_request_get_response(&grf250, &request, &response, 1000), "Failed to run request");

    // ----------------------------------------------------------------------------
    // Poll for distance data.
    // ----------------------------------------------------------------------------
    lw_grf250_distance_data distance_data;
    check_success(lw_grf250_create_request_read_distance_data(&request), "Failed to create request");
    check_success(send_request_get_response(&grf250, &request, &response, 1000), "Failed to run request");
    check_success(lw_grf250_parse_response_distance_data(&response, distance_config, &distance_data), "Failed to parse response");

    printf("Polled distance: %d mm\n", distance_data.first_return_raw_mm);

    // ----------------------------------------------------------------------------
    // Enable distance streaming.
    // ----------------------------------------------------------------------------
    check_success(lw_grf250_create_request_write_stream(&request, LW_GRF250_STREAM_DISTANCE), "Failed to create request");
    check_success(send_request_get_response(&grf250, &request, &response, 1000), "Failed to run request");

    // Wait for streamed distance data using the blocking 'wait_for_next_response' function.
    for (int i = 0; i < 10; ++i) {
        lw_result result = wait_for_next_response(&grf250, &response, LW_GRF250_COMMAND_DISTANCE_DATA, 1000);

        if (result == LW_RESULT_SUCCESS) {
            check_success(lw_grf250_parse_response_distance_data(&response, distance_config, &distance_data), "Failed to parse response");
            printf("Streamed distance: %d mm\n", distance_data.first_return_raw_mm);
        } else if (result == LW_RESULT_TIMEOUT) {
            // TODO: Could be timeout or communication error.
            printf("Stream timeout\n");
        } else if (result == LW_RESULT_ERROR) {
            printf("Communication error\n");
            return 1;
        }
    }

    // Wait for streamed distance by using the non-blocking 'get_next_response' function.
    for (int i = 0; i < 10; ++i) {
        lw_init_response(&response);

        while (1) {
            printf("Attempting to get response...\n");
            lw_result result = get_next_response(&grf250, &response);

            if (result == LW_RESULT_SUCCESS) {
                if (lw_grf250_parse_response_distance_data(&response, distance_config, &distance_data) == LW_RESULT_SUCCESS) {
                    printf("Streamed distance: %d mm\n", distance_data.first_return_raw_mm);
                } else {
                    // NOTE: Response was found, but has a different command ID.
                }
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
    // Set streaming to none.
    // ----------------------------------------------------------------------------
    check_success(lw_grf250_create_request_write_stream(&request, LW_GRF250_STREAM_NONE), "Failed to create request");
    check_success(send_request_get_response(&grf250, &request, &response, 1000), "Failed to run request");

    printf("Sample completed\n");

    return 0;
}
