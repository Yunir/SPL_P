#include <osapi.h>
#include "driver/uart.h"
#include "user_interface.h"
#include "user_config.h"

// magic numbers
#define DELAY 5000

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

static char macaddr[6];
static ETSTimer WiFiLinker;
static void wifi_check_ip(void *arg);

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(100);
	//structure with STA configuration information
	struct station_config stationConfig;
	char info[150];
	if(wifi_get_opmode() != STATION_MODE)
	{
			wifi_set_opmode(STATION_MODE);
	}
	if(wifi_get_opmode() == STATION_MODE)
	{
		wifi_station_get_config(&stationConfig);
		os_memset(stationConfig.ssid, 0, sizeof(stationConfig.ssid));
		os_memset(stationConfig.password, 0, sizeof(stationConfig.password));
		os_sprintf(stationConfig.ssid, "%s", WIFI_CLIENTSSID);
		os_sprintf(stationConfig.password, "%s", WIFI_CLIENTPASSWORD);
		wifi_station_set_config(&stationConfig);
		wifi_get_macaddr(SOFTAP_IF, macaddr);
	}
	os_timer_disarm(&WiFiLinker);
	os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&WiFiLinker, 1000, 0);
}
