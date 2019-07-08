#include <stdlib.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_ota_ops.h"

#include "cJSON.h"

#include "app.h"
#include "http_task.h"

#define ARRAY_SIZE_OF(a) (sizeof(a) / sizeof(a[0]))

#define APP_PATH "/app"
#define DRIVE_PERIOD_MS 300

static wifi_ap_record_t ap_list[32];
static uint16_t ap_count = ARRAY_SIZE_OF(ap_list);

static const wifi_scan_time_t SCAN_TIME = {
    .passive = 1000,
};

static const wifi_scan_config_t SCAN_CONFIG = {
    .ssid = 0,
    .bssid = 0,
    .channel = 0,
    .show_hidden = true,
    .scan_type = WIFI_SCAN_TYPE_PASSIVE,
    .scan_time = SCAN_TIME,
};

extern const unsigned char index_html_start[]       asm("_binary_index_html_start");
extern const unsigned char index_html_end[]         asm("_binary_index_html_end");
extern const unsigned char main_css_start[]         asm("_binary_main_css_gz_start");
extern const unsigned char main_css_end[]           asm("_binary_main_css_gz_end");
extern const unsigned char main_chunk_js_start[]    asm("_binary_main_chunk_js_gz_start");
extern const unsigned char main_chunk_js_end[]      asm("_binary_main_chunk_js_gz_end");
extern const unsigned char two_chunk_js_start[]     asm("_binary_2_chunk_js_gz_start");
extern const unsigned char two_chunk_js_end[]       asm("_binary_2_chunk_js_gz_end");
extern const unsigned char runtime_main_js_start[]  asm("_binary_runtime_main_js_gz_start");
extern const unsigned char runtime_main_js_end[]    asm("_binary_runtime_main_js_gz_end");

extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_gz_start");
extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_gz_end");

typedef struct static_content {
    const char *path;
    const unsigned char *data_start;
    const unsigned char *data_end;
    const char *content_type;
    bool is_gzip;
} static_content_t;

static static_content_t content_list[] = {
    { "index.htm", index_html_start, index_html_end, "text/html", false, },
    { "main.css", main_css_start, main_css_end, "text/css", true, },
    { "main.chunk.js", main_chunk_js_start, main_chunk_js_end, "text/javascript", true, },
    { "2.chunk.js", two_chunk_js_start, two_chunk_js_end, "text/javascript", true, },
    { "runtime~main.js", runtime_main_js_start, runtime_main_js_end, "text/javascript", true, },
    { "favicon.ico", favicon_ico_start, favicon_ico_end, "image/x-icon", true, },
};

static esp_err_t http_handle_app(httpd_req_t *req)
{
    static_content_t *content = NULL;

    for (uint32_t i = 0; i < ARRAY_SIZE_OF(content_list); i++) {
        if (strstr(req->uri, content_list[i].path) != NULL) {
            content = &(content_list[i]);
        }
    }
    if (content == NULL) {
        content = &(content_list[0]);
    }

    ESP_ERROR_CHECK(httpd_resp_set_type(req, content->content_type));
    if (content->is_gzip) {
        ESP_ERROR_CHECK(httpd_resp_set_hdr(req, "Content-Encoding", "gzip"));
    }
    httpd_resp_send(req,
                    (const char *)content->data_start,
                    content->data_end - content->data_start);

    return ESP_OK;
}

static esp_err_t http_handle_app_redirect(httpd_req_t *req) {
    ESP_ERROR_CHECK(httpd_resp_set_status(req, "301 Moved Permanently"));
    ESP_ERROR_CHECK(httpd_resp_set_hdr(req, "Location", APP_PATH "/"));
    httpd_resp_sendstr(req, "");

    return ESP_OK;
}

static int cmp_ap_list(const void *a, const void *b)
{
    return ((wifi_ap_record_t *)b)->rssi - ((wifi_ap_record_t *)a)->rssi;
}

static void ap_list_get()
{
    ESP_ERROR_CHECK(esp_wifi_scan_start(&SCAN_CONFIG, true));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_list));
    if (ap_count == 0) {
        return;
    }
    qsort(ap_list, ap_count, sizeof(wifi_ap_record_t), cmp_ap_list);
}

static void ap_list_get_json(cJSON *parent)
{
    ap_list_get();

    for (uint8_t i = 0 ; i < ap_count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ssid", (const char *)ap_list[i].ssid);
        cJSON_AddNumberToObject(item, "ch", ap_list[i].primary);
        cJSON_AddNumberToObject(item, "rssi", ap_list[i].rssi);
        cJSON_AddItemToArray(parent, item);
    }
}

static esp_err_t http_handle_api(httpd_req_t *req)
{
    cJSON *json = cJSON_CreateArray();
    ap_list_get_json(json);

    ESP_ERROR_CHECK(httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"));
    ESP_ERROR_CHECK(httpd_resp_set_type(req, "text/json"));

    httpd_resp_sendstr(req, cJSON_PrintUnformatted(json));

    cJSON_Delete(json);

    return ESP_OK;
}

static esp_err_t http_handle_status(httpd_req_t *req)
{
    const esp_partition_t *part_info;
    esp_app_desc_t app_info;
    char elapsed_str[32];
    uint32_t elapsed_sec, day, hour, min, sec;

    part_info = esp_ota_get_running_partition();
    ESP_ERROR_CHECK(esp_ota_get_partition_description(part_info, &app_info));

    elapsed_sec = (uint32_t)(esp_timer_get_time() / 1000000);
    day = elapsed_sec / 86400;
    hour = (elapsed_sec % 86400) / 3600;
    min = (elapsed_sec % 3600) / 60;
    sec = elapsed_sec % 60;

    sprintf(elapsed_str, "%d day(s) %02d:%02d:%02d", day, hour, min, sec);

    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "name", app_info.project_name);
    cJSON_AddStringToObject(json, "version", app_info.version);
    cJSON_AddStringToObject(json, "esp_idf", app_info.idf_ver);
    cJSON_AddStringToObject(json, "compile_date", app_info.date);
    cJSON_AddStringToObject(json, "compile_time", app_info.time);
    cJSON_AddStringToObject(json, "elapsed", elapsed_str);

    httpd_resp_sendstr(req, cJSON_PrintUnformatted(json));

    cJSON_Delete(json);

    return ESP_OK;
}

static httpd_uri_t http_uri_app = {
    .uri       = APP_PATH "*",
    .method    = HTTP_GET,
    .handler   = http_handle_app,
    .user_ctx  = NULL
};


static httpd_uri_t http_uri_app_redirect = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = http_handle_app_redirect,
    .user_ctx  = NULL
};

static httpd_uri_t http_uri_api = {
    .uri       = "/api*",
    .method    = HTTP_GET,
    .handler   = http_handle_api,
    .user_ctx  = NULL
};

static httpd_uri_t http_uri_status = {
    .uri       = "/status*",
    .method    = HTTP_GET,
    .handler   = http_handle_status,
    .user_ctx  = NULL
};


httpd_handle_t http_task_start(void)
{
    ESP_LOGI(TAG, "Start HTTP server.");

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_app));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_app_redirect));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_api));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &http_uri_status));

    return server;
}

void http_task_stop(httpd_handle_t server)
{
    ESP_ERROR_CHECK(httpd_stop(server));
}
