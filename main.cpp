#include <ChainableLED.h>
#include <Wire.h>
#include "DS1307.h" // horloge RTC
#include <Seeed_BME280.h> // capteurs temperature + pression + humidite
#include <TinyGPS.h>
#include <Get_data.h>
#include <eeprom.h>
#include <param.h>
#include <Fat16.h>
#include <SdCard.h>
#include <string.h>
#include <Arduino.h>
#include <AltSoftSerial.h>

Config_parameters cfg_prmtrs; // Initialisation de la struct des paramètres

void sauvegarder() { // sauvegarde dans l'EEPROM (mémoire permanente)
  EEPROM.put(0, cfg_prmtrs); // On écrit la structure entière à l’adresse 0
}

void charger() {
  EEPROM.get(0, cfg_prmtrs); // On lit la structure depuis l’adresse 0
}

const uint8_t chipSelect = 4; // (SD card reader model)
const uint8_t analogPin = 0; // Luminosity captor on A2 port on Grove Shield

AltSoftSerial SoftSerial(8, 9); // Serial already used for serial communication GPS connected on D2 port on Grove Shield

DS1307 clock; // horloge RTC

BME280 bme280; // capteur temperature + pression + humidite

TinyGPS gps; // library GPS

ChainableLED leds(6, 7, 1); // LED

// Carte SD
SdCard card;
Fat16 file;

// changement de mode
volatile unsigned long temps = 0;
uint8_t mode;
volatile uint8_t bouton_appuye = 0;

void crea(char name[]){
  file.open(name, O_CREAT | O_RDWR );
  if (!file.isOpen()) {
    erreur_led(250,250,250,250,0,0,false);
  }
  // close file and force write of all data to the SD card
  file.close();
}

void ajout(char name[],char donnee[130]) {
  file.open(name, O_APPEND | O_WRITE );
  if (!file.isOpen()) {
    erreur_led(250,250,250,250,0,0,true);
  }
  file.write(donnee); // write string from RAM
  file.write("\r\n"); // file.println() would work also
  file.close();
}

uint32_t check(char name[]) {
  /** renvoie la taille du fichier */
  file.open(name, O_RDWR );
  if (!file.isOpen()) {
    erreur_led(250,250,250,250,0,0,false);
  }
  file.close();
  return file.fileSize();
}

bool exists(char name[]) {
  file.open(name, O_RDWR );
  if (!file.isOpen()) {
    return false;
  }
  file.close();
  return true;
}

void ecrire(char name[13], char donnee[]) {
  if (!exists(name)) {
    crea(name);
  }
  if (check(name) > (cfg_prmtrs.file_max_size-110)){
    // Incrémenter le suffixe XX dans le nom
    int xx = (name[6] - '0') * 10 + (name[7] - '0');
    xx++;
    name[6] = '0' + (xx / 10);
    name[7] = '0' + (xx % 10);
    ecrire(name, donnee);

    return;
  }
  ajout(name, donnee);
}

void fermeture() {
  file.close();
}

