const byte AnemometrePin        = D6;    // PIN de connexion de l'anémomêtre.
unsigned int anemometreCnt      = 0;     // Initialisation du compteur.
unsigned long lastSendVent      = 0;     // Millis du dernier envoi (permet de récupérer l'intervale réel, et non la valeur de souhait).
unsigned long t_lastActionVent  = 0;     // enregistre le Time de la dernière intérogation des capteurs vent.
unsigned long t_lastRafaleVent  = 0;     // Enregistre le Time du dernier relevé de Rafale de vent.
unsigned int rafalecnt          = 0;     // Compteur pour le calcul des rafales de vent.
unsigned int anemometreOld      = 0;     // Mise en mémoire du relevé "anemometreCnt" pour calcul de Rafale.
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
  // On reléve la rafale de vent des 5 dernières secondes.
  if (millis() - t_lastRafaleVent >= (5 * 1000)) {
    // On met à jour la valeur du dernier traitement de Rafale de vent à maintenant.
    t_lastRafaleVent = millis();
    
    // On a atteint l'interval souhaité, on exécute le traitement Vent.
    getRafale();
  }
  
  // On vérifie si l'intervale d'envoi des informations "Vent" sont atteintes. (120s)
  if (millis() - t_lastActionVent >= (INTERO_VENT * 1000)) {
    // On met à jour la valeur du dernier traitement de l'anémometre à maintenant.
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

  // On calcul la vitesse du vent.
  float vitesseVent = (((float)anemometreCnt / 2) / (float)temps_sec) * 2.4;    // Vitesse du vent en km/h = (Nbre de tour / temps de comptage en sec) * 2,4
  vitesseVent       = round(vitesseVent * 10) / 10;                             // On tranforme la vitesse du vent à 1 décimale.

  // On calcul la vitesse de la rafale.
  float vitesseRafale = (((float)rafalecnt / 2) / 5) * 2.4;                     // Vitesse du vent en km/h = (Nbre de tour / temps de comptage en sec) * 2,4
  vitesseRafale       = round(vitesseRafale * 10) / 10;                         // On tranforme la vitesse du vent à 1 décimale.

  // On réinitialise les compteurs.
  anemometreCnt     = 0;                                          // Envoi des données, On réinitialise le compteur de vent à 0.
  anemometreOld     = 0;                                          // Envoi des données, On réinitialise le compteur de mémoire à 0.
  rafalecnt         = 0;                                          // Envoi des données, On réinitialise le compteur de Rafale à 0.

  // On affiche la vitesse du vent.
  Serial.print("Vitesse du vent = "); Serial.println(vitesseVent,1); 
  Serial.print("Rafale du vent = "); Serial.println(vitesseRafale,1); 
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