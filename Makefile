# numéro de version
VERSION = 1.0.0

# variables d'environnement (debug, uat, release)
ENV ?= release

# configuration du microcontrôleur
MCU = atmega328p
F_CPU = 16000000UL

# outils de compilation
CC = avr-gcc
CXX = avr-g++
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

# variables de téléversement
PROGRAMMER = arduino
PORT = COM6   # Port série à adapter
BAUD = 115200

# répertoires Arduino
ARDUINO_CORE_DIR = ./arduino_lib/arduino
ARDUINO_VARIANT_DIR = ./arduino_lib/variants/standard
ArduinoLibs = ./arduino_lib/libraries

# Description des options de compilation

# -Os : optimisation pour réduire la taille du code généré (important sur microcontrôleur).
# -Wall : active tous les avertissements du compilateur.
# -mmcu=$(MCU) : cible le microcontrôleur spécifié (ex : atmega328p).
# -DF_CPU=$(F_CPU) : définit la fréquence du CPU (ex : 16 MHz) pour le préprocesseur.
# -DVERSION=\"$(VERSION)\" : définit une macro VERSION avec la valeur de version.
# -DARDUINO : définit la macro ARDUINO (souvent utilisée dans les bibliothèques).
# -flto : active l’optimisation Link Time Optimization (LTO) pour réduire la taille et améliorer les performances.
# -fdata-sections : place chaque variable globale dans une section séparée (utile pour l’optimisation).
# -ffunction-sections : place chaque fonction dans une section séparée (pour supprimer le code inutilisé).
# -fno-exceptions : désactive la gestion des exceptions C++ (réduit la taille, car inutilisé sur Arduino).
# -fno-rtti : désactive l’information de type dynamique C++ (RTTI), inutile sur microcontrôleur.
# -Wl,--gc-sections : option du linker : supprime les sections de code et de données non utilisées (garbage collection).

ifeq ($(ENV),debug)
	CFLAGS   = -g -O0 -Wall -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DDEBUG -DVERSION=\"$(VERSION)\" -DARDUINO
	CXXFLAGS = -g -O0 -Wall -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DDEBUG -DVERSION=\"$(VERSION)\" -DARDUINO
	LDFLAGS  = -Wl,--gc-sections
	MSG = [MODE DEBUG]
else ifeq ($(ENV),uat)
	CFLAGS   = -Os -Wall -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DUAT -DVERSION=\"$(VERSION)\" -DARDUINO -flto
	CXXFLAGS = -Os -Wall -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DUAT -DVERSION=\"$(VERSION)\" -DARDUINO -flto
	LDFLAGS  = -Wl,--gc-sections
	MSG = [MODE UAT]
else
	CFLAGS   = -Os -Wall -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DVERSION=\"$(VERSION)\" -DARDUINO -flto -fdata-sections -ffunction-sections
	CXXFLAGS = -Os -Wall -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DVERSION=\"$(VERSION)\" -DARDUINO -flto -fdata-sections -ffunction-sections -fno-exceptions -fno-rtti
	LDFLAGS  = -Wl,--gc-sections
	MSG = [MODE RELEASE]
endif

# includes pour le préprocesseur
INCLUDES = \
   -I$(ARDUINO_CORE_DIR) \
   -I$(ARDUINO_VARIANT_DIR)  \
   -I./lib/chainableLED-master \
   -I./lib/Grove_BME280-master \
   -I./lib/FAT16 \
   -I./lib/RTC_DS1307-master \
   -I./lib/TinyGPS-master \
   -I$(ArduinoLibs)/Wire/src \
   -I$(ArduinoLibs)/EEPROM/src \
   -I./lib/AltSoftSerial-master \
   -I./lib/AltSoftSerial-master/config \
   -I$(ArduinoLibs)/DS1307/src \
   -I$(ArduinoLibs)/parameter/src \
   -I./src

# Fichiers sources C++
SRC_CPP = $(wildcard src/*.cpp) \
   $(wildcard lib/chainableLED-master/*.cpp) \
   $(wildcard lib/FAT16/*.cpp) \
   $(wildcard lib/Grove_BME280-master/*.cpp) \
   $(wildcard lib/RTC_DS1307-master/*.cpp) \
   $(wildcard lib/TinyGPS-master/*.cpp) \
   $(wildcard lib/AltSoftSerial-master/*.cpp) \
   $(ARDUINO_CORE_DIR)/main.cpp \
   $(ARDUINO_CORE_DIR)/HardwareSerial.cpp \
   $(ARDUINO_CORE_DIR)/HardwareSerial0.cpp \
   $(ARDUINO_CORE_DIR)/Print.cpp \
   $(ARDUINO_CORE_DIR)/WString.cpp \
   $(ARDUINO_CORE_DIR)/Stream.cpp \
   $(ARDUINO_CORE_DIR)/WMath.cpp \
   $(ArduinoLibs)/Wire/src/Wire.cpp

# Fichiers sources C
SRC_C = \
   $(ARDUINO_CORE_DIR)/wiring_digital.c \
   $(ARDUINO_CORE_DIR)/wiring.c \
   $(ARDUINO_CORE_DIR)/WInterrupts.c \
   $(ARDUINO_CORE_DIR)/hooks.c \
   $(ARDUINO_CORE_DIR)/wiring_analog.c \
   $(ARDUINO_CORE_DIR)/wiring_pulse.S \
   $(ARDUINO_CORE_DIR)/wiring_shift.c \
   $(ArduinoLibs)/Wire/src/utility/twi.c

# OBJ fichiers objets
OBJ = $(SRC_CPP:.cpp=.o) $(SRC_C:.c=.o)

# cible par défaut : création du fichier hex
all: $(TARGET).hex
	@echo $(MSG)

# compilation C
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# compilation C++
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# compilation assembleur
%.o: %.S
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# édition des liens (utilisation du linker C++ pour Arduino)
$(TARGET).elf: $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

# génération du fichier .hex
$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# téléversement
upload: $(TARGET).hex
	$(AVRDUDE) -v -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -b $(BAUD) -D -U flash:w:$<:i

# nettoyage des fichiers objets et binaires
clean:
	del /Q *.o *.elf *.hex 2>nul

size: $(TARGET).elf
	avr-size $(TARGET).elf