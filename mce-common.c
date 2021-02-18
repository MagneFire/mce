/**
 * @file mce-common.c
 * Common state logic for Mode Control Entity
 * <p>
 * Copyright (C) 2017 Jolla Ltd.
 * <p>
 * @author Simo Piiroinen <simo.piiroinen@jollamobile.com>
 *
 * mce is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * mce is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with mce.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "mce-common.h"

#include "mce.h"
#include "mce-dbus.h"
#include "mce-log.h"

#include <string.h>

#include <mce/dbus-names.h>
#include <mce/mode-names.h>

/* ========================================================================= *
 * PROTOTYPES
 * ========================================================================= */

// DBUS_FUNCTIONS
static void     common_dbus_send_usb_cable_state  (DBusMessage *const req);
static gboolean common_dbus_get_usb_cable_state_cb(DBusMessage *const req);
static void     common_dbus_send_charger_state    (DBusMessage *const req);
static gboolean common_dbus_get_charger_state_cb  (DBusMessage *const req);
static void     common_dbus_send_battery_status   (DBusMessage *const req);
static gboolean common_dbus_get_battery_status_cb (DBusMessage *const req);
static void     common_dbus_send_battery_level    (DBusMessage *const req);
static gboolean common_dbus_get_battery_level_cb  (DBusMessage *const req);
static void     common_dbus_init                  (void);
static void     common_dbus_quit                  (void);

// DATAPIPE_FUNCTIONS
static void     common_datapipe_usb_cable_state_cb(gconstpointer data);
static void     common_datapipe_charger_state_cb  (gconstpointer data);
static void     common_datapipe_battery_status_cb (gconstpointer data);
static void     common_datapipe_battery_level_cb  (gconstpointer data);
static void     common_datapipe_init              (void);
static void     common_datapipe_quit              (void);

// MODULE_INIT_QUIT
bool mce_common_init(void);
void mce_common_quit(void);

/* ========================================================================= *
 * STATE_DATA
 * ========================================================================= */

/** USB cable status; assume undefined */
static usb_cable_state_t usb_cable_state = USB_CABLE_UNDEF;

/** Charger state; assume undefined */
static charger_state_t charger_state = CHARGER_STATE_UNDEF;

/** Battery status; assume undefined */
static battery_status_t battery_status = BATTERY_STATUS_UNDEF;

/** Battery charge level: assume 100% */
static gint battery_level = BATTERY_LEVEL_INITIAL;

/* ========================================================================= *
 * DBUS_FUNCTIONS
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * usb_cable_state
 * ------------------------------------------------------------------------- */

/** Send usb_cable_state D-Bus signal / method call reply
 *
 * @param req  method call message to reply, or NULL to send signal
 */
static void
common_dbus_send_usb_cable_state(DBusMessage *const req)
{
    static const char *last = 0;

    DBusMessage *msg = NULL;

    const char *value = usb_cable_state_to_dbus(usb_cable_state);

    if( req ) {
        msg = dbus_new_method_reply(req);
    }
    else if( last == value ) {
        goto EXIT;
    }
    else {
        last = value;
        msg = dbus_new_signal(MCE_SIGNAL_PATH,
                              MCE_SIGNAL_IF,
                              MCE_USB_CABLE_STATE_SIG);
    }

    if( !dbus_message_append_args(msg,
                                  DBUS_TYPE_STRING, &value,
                                  DBUS_TYPE_INVALID) )
        goto EXIT;

    mce_log(LL_DEBUG, "%s: %s = %s",
            req ? "reply" : "broadcast",
            "usb_cable_state", value);

    dbus_send_message(msg), msg = 0;

EXIT:

    if( msg )
        dbus_message_unref(msg);
}

/** Callback for handling usb_cable_state D-Bus queries
 *
 * @param req  method call message to reply
 */
static gboolean
common_dbus_get_usb_cable_state_cb(DBusMessage *const req)
{
    mce_log(LL_DEBUG, "usb_cable_state query from: %s",
            mce_dbus_get_message_sender_ident(req));

    if( !dbus_message_get_no_reply(req) )
        common_dbus_send_usb_cable_state(req);

    return TRUE;
}

/* ------------------------------------------------------------------------- *
 * charger_state
 * ------------------------------------------------------------------------- */

/** Send charger_state D-Bus signal / method call reply
 *
 * @param req  method call message to reply, or NULL to send signal
 */
