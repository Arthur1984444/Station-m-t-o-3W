# Station météorologique embarquée 3W

## Contexte du projet

Ce projet, nommé **Worldwide Weather Watcher (3W)** vise à réaliser un prototype de station météo embarquée destinée à l’équipement de navires pour la collecte et l'archivage à long terme de données météorologiques. La station sauvegarde ces données sur carte SD et propose plusieurs modes (standard, configuration, maintenance, économique) pour s’adapter aux besoins d’utilisation.

## Présentation de l’équipe

- Arthur Boinier
- Marco Giraud-Thomas
- Mélisande Coutarel
- Lisa Cardinal

---

## Architecture matérielle (CAO)

Prototype en trois parties :

- **Partie inférieure** (« bas champi »)
- **Couvercle inférieur**
- **Couvercle supérieur** (« tête champi »)

La boîte accueille les éléments :
- Arduino UNO
- Grove BME 280 (température, pression, humidité)
- Grove RTC (horloge temps réel)
- SD Card Shield v4
- Basic Shield
- 2 boutons Grove
- Grove GPS
- Grove LED RGB (104020048)
- Grove capteur lumière 2.4×2.4
- 3 sorties externes

Montage : fixation possible au mur ou sur socle, veillez à utiliser des vis M6x(1+) de classe 6H (contour métrique ISO). Le prototype n’est pas totalement étanche : à améliorer dans une version finale future.

<img width="598" height="619" alt="image" src="https://github.com/user-attachments/assets/a9ed7cf6-a8e1-43f4-bdf7-73efbe9ec152" />

## Fonctionnalités principales

- **Acquisition et archivage de données capteurs** (température, pression, humidité, luminosité, GPS)
- **Modes de fonctionnement** :
  - **Standard :** Acquisition automatique des données (toutes les x minutes)
  - **Configuration :** Modification des paramètres via interface série (lors du démarrage)
  - **Maintenance :** affichage des données sur le port série, permet le remplacement de la carte SD en toute sécurité
  - **Économique :** Doublement de l’intervalle de mesure pour économie d’énergie et réduit à une fois sur 2 la récuperration de données GPS

- **Sauvegarde sur carte SD** (format FAT16)
- **Affichage des états/erreurs via LED RGB**
- **Interaction via boutons et port série**
- **Paramétrage complet de l’intervalle, seuils de capteurs, timeout, etc.**
- **Gestion intelligente des erreurs et désactivation automatique en cas d’anomalie répétée**

## Paramètres configurables (exemples)

- **log_interval** : intervalle entre deux mesures (1–1440 min, défaut : 10)
- **file_max_size** : taille max fichier log (1–65535 Ko, défaut : 4096)
- **timeout** : durée max d’attente du capteur (1–3600s, défaut : 30)
- Activation/désactivation de chaque capteur, seuils de valeurs normale/anormale, etc.

Commandes spéciales :  
- `RESET` (remise à zéro), `VERSION` (version du programme), modification directe date/heure/jour.

## Erreurs & dépannage

#### Indications LED

indique les types d’erreurs via combinaisons de couleurs : 

| Situation / Mode de fonctionnement         | Couleurs LED                   | Type d'affichage  |
|--------------------------------------------|-------------------------------|--------------------|
| Mode standard                             | Vert                          | Continue           |
| Mode configuration                        | Bleu                          | Continue           |
| Mode maintenance                          | Orange                        | Continue           |
| Mode économique                           | Jaune                         | Continue           |
| Erreur d’accès à l’horloge RTC            | Rouge et Bleu                 | Alternance (2s)    |
| Erreur d’accès aux données GPS            | Rouge et Jaune                | Alternance (2s)    |
| Erreur d’accès aux données d’un capteur   | Rouge et Vert                 | Alternance (2s)    |
| Donnée capteur incohérente                | Rouge et Vert                 | Alternance (Vert 1s, Rouge 0.5s, 2 cycles) |
| Carte SD pleine                           | Rouge et Blanc                | Alternance (2s)    |
| Erreur accès/écriture carte SD            | Rouge et Blanc                | Alternance (Blanc 1s, Rouge 0.5s, 2 cycles) |


> Pour chaque erreur : alternance deux fois sur une durée totale de 2 secondes (sauf si durée doublée).

#### Lettres d’erreur affichées sur le terminal série

Affiche le code erreur lors de l'exécution du mode maintenance (par le port série) :

