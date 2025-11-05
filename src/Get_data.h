#ifndef PROSIT5_GET_DATA_H
#define PROSIT5_GET_DATA_H

#include "DS1307.h"
#include <Seeed_BME280.h>
#include <TinyGPS.h>
#include <ChainableLED.h>
#include <param.h>
#include <AltSoftSerial.h>

extern DS1307 clock;
extern BME280 bme280; // capteur meteo
extern TinyGPS gps; // library GPS
extern AltSoftSerial SoftSerial;
extern const uint8_t analogPin;
extern ChainableLED leds;
extern Config_parameters cfg_prmtrs;


byte getGps(int32_t * latitude, int32_t * longitude);
byte gettemp(int16_t * temperature);
byte getpressure(uint16_t * pressure);
byte gethumidity(uint8_t * humidity);
byte get_time_rtc(uint8_t * secondes, uint8_t * minutes, uint8_t * heures, uint8_t * jour, uint8_t * mois, uint8_t * annee);
void get_luminosite(uint16_t * luminosite);
void erreur_led(uint8_t red1, uint8_t green1, uint8_t blue1, uint8_t red2, uint8_t green2, uint8_t blue2, bool delai);

#endif