static void
common_dbus_send_charger_state(DBusMessage *const req)
{
    static const char *last = 0;

    DBusMessage *msg = NULL;

    const char *value = charger_state_to_dbus(charger_state);

    if( req ) {
        msg = dbus_new_method_reply(req);
    }
    else if( last == value ) {
        goto EXIT;
    }
    else {
        last = value;
        msg = dbus_new_signal(MCE_SIGNAL_PATH,
                              MCE_SIGNAL_IF,
                              MCE_CHARGER_STATE_SIG);
    }

    if( !dbus_message_append_args(msg,
                                  DBUS_TYPE_STRING, &value,
                                  DBUS_TYPE_INVALID) )
        goto EXIT;

    mce_log(LL_DEBUG, "%s: %s = %s",
            req ? "reply" : "broadcast",
            "charger_state", value);

    dbus_send_message(msg), msg = 0;

EXIT:

    if( msg )
        dbus_message_unref(msg);
}

/** Callback for handling charger_state D-Bus queries
 *
 * @param req  method call message to reply
 */
static gboolean
common_dbus_get_charger_state_cb(DBusMessage *const req)
{
    mce_log(LL_DEBUG, "charger_state query from: %s",
            mce_dbus_get_message_sender_ident(req));

    if( !dbus_message_get_no_reply(req) )
        common_dbus_send_charger_state(req);

    return TRUE;
}

/* ------------------------------------------------------------------------- *
 * battery_status
 * ------------------------------------------------------------------------- */

/** Send battery_status D-Bus signal / method call reply
 *
 * @param req  method call message to reply, or NULL to send signal
 */
static void
common_dbus_send_battery_status(DBusMessage *const req)
{
    static const char *last = 0;

    DBusMessage *msg = NULL;

    const char *value = battery_status_to_dbus(battery_status);

    if( req ) {
        msg = dbus_new_method_reply(req);
    }
    else if( last == value ) {
        goto EXIT;
    }
    else {
        last = value;
        msg = dbus_new_signal(MCE_SIGNAL_PATH,
                              MCE_SIGNAL_IF,
                              MCE_BATTERY_STATUS_SIG);
    }

    if( !dbus_message_append_args(msg,
                                  DBUS_TYPE_STRING, &value,
                                  DBUS_TYPE_INVALID) )
        goto EXIT;

    mce_log(LL_DEBUG, "%s: %s = %s",
            req ? "reply" : "broadcast",
            "battery_status", value);

    dbus_send_message(msg), msg = 0;

EXIT:

    if( msg )
        dbus_message_unref(msg);
}

/** Callback for handling battery_status D-Bus queries
 *
 * @param req  method call message to reply
 */
static gboolean
common_dbus_get_battery_status_cb(DBusMessage *const req)
{
    mce_log(LL_DEBUG, "battery_status query from: %s",
            mce_dbus_get_message_sender_ident(req));

    if( !dbus_message_get_no_reply(req) )
        common_dbus_send_battery_status(req);

    return TRUE;
}

/* ------------------------------------------------------------------------- *
 * battery_level
 * ------------------------------------------------------------------------- */

/** Send battery_level D-Bus signal / method call reply
 *
 * @param req  method call message to reply, or NULL to send signal
 */
static void
common_dbus_send_battery_level(DBusMessage *const req)
{
    static dbus_int32_t last = MCE_BATTERY_LEVEL_UNKNOWN - 1;

    DBusMessage *msg = NULL;

    dbus_int32_t value = battery_level;

    /* Normalize to values allowed by MCE D-Bus api documentation */
    if( value < 0 )
        value = MCE_BATTERY_LEVEL_UNKNOWN;
    else if( value > 100 )
        value = 100;

    if( req ) {
        msg = dbus_new_method_reply(req);
    }
    else if( last == value ) {
        goto EXIT;
    }
    else {
        last = value;
        msg = dbus_new_signal(MCE_SIGNAL_PATH,
                              MCE_SIGNAL_IF,
                              MCE_BATTERY_LEVEL_SIG);
    }

    if( !dbus_message_append_args(msg,
                                  DBUS_TYPE_INT32, &value,
                                  DBUS_TYPE_INVALID) )
        goto EXIT;

    mce_log(LL_DEBUG, "%s: %s = %d",
            req ? "reply" : "broadcast",
            "charger_state", value);

    dbus_send_message(msg), msg = 0;

EXIT:

    if( msg )
        dbus_message_unref(msg);
}

/** Callback for handling battery_level D-Bus queries
 *
 * @param req  method call message to reply
 */
static gboolean
common_dbus_get_battery_level_cb(DBusMessage *const req)
{
    mce_log(LL_DEBUG, "battery_level query from: %s",
            mce_dbus_get_message_sender_ident(req));

    if( !dbus_message_get_no_reply(req) )
        common_dbus_send_battery_level(req);

    return TRUE;
}

