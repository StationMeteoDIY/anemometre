#include <Arduino.h>
#include "mqtt.h"

const byte ventPin              = A0;    // PIN de connexion de capteur girouette.
int idxVent                     = 2;     // Index du Device de la girouette.
const byte AnemometrePin        = D6;    // PIN de connexion de l'anémomêtre.
unsigned int anemometreCnt      = 0;     // Initialisation du compteur.
unsigned long lastSendVent      = 0;     // Millis du dernier envoi (permet de récupérer l'intervale réel, et non la valeur de souhait).
unsigned int rafalecnt          = 0;     // Compteur pour le calcul des rafales de vent.
unsigned int anemometreOld      = 0;     // Mise en mémoire du relevé "anemometreCnt" pour calcul de Rafale.
// Données récupérées dans cette bibliothéque.
String wd = "other";                     // Sens du vent suivant points cardinaux.
int angw  = 0;                           // Sens du vent suivant angle cardinal.
float vitesseVent = 0.0;
float vitesseRafale = 0.0;


ICACHE_RAM_ATTR void cntAnemometre() {
  anemometreCnt++;
  Serial.println("Action... ");
}

void init_vent() {
  // Initialisation.
  Serial.begin(115200);
  Serial.println("Initialisation Vent...");

  // Initialisation du PIN et création de l'interruption.
  analogRead(ventPin);                                                            // Initialisation de la lecture analogique Girouette.
  pinMode(AnemometrePin, INPUT_PULLUP);                                           // Montage PullUp avec Condensateur pour éviter l'éffet rebond.
  attachInterrupt(digitalPinToInterrupt(AnemometrePin), cntAnemometre, FALLING);  // CHANGE: Déclenche de HIGH à LOW ou de LOW à HIGH - FALLING : HIGH à LOW - RISING : LOW à HIGH.

   // On initialise lastSend à maintenant.
   lastSendVent = millis();
}

void getDirVent(){
  Serial.println("Execution de la fonction getDirVent().");
  static String old_wd;
  int sensorValue = analogRead(ventPin);
  
  // Initialisation des varibles utilisées dans cette procédure.
  float dirvent = sensorValue / 3.3 ;
  wd = "other";
  angw  = 0;
  
  if(dirvent > 8 &&  dirvent < 14 ){
    wd = "W";
    angw  = 270;
  }
  if(dirvent > 17 &&  dirvent < 23 ){
    wd = "NW";
    angw  = 315;
  }
  if(dirvent > 28 &&  dirvent < 35 ){
    wd = "N";
    angw  = 0;
  }
  if(dirvent > 56 &&  dirvent < 62 ){
    wd = "SW";
    angw  = 225;
  }
  if(dirvent > 91 &&  dirvent < 99 ){
    wd = "NE";
    angw  = 45;
  }
  if(dirvent > 124 &&  dirvent < 154 ){
    wd = "S";
    angw  = 180;
  }
  if(dirvent > 188 &&  dirvent < 201 ){
    wd = "SE";
    angw  = 135;
  }
  if(dirvent > 224 &&  dirvent < 240 ){
    wd = "E";
    angw  = 90;
  }    

  if(wd == "other") {
    wd = old_wd;
  } else {
    old_wd = wd;
  }
}

void getSendVitesseVent() {
  // On effectue le calcul de la vitesse du vent.
  Serial.println("Execution de la fonction getSendVitesseVent().");
  // On initialise lastSendVent à maintenant.
  int temps_sec = (millis() - lastSendVent) / 1000;
  lastSendVent = millis();
  Serial.print("Temps pour le calcul = "); Serial.print(temps_sec); Serial.println(" sec");
  Serial.print("Nombre de déclenchement = "); Serial.println(anemometreCnt);

  // On calcul la vitesse du vent.
  vitesseVent = (((float)anemometreCnt / 2) / (float)temps_sec) * 2.4;    // Vitesse du vent en km/h = (Nbre de tour / temps de comptage en sec) * 2,4
  // vitesseVent = round(vitesseVent * 10) / 10;                             // On tranforme la vitesse du vent à 1 décimale.

  // On calcul la vitesse de la rafale.
  vitesseRafale = (((float)rafalecnt / 2) / 5) * 2.4;                     // Vitesse du vent en km/h = (Nbre de tour / temps de comptage en sec) * 2,4
  // vitesseRafale = round(vitesseRafale * 10) / 10;                         // On tranforme la vitesse du vent à 1 décimale.

  // On réinitialise les compteurs.
  anemometreCnt     = 0;                                                  // Envoi des données, On réinitialise le compteur de vent à 0.
  anemometreOld     = 0;                                                  // Envoi des données, On réinitialise le compteur de mémoire à 0.
  rafalecnt         = 0;                                                  // Envoi des données, On réinitialise le compteur de Rafale à 0.
}

void getRafale() {
  // Relevé et comparaison du relevé précédent.
  Serial.println("Execution de la fonction getRafale().");
  // On calcul le nombre d'impulsion sur les 5 dernières secondes.
  int compteur = anemometreCnt - anemometreOld;
  // On vérifie si la rafale est supérieure à la précédente.
  Serial.print(compteur); Serial.print(" = "); Serial.print(anemometreCnt); Serial.print(" - "); Serial.println(anemometreOld); 
  // On stock la nouvelle valeur comme étant l'ancienne pour le prochain traitement.
  anemometreOld = anemometreCnt;
  // On vérifie si la rafale est supérieure à la précédente.
  if (compteur > rafalecnt) {
    // La rafale est supérieure, on enregistre l'information.
    rafalecnt = compteur;
    Serial.print("Nouvelle valeur de rafale : "); Serial.println(rafalecnt);
  }
}

void setInfo_vent() {
  // On récupére les données de Vent
  getDirVent();
  getSendVitesseVent();
  
  // envoi des données.
  if (idxVent != 0) {
    // Envoi dans la console de debug.
    Serial.print("Vent venant de : "); Serial.print(wd); 
    Serial.print(" soit à "); Serial.print(String(angw)); Serial.println("° ");
    // On affiche la vitesse du vent.
    Serial.print("Vitesse du vent = "); Serial.println(vitesseVent,1); 
    Serial.print("Rafale du vent = "); Serial.println(vitesseRafale,1); 
    
    // Envoi des données à Domoticz
    String svalue = String(angw) + ";" + wd + ";"+String((vitesseVent/3.6)*10)+";"+String((vitesseRafale/3.6)*10)+";22;24";
    SendData("udevice", idxVent, 0, svalue);
  }
}
