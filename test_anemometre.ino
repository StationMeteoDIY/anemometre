const byte AnemometrePin        = D6;    // PIN de connexion de l'anémomêtre.
unsigned int anemometreCnt      = 0;     // Initialisation du compteur.
unsigned long lastSendVent      = 0;     // Millis du dernier envoi.
unsigned long t_lastActionVent  = 0;     // enregistre le Time de la dernière intérogation des capteurs vent.
#define INTERO_VENT 120                  // Valeur de l'intervale en secondes entre 2 relevés des capteurs Vent.


ICACHE_RAM_ATTR void cntAnemometre() {
  anemometreCnt++;
  Serial.println("Action... ");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initialisation...");
  
  // Initialisation du PIN et création de l'interruption.
  pinMode(AnemometrePin, INPUT_PULLUP);          // Montage PullUp avec Condensateur pour éviter l'éffet rebond.
  attachInterrupt(digitalPinToInterrupt(AnemometrePin), cntAnemometre, FALLING); // CHANGE: Déclenche de HIGH à LOW ou de LOW à HIGH - FALLING : HIGH à LOW - RISING : LOW à HIGH.

   // On initialise lastSend à maintenant.
   lastSendVent = millis();
}

void loop() {
    // On vérifie si l'intervale d'envoi des informations "Vent" sont atteintes. (120s)
  if (millis() - t_lastActionVent >= INTERO_VENT * 1000) {
    // On met à jour la valeur du dernier traitement à maintenant.
    t_lastActionVent = millis();
    
    // On a atteint l'interval souhaité, on exécute le traitement Vent.
    getSendVitesseVent();
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

  // On calcul la vitesse du vent
  float vitesseVent = (((float)anemometreCnt / 2) / (float)temps_sec) * 2.4;    // Vitesse du vent en km/h = (Nbre de tour / temps de comptage en sec) * 2,4
  vitesseVent       = round(vitesseVent * 10) / 10;               // On tranforme la vitesse du vent à 1 décimale.
  anemometreCnt     = 0;                                          // On réinitialise le compteur à 0.

  // On affiche la vitesse du vent.
  Serial.print("Vitesse du vent = "); Serial.println(vitesseVent,1); 
}