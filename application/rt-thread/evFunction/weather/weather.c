#include "stdlib.h"

#include <rtthread.h>
#include <rtdevice.h>
#include "rtdbg.h"
#include "aic_core.h"

#include "weather.h"
#include "http_request.h"
#include "cJSON.h"

static weather_info_t *weather_parse(uint8_t *data) {
    weather_info_t *info = malloc(sizeof(weather_info_t));
    cJSON *json = cJSON_Parse((const char *)data);
    if (!json) {
        LOG_E("json解析错误");
        return NULL;
    }
    cJSON *results = cJSON_GetObjectItem(json, "results");
    if (results && cJSON_IsArray(results)) {
        cJSON *item = cJSON_GetArrayItem(results, 0);
        if (item) {
            cJSON *location = cJSON_GetObjectItem(item, "location");
            if (location) {
                info->city = strdup(cJSON_GetObjectItem(location, "name")->valuestring);
            }
            cJSON *now = cJSON_GetObjectItem(item, "now");
            if (now) {
                info->weather = strdup(cJSON_GetObjectItem(now, "text")->valuestring);
                info->temperature = atof(cJSON_GetObjectItem(now, "temperature")->valuestring);
                info->code = atoi(cJSON_GetObjectItem(now, "code")->valuestring);
            }
            cJSON *last_update = cJSON_GetObjectItem(item, "last_update");
            if (last_update) {
                info->update_time = strdup(last_update->valuestring);
            }
        }
    }
    cJSON_Delete(json);
    return info;
}

weather_info_t *weather_get(weather_config_t *cfg) {
    weather_info_t *weather_info = malloc(sizeof(weather_info_t));
    char url[256];
    snprintf(url, sizeof(url), "http://api.seniverse.com/v3/weather/now.json?key=%s&location=%s", cfg->api_key, cfg->city);
    uint8_t *weather_response = http_get_request(url);
    weather_info = weather_parse(weather_response);

    return weather_info;
}

void obtain_weather_info(void) {
    weather_config_t weather_config = {
        .api_key = "STMUWUV5jyKzTL4nm",
        .city = "shenzhen",
    };
    weather_info_t *info = NULL;
    info = weather_get(&weather_config);
    if (info) {
        LOG_I("city:%s, weather:%s, code:%d, update_time:%s", info->city, info->weather, info->code, info->update_time);
    } else {
        LOG_E("get info error");

    }
}

MSH_CMD_EXPORT(obtain_weather_info, get_weather);