#pragma once

typedef struct {
    char *api_key;
    char *city;
} weather_config_t;

typedef struct {
    char *ip_address;
    char *province;
    char *city;
}location_info_t;

typedef struct {
    char *city;
    char *weather;
    int code;
    float temperature;
    char *update_time;
    location_info_t *location;
}weather_info_t;

weather_info_t *weather_get(weather_config_t *cfg);

