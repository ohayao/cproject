#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_pwr_mgmt.h"
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "bsp_btn_ble.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_db_discovery.h"
#include "ble_lbs_c.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_scan.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_rtt.h"
#include "SEGGER_RTT.h"

#define LEDBUTTON_BUTTON_PIN BSP_BUTTON_0
#define BUTTON_DETECTION_DELAY APP_TIMER_TICKS(50)
#define APP_BLE_CONN_CFG_TAG 1
#define APP_BLE_OBSERVER_PRIO 3
#define CENTRAL_SCANNING_LED BSP_BOARD_LED_0  //扫描灯
#define CENTRAL_CONNECTED_LED BSP_BOARD_LED_1 //连接灯
#define LEDBUTTON_LED BSP_BOARD_LED_2         //连接状态灯

BLE_LBS_C_DEF(m_ble_lbs_c);
BLE_DB_DISCOVERY_DEF(m_db_disc);
NRF_BLE_SCAN_DEF(m_scan);
NRF_BLE_GATT_DEF(m_gatt);
NRF_BLE_GQ_DEF(m_ble_gatt_queue, NRF_SDH_BLE_CENTRAL_LINK_COUNT, NRF_BLE_GQ_QUEUE_SIZE);

static char const m_target_periph_name[] = "nnnrf52832";

static void idle_state_handle(void)
{
    NRF_LOG_FLUSH();
    nrf_pwr_mgmt_run();
}

// 日志初始化
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    nrf_log_backend_rtt_init();
    SEGGER_RTT_Init();
}

// 时钟初始化
static void timer_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

// 灯初始化
static void leds_init(void)
{
    bsp_board_init(BSP_INIT_LEDS);
}

// 按键回调
static void button_event_handler(uint8_t pin_no, uint8_t button_cation)
{
    ret_code_t err_code;
    switch (pin_no)
    {
    case LEDBUTTON_BUTTON_PIN:
        err_code = ble_lbs_led_status_send(&m_ble_lbs_c, button_cation);
        if (err_code != NRF_SUCCESS && err_code != BLE_ERROR_INVALID_CONN_HANDLE && err_code != NRF_ERROR_INVALID_STATE)
        {
            APP_ERROR_CHECK(err_code);
        }
        if (err_code == NRF_SUCCESS)
        {
            SEGGER_RTT_printf(0, "LBS write led state %d\n", button_cation);
        }
        break;

    default:
        APP_ERROR_HANDLER(pin_no);
        break;
    }
}

// 按键初始化
static void buttons_init(void)
{
    ret_code_t err_code;
    static app_button_cfg_t buttons[] = {
        {LEDBUTTON_BUTTON_PIN, false, BUTTON_PULL, button_event_handler}};
    err_code = app_button_init(buttons, ARRAY_SIZE(buttons), BUTTON_DETECTION_DELAY);
    APP_ERROR_CHECK(err_code);
}

