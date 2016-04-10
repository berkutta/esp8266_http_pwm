#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"

#include "ip_addr.h"
#include "c_types.h"
#include "espconn.h"

#include "../gdbstub/gdbstub.h"

#include "pwm.h"

#define PWM_0_OUT_IO_MUX PERIPHS_IO_MUX_MTDI_U
#define PWM_0_OUT_IO_NUM 12
#define PWM_0_OUT_IO_FUNC  FUNC_GPIO12

#define PWM_1_OUT_IO_MUX PERIPHS_IO_MUX_MTDO_U
#define PWM_1_OUT_IO_NUM 15
#define PWM_1_OUT_IO_FUNC  FUNC_GPIO15

#define PWM_2_OUT_IO_MUX PERIPHS_IO_MUX_MTCK_U
#define PWM_2_OUT_IO_NUM 13
#define PWM_2_OUT_IO_FUNC  FUNC_GPIO13

uint32 io_info[][3] = {   {PWM_0_OUT_IO_MUX,PWM_0_OUT_IO_FUNC,PWM_0_OUT_IO_NUM},
                          {PWM_1_OUT_IO_MUX,PWM_1_OUT_IO_FUNC,PWM_1_OUT_IO_NUM},
                          {PWM_2_OUT_IO_MUX,PWM_2_OUT_IO_FUNC,PWM_2_OUT_IO_NUM},
                          };

uint32 duty[3] = {600,604,634};

static char s_ReplyFormat[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n<html><body><h1>Hello World!</h1></body></html>";

static char s_Reply[sizeof(s_ReplyFormat) + 20];

static char convert_char_int(char input)
{
	switch(input)
	{
		case '0':
			return 0;
			break;
		case '1':
			return 1;
			break;
		case '2':
			return 2;
			break;
		case '3':
			return 3;
			break;
		case '4':
			return 4;
			break;
		case '5':
			return 5;
			break;
		case '6':
			return 6;
			break;
		case '7':
			return 7;
			break;
		case '8':
			return 8;
			break;
		case '9':
			return 9;
			break;
	}
}

static void httpDisconCb(void *arg)
{
	struct espconn *pConn = (struct espconn*)arg;

	espconn_disconnect(pConn);
}

void httpRecvCb(void *arg, char *pusrdata, unsigned short length)
{
	struct espconn *pConn = (struct espconn*)arg;
	os_sprintf(s_Reply, s_ReplyFormat);
	espconn_sent(pConn, (uint8_t *)s_Reply, strlen(s_Reply));


	if (strstr(pusrdata, "pwm="))
	{
		char* argument = strstr(pusrdata, "pwm=");

		// Get everyting from string where "pwm=" starts
		char* buzzer = argument + 4;

		// Get the 3 digits
		char number[3];
		int value = 0;
		strncpy(number, buzzer, 3);

		// Convert the 3 ASCII coded digits to an standard INT value
		value += convert_char_int(number[0]) * 100;
		value += convert_char_int(number[1]) * 10;
		value += convert_char_int(number[2]);

		// Apply the INT value to the PWM timer
		pwm_set_duty(value, 2);
		pwm_start();
	}

	espconn_disconnect(pConn);
}

static void httpdConnectCb(void *arg)
{
	struct espconn *pConn = (struct espconn*)arg;

	espconn_regist_recvcb(pConn, httpRecvCb);

	espconn_regist_disconcb(pConn, httpDisconCb);
}

static void wifi_init(char *SSID)
{
	struct ip_info info;
	struct softap_config cfg;
	wifi_softap_get_config(&cfg);
	strcpy((char *)cfg.ssid, SSID);
	cfg.ssid_len = strlen((char*)cfg.ssid);
	wifi_softap_set_config_current(&cfg);
	wifi_set_opmode(SOFTAP_MODE);
}

static void http_event_init(void)
{
	static struct espconn httpdConn;
	static esp_tcp httpdTcp;
	httpdConn.type = ESPCONN_TCP;
	httpdConn.state = ESPCONN_NONE;
	httpdTcp.local_port = 80;
	httpdConn.proto.tcp = &httpdTcp;

	espconn_regist_connectcb(&httpdConn, httpdConnectCb);
	espconn_accept(&httpdConn);
	espconn_regist_time(&httpdConn, 10, 0);
}

static void pwm_hw_init(void)
{
	pwm_init(1000, duty,3,io_info);

	// Set duty, the max duty is 8191 which is set in pwm.h
	pwm_set_duty(4000, 0);   // 0~2
	pwm_set_duty(4000, 1);   // 0~2
	pwm_set_duty(0, 2);   // 0~2
	pwm_set_period(500);  // set period; 500ns ->
	pwm_start();
}

void user_init(void)
{
	system_soft_wdt_stop();
	uart_div_modify(0, UART_CLK_FREQ / 115200);
	//gdbstub_init();

	wifi_init("BMARTY_ESP8266_PWM");

	http_event_init();

	pwm_hw_init();
}