/* ------------------------------------------------------------------------- *
 * init/quit
 * ------------------------------------------------------------------------- */

/** Array of dbus message handlers */
static mce_dbus_handler_t common_dbus_handlers[] =
{
    /* signals - outbound (for Introspect purposes only) */
    {
        .interface = MCE_SIGNAL_IF,
        .name      = MCE_USB_CABLE_STATE_SIG,
        .type      = DBUS_MESSAGE_TYPE_SIGNAL,
        .args      =
            "    <arg name=\"usb_cable_state\" type=\"s\"/>\n"
    },
    {
        .interface = MCE_SIGNAL_IF,
        .name      = MCE_CHARGER_STATE_SIG,
        .type      = DBUS_MESSAGE_TYPE_SIGNAL,
        .args      =
            "    <arg name=\"charger_state\" type=\"s\"/>\n"
    },
    {
        .interface = MCE_SIGNAL_IF,
        .name      = MCE_BATTERY_STATUS_SIG,
        .type      = DBUS_MESSAGE_TYPE_SIGNAL,
        .args      =
            "    <arg name=\"battery_status\" type=\"s\"/>\n"
    },
    {
        .interface = MCE_SIGNAL_IF,
        .name      = MCE_BATTERY_LEVEL_SIG,
        .type      = DBUS_MESSAGE_TYPE_SIGNAL,
        .args      =
            "    <arg name=\"battery_level\" type=\"i\"/>\n"
    },
    /* method calls */
    {
        .interface = MCE_REQUEST_IF,
        .name      = MCE_USB_CABLE_STATE_GET,
        .type      = DBUS_MESSAGE_TYPE_METHOD_CALL,
        .callback  = common_dbus_get_usb_cable_state_cb,
        .args      =
            "    <arg direction=\"out\" name=\"usb_cable_state\" type=\"s\"/>\n"
    },
    {
        .interface = MCE_REQUEST_IF,
        .name      = MCE_CHARGER_STATE_GET,
        .type      = DBUS_MESSAGE_TYPE_METHOD_CALL,
        .callback  = common_dbus_get_charger_state_cb,
        .args      =
            "    <arg direction=\"out\" name=\"charger_state\" type=\"s\"/>\n"
    },
    {
        .interface = MCE_REQUEST_IF,
        .name      = MCE_BATTERY_STATUS_GET,
        .type      = DBUS_MESSAGE_TYPE_METHOD_CALL,
        .callback  = common_dbus_get_battery_status_cb,
        .args      =
            "    <arg direction=\"out\" name=\"battery_status\" type=\"s\"/>\n"
    },
    {
        .interface = MCE_REQUEST_IF,
        .name      = MCE_BATTERY_LEVEL_GET,
        .type      = DBUS_MESSAGE_TYPE_METHOD_CALL,
        .callback  = common_dbus_get_battery_level_cb,
        .args      =
            "    <arg direction=\"out\" name=\"battery_level\" type=\"i\"/>\n"
    },
    /* sentinel */
    {
        .interface = 0
    }
};

/** Timer callback id for broadcasting initial states */
static guint common_dbus_initial_id = 0;

/** Timer callback function for broadcasting initial states
 *
 * @param aptr (not used)
 *
 * @return FALSE to stop idle callback from repeating
 */
static gboolean common_dbus_initial_cb(gpointer aptr)
{
    (void)aptr;

    /* Do explicit broadcast of initial states.
     *
     * Note that we expect nothing to happen here, unless the
     * datapipe initialization for some reason ends up leaving
     * some values to undefined state.
     */
    common_dbus_send_usb_cable_state(0);
    common_dbus_send_charger_state(0);
    common_dbus_send_battery_status(0);
    common_dbus_send_battery_level(0);

    common_dbus_initial_id = 0;
    return FALSE;
}

/** Add dbus handlers
 */
static void common_dbus_init(void)
{
    mce_dbus_handler_register_array(common_dbus_handlers);

    /* To avoid unnecessary jitter on startup, allow dbus service tracking
     * and datapipe initialization some time to come up with proper initial
     * state values before forcing broadcasting to dbus */
    if( !common_dbus_initial_id )
        common_dbus_initial_id = g_timeout_add(1000, common_dbus_initial_cb, 0);
}

/** Remove dbus handlers
 */
static void common_dbus_quit(void)
{
    if( common_dbus_initial_id ) {
        g_source_remove(common_dbus_initial_id),
            common_dbus_initial_id = 0;
    }

    mce_dbus_handler_unregister_array(common_dbus_handlers);
}