void mode_0(bool mode3 = false) {
  // Mode standard (ou économique)
  static bool faire_mesure_GPS = true;
  //+luminosité, température, pression, humidité

  char msg_temp[5];

  // température
  if (cfg_prmtrs.t_air) {
    int16_t temperature = 0;
    switch (gettemp(&temperature)) {
      case 1:
        strcpy(msg_temp, "NA");
        break;
      case 2:
        strcpy(msg_temp, "NA"); // 2e erreur de donnée donc capteur en erreur
        cfg_prmtrs.t_air = false;
        break;
      default:
        dtostrf(temperature / 10.0, 4, 1, msg_temp);
        break;
    }
  } else {
    strcpy(msg_temp, "D"); // D pour capteur Désactivé
  }

  char msg_pre[7];
  // pression
  if (cfg_prmtrs.pressure) {
    uint16_t pressure = 0;
    switch (getpressure(&pressure)) {
      case 1:
        strcpy(msg_pre, "NA");
        break;
      case 2:
        strcpy(msg_pre, "NA"); // 2e erreur de donnée donc capteur en erreur
        cfg_prmtrs.t_air = false;
        break;
      default:
        dtostrf(pressure / 10.0, 6, 1, msg_pre);
        break;
    }
  } else {
    strcpy(msg_pre, "D"); // D pour capteur Désactivé
  }

  char msg_hum[6];
  // hygrométrie
  if (cfg_prmtrs.hygr) {
    uint8_t humidite = 0;
    switch (gethumidity(&humidite)) {
      case 1:
        strcpy(msg_hum, "NA");
        break;
      case 2:
        strcpy(msg_hum, "NA"); // 2e erreur de donnée donc capteur en erreur
        cfg_prmtrs.t_air = false;
        break;
      default:
        dtostrf(humidite / 2.0, 5, 1, msg_hum);
        break;
    }
  } else {
    strcpy(msg_hum, "D"); // D pour capteur Désactivé
  }

  char msg_lum[5];
  // luminosité
  if (cfg_prmtrs.lumin) {
    // Luminosite
    uint16_t luminosite = 0;
    get_luminosite(&luminosite);
    dtostrf(luminosite, 4, 0, msg_lum);
  } else {
    strcpy(msg_lum, "D"); // D pour capteur Désactivé
  }

  //Horloge RTC
  uint8_t secondes, minutes, heures, jour, mois, annee;
  get_time_rtc(&secondes, &minutes, &heures, &jour, &mois, &annee);
  char msg_time[20]; // TO DO
  sprintf(msg_time, "%02u/%02u/%04u %02u:%02u:%02u", jour, mois, annee + 2000, heures, minutes, secondes);


  // Pour gérer le GPS dans le mode 3
  //mettre un booléen si on est en mode 2 --> si oui, ne pas faire la mesure GPS 1 fois sur 2
  int32_t latitude = 0, longitude = 0;

  char msg_lat[12];
  char msg_lon[12];

  if (mode3) {
    faire_mesure_GPS = !faire_mesure_GPS;
  }
  if (!mode3 || faire_mesure_GPS) {
    switch (getGps(&latitude, &longitude)) {
      case 1:
        strcpy(msg_lat, "NA");
        strcpy(msg_lon, "NA");
        break;
      default:
        dtostrf(((float)latitude / 1000000.0), 11, 6, msg_lat);
        dtostrf(((float)longitude / 1000000.0), 11, 6, msg_lon);
        break;
    }
  } else {
    strcpy(msg_lat, "D"); // D pour capteur Désactivé
    strcpy(msg_lon, "D"); // D pour capteur Désactivé
  }
  char data_log[130]; // Marge de 20 caractères au max 110 caractères dans data_log
  // enregistrements de données séparer par les ";"
  sprintf(data_log, "%s;%s;%s;%s;%s;%s;%s", msg_time, msg_lat, msg_lon, msg_temp, msg_pre, msg_hum, msg_lum);
  // fonction pour sauvegarder les données dans la carte SD
  char nom_fichier[13];
  // nom du fichier du type AAMMJJ_X.TXT
  sprintf(nom_fichier, "%02u%02u%02u01.TXT", annee % 100, mois, jour);
  ecrire (nom_fichier, data_log);
}

