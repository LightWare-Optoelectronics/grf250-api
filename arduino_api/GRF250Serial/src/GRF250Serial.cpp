#include "GRF250Serial.h"

GRF250Serial::GRF250Serial(Stream *stream) : port(stream) {
    port = stream;
    device = lw_create_callback_device(this, &GRF250Serial::sleep_callback,
                                       &GRF250Serial::get_time_ms_callback,
                                       &GRF250Serial::serial_send_callback,
                                       &GRF250Serial::serial_receive_callback);
}

void GRF250Serial::set_callbacks(lw_device_callback_get_time_ms get_time_ms_callback,
                                 lw_device_callback_sleep sleep_callback,
                                 lw_device_callback_serial_send serial_send_callback,
                                 lw_device_callback_serial_receive serial_receive_callback) {
    device.get_time_ms = get_time_ms_callback;
    device.sleep = sleep_callback;
    device.serial_send = serial_send_callback;
    device.serial_receive = serial_receive_callback;
}

uint32_t GRF250Serial::get_time_ms_callback(lw_callback_device *device) {
    return (uint32_t)millis();
}

void GRF250Serial::sleep_callback(lw_callback_device *device, uint32_t time_ms) {
    delay(time_ms);
}

uint32_t GRF250Serial::serial_send_callback(lw_callback_device *device, uint8_t *buffer, uint32_t size) {
    GRF250Serial *device_context = (GRF250Serial *)device->user_data;
    return device_context->port->write(buffer, size);
}

int32_t GRF250Serial::serial_receive_callback(lw_callback_device *device,
                                              uint8_t *buffer, uint32_t size, uint32_t timeout_ms) {
    (void)timeout_ms;

    GRF250Serial *device_context = (GRF250Serial *)device->user_data;

    device_context->port->setTimeout(timeout_ms);
    return device_context->port->readBytes(buffer, size);
}