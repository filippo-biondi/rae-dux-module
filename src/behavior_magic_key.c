#define DT_DRV_COMPAT zmk_behavior_magic_key

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

struct key {
    uint16_t page;
    uint32_t keycode;
};

struct behavior_magic_key_config {
    uint8_t index;
    uint8_t dictionary_count;
    struct key *dictionary_keys;
    struct zmk_behavior_binding *dictionary_values;
    uint8_t usage_pages_count;
    uint16_t usage_pages[];
};

struct behavior_magic_key_data {
    struct key last_pressed;
    uint8_t current_behavior_index;
};

static int on_magic_key_binding_pressed(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    LOG_INF("magic_key: pressed");
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_magic_key_data *data = dev->data;
    const struct behavior_magic_key_config *config = dev->config;

    LOG_INF("magic_key: last keycode is 0x%02X - 0x%02X", data->last_pressed.page, data->last_pressed.keycode);

//    if (data->last_pressed == 0) {
//        return ZMK_BEHAVIOR_OPAQUE;
//    }

    for (int i=0; i < config->dictionary_count; i++) {
        LOG_INF("magic_key: checking key with 0x%02X - 0x%02X", config->dictionary_keys[i].page, config->dictionary_keys[i].keycode);
        if (data->last_pressed.page == config->dictionary_keys[i].page && data->last_pressed.keycode == config->dictionary_keys[i].keycode) {
            LOG_INF("magic_key: match found");

            struct zmk_behavior_binding selected_behavior = config->dictionary_values[i];

            LOG_INF("magic_key: matching behavior is %s with param1 %u and param2 %u",
                    selected_behavior.behavior_dev,
                    selected_behavior.param1,
                    selected_behavior.param2);

            data->current_behavior_index = i;
            behavior_keymap_binding_pressed(&selected_behavior, event);
            break;
	    }
    }
    // data->last_pressed = 0;
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_magic_key_binding_released(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_magic_key_data *data = dev->data;
    const struct behavior_magic_key_config *config = dev->config;

//    if (data->last_pressed == 0) {
//        return ZMK_BEHAVIOR_OPAQUE;
//    }

    struct zmk_behavior_binding selected_behavior = config->dictionary_values[data->current_behavior_index];
    behavior_keymap_binding_released(&selected_behavior, event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_magic_key_driver_api = {
    .binding_pressed = on_magic_key_binding_pressed,
    .binding_released = on_magic_key_binding_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

static int magic_key_keycode_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_magic_key, magic_key_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_magic_key, zmk_keycode_state_changed);

static const struct device *devs[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)];

static int magic_key_keycode_state_changed_listener(const zmk_event_t *eh) {
    LOG_INF("magic_key: startig last key update");
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    for (int i = 0; i < DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT); i++) {
        const struct device *dev = devs[i];
        if (dev == NULL) {
            continue;
        }

        struct behavior_magic_key_data *data = dev->data;
        const struct behavior_magic_key_config *config = dev->config;

        for (int u = 0; u < config->usage_pages_count; u++) {
            if (config->usage_pages[u] == ev->usage_page) {
		        data->last_pressed.page = ev->usage_page;
		        data->last_pressed.keycode = ev->keycode;
                break;
            }
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

static int behavior_magic_key_init(const struct device *dev) {
    const struct behavior_magic_key_config *config = dev->config;
    devs[config->index] = dev;
    return 0;
}

#define _TRANSFORM_BINDING_ENTRY(idx, node) ZMK_KEYMAP_EXTRACT_BINDING(idx, node)

#define CREATE_KEY_STRUCT(i) \
    {.page = ZMK_HID_USAGE_PAGE(i), .keycode = ZMK_HID_USAGE_ID(i)}

#define _TRANSFORM_KEY_ENTRY(idx, node) CREATE_KEY_STRUCT(DT_INST_PROP_BY_IDX(node, dictionary_keys, idx))

#define TRANSFORMED_BINDINGS(node)                                                           \
    {LISTIFY(DT_INST_PROP_LEN(node, bindings), _TRANSFORM_BINDING_ENTRY, (, ), DT_DRV_INST(node))}

#define TRANSFORMED_KEYS(node)                                                               \
    {LISTIFY(DT_INST_PROP_LEN(node, dictionary_keys), _TRANSFORM_KEY_ENTRY, (, ), node)}

#define KR_INST(n)                                                                                 \
    static struct zmk_behavior_binding                                                             \
        behavior_magic_key_config_##n##_bindings[DT_INST_PROP_LEN(n, bindings)] =                  \
            TRANSFORMED_BINDINGS(n);                                                               \
    static struct key behavior_magic_key_config_##n##_keys[DT_INST_PROP_LEN(n, dictionary_keys)] =   \
        TRANSFORMED_KEYS(n);                                                                         \
    static struct behavior_magic_key_data behavior_magic_key_data_##n = {};                        \
    static struct behavior_magic_key_config behavior_magic_key_config_##n = {                      \
        .index = n,                                                                                \
        .usage_pages = DT_INST_PROP(n, usage_pages),                                               \
        .usage_pages_count = DT_INST_PROP_LEN(n, usage_pages),                                     \
        .dictionary_count = DT_INST_PROP_LEN(n, bindings),                                         \
        .dictionary_keys = behavior_magic_key_config_##n##_keys,                                   \
        .dictionary_values = behavior_magic_key_config_##n##_bindings,                             \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_magic_key_init, NULL, &behavior_magic_key_data_##n,        \
                            &behavior_magic_key_config_##n, POST_KERNEL,                           \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_magic_key_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KR_INST)

#endif 
