#include "espconn.h"
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

typedef enum {
	WIFI_CONNECTING,
	WIFI_CONNECTING_ERROR,
	WIFI_CONNECTED,
	TCP_DISCONNECTED,
	TCP_CONNECTING,
	TCP_CONNECTING_ERROR,
	TCP_CONNECTED,
	TCP_SENDING_DATA_ERROR,
	TCP_SENT_DATA
} tConnState;

static char macaddr[6];
static ETSTimer WiFiLinker;
static tConnState connState = WIFI_CONNECTING;
static void wifi_check_ip(void *arg);
struct espconn Conn;
esp_tcp ConnTcp;
static unsigned char tcpReconCount;

static void ICACHE_FLASH_ATTR platform_reconnect(struct espconn *pespconn)
{
	wifi_check_ip(NULL);
}

static void ICACHE_FLASH_ATTR tcpclient_connect_cb(void *arg)
{
	struct espconn *pespconn = arg;
	tcpReconCount = 0;
	char payload[128];
	espconn_regist_sentcb(pespconn, tcpclient_sent_cb);
	connState = TCP_CONNECTED;
	os_sprintf(payload, MACSTR ",%s", MAC2STR(macaddr), "ESP8266");
	sint8 espsent_status = espconn_sent(pespconn, payload, strlen(payload));
	if(espsent_status == ESPCONN_OK) {
		connState = TCP_SENT_DATA;
	} else {
		connState = TCP_SENDING_DATA_ERROR;
	}
}

static void ICACHE_FLASH_ATTR tcpclient_recon_cb(void *arg, sint8 err)
{
	struct espconn *pespconn = arg;
	connState = TCP_DISCONNECTED;
    if (++tcpReconCount >= 5)
    {
		connState = TCP_CONNECTING_ERROR;
		tcpReconCount = 0;
		os_timer_disarm(&WiFiLinker);
		os_timer_setfn(&WiFiLinker, (os_timer_func_t *)platform_reconnect, pespconn);
		os_timer_arm(&WiFiLinker, 10000, 0);
    }
    else
    {
		os_timer_disarm(&WiFiLinker);
		os_timer_setfn(&WiFiLinker, (os_timer_func_t *)platform_reconnect, pespconn);
		os_timer_arm(&WiFiLinker, 2000, 0);
	}
}

static void ICACHE_FLASH_ATTR tcpclient_discon_cb(void *arg)
{
	struct espconn *pespconn = arg;
	connState = TCP_DISCONNECTED;
	if (pespconn == NULL)
	{
		return;
	}
	os_timer_disarm(&WiFiLinker);
	os_timer_setfn(&WiFiLinker, (os_timer_func_t *)platform_reconnect, pespconn);
	os_timer_arm(&WiFiLinker, 2000, 0);
}

static void ICACHE_FLASH_ATTR send_data()
{
	os_timer_disarm(&WiFiLinker);
	char info[150];
	char tcpserverip[15];
	Conn.proto.tcp = &ConnTcp;
	Conn.type = ESPCONN_TCP;
	Conn.state = ESPCONN_NONE;
	os_sprintf(tcpserverip, "%s", TCPSERVERIP);
	uint32_t ip = ipaddr_addr(tcpserverip);
	os_memcpy(Conn.proto.tcp->remote_ip, &ip, 4);
	Conn.proto.tcp->local_port = espconn_port();
	Conn.proto.tcp->remote_port = TCPSERVERPORT;
	espconn_regist_connectcb(&Conn, tcpclient_connect_cb);
	espconn_regist_reconcb(&Conn, tcpclient_recon_cb);
	espconn_regist_disconcb(&Conn, tcpclient_discon_cb);
	sint8 espcon_status = espconn_connect(&Conn);
	if(espcon_status != ESPCONN_OK) {
		os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
		os_timer_arm(&WiFiLinker, 1000, 0);
	}
}

static void ICACHE_FLASH_ATTR wifi_check_ip(void *arg)
{
	struct ip_info ipConfig;
	os_timer_disarm(&WiFiLinker);
	switch(wifi_station_get_connect_status())
	{
		case STATION_GOT_IP:
			wifi_get_ip_info(STATION_IF, &ipConfig);
			if(ipConfig.ip.addr != 0) {
				connState = WIFI_CONNECTED;
				connState = TCP_CONNECTING;
				send_data();
				return;
			}
			break;
		case STATION_WRONG_PASSWORD:
			connState = WIFI_CONNECTING_ERROR;
			break;
		case STATION_NO_AP_FOUND:
			connState = WIFI_CONNECTING_ERROR;
			break;
		case STATION_CONNECT_FAIL:
			connState = WIFI_CONNECTING_ERROR;
			break;
		default:
			connState = WIFI_CONNECTING;
	}
	os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&WiFiLinker, 1000, 0);
}

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
