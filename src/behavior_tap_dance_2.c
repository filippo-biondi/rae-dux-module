#define DT_DRV_COMPAT zmk_behavior_tap_dance_2

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_tap_dance_config {
    uint32_t tapping_term_ms;
    size_t behavior_count;
    struct zmk_behavior_binding *behaviors;
};

struct behavior_tap_dance_2_data {
    struct zmk_behavior_binding child;
};

static int on_tap_dance_2_binding_pressed(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    LOG_INF("tap_dance_2: pressed");
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_tap_dance_2_data *data = dev->data;
    const struct device *dev_child = zmk_behavior_get_binding(data->child.behavior_dev);
    const struct behavior_tap_dance_config *tap_dance_config = dev_child->config;

    tap_dance_config->behaviors[0].param1 = binding->param1;
    tap_dance_config->behaviors[1].param1 = binding->param2;
    behavior_keymap_binding_pressed(&(data->child), event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_tap_dance_2_binding_released(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_tap_dance_2_data *data = dev->data;

    behavior_keymap_binding_released(&(data->child), event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_tap_dance_2_driver_api = {
    .binding_pressed = on_tap_dance_2_binding_pressed,
    .binding_released = on_tap_dance_2_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};



static int behavior_tap_dance_2_init(const struct device *dev) {
    return 0;
}


#define KR_INST(n)                                                                                 \
    static struct behavior_tap_dance_2_data behavior_tap_dance_2_data_##n = {                      \
	.child = { .behavior_dev = DEVICE_DT_NAME(DT_CHILD(DT_DRV_INST(n), child))}                                                            \
    };                                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_tap_dance_2_init, NULL, &behavior_tap_dance_2_data_##n,        \
                            NULL, POST_KERNEL,                           \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_tap_dance_2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KR_INST)

#endif 
