#include <osapi.h>
#include "driver/uart.h"

static os_timer_t hello_timer;

static void ICACHE_FLASH_ATTR hello_func(void *arg)
{
	ets_uart_printf("Hello World!\r\n");
}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_timer_setfn(&hello_timer, (os_timer_func_t *)hello_func, (void *)0);
	os_timer_arm(&hello_timer, 5000, 1);
}
