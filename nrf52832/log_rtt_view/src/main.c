#include <stdio.h>
#include <stdlib.h>
#include "nrf_delay.h"
#include "boards.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_rtt.h"
#include "SEGGER_RTT.h"

static void idle_state_handle(void)
{
    NRF_LOG_FLUSH();
}

int main(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    SEGGER_RTT_Init();
    nrf_log_backend_rtt_init();
    bsp_board_init(BSP_INIT_LEDS);
    SEGGER_RTT_printf(0, "Welcome to use `J-Link RTT Viewer` to view logs\n");
    while (true)
    {
        for (int i = 0; i < LEDS_NUMBER; i++)
        {
            bsp_board_led_invert(i);
            nrf_delay_ms(500);
            SEGGER_RTT_printf(0, "Led invert [%d]\n", i);
            idle_state_handle();
        }
    }
}