void mode_1() {
  // mode configuration
  uint8_t commande;
  long valeur_parameter; // avec int16_t, le résultat de serial.parseInt() est tronqué pour les valeurs > 32767 ou < -32768

  // Gestion du temps :
  unsigned long start_config = millis(); // stockage du début de la configuration
  uint16_t temps_inactif = 1800; // 30 minutes=1800 en secondes

  while(millis() - start_config < temps_inactif * 1000) { // compte le temps passé
    Serial.println(F("Entrez le numéro correspondant.\nCommandes:\n1) RESET\n2) VERSION\nParamètres modifiables :\n3) log_intervall\n4) timeout\n5) file_max_size\n6) lumin\n7) lumin_low\n8) lumin_high\n9) t_air\n10) t_air_min\n11) t_air_max\n12) hygr\n13) hygr_min\n14) hygr_max\n15) pressure\n16) pressure_min\n17) pressure_max\n18) Date/Heure/Jour de la semaine"));
    while (Serial.available() == 0 && millis() - start_config < temps_inactif * 1000) {
      // En attente de la saisie de l'utilisateur
    }
    commande = Serial.parseInt();
    if (millis() - start_config < temps_inactif * 1000 && commande != 0) { // en fonction de commande
      start_config = millis();// remet le compteur à 0 parce qu'une commande a été saisie
      if (commande == 1 || isnan(cfg_prmtrs.log_interval)) { // si reset est la commande ou si cfg_prmtrs est vide
        // RESET
        strcpy(cfg_prmtrs.version, "1.0 - lot 000"); // Copie la chaîne de caractères directement dans version car c'est un tableau de char (donc non-modifiable après initialisation)
        cfg_prmtrs.log_interval = 10; // en minutes
        cfg_prmtrs.timeout = 30; // en secondes
        cfg_prmtrs.file_max_size = 32000; // en octets
        cfg_prmtrs.lumin = true;
        cfg_prmtrs.lumin_low = 255;
        cfg_prmtrs.lumin_high = 768;
        cfg_prmtrs.t_air = true;
        cfg_prmtrs.t_air_min = -10;
        cfg_prmtrs.t_air_max = 60;
        cfg_prmtrs.hygr = true;
        cfg_prmtrs.hygr_min = 0;
        cfg_prmtrs.hygr_max = 50;
        cfg_prmtrs.pressure = true;
        cfg_prmtrs.pressure_min = 850;
        cfg_prmtrs.pressure_max = 1080;
      }
      else if (commande == 2) {
        // VERSION
        Serial.print(F("Version et lot : "));
        Serial.println((cfg_prmtrs.version));
      }
      else if (commande < 18 && commande > 2) {
        Serial.println(F("Entrez une valeur : "));
        while (Serial.available() == 0 && millis() - start_config < temps_inactif * 1000) { // en fonction de valeur_parameter
          // En attente de la saisie de l'utilisateur
        }
        valeur_parameter = Serial.parseInt();
        switch (commande) {
          case 3: // log_interval
            if (valeur_parameter > 0 && valeur_parameter < 1440) { // max 24h
              cfg_prmtrs.log_interval = (uint8_t)valeur_parameter;
              Serial.print(F("log_interval: "));
              Serial.println(cfg_prmtrs.log_interval);
            }
            else {
              Serial.println(F("A")); // Erreur ! Valeur not in [0;1440]
            }
            break;
          case 4: // timeout
            if (valeur_parameter > 0 && valeur_parameter < 3600) { // max 1h
              cfg_prmtrs.timeout = (uint8_t)valeur_parameter;
              Serial.print(F("timeout: "));
              Serial.println(cfg_prmtrs.timeout);
            }
            else {
              Serial.println(F("B")); // Erreur ! Valeur not in [0;3600]
            }
            break;
          case 5: // file_max_size
            if (valeur_parameter > 0 && valeur_parameter < 65536) {//serial.parseInt return un long mais valeur_parameter est un int16_t
              cfg_prmtrs.file_max_size = (uint16_t)valeur_parameter;
              Serial.print(F("file_max_size: "));
              Serial.println(cfg_prmtrs.file_max_size);
            }
            else {
              Serial.println(F("C")); // Erreur ! Valeur not in [0; 65535]
            }
            break;
          case 6: // lumin
            if (valeur_parameter == 0 || valeur_parameter == 1) {
              cfg_prmtrs.lumin = (bool)valeur_parameter;
              Serial.print(F("lumin: "));
              Serial.println(cfg_prmtrs.lumin);
            }
            else {
              Serial.println(F("D")); // Erreur ! Valeur != 0 ou 1
            }
            break;
          case 7: // lumin_low
            if (valeur_parameter >= 0 && valeur_parameter <= 1023) {
              cfg_prmtrs.lumin_low = (uint16_t)valeur_parameter;
              Serial.print(F("lumin_low: "));
              Serial.println(cfg_prmtrs.lumin_low);
            }
            else {
              Serial.println(F("E")); // Erreur ! Valeur not in [0; 1023]
            }
            break;
          case 8: // lumin_high
            if (valeur_parameter >= 0 && valeur_parameter <= 1023) {
              cfg_prmtrs.lumin_high = (uint16_t)valeur_parameter;
              Serial.print(F("lumin_high: "));
              Serial.println(cfg_prmtrs.lumin_high);
            }
            else {
              Serial.println(F("E")); // Erreur ! Valeur not in [0; 1023]
            }
            break;
          case 9: // t_air
            if (valeur_parameter == 0 || valeur_parameter == 1) {
              cfg_prmtrs.t_air = (bool)valeur_parameter;
              Serial.print(F("t_air: "));
              Serial.println(cfg_prmtrs.t_air);
            }
            else {
              Serial.println(F("D")); // Erreur ! Valeur != 0 ou 1
            }
            break;
          case 10: // t_air_min
            if (valeur_parameter >= -40 && valeur_parameter <= 85) {
              cfg_prmtrs.t_air_min = (int8_t)valeur_parameter;
              Serial.print(F("t_air_min: "));
              Serial.println(cfg_prmtrs.t_air_min);
            }
            else {
              Serial.println(F("F")); // Erreur ! Valeur not in [-40; 85]
            }
            break;
          case 11: // t_air_max
            if (valeur_parameter >= -40 && valeur_parameter <= 85) {
              cfg_prmtrs.t_air_max = (int8_t)valeur_parameter;
              Serial.print(F("t_air_max: "));
              Serial.println(cfg_prmtrs.t_air_max);
            }
            else {
              Serial.println(F("F")); // Erreur ! Valeur not in [-40; 85]
            }
            break;
          case 12: // hygr
            if (valeur_parameter == 0 || valeur_parameter == 1) {
              cfg_prmtrs.hygr = (bool)valeur_parameter;
              Serial.print(F("hygr : "));
              Serial.println(cfg_prmtrs.hygr);
            }
            else {
              Serial.println(F("D")); // Erreur ! Valeur != 0 ou 1
            }
            break;
          case 13: // hygr_min
            if (valeur_parameter >= 0 && valeur_parameter <= 100) {
              cfg_prmtrs.hygr_min = (int8_t)valeur_parameter;
              Serial.print(F("hygr_min: "));
              Serial.println(cfg_prmtrs.hygr_min);
            }
            else {
              Serial.println(F("G")); // Erreur ! Valeur not in [0; 100]
            }
            break;
          case 14: // hygr_max
            if (valeur_parameter >= 0 && valeur_parameter <= 100) {
              cfg_prmtrs.hygr_max = (int8_t)valeur_parameter;
              Serial.print(F("hygr_max: "));
              Serial.println(cfg_prmtrs.hygr_max);
            }
            else {
              Serial.println(F("G")); // Erreur ! Valeur not in [0; 100]
            }
            break;
          case 15: // pressure
            if (valeur_parameter == 0 || valeur_parameter == 1) {
              cfg_prmtrs.pressure = (bool)valeur_parameter;
              Serial.print(F("pressure: "));
              Serial.println(cfg_prmtrs.pressure);
            }
            else {
              Serial.println(F("D")); // Erreur ! Valeur != 0 ou 1
            }
            break;
          case 16: // pressure_min
            if (valeur_parameter >= 300 && valeur_parameter <= 1100) {
              cfg_prmtrs.pressure_min = (uint16_t)valeur_parameter;
              Serial.print(F("pressure_min: "));
              Serial.println(cfg_prmtrs.pressure_min);
            }
            else {
              Serial.println(F("H")); // Erreur ! valeur not in [300;1100]
            }
            break;
          case 17: // pressure_max
            if (valeur_parameter >= 300 && valeur_parameter <= 1100) {
              cfg_prmtrs.pressure_max = (uint16_t)valeur_parameter;
              Serial.print(F("pressure_max: "));
              Serial.println(cfg_prmtrs.pressure_max);
            }
            else {
              Serial.println(F("H")); // Erreur ! Valeur not in [300; 1100]
            }
            break;
        }
      }
      else if (commande == 18) {
        char date_time[9];
        Serial.println(F("Entrez la date au format AAAAMMJJ, l'heure au format HHMMSS ou les 3 premières lettres du jour en anglais"));
        while (Serial.available() == 0 && millis() - start_config < temps_inactif * 1000) {
          // En attente de la saisie de l'utilisateur
        }
        const uint8_t len = Serial.readBytesUntil('\n', date_time, 8);
        if (len == 8 && isdigit(date_time[0])) { // date
          date_time[8] = '\0'; // ajouter le caractère de fin de chaîne
          char tmp[5];
          memcpy(tmp, date_time, 4);
          tmp[4] = '\0';
          uint16_t annee = atoi(tmp);
          memcpy(tmp, date_time + 4, 2);
          tmp[2] = '\0';
          uint8_t mois = atoi(tmp);
          memcpy(tmp, date_time + 6, 2);
          tmp[2] = '\0';
          uint8_t jour = atoi(tmp);

          if (annee >= 2000 && annee <= 2099 && mois >= 1 && mois <= 12 && jour >= 1 && jour <= 31) {
            clock.fillByYMD(annee, mois, jour);
            clock.setTime(); // write time to the RTC chip
            Serial.print(F("Date mise à jour: "));
            Serial.print(jour);
            Serial.print(F("/"));
            Serial.print(mois);
            Serial.print(F("/"));
            Serial.println(annee);
          }
          else {
            Serial.println(F("I")); // Date invalide
          }
        }
        if (len == 6 && isdigit(date_time[0])) { // heure
          date_time[6] = '\0'; // ajouter le caractère de fin de chaîne
          char tmp[3];
          memcpy(tmp, date_time, 2);
          tmp[2] = '\0';
          uint8_t heures = atoi(tmp);
          memcpy(tmp, date_time + 2, 2);
          tmp[2] = '\0';
          uint8_t minutes = atoi(tmp);
          memcpy(tmp, date_time + 4, 2);
          tmp[2] = '\0';
          uint8_t secondes = atoi(tmp);

          if (heures < 24 && minutes < 60 && secondes < 60) {
            clock.fillByHMS(heures, minutes, secondes);
            clock.setTime(); // write time to the RTC chip
            Serial.print(F("Heure mise à jour: "));
            Serial.print(heures);
            Serial.print(F(":"));
            Serial.print(minutes);
            Serial.print(F(":"));
            Serial.println(secondes);
          }
          else {
            Serial.println(F("J")); // Heure invalide
          }
        }
        else if (len == 3) { // jour de la semaine
          date_time[3] = '\0'; // ajouter le caractère de fin de chaîne
          for (uint8_t i = 0; i < 3; i++) date_time[i] = toupper(date_time[i]); // toupper() met en majuscule

          const char* jours[] = {"MON","TUE","WED","THU","FRI","SAT","SUN"};
          bool valide = false;

          for (uint8_t i = 0; i < 7; i++) {
            if (strncmp(date_time, jours[i], 3) == 0) { // strncmp() compare 2 chaînes de caractères, élément par élément
              clock.fillDayOfWeek(i+1);
              clock.setTime();
              Serial.print(F("Jour mis à jour: "));
              Serial.println(jours[i]);
              valide = true;
              break;
            }
          }
          if (!valide) Serial.println(F("K")); // Jour invalide
        }
      }
      else {
        Serial.println(F("L")); // Numéro invalide
      }
    }
  }
  sauvegarder(); // écriture des paramètres dans l'EEPROM
  Serial.println(F("Paramètres sauvegardés"));
}

