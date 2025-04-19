#include <zephyr/kernel.h>
#include <zephyr/bluetooth/gatt.h>
#include <zmk/ble.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zmk/matrix.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/hid.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static struct bt_uuid_128 my_service_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x054e72b1, 0x666d, 0x4e65, 0xa1cd, 0x6cf6a7f3a046));

static struct bt_uuid_128 my_char_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x9aaa82c9, 0x4848, 0x49c4, 0x8aae, 0xa6f7cecdc993));

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notify_enabled = (value == BT_GATT_CCC_NOTIFY);

    if (notify_enabled) {
        // Notifications have been enabled
        LOG_DBG("Notifications enabled");
    } else {
        // Notifications have been disabled
        LOG_DBG("Notifications disabled");
    }
}

// Variable to store Bluetooth connection
static struct bt_conn *conn;

BT_GATT_SERVICE_DEFINE(notify_svc,
    BT_GATT_PRIMARY_SERVICE(&my_service_uuid),               // Attribute 0: Service Declaration
    BT_GATT_CHARACTERISTIC(&my_char_uuid.uuid,               // Attribute 1: Characteristic Declaration
                           BT_GATT_CHRC_NOTIFY,              // - with NOTIFY property
                           BT_GATT_PERM_NONE,                // - No permissions needed for NOTIFY
                           NULL, NULL, NULL),                // No read or write handler

    // Client Characteristic Configuration Descriptor (CCC) for notifications
    BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

// Function to send a notification to the connected client
void send_notification(void *data, size_t size)
{
    if (!conn) {
        LOG_DBG("no active bluetooth connection"); 
        return;
    }

//    if (!bt_gatt_is_subscribed(conn, &notify_svc.attrs[1], BT_GATT_CCC_NOTIFY)) {
//        LOG_DBG("no client subscribed"); 
//        return;
//    }

    int err = bt_gatt_notify(conn, &notify_svc.attrs[1], data, size);
    if (err) {
        LOG_DBG("Notification failed (err %d)", err); 
    } else {
        LOG_DBG("Notification sent"); 
    }
}


// Connection callbacks
static void change_connection()
{
    conn = zmk_ble_active_profile_conn(); // Save connection reference
    LOG_DBG("New Connection"); 
}


static int position_state_changed_listener(const struct zmk_position_state_changed *ev);
static int keycode_state_changed_listener(const struct zmk_keycode_state_changed *ev); 
static int layer_state_changed_listener(const struct zmk_layer_state_changed *ev); 
static int modifiers_state_changed_listener(const struct zmk_modifiers_state_changed *ev); 

static int duce_listener(const zmk_event_t *eh){
    struct zmk_position_state_changed *ev_pos = as_zmk_position_state_changed(eh);
    if (ev_pos != NULL) {
        return position_state_changed_listener(ev_pos);
    } 
    struct zmk_keycode_state_changed *ev_key = as_zmk_keycode_state_changed(eh);
    if (ev_key != NULL) {
        return keycode_state_changed_listener(ev_key);
    }
    struct zmk_layer_state_changed *ev_layer = as_zmk_layer_state_changed(eh);
    if (ev_layer != NULL) {
        return layer_state_changed_listener(ev_layer);
    }
    struct zmk_modifiers_state_changed *ev_mod = as_zmk_modifiers_state_changed(eh);
    if (ev_layer != NULL) {
        return modifiers_state_changed_listener(ev_mod);
    }
    struct zmk_ble_active_profile_changed *ev_ble = as_zmk_ble_active_profile_changed(eh);
    if (ev_ble != NULL) {
	change_connection();
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(behavior_duce, duce_listener);
ZMK_SUBSCRIPTION(behavior_duce, zmk_position_state_changed);
ZMK_SUBSCRIPTION(behavior_duce, zmk_keycode_state_changed);
ZMK_SUBSCRIPTION(behavior_duce, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(behavior_duce, zmk_modifiers_state_changed);
ZMK_SUBSCRIPTION(behavior_duce, zmk_ble_active_profile_changed);

static int position_state_changed_listener(const struct zmk_position_state_changed *ev) {
    uint8_t payload[3];
    
    LOG_DBG("Position event, state: %d", ev->state);

    payload[0] = 0;
    payload[1] = ev->position;
    payload[2] = ev->state;

    send_notification(payload, sizeof(payload));

    return ZMK_EV_EVENT_BUBBLE;
}

static int keycode_state_changed_listener(const struct zmk_keycode_state_changed *ev) {
    LOG_DBG("Keycode event, state: %d", ev->state);
    uint8_t payload[10];

    payload[0] = 1;
    *((uint16_t*) &payload[1]) = ev->usage_page;
    *((uint32_t*) &payload[3]) = ev->keycode;
    payload[7] = ev->implicit_modifiers;
    payload[8] = ev->explicit_modifiers;
    payload[9] = ev->state;

    send_notification(payload, sizeof(payload));

    return ZMK_EV_EVENT_BUBBLE;
}

static int layer_state_changed_listener(const struct zmk_layer_state_changed *ev) {
    uint8_t payload[3];

    LOG_DBG("layer event, state: %d", ev->state);
    
    payload[0] = 2;
    payload[1] = ev->layer;
    payload[2] = ev->state;

    send_notification(payload, sizeof(payload));

    return ZMK_EV_EVENT_BUBBLE;
} 

static int modifiers_state_changed_listener(const struct zmk_modifiers_state_changed *ev){
    uint8_t payload[3];

    LOG_DBG("modifiers event, state: %d", ev->state);
    
    payload[0] = 3;
    payload[1] = ev->modifiers;
    payload[2] = ev->state;

    send_notification(payload, sizeof(payload));

    return ZMK_EV_EVENT_BUBBLE;
}
