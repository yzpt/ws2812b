#include <FastLED.h>
#define LED_TYPE WS2812B
#define NUM_LEDS 100
#define DATA_PIN 6
#define BRIGHTNESS 180
#define maxValue 159
CRGB leds[NUM_LEDS];

// nombre d'objets (=motif) max 
// pour une strip de 100 leds, nous
// sommes à la limite de la mémoire
// dynamique de l'arduino nano
// avec 6 objets 
int nombreObjets = 6;
int indexObjets = 0;

// attributs des pseudo-objets
int objet_hue[6];
int objet_sat[6];
int objet_valeurs[6][100];
int objet_centre[6];
int objet_spread[6];
int objet_interval[6];
int objet_sens[6];
int objet_enveloppe_centre[6];
unsigned long objet_enveloppe_spread[6];
unsigned long objet_prevRefresh[6];
unsigned long objet_prevCreate[6];

// composition d'une palette de couleurs voulue pour certains thèmes
// ici rouge(0) et bleu(160)
int palette_RB[] = {0, 160};
int palette_centres[] = {0, NUM_LEDS/2,  NUM_LEDS - 1};
int index_palette_RB = 0;

// durée un loop complet (3min30)
int moduloLoopProg = 210;

// gaussienne paramétrée
float gauss(float centre, float spread, float x) {
  return round( maxValue * exp( -(x - centre) * (x - centre) / (spread)) );
}

// enveloppe gaussienne paramétrée
float enveloppeGauss(float centre, float spread, float x) {
  return exp( -(x - centre) * (x - centre) / (spread));
}

// update du tableau à push en fin de loop
void afficherObjet (int hue, int saturation, int centre, int spread) {
  for (int i = 0; i < NUM_LEDS; i++) {
    addLedHSV(i, hue, saturation, gauss(centre, spread, i));
  }
}

// addition du HSV lorsque plusieurs objets se superposent
void addLedHSV(int ledPosition, int hue, int saturation, int value) {
  leds[ledPosition] += CHSV(hue, saturation, value);
}


void setLedHSV(int ledPosition, int hue, int saturation, int value) {
  leds[ledPosition] = CHSV(hue, saturation, value);
}

void calculerTableauValeurs (float centre, float spread, int indexTableau) {
  for (int i = 0; i < NUM_LEDS; i++) {
    int temp = centre;
    objet_valeurs[indexTableau][i] = gauss(centre, spread, (i + temp) % NUM_LEDS);
  }
}

// création d'un objet gaussien, les paramètres d'entrées sont générés semi-aléatoirement en amont:
void creerObjet(int hue, int sat, int centre, int spread, int interval, int sens, int enveloppe_centre, unsigned long enveloppe_spread) {
  objet_hue[indexObjets] = hue;
  objet_sat[indexObjets] = sat;
  objet_centre[indexObjets] = centre;
  objet_spread[indexObjets] = spread;
  objet_interval[indexObjets] = interval;
  objet_sens[indexObjets] = sens;
  objet_prevCreate[indexObjets] = millis();
  objet_enveloppe_centre[indexObjets] = enveloppe_centre;
  objet_enveloppe_spread[indexObjets] = enveloppe_spread;

  calculerTableauValeurs(centre, spread, indexObjets);
  indexObjets = (indexObjets + 1) % nombreObjets;
}

// décalage du tableau de valeurs d'un objet (déplacement de l'objet)
void shiftTableau(int indexObjets) {
  if (millis() - objet_prevRefresh[indexObjets] > objet_interval[indexObjets]) {
    objet_prevRefresh[indexObjets] = millis();
    if (objet_sens[indexObjets] == 1) {
      int temp = objet_valeurs[indexObjets][0];
      for (int i = 0; i < (NUM_LEDS - 1); i++) {
        objet_valeurs[indexObjets][i] = objet_valeurs[indexObjets][(i + 1)];
      }
      objet_valeurs[indexObjets][NUM_LEDS - 1] = temp;
    } else {
      int temp = objet_valeurs[indexObjets][NUM_LEDS - 1];
      for (int i = (NUM_LEDS - 1); i > 0; i--) {
        objet_valeurs[indexObjets][i] = objet_valeurs[indexObjets][(i - 1)];
      }
      objet_valeurs[indexObjets][0] = temp;
    }
  }
}


void setObjet(int indexObjets) {
  float tempEnv = enveloppeGauss(objet_enveloppe_centre[indexObjets], objet_enveloppe_spread[indexObjets], millis() - objet_prevCreate[indexObjets]);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] += CHSV(objet_hue[indexObjets], objet_sat[indexObjets], objet_valeurs[indexObjets][i] * tempEnv);
  }
}

void clr() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
}

void afficherObjets() {
  clr();
  for (int i = 0; i < nombreObjets; i++) {
    shiftTableau(i);
    setObjet(i);
  }
}

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {

// génération aléatoire d'objets gaussiens
// ici selon 4 thèmes en boucle

// paramètrage de la probabilité d'apparition d'un objet
  if (random16() < 375) {

    // thème 1 : objets bleus, 30s
    if (bseconds16()%180 < 30) {
      creerObjet(   160,                            // hue
                    255,                            // sat
                    NUM_LEDS/2,                     // centreGauss
                    random(50, 1500),               // spreadGauss
                    random(2, 20),                  // interval
                    random(2),                      // sens
                    random(3000, 6000),             // enveloppe gauss centre
                    random(1000000, 5000000)        // enveloppe gauss spread
                );

    // thème 2 : objets rouges ou bleus (selon la palette), 60s
    } else if (bseconds16()%180 < 90) {
      creerObjet(   palette_RB[index_palette_RB],   // hue
                    255,                            // sat
                    NUM_LEDS/2,                     // centreGauss
                    random(50, 1500),               // spreadGauss
                    random(2, 20),                  // interval
                    random(2),                      // sens
                    random(3000, 6000),             // enveloppe gauss centre
                    random(1000000, 5000000)        // enveloppe gauss spread
                );
      index_palette_RB = (index_palette_RB + 1) % 2;

    // thème 3 : objets de couleurs aléatoires situées entre le -hue- 224 (rose) et 287 = 32[255] = orange
    // https://github.com/FastLED/FastLED/wiki/FastLED-HSV-Colors
    } else if (bseconds16()%180 < 120){
      creerObjet(   random(224,287)%255,            // hue
                    255,                            // sat
                    NUM_LEDS/2,                     // centreGauss
                    random(50, 1500),               // spreadGauss
                    random(2, 40),                  // interval
                    random(2),                      // sens
                    random(3000, 6000),             // enveloppe gauss centre
                    random(1000000, 5000000)        // enveloppe gauss spread
                );
    
    // couleurs aléatoires sur l'entièreté du spectre
    } else if (bseconds16()%180 < 180){
      creerObjet(   random(255),                    // hue
                    255,                            // sat
                    NUM_LEDS/2,                     // centreGauss
                    random(50, 1500),               // spreadGauss
                    random(2, 40),                  // interval
                    random(2),                      // sens
                    random(3000, 6000),             // enveloppe gauss centre
                    random(1000000, 5000000)        // enveloppe gauss spread
                );
    } 
  }
  
  afficherObjets();
  FastLED.show();
}