void mode_2() {
  //mode maintenance
  fermeture(); // arrêt de l'écriture en carte SD pour éviter la corruption des fichiers

  // RTC
  uint8_t secondes, minutes, heures, jour, mois, annee;
  switch (get_time_rtc(&secondes, &minutes, &heures, &jour, &mois, &annee)) {
    case 1:
      Serial.println(F("M")); //Erreur RTC
      break;
    case 2:
      Serial.println(F("N")); //Erreur RTC redondante
      break;
    default:
      Serial.print(F("Heure: "));
      Serial.print(heures);
      Serial.print(F(":"));
      Serial.print(minutes);
      Serial.print(F(":"));
      Serial.print(secondes);
      Serial.print(F(" Date: "));
      Serial.print(jour);
      Serial.print(F("/"));
      Serial.print(mois);
      Serial.print(F("/"));
      Serial.println(annee + 2000);
      break;
  }

  //GPS
  int32_t latitude = 0, longitude = 0;
  switch (getGps(&latitude, &longitude)) {
    case 1:
      Serial.println(F("O")); //Erreur GPS
      break;
    case 2:
      Serial.println(F("P")); //Erreur GPS redondante
      break;
    default:
      Serial.print(F("Latitude: "));
      Serial.println(latitude / 1000000.0, 6);
      Serial.print(F("Longitude: "));
      Serial.println(longitude / 1000000.0, 6);
  }

  // BME280
  if (cfg_prmtrs.t_air) {
    int16_t temperature = 0;
    switch (gettemp(&temperature)) {
      case 1:
        Serial.println(F("Q")); //Erreur temperature
        break;
      case 2:
        Serial.println(F("R")); //Erreur temperature, off
        cfg_prmtrs.t_air = false;
        break;
      default:
        Serial.print(F("Temperature: "));
        Serial.print(temperature / 10.0);
        Serial.println(temperature);
        Serial.println(F(" °C"));
        break;
    }
  }
  if (cfg_prmtrs.pressure) {
    uint16_t pressure = 0;
    switch (getpressure(&pressure)) {
      case 1:
        Serial.println(F("S")); //Erreur pression
        break;
      case 2:
        Serial.println(F("T")); //Erreur pression, off
        cfg_prmtrs.pressure = false;
        break;
      default:
        Serial.print(F("Pressure: "));
        Serial.print(pressure/10.0);
        Serial.println(F(" hPa"));
        break;
    }
  }
  if (cfg_prmtrs.hygr) {
    uint8_t humidite = 0;
    switch (gethumidity(&humidite)) {
      case 1:
        Serial.println(F("U")); //Erreur humidité
        break;
      case 2:
        Serial.println(F("V")); //Erreur humidité, off
        cfg_prmtrs.hygr = false;
        break;
      default:
        Serial.print(F("Humidite: "));
        Serial.print(humidite / 2.0);
        Serial.println(F(" %"));
        break;
    }
  }

  //Luminosité
  if (cfg_prmtrs.lumin) {
    // Luminosite
    uint16_t luminosite = 0;
    get_luminosite(&luminosite);
    Serial.print(F("Luminosite: "));
    Serial.println(luminosite);
  }
}

