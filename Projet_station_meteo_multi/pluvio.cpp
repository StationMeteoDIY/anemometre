#include <Arduino.h>
#include <CircularBuffer.h>
#include "mqtt.h"

const byte rainPin              = D5;    // PIN de connexion de capteur.
unsigned int raincnt            = 0;     // Initialisation du compteur.
int idxpluviometre              = 1;     // Index du Device de pluviométrie.
static int jourCourant          = 0;     // Numéro du jour courant.
static float pluieJour          = 0;     // Enregistre le total de pluie tombé dans la journée.

CircularBuffer<float, 3> h_fifo;    // On initialise la taille du buffer pour 6 mn glissantes. (3 = 1 relevé toutes les 2 min)


ICACHE_RAM_ATTR void cntRain() {
  raincnt++;
  Serial.println("Action... ");
}


void init_pluvio() {
  // Initialisation du PIN et création de l'interruption.
  pinMode(rainPin, INPUT_PULLUP);          // Montage PullUp avec Condensateur pour éviter l'éffet rebond.
  attachInterrupt(digitalPinToInterrupt(rainPin), cntRain, FALLING); // CHANGE: Déclenche de HIGH à LOW ou de LOW à HIGH - FALLING : HIGH à LOW - RISING : LOW à HIGH.

}


void getDataPluvio() {
  Serial.println("Execution de la fonction getDataPluvio().");
  float h_total = 0.00;                 // Initialisation de la variable qui contiendra le nbre total d'eau tombée sur 1 heure

  // On calcul le niveau depuis la dernière interogation.
  float pluie = raincnt * 0.2794;       // Nombre d'impulsion multiplié par la hauteur d'eau en mm d'un godet
  raincnt = 0;                          // On réinitialise le compteur.

  // On ajoute le niveau à h_fifo
  h_fifo.push(pluie);  
  Serial.print("Pluie = "); Serial.print(String(pluie)); Serial.println(" mm ");

  // Calcul des précipitations en mm/h, On récupére la hauteur d'eau tombée sur les 6 dernières minutes.
  using index_h = decltype(h_fifo)::index_t;
  for (index_h i = 0; i < h_fifo.size(); i++) {
    h_total += h_fifo[i];
  }
  // On calcul la valeur à envoyer à Domoticz pour une heure.
  h_total = h_total * 10;


  // envoi des données (RAINRATE;RAINCOUNTER)
  if (idxpluviometre != 0) {
    Serial.print("RAINRATE = "); Serial.println(String((int)(round(h_total)))); 
    
    // Calcul de quantité pluie par jour.
    int currentJour = NTP_Jour();
    if(currentJour != jourCourant) {
      jourCourant = currentJour;
      pluieJour = 0;
    }
    pluieJour += pluie;
    Serial.print("RAINCOUNT = "); Serial.println(String((round(pluieJour*100)/100))); 
    
    // Envoi des données à Domoticz
    String svalue = String(round(h_total*100)) + ";" + String(round(pluieJour*100)/100);
    SendData("udevice", idxpluviometre, 0, svalue);
  }
  
}
