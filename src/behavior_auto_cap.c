#define DT_DRV_COMPAT zmk_behavior_auto_cap

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

struct behavior_auto_cap_config {
    uint8_t trigger_keys_count;
    struct key *trigger_keys;
};

enum behavior_state {
    INIT,  // no key pressed yet
    READY, // some key pressed but no trigger
    TRIGGER, // trigger received
    ACTIVE  // space after trigger 
};

struct behavior_auto_cap_data {
    enum behavior_state state;
};

static bool is_trigger(uint16_t page, uint32_t keycode, const struct behavior_auto_cap_config *config) {
    for (u_int8_t i=0; i < config->trigger_keys_count; i++) {
        if (page == config->trigger_keys[i].page && keycode == config->trigger_keys[i].keycode) {
	    return true;
	}
    }
    return false;
}

static int auto_cap_keycode_state_changed_listener(const zmk_event_t *eh);

ZMK_LISTENER(behavior_auto_cap, auto_cap_keycode_state_changed_listener);
ZMK_SUBSCRIPTION(behavior_auto_cap, zmk_keycode_state_changed);

static const struct device *dev;

static int auto_cap_keycode_state_changed_listener(const zmk_event_t *eh) {
    struct behavior_auto_cap_data *data = dev->data;
    const struct behavior_auto_cap_config *config = dev->config;
    struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    
    if (ev == NULL || !ev->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    LOG_INF("auto_cap: state before is %u", data->state);

    bool trigger = is_trigger(ev->usage_page, ev->keycode, config);
    bool space = ev->usage_page == HID_USAGE_KEY && ev->keycode == HID_USAGE_KEY_KEYBOARD_SPACEBAR;

    LOG_DBG("auto_cap: recived: 0x%02X - 0x%02X", ev->usage_page, ev->keycode);   

//    ev->keycode = HID_USAGE_KEY_KEYBOARD_SPACEBAR;
    switch (data->state) {
        case INIT:
	    if (!trigger && !space) {
		data->state = READY;
	    }
	    break;
	case READY:
	    if (trigger) {
		data->state = TRIGGER;
	    } else if (space) {
		data->state = INIT;
	    }
	    break;
	case TRIGGER:
	    if (space) {
		data->state = ACTIVE;
	    } else {
		data->state = INIT;
	    }
	    break;
	case ACTIVE:
	    ev->implicit_modifiers |= MOD_LSFT;
	    data->state = READY;
	    break;
    }

    LOG_INF("auto_cap: state after is %u", data->state);

    return ZMK_EV_EVENT_BUBBLE;
}

static int behavior_auto_cap_init(const struct device *dev_init) {
    dev = dev_init;
    return 0;
}

#define CREATE_KEY_STRUCT(i) \
    {.page = ZMK_HID_USAGE_PAGE(i), .keycode = ZMK_HID_USAGE_ID(i)}

#define _TRANSFORM_KEY_ENTRY(idx, node) CREATE_KEY_STRUCT(DT_INST_PROP_BY_IDX(node, trigger_keys, idx))

#define TRANSFORMED_KEYS(node)                                                               \
    {LISTIFY(DT_INST_PROP_LEN(node, trigger_keys), _TRANSFORM_KEY_ENTRY, (, ), node)}

#define KR_INST(n)                                                                                 \
    static struct key behavior_auto_cap_config_##n##_trigger_keys[DT_INST_PROP_LEN(n, trigger_keys)] =   \
        TRANSFORMED_KEYS(n);                                                                         \
    static struct behavior_auto_cap_data behavior_auto_cap_data_##n = {     \
        .state = INIT,   	\
    };                        \
    static struct behavior_auto_cap_config behavior_auto_cap_config_##n = {                      \
        .trigger_keys_count = DT_INST_PROP_LEN(n, trigger_keys),                                     \
        .trigger_keys = behavior_auto_cap_config_##n##_trigger_keys,                                   \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_auto_cap_init, NULL, &behavior_auto_cap_data_##n,        \
                            &behavior_auto_cap_config_##n, POST_KERNEL,                           \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(KR_INST)

#endif 