/* ========================================================================= *
 * DATAPIPE_FUNCTIONS
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * usb_cable_state
 * ------------------------------------------------------------------------- */

/** Callback for handling usb_cable_state_pipe state changes
 *
 * @param data usb_cable_state (as void pointer)
 */
static void common_datapipe_usb_cable_state_cb(gconstpointer data)
{
    usb_cable_state_t prev = usb_cable_state;
    usb_cable_state = GPOINTER_TO_INT(data);

    if( usb_cable_state == prev )
        goto EXIT;

    /* The enumerated states do not have 1:1 string mapping, so to
     * avoid sending duplicate signals also the representation
     * values need to be checked. */
    const char *value_old = usb_cable_state_to_dbus(prev);
    const char *value_new = usb_cable_state_to_dbus(usb_cable_state);

    if( !strcmp(value_old, value_new) )
        goto EXIT;

    mce_log(LL_DEBUG, "usb_cable_state = %s -> %s",
            value_old, value_new);

    common_dbus_send_usb_cable_state(0);

EXIT:
    return;
}

/* ------------------------------------------------------------------------- *
 * charger_state
 * ------------------------------------------------------------------------- */

/** Callback for handling charger_state_pipe state changes
 *
 * @param data charger_state (as void pointer)
 */
static void common_datapipe_charger_state_cb(gconstpointer data)
{
    charger_state_t prev = charger_state;
    charger_state = GPOINTER_TO_INT(data);

    if( charger_state == prev )
        goto EXIT;

    mce_log(LL_DEBUG, "charger_state = %s -> %s",
            charger_state_repr(prev),
            charger_state_repr(charger_state));

    common_dbus_send_charger_state(0);

EXIT:
    return;
}

/* ------------------------------------------------------------------------- *
 * battery_status
 * ------------------------------------------------------------------------- */

/** Callback for handling battery_status_pipe state changes
 *
 * @param data battery_status (as void pointer)
 */
static void common_datapipe_battery_status_cb(gconstpointer data)
{
    battery_status_t prev = battery_status;
    battery_status = GPOINTER_TO_INT(data);

    if( battery_status == prev )
        goto EXIT;

    mce_log(LL_DEBUG, "battery_status = %s -> %s",
            battery_status_repr(prev),
            battery_status_repr(battery_status));

    common_dbus_send_battery_status(0);

EXIT:
    return;
}

/* ------------------------------------------------------------------------- *
 * battery_level
 * ------------------------------------------------------------------------- */

/** Callback for handling battery_level_pipe state changes
 *
 * @param data battery_level (as void pointer)
 */
static void common_datapipe_battery_level_cb(gconstpointer data)
{
    gint prev = battery_level;
    battery_level = GPOINTER_TO_INT(data);

    if( battery_level == prev )
        goto EXIT;

    mce_log(LL_DEBUG, "battery_level = %d -> %d",
            prev, battery_level);

    common_dbus_send_battery_level(0);

EXIT:
    return;
}

/* ------------------------------------------------------------------------- *
 * init/quit
 * ------------------------------------------------------------------------- */

/** Array of datapipe handlers */
static datapipe_handler_t common_datapipe_handlers[] =
{
    // output triggers
    {
        .datapipe  = &usb_cable_state_pipe,
        .output_cb = common_datapipe_usb_cable_state_cb,
    },
    {
        .datapipe  = &charger_state_pipe,
        .output_cb = common_datapipe_charger_state_cb,
    },
    {
        .datapipe  = &battery_status_pipe,
        .output_cb = common_datapipe_battery_status_cb,
    },
    {
        .datapipe  = &battery_level_pipe,
        .output_cb = common_datapipe_battery_level_cb,
    },
    // sentinel
    {
        .datapipe = 0,
    }
};

static datapipe_bindings_t common_datapipe_bindings =
{
    .module   = "common",
    .handlers = common_datapipe_handlers,
};

/** Append triggers/filters to datapipes
 */
static void common_datapipe_init(void)
{
    datapipe_bindings_init(&common_datapipe_bindings);
}

/** Remove triggers/filters from datapipes
 */
static void common_datapipe_quit(void)
{
    datapipe_bindings_quit(&common_datapipe_bindings);
}

/* ========================================================================= *
 * MODULE_INIT_QUIT
 * ========================================================================= */

/** Initialize common functionality
 */
bool
mce_common_init(void)
{
    /* attach to internal state variables */
    common_datapipe_init();

    /* set up dbus message handlers */
    common_dbus_init();

    return true;
}

/** De-initialize common functionality
 */
void
mce_common_quit(void)
{
    /* remove all handlers */
    common_dbus_quit();
    common_datapipe_quit();
}
