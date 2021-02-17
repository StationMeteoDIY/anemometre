/*
 Station météo à base d'un ESP8266, et envoi des données vers Domoticz via MQTT.
 Création Dominique PAUL.
 Dépot Github : https://github.com/DomoticDIY/.........
 Chaine YouTube du Tuto Vidéo : https://www.youtube.com/c/DomoticDIY
  
 Bibliothéques nécessaires, et version utilisées :
  - pubsubclient v2.7.0 : https://github.com/knolleary/pubsubclient
  - ArduinoJson v6.15.0 : https://github.com/bblanchon/ArduinoJson
  - ESP8266Wifi v1.0.0
  - ArduinoOTA v1.0.0
  - Adafruit unified sensor v 1.1.2
  - Adafruit BME280 v2.0.2
  - Adafruit VEML6070 Library v1.0.4
  - DHT sensor library by Adafruit v1.4.0
  - NTPClient by Fabrice Weinberg v3.2.0
  - CircularBuffer by AgileWare v1.3.3

 Installer le gestionnaire de carte ESP8266 version 2.7.1 
 Si besoin : URL à ajouter pour le Bord manager : http://arduino.esp8266.com/stable/package_esp8266com_index.json
 
 Adaptation pour reconnaissance dans Domoticz :
 Dans le fichier PubSubClient.h : La valeur du paramètre "MQTT_MAX_PACKET_SIZE" doit être augmentée à 512 octets. Cette définition se trouve à la ligne 26 du fichier.
 Sinon cela ne fonctionne pas avec Domoticz
 
 Pour prise en compte du matériel :
 Installer si besoin le Driver USB CH340G : https://wiki.wemos.cc/downloads
 dans Outils -> Type de carte : generic ESP8266 module
  Flash mode 'QIO' (régle générale, suivant votre ESP, si cela ne fonctionne pas, tester un autre mode.
  Flash size : 1M (no SPIFFS)
  Port : Le port COM de votre ESP vu par windows dans le gestionnaire de périphériques.
*/

#include "connect.h"
#include "parametre.h"
#include "wifi.h"
#include "mqtt.h"
#include "ota.h"
// #include "bme280.h"
// #include "veml6070.h"
// #include "dht22.h"
// #include "pluvio.h"
#include "vent.h"

// Variables et constantes utilisateur :
unsigned long t_lastActionCapteur = 0;            // enregistre le Time de la dernière intérogation des capteurs.
#define INTERO_CAPTEURS 60                        // Valeur de l'intervale en seconde entre 2 relevés des capteurs.
unsigned long t_lastActionPluvio = 0;             // enregistre le Time de la dernière intérogation des capteurs Pluviomètrie.
#define INTERO_PLUVIO 120                         // Valeur de l'intervale en secondes entre 2 relevés des capteurs Pluviomètrie.
unsigned long t_lastActionVent = 0;               // enregistre le Time de la dernière intérogation des capteurs vent.
unsigned long t_lastRafaleVent  = 0;              // Enregistre le Time du dernier relevé de Rafale de vent.
#define INTERO_VENT 120                           // Valeur de l'intervale en secondes entre 2 relevés des capteurs Vent.


void setup() {
  Serial.begin(115200);                           // On initialise la vitesse de transmission de la console.
  setup_wifi();                                   // Connexion au Wifi
  NTP_setup();                                    // Connexion au NTP
  initOTA();                                      // Initialisation de l'OTA
  init_connect_mqtt();                            // On défini la connexion MQTT
  init_capteurs();                                // Initialisation des Capteurs de relevés
}

void loop() {
  maj_ota();                                      // On verifie si une mise a jour OTA nous est envoyée. Si OUI, la lib ArduinoOTA se charge de faire la MAJ.
  unsigned long currentMillis = millis();         // On enregistre le Time courant

  // On vérifie si l'intervale de relevé des capteurs est atteint. (60s)
  if (currentMillis - t_lastActionCapteur >= INTERO_CAPTEURS * 1000) {
    // On met à jour la valeur du dernier traitement à maintenant.
    t_lastActionCapteur = currentMillis;
    
    // MQTT connecté, on exécute les traitements
    #ifdef BME280_H
      getDataBME280();    // Relevé des données du BME280, et envoi à Domoticz
    #endif
    #ifdef VEML6070_H
      getDataVEML6070();  // Relevé des données du VEML6070, et envoi à Domoticz
    #endif
    #ifdef DHT22_H
      getDataDHT();       // Relevé des données du DHT, et envoi à Domoticz
    #endif
  }

  // On vérifie si l'intervale d'envoi des informations pluie sont atteintes. (120s)
  if (currentMillis - t_lastActionPluvio >= INTERO_PLUVIO * 1000) {
    // On met à jour la valeur du dernier traitement à maintenant.
    t_lastActionPluvio = currentMillis;
    
    // Si il est actif, on envoi les info de pluviometre.
    #ifdef PLUVIO_H
      getDataPluvio();    // Envoi des données de pluviomètrie à Domoticz
    #endif
  }

  // On vérifie si l'intervale d'envoi des informations "Vent" sont atteintes. (120s)
  if (currentMillis - t_lastActionVent >= INTERO_VENT * 1000) {
    // On met à jour la valeur du dernier traitement à maintenant.
    t_lastActionVent = currentMillis;
    
    // Si il est actif, on envoi les info de vent.
    #ifdef VENT_H
      setInfo_vent();    // Envoi des données de vent à Domoticz
    #endif
  }

  // On reléve la rafale de vent des 5 dernières secondes.
  if (millis() - t_lastRafaleVent >= (5 * 1000)) {
    // On met à jour la valeur du dernier traitement de Rafale de vent à maintenant.
    t_lastRafaleVent = millis();
    
    // S'il est actif, On a atteint l'interval souhaité, on exécute le traitement Vent.
    #ifdef VENT_H
      getRafale();
    #endif
  }
  
}



// INITIALISATION des Capteurs
// ***************************
void init_capteurs() {
  #ifdef BME280_H
    init_bme();             // Initialisation de BME280.
  #endif
  #ifdef VEML6070_H
    init_veml();            // Initialisation du VEML6070. 
  #endif
  #ifdef DHT22_H
    init_dht();             // Initialisation du capteur DHT
  #endif
  #ifdef PLUVIO_H
    init_pluvio();          // Initialisation de l'interruption du capteur de pluviomètrie.
  #endif
  #ifdef VENT_H
    init_vent();            // Initialisation Girouette et anémomêtre.
  #endif
}



 