| Erreur détectée                                    | Lettre |
|----------------------------------------------------|--------|
| Valeur log_interval hors [0;1440]                  | A      |
| Valeur timeout hors [0;3600]                       | B      |
| Valeur file_max_size hors [0;65535]                | C      |
| Valeur différente de 0 ou 1                        | D      |
| Valeur luminosité hors [0;1023]                    | E      |
| Valeur température hors [-40;85]                   | F      |
| Valeur hygrométrie hors [0;100]                    | G      |
| Valeur pression hors [300;1100]                    | H      |
| Date invalide                                      | I      |
| Heure invalide                                     | J      |
| Jour invalide                                      | K      |
| Numéro invalide                                    | L      |
| Erreur RTC                                         | M      |
| Erreur RTC redondante                              | N      |
| Erreur GPS                                         | O      |
| Erreur GPS redondante                              | P      |
| Erreur température                                 | Q      |
| Erreur température, off                            | R      |
| Erreur pression                                    | S      |
| Erreur pression, off                               | T      |
| Erreur humidité                                    | U      |
| Erreur humidité, off                               | V      |

Remplacement SD : passer en **mode maintenance** avant de retirer la carte. L’emplacement est accessible dans le boîtier (cf. schémas).

---

## Guide utilisateur

### Installation
1. Monter la station sur le support choisi (socle ou mur).
2. Utiliser des vis adaptées (voir plus haut).
3. Effectuer la configuration initiale : appuyer sur le bouton rouge lors du démarrage pour programmer les paramètres sur le port série.

### Utilisation
- Accès aux modes par appui/maintien des boutons :
  
| Mode actuel    | Action à effectuer                   | Passage au mode      |
|----------------|-------------------------------------|----------------------|
| Au démarrage   | Appuyer sur le bouton rouge          | Configuration        |
| Au démarrage   | Ne rien faire (aucun bouton)         | Standard             |
| Standard       | Maintenir bouton rouge 5 secondes    | Maintenance          |
| Standard       | Maintenir bouton vert 5 secondes     | Économique           |
| Maintenance    | Maintenir bouton rouge 5 secondes    | Retour Standard      |
| Économique     | Maintenir bouton rouge 5 secondes    | Retour Standard      |

> Remarque : Le passage d’un mode à l’autre ne se fait que lorsque la station a fini une mesure ou une sauvegarde, pour garantir l’intégrité des données.
  
- Suivi des erreurs/mois via LED et messages série.
- Archivage automatique des fichiers SD à seuil atteint (voir paramètre `file_max_size`).

---

## Développement logiciel

- **Langages principaux** : C, C++ (Arduino)
- **Compilation/flash** via Makefile (prévu pour Windows, portable Linux)
  - `make` pour compiler, `make upload` pour flasher, `make size` pour la taille, `make clean` pour nettoyer.
  - Penser à installer `avr-gcc` et éditer le Makefile selon l’OS/ports utilisés.

- **Optimisation mémoire** activée pour fitting sur Arduino UNO (attention : limites proches de la capacité max)
- **Librairies utilisées principales** : `Arduino.h`, `Wire.h`, `AltSoftSerial.h`, `TinyGPS.h`, `DS1307.h`, `Seeed_BME280.h`, `eeprom.h`, `Fat16.h`, `SdCard.h`, `string.h`

#### Fonctions principales du code
- **sauvegarder()/charger()** : gestion des paramètres en EEPROM
- **crea()/ajout()/check()/exists()/ecrire()/fermeture()** : gestion des fichiers de logs
- **mode_0/1/2** : modes standard, configuration et maintenance
- **boutonInterupt_1/2** : gestion ISR des boutons
- **setup()/loop()** : initialisation et boucle principale

#### Acquisition :  
- `getGps()`, `gettemp()`, `getpressure()`, `gethumidity()`, `get_time_rtc()`, `get_luminosite()` : fonctions associées à chaque capteur
- `erreur_led()` : gestion clignotement LED sur erreurs

---

## Améliorations techniques possibles

- Prototype évolutif : ajout d’autres capteurs (eau, vent, particules fines)
- Renforcement boîtier, étanchéité accrue, optimisation assemblage
- Boîtier à revoir pour plus de praticité et intégration future batterie

## Financement du prototype

Détail des coûts :  
**Total : ~137 €** (Arduino, shields, capteurs, GPS, boutons, LED, supports, PLA imprimante 3D, etc.)