void boutonInterrupt_1() {
  if (bouton_appuye != 1) {
    // Pression
    temps = millis();
    bouton_appuye = 1;
  } else {
    // Relâchement
    if ((millis() - temps) >= 4000) {
      bouton_appuye = 3; // mode long press bouton 2
    } else {
      bouton_appuye = 0; // short press bouton 2
    }
    temps = 0;
  }
}

void boutonInterrupt_2() {
  if (bouton_appuye != 2) {
    // Pression
    temps = millis();
    bouton_appuye = 2;
  } else {
    // Relâchement
    if ((millis() - temps) >= 4000) {
      bouton_appuye = 4; // mode long press bouton 2
    } else {
      bouton_appuye = 0; // short press bouton 2
    }
    temps = 0;
  }
}

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  leds.init();

  pinMode(2, INPUT_PULLUP); // bouton rouge
  pinMode(3, INPUT_PULLUP); // bouton vert

  SoftSerial.begin(9600); // Open SoftwareSerial for GPS
  SoftSerial.setTimeout(50);

  //Initialize Clock
  clock.begin();
  clock.startClock();

  while (!clock.isStarted()) {
    erreur_led(250,0,0,0,0,250,false);
  }

  if (!bme280.init()) {
    erreur_led(250,0,0,0,250,0,false);
  }

  charger(); // Charger les paramètres de configuration au démarrage
  if (digitalRead(2) == LOW) { // mode config si bouton rouge pressé au démarrage
    leds.setColorRGB(0,250,250,0);
    mode_1();
  }

  // initialize the SD card
  if (!card.init(SPI_FULL_SPEED,4)) {
    erreur_led(250,0,0,250,250,250,true);
  }
  // initialize a FAT16 volume
  if (!Fat16::init(&card)) {
    erreur_led(250,0,0,250,250,250,true);
  }

  attachInterrupt(digitalPinToInterrupt(2), boutonInterrupt_1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), boutonInterrupt_2, CHANGE);

  mode = 0;

  leds.setColorRGB(0, 0, 250, 0); // vert
}

