#ifndef GRF250SERIAL_H
#define GRF250SERIAL_H

#include "lw_serial_api_grf250.h"
#include <Arduino.h>

/*
 * Wrapper around the GRF-250 API to use with Arduino Stream objects.
 * You will still need to call the lw_grf250_... functions directly to communicate with the device.
 *
 * NOTE: See the lw_serial_api_grf250.c file for all available GRF-250 commands.
 *
 */
class GRF250Serial {
public:
    lw_callback_device device;
    Stream *port;

    lw_grf_distance_config distance_config = LW_GRF250_DISTANCE_CONFIG_ALL;

    GRF250Serial(Stream *stream);

    void set_callbacks(lw_device_callback_get_time_ms get_time_ms_callback,
                       lw_device_callback_sleep sleep_callback,
                       lw_device_callback_serial_send serial_send_callback,
                       lw_device_callback_serial_receive serial_receive_callback);

private:
    static uint32_t get_time_ms_callback(lw_callback_device *device);
    static void sleep_callback(lw_callback_device *device, uint32_t time_ms);
    static uint32_t serial_send_callback(lw_callback_device *device, uint8_t *buffer, uint32_t size);
    static int32_t serial_receive_callback(lw_callback_device *device, uint8_t *buffer, uint32_t size, uint32_t timeout_ms);
};

#endif // GRF250SERIAL_H
