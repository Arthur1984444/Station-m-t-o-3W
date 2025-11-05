#ifndef PROSIT5_PARAM_H
#define PROSIT5_PARAM_H
struct Config_parameters {
    char version[14]; // exemple : "1.2 - lot 003"
    uint8_t log_interval; // en minutes
    uint8_t timeout; // en secondes
    uint16_t file_max_size; // en octets
    bool lumin;
    uint16_t lumin_low;
    uint16_t lumin_high;
    bool t_air;
    int8_t t_air_min;
    int8_t t_air_max;
    bool hygr;
    int8_t hygr_min;
    int8_t hygr_max;
    bool pressure;
    uint16_t pressure_min;
    uint16_t pressure_max;
};
#endif