void loop() {

  static unsigned long dernierAppelMode = 0;
  // changement de mode
  if (bouton_appuye == 3 || bouton_appuye == 4) {
    if (bouton_appuye == 4 && mode == 0) {
      mode = 3;
      leds.setColorRGB(0, 0, 0, 250); //bleu
    }
    else if (bouton_appuye == 3 && mode == 0) {
      mode = 2;
      leds.setColorRGB(0, 250, 75, 0); // orange
    }
    else if (bouton_appuye == 3 && (mode == 2 || mode == 3)) {
      mode = 0;
      leds.setColorRGB(0, 0, 250, 0); // vert
    }
    bouton_appuye = 0;
    temps = 0;
  }

  // appels de fonctions selon le mode
  switch (mode) {
  case 0:
    // mode standard
    if (((millis() - dernierAppelMode)/1000/60 >= cfg_prmtrs.log_interval) || (millis() - dernierAppelMode) < 0) {
      dernierAppelMode = millis();
      mode_0(); // appel de la fonction toutes les x minutes
      leds.setColorRGB(0, 0, 250, 0); // vert
    }
    break;
  case 2:
    // mode maintenance
    if (((millis() - dernierAppelMode)/1000/60 >= cfg_prmtrs.log_interval) || (millis() - dernierAppelMode) < 0) {
      dernierAppelMode = millis();
      mode_2(); // appel de la fonction toutes les x minutes
      leds.setColorRGB(0, 250, 75, 0); // orange
    }
    break;
  case 3:
    // mode economie
    if (((millis() - dernierAppelMode)/1000/60 >= cfg_prmtrs.log_interval*2) || (millis() - dernierAppelMode) < 0) {
      dernierAppelMode = millis();
      mode_0(true); // appel de la fonction toutes les x*2 minutes
      leds.setColorRGB(0, 0, 0, 250); //bleu
    }
    break;
  }
}