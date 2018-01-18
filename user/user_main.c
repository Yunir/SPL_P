#include "ets_sys.h"
#include "ip_addr.h"
#include "espconn.h"
#include <osapi.h>
#include "driver/uart.h"
#include "user_interface.h"
#include "user_config.h"
#include "driver/ds18b20.h"

// magic numbers
#define DELAY 2000 /* milliseconds */

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
extern int ets_uart_printf(const char *fmt, ...);
int (*console_printf)(const char *fmt, ...) = ets_uart_printf;
char temperature[128];

int ICACHE_FLASH_ATTR ds18b20()
{
	int r, i;
	uint8_t addr[8], data[12];

	ds_init();

	r = ds_search(addr);
	if(r)
	{
		console_printf("Found Device @ %02x %02x %02x %02x %02x %02x %02x %02x\r\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
		if(crc8(addr, 7) != addr[7])
			console_printf( "CRC mismatch, crc=%xd, addr[7]=%xd\r\n", crc8(addr, 7), addr[7]);

		switch(addr[0])
		{
		case 0x10:
			console_printf("Device is DS18S20 family\r\n");
			break;

		case 0x28:
			console_printf("Device is DS18B20 family\r\n");
			break;

		default:
			console_printf("Device is unknown family\r\n");
			return 1;
		}
	}
	else {
		console_printf("No DS18B20 detected\r\n");
		return 1;
	}
	reset();
	select(addr);

	write(DS1820_CONVERT_T, 1);

	os_delay_us(65000);

	console_printf("Scratchpad: ");
	reset();
	select(addr);
	write(DS1820_READ_SCRATCHPAD, 0);

	for(i = 0; i < 9; i++)
	{
		data[i] = read();
		console_printf("%2x ", data[i]);
	}
	console_printf("\r\n");

	int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
	LowByte = data[0];
	HighByte = data[1];
	TReading = (HighByte << 8) + LowByte;
	SignBit = TReading & 0x8000;
	if (SignBit)
		TReading = (TReading ^ 0xffff) + 1;

	Whole = TReading >> 4;
	Fract = (TReading & 0xf) * 100 / 16;

	os_sprintf(temperature, "Temperature: %c%d.%d Celsius\r\n", SignBit ? '-' : '+', Whole, Fract < 10 ? 0 : Fract);
	return r;
}

static void ICACHE_FLASH_ATTR send_temperature(void *arg) {
		ds18b20();
		console_printf(temperature);
		sint8 espsent_status = espconn_sent(&Conn, temperature, strlen(temperature));
		if(espsent_status == ESPCONN_OK) {
			connState = TCP_SENT_DATA;
		} else {
			connState = TCP_SENDING_DATA_ERROR;
		}

}

static void ICACHE_FLASH_ATTR tcpclient_sent_cb(void *arg)
{
	struct espconn *pespconn = arg;
	//espconn_disconnect(pespconn);
}

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
	os_sprintf(payload, MACSTR ",%s", MAC2STR(macaddr), " - ESP8266\r\n");
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
	os_timer_setfn(&WiFiLinker, (os_timer_func_t *)send_temperature, NULL);
	os_timer_arm(&WiFiLinker, DELAY, 1);
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
	os_timer_arm(&WiFiLinker, 2000, 0);
}

void connect_to_wifi(void)
{
	wifi_set_opmode(STATION_MODE);
	struct station_config stconfig;
	wifi_station_disconnect();
	wifi_station_dhcpc_stop();
	if(wifi_station_get_config(&stconfig))
	{
		os_memset(stconfig.ssid, 0, sizeof(stconfig.ssid));
		os_memset(stconfig.password, 0, sizeof(stconfig.password));
		os_sprintf(stconfig.ssid, "%s", WIFI_CLIENTSSID);
		os_sprintf(stconfig.password, "%s", WIFI_CLIENTPASSWORD);
	}
	wifi_station_connect();
	wifi_station_dhcpc_start();
	wifi_station_set_auto_connect(1);
}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000);

	connect_to_wifi();
	wifi_get_macaddr(STATION_IF, macaddr);

	if(wifi_get_phy_mode() != PHY_MODE_11N)
		wifi_set_phy_mode(PHY_MODE_11N);
	if(wifi_station_get_auto_connect() == 0)
		wifi_station_set_auto_connect(1);


	os_timer_disarm(&WiFiLinker);
	os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&WiFiLinker, 1000, 0);
}
