#include "esp_err.h"

#ifndef  ___MY_CFG_HTML_PAGE_SERVER___
#define  ___MY_CFG_HTML_PAGE_SERVER___

struct my_config_data
{
	uint8_t Byte1;
	uint8_t Byte2;
	uint8_t Selector;
};

esp_err_t start_cfg_http_server(struct my_config_data *conf);

esp_err_t stop_cfg_http_server();

#endif