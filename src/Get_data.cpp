#include <Get_data.h>

byte getGps(int32_t * latitude, int32_t * longitude) {
    unsigned long t = millis() + cfg_prmtrs.timeout*1000;
    long lat, lng;
    while (millis() < t) {
        while (SoftSerial.available() > 0) {
            if (gps.encode(SoftSerial.read())) {
                gps.get_position(&lat, &lng);
                if (lat != TinyGPS::GPS_INVALID_ANGLE && lng != TinyGPS::GPS_INVALID_ANGLE) {
                    *latitude = (int32_t)(lat);
                    *longitude = (int32_t)(lng);
                    return 0;
                }
            }
        }

    }
    *latitude = 0;
    *longitude = 0;
    erreur_led(250,0,0,250,250,0,false);
    return 1;
}

byte gettemp(int16_t * temperature) {
    static bool out = false;
    unsigned long t = millis() + cfg_prmtrs.timeout*1000;
    while (millis() < t) {
        *temperature = (int16_t)(bme280.getTemperature() * 10);
        if (*temperature != -999) {
            break;
        }
    }
    if (*temperature == -999) {
        erreur_led(250,0,0,0,250,0,false);
        if (out){
            return 2;
        }
        out = true;
        return 1;
    }if (*temperature/10 > cfg_prmtrs.t_air_max || *temperature/10 < cfg_prmtrs.t_air_min) {
        erreur_led(0,250,0,250,0,0,true);
    }
    out = false;
    return 0;
}

byte getpressure(uint16_t * pressure) {
    static bool out = false;
    unsigned long t = millis() + cfg_prmtrs.timeout*1000;
    while (millis() < t) {
        *pressure = (uint16_t)(bme280.getPressure()/10);
        if (*pressure != 0) {
            break;
        }
    }
    if (*pressure == 0) {
        erreur_led(250,0,0,0,250,0,false);
        if (out){
            return 2;
        }
        out = true;
        return 1;
    }if (*pressure/10 > cfg_prmtrs.pressure_max || *pressure/10 < cfg_prmtrs.pressure_min) {
        erreur_led(0,250,0,250,0,0,true);
    }
    out = false;
    return 0;
}

byte gethumidity(uint8_t * humidity) {
    static bool out = false;
    unsigned long t = millis() + cfg_prmtrs.timeout*1000;
    while (millis() < t) {
        *humidity = (uint8_t)(bme280.getHumidity() * 2);
        if (*humidity != 101) {
            break;
        }
    }
    if (*humidity == 101) {
        erreur_led(250,0,0,0,250,0,false);
        if (out){
            return 2;
        }
        out = true;
        return 1;
    }if (*humidity/2 > cfg_prmtrs.hygr_max || *humidity < cfg_prmtrs.hygr_min) {
        erreur_led(250,0,0,0,250,0,true);
    }
    out = false;
    return 0;
}

byte get_time_rtc(uint8_t * secondes, uint8_t * minutes, uint8_t * heures, uint8_t * jour, uint8_t * mois, uint8_t * annee) {
    static bool out = false;
    unsigned long t = millis() + cfg_prmtrs.timeout*1000;
    while (millis() < t) {
        if (clock.isStarted()) {
            clock.getTime();
            *secondes = clock.second;
            *minutes = clock.minute;
            *heures = clock.hour;
            *jour = clock.dayOfMonth;
            *mois = clock.month;
            *annee = clock.year;
            out = false;
            return 0;
        }
    }
    erreur_led(250,0,0,0,0,250,false);
    if (out) {    return 2;
    }
    out = true;

    return 1;
}

void get_luminosite(uint16_t * luminosity) {
    *luminosity = analogRead(analogPin);
    if (*luminosity > cfg_prmtrs.lumin_high || *luminosity < cfg_prmtrs.lumin_low) {
       erreur_led(250, 0, 0, 0, 250, 0, true);
    }
}

void erreur_led(uint8_t red1, uint8_t green1, uint8_t blue1, uint8_t red2, uint8_t green2, uint8_t blue2, bool delai) {
    for (int i = 0; i < 2; i++) {
        leds.setColorRGB(0, red1, green1, blue1);
        delay(500);
        leds.setColorRGB(0, red2, green2, blue2);
        if (delai) {
            delay(1000);
        } else {
            delay(500);
        }
    }
}