// 电源管理初始化
static void power_management_init(void)
{
    ret_code_t err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

// 开始扫描
static void scan_start(void)
{
    ret_code_t err_code;
    err_code = nrf_ble_scan_start(&m_scan);
    APP_ERROR_CHECK(err_code);

    //更新灯的状态
    bsp_board_led_off(CENTRAL_CONNECTED_LED);
    bsp_board_led_on(CENTRAL_SCANNING_LED);
}

// 蓝牙回调事件
static void ble_evt_handler(ble_evt_t const *p_ble_evt, void *p_context)
{
    ret_code_t err_code;
    // for readability
    ble_gap_evt_t const *p_gav_evt = &p_ble_evt->evt.gap_evt;
    switch (p_ble_evt->header.evt_id)
    {
    case BLE_GAP_EVT_CONNECTED:
        SEGGER_RTT_printf(0, "ble_gap_evt_connected\n");
        err_code = ble_lbs_c_handles_assign(&m_ble_lbs_c, p_gav_evt->conn_handle, NULL);
        APP_ERROR_CHECK(err_code);

        err_code = ble_db_discovery_start(&m_db_disc, p_gav_evt->conn_handle);
        APP_ERROR_CHECK(err_code);

        // 更新灯的状态
        bsp_board_led_on(CENTRAL_CONNECTED_LED);
        bsp_board_led_off(CENTRAL_SCANNING_LED);
        break;
    case BLE_GAP_EVT_DISCONNECTED:
        SEGGER_RTT_printf(0, "ble_gap_evt_disconnected\n");
        scan_start();
        break;
    case BLE_GAP_EVT_TIMEOUT:
        // we have not a specified a timeout for scanning, so only conenction attemps can timeout
        if (p_gav_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
        {
            SEGGER_RTT_printf(0, "ble_gap_evt_timeout connection requet timed out\n");
        }
        break;
    case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        // accept paramters requested by peer.
        err_code = sd_ble_gap_conn_param_update(p_gav_evt->conn_handle, &p_gav_evt->params.conn_param_update_request.conn_params);
        APP_ERROR_CHECK(err_code);
        break;
    case BLE_GATTC_EVT_TIMEOUT:
        // disconnected on gatt client timeout event
        SEGGER_RTT_printf(0, "ble_gattc_evt_timeout gatt client timeout.");

        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;
    case BLE_GATTS_EVT_TIMEOUT:
        // disconnected on gatt Server timeout event
        SEGGER_RTT_printf(0, "ble_gatts_evt_timeout gatt Server timeout.");

        err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
        break;
    default:
        break;
    }
}

// 蓝牙协议栈初始化
static void ble_stack_init(void)
{
    ret_code_t err_code;
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    //使用默认设置配置蓝牙栈
    //获取应用程序RAM的开始地址
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    //启用蓝牙栈
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    //注册蓝牙事件回调函数
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}
// 扫描初始化
static void scan_init(void)
{
    ret_code_t err_code;
    nrf_ble_scan_init_t init_scan;
    memset(&init_scan, 0, sizeof(init_scan));
    init_scan.connect_if_match = true;
    init_scan.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;

    // 开启扫描过滤条件
    err_code = nrf_ble_scan_filters_enable(&m_scan, NRF_BLE_SCAN_NAME_FILTER, false);
    APP_ERROR_CHECK(err_code);

    // 设置扫描过滤条件
    err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_NAME_FILTER, m_target_periph_name);
    APP_ERROR_CHECK(err_code);
}
// 连接参数初始化
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}
// Function for handling database discovery events
// This function is callback functionto handle events from the database discovery module.
// Depending on the UUIDs that are discovered, this function should forward the events to their respectve servces.
static void db_disc_handler(ble_db_discovery_evt_t *p_evt)
{
    ble_lbs_on_db_disc_evt(&m_ble_lbs_c, p_evt);
}
// 设备发现初始化
static void db_discovery_init(void)
{
    ble_db_discovery_init_t db_dscv_init;
    memset(&db_dscv_init, 0, sizeof(db_dscv_init));
    db_dscv_init.evt_handler = db_disc_handler;
    db_dscv_init.p_gatt_queue = &m_ble_gatt_queue;

    ret_code_t err_code = ble_db_discovery_init(&db_dscv_init);
    APP_ERROR_CHECK(err_code);
}
// handles events coming from the LED Button central module
static void lbs_c_evt_handler(ble_lbs_c_t *p_lbs_c, ble_lbs_c_evt_t *p_lbs_c_evt)
{
    switch (p_lbs_c_evt->evt_type)
    {
    case BLE_LBS_C_EVT_DISCOVERY_COMPLETE:
    {
        ret_code_t err_code;
        err_code = ble_lbs_c_handles_assign(&m_ble_lbs_c, p_lbs_c_evt->conn_handle, &p_lbs_c_evt->params.peer_db);
        SEGGER_RTT_printf(0, "Led button services discovered on conn_handle 0x%x.\n", p_lbs_c_evt->conn_handle);

        err_code = app_button_enable();
        APP_ERROR_CHECK(err_code);

        // led button servce discovered. enable notification of button.
        err_code = ble_lbs_c_button_notif_enable(p_lbs_c);
        APP_ERROR_CHECK(err_code);
    }
    break;
    case BLE_LBS_C_EVT_BUTTON_NOTIFICATION:
    {
        SEGGER_RTT_printf(0, "button state changed on peer to 0x%x.\n", p_lbs_c_evt->params.button.button_state);
        if (p_lbs_c_evt->params.button.button_state)
        {
            bsp_board_led_on(LEDBUTTON_LED);
        }
        else
        {
            bsp_board_led_off(LEDBUTTON_LED);
        }
    }
    break;
    default:
        break;
    }
}
// function for handling the led button service client errors.
// nrf_error : Error code containing information about what went wrong.
static void lbs_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}
// lbs_c 初始化
static void lbs_c_init(void)
{
    ret_code_t err_code;
    ble_lbs_c_init_t lbs_c_init_obj;
    lbs_c_init_obj.evt_handler = lbs_c_evt_handler;
    lbs_c_init_obj.p_gatt_queue = &m_ble_gatt_queue;
    lbs_c_init_obj.error_handler = lbs_error_handler;
    err_code = ble_lbs_c_init(&m_ble_lbs_c, &lbs_c_init_obj);
    APP_ERROR_CHECK(err_code);
}

int main(void)
{
    log_init();
    SEGGER_RTT_printf(0, "Log init\n");

    timer_init();
    SEGGER_RTT_printf(0, "app_timer init\n");

    leds_init();
    SEGGER_RTT_printf(0, "leds_init init\n");

    buttons_init();
    SEGGER_RTT_printf(0, "buttons init\n");

    power_management_init();
    SEGGER_RTT_printf(0, "power_mangement init\n");

    ble_stack_init();
    SEGGER_RTT_printf(0, "ble_stack_init init\n");

    scan_init();
    SEGGER_RTT_printf(0, "scan_init init\n");

    gatt_init();
    SEGGER_RTT_printf(0, "gatt_init init\n");

    db_discovery_init();
    SEGGER_RTT_printf(0, "db_discovery_init init\n");

    lbs_c_init();
    SEGGER_RTT_printf(0, "lbs_c_init init\n");

    scan_start();
    SEGGER_RTT_printf(0, "scan_start from main\n");

    // turn oon the led to signal scanning
    bsp_board_led_on(CENTRAL_SCANNING_LED);

    for (;;)
    {
        idle_state_handle();
    }
}
