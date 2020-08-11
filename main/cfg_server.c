/* HTTP File Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "http_server.h"
#include "cfg_server.h"

static const char *TAG = "http_server";

/* Handler to redirect incoming GET request for /index.html to / */
static esp_err_t index_html_get_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "301 Permanent Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);  // Response body can be empty
    return ESP_OK;
}

/* Handler to respond with an icon file embedded in flash.
 * Browsers expect to GET website icon at URI /favicon.ico */
static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_end");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
    return ESP_OK;
}

/* Send HTTP response - embedded html page  cfg_html_page.html */
static esp_err_t http_resp_main_html(httpd_req_t *req)
{
    /* Get handle to embedded html page */
    extern const unsigned char html_start[] asm("_binary_cfg_html_page_html_start");
    extern const unsigned char html_end[]   asm("_binary_cfg_html_page_html_end");
    const size_t upload_script_size = (html_end - html_start);
    /* Send embedded html page */
    httpd_resp_send(req, (const char *)html_start, upload_script_size);
    return ESP_OK;
}

#ifndef httpd_resp_sendstr
esp_err_t httpd_resp_sendstr(httpd_req_t *req,char *s) {return httpd_resp_send(req,s,strlen(s));};
#endif

static esp_err_t cfg_set_handler(httpd_req_t *req)// POST request to /config
{
    char buf[10];
    uint8_t newByte1=0;
    uint8_t newByte2=0;
    uint8_t newSelector=0;
    bool Error_in_req=false;
    if (httpd_req_get_hdr_value_str(req, "var1", buf, sizeof(buf)-1)==ESP_OK)
    {
        newByte1=atoi(buf);
    } else Error_in_req=true;
    if (httpd_req_get_hdr_value_str(req, "var2", buf, sizeof(buf)-1)==ESP_OK)
    {
        newByte2=atoi(buf);
    } else Error_in_req=true;
    if (httpd_req_get_hdr_value_str(req, "sel1", buf, sizeof(buf)-1)==ESP_OK)
    {
        newSelector=atoi(buf);
    } else Error_in_req=true;
    // response
    if (Error_in_req==false)
    {
        struct my_config_data * cfg_data = (struct my_config_data *)req->user_ctx;
        cfg_data->Byte1 = newByte1;
        cfg_data->Byte2 = newByte2;
        cfg_data->Selector = newSelector;
        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_set_hdr(req, "Location", "/");
        httpd_resp_sendstr(req, "config uploaded successfully");
    }
    else
    {
        httpd_resp_set_status(req, HTTPD_200);
        httpd_resp_set_hdr(req, "Location", "/");
        httpd_resp_sendstr(req, "config contain error");
    }
    return ESP_OK;
}

static esp_err_t cfg_get_handler(httpd_req_t *req)// GET request to /config
{
    struct my_config_data * cfg_data = (struct my_config_data *)req->user_ctx;
    char buf[40];
    httpd_resp_set_status(req, HTTPD_200);
    itoa(cfg_data->Byte1,   buf,    10);httpd_resp_set_hdr(req, "var1", buf);
    itoa(cfg_data->Byte2,   buf+10, 10);httpd_resp_set_hdr(req, "var2", buf+10);
    itoa(cfg_data->Selector,buf+20, 10);httpd_resp_set_hdr(req, "sel1", buf+20);
    httpd_resp_sendstr(req, "config readed successfully");
    return ESP_OK;
}

static httpd_handle_t server = NULL;
static httpd_config_t config = HTTPD_DEFAULT_CONFIG();

/* Function to stop the http server */
esp_err_t stop_cfg_http_server()
{
	esp_err_t r=httpd_stop(server);
	if (r==ESP_OK) server=NULL;
	return r;
}
/* Function to start the http server */
esp_err_t start_cfg_http_server(struct my_config_data *conf)
{
    ESP_LOGI(TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    };

    /* Register handler for index.html which should redirect to / */
    httpd_uri_t index_html = {
        .uri       = "/index.html",
        .method    = HTTP_GET,
        .handler   = index_html_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &index_html);

    /* Handler for URI used by browsers to get website icon */
    httpd_uri_t favicon_ico = {
        .uri       = "/favicon.ico",
        .method    = HTTP_GET,
        .handler   = favicon_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &favicon_ico);

    /* URI handler for set new config */
    httpd_uri_t set_cfg = {
        .uri       = "/config",
        .method    = HTTP_POST,
        .handler   = cfg_set_handler,
        .user_ctx  = conf
    };
    httpd_register_uri_handler(server, &set_cfg);

    /* URI handler for get current config */
    httpd_uri_t get_cfg = {
    .uri       = "/config",
    .method    = HTTP_GET,
    .handler   = cfg_get_handler,
    .user_ctx  = conf
    };
    httpd_register_uri_handler(server, &get_cfg);

    /* URI handler for read main html page */
    httpd_uri_t main_html = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = http_resp_main_html,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(server, &main_html);

    return ESP_OK;
}