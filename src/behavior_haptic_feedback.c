#define DT_DRV_COMPAT zmk_behavior_haptic_feedback

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zmk/keymap.h>
#include <zmk/behavior.h>

#include <zmk/ble.h>    
#include <zmk/event_manager.h>
#include <zmk/events/ble_active_profile_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define RUMBLE_NODE	DT_CHOSEN(DT_DRV_COMPAT)

// struct behavior_rumble_config {
//     uint8_t index;
//     uint8_t count;
// };

// struct behavior_rumble_data {
// };
static const struct gpio_dt_spec rmbl = GPIO_DT_SPEC_GET(RUMBLE_NODE, gpios);

void _brr(const int count)
{
    if (!gpio_is_ready_dt(&rmbl)) {
		LOG_ERR("Error: rumble device %d is not ready\n", rmbl.pin);
		return;
	}

	if (gpio_pin_configure_dt(&rmbl, GPIO_OUTPUT_ACTIVE) < 0) {
        LOG_ERR("Error: Couldn't activate rumble device %d\n", rmbl.pin);
		return;
	}

    for (int i = 0; i < count; i++) {
        LOG_DBG("  Rumble %d ", i);
        gpio_pin_toggle_dt(&rmbl);
        k_msleep(250);
        gpio_pin_toggle_dt(&rmbl);
        k_msleep(50);
    }
}

static int behavior_haptic_feedback_init(const struct device *dev) { return 0; };

static int haptic_feedback_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    LOG_WRN("rumbling %d", binding->param1);
    _brr(binding->param1);
    LOG_DBG("DONE rumbling %d times!", binding->param1);

    return ZMK_BEHAVIOR_OPAQUE;
}

static int haptic_feedback_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_haptic_feedback_driver_api = {
    .binding_pressed = haptic_feedback_keymap_binding_pressed,
    .binding_released = haptic_feedback_keymap_binding_released,
};

BEHAVIOR_DT_INST_DEFINE(0, behavior_haptic_feedback_init, NULL, NULL, NULL, POST_KERNEL,
                        CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_haptic_feedback_driver_api);


int ble_active_profile_change_listener(const zmk_event_t *eh)
{
    const struct zmk_ble_active_profile_changed *profile_ev = NULL;
    if ((profile_ev = as_zmk_ble_active_profile_changed(eh)) == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    const int count = profile_ev->index + 1;
    LOG_WRN("Bluetooth profile [%d] is active -> rumbling [%d] times!", profile_ev->index, count);
    _brr(count);
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(ble_active_profile_change_status, ble_active_profile_change_listener)
#if defined(CONFIG_ZMK_BLE)
    ZMK_SUBSCRIPTION(ble_active_profile_change_status, zmk_ble_active_profile_changed);
#endif

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */