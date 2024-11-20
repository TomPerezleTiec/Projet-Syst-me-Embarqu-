/////////////////////////Bibliothèque/////////////////

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <ChainableLED.h>
#include <BME280I2C.h>
#include <avr/pgmspace.h>
#include <SoftwareSerial.h>

///////////////////erreur/////////////////////////
uint8_t ereurLum = 0;
uint8_t ereurtemp = 0;
uint8_t ereurhum = 0;
uint8_t ereurpres = 0;
uint8_t ereurSD = 0;

///////////////////def variable de base//////////////////////////
int8_t LUMIN_LOW = 200;
int LUMIN_HIGH = 1023;
int8_t MIN_TEMP_AIR = -5;
int8_t MAX_TEMP_AIR = 30;
int8_t HYGR_MINT = 0;
int8_t HYGR_MAXT = 50;
int PRESSURE_MIN = 450;
int PRESSURE_MAX = 1030;
uint8_t LOG_INTERVALL = 1;
uint16_t FILE_MAX_SIZE = 2048; // 2 ko (par défaut)
int8_t LUMIN = 1;
int8_t TEMP_AIR = 1;
int8_t HYGR = 1;
int8_t PRESSURE = 1;

unsigned long previousMillis = 0;
unsigned long interval = LOG_INTERVALL * 60UL * 1000UL;

//////////////////////////SD////////////////////////////////
const byte PROGMEM chipSelect = 4;
File fichier;

void printSD();
//void closeFileSafely(File& file);
//void creerFichier();
//void incrementerEtCreerFichier();
//void ajouterContenuFichier(File& fichier);
//void copierEtRenommerFichier(File& ancienFichier, const String& nouveauNomFichier);

//////////////////////////////bouton//////////////////////////////////
SoftwareSerial softSerial(2, 3);
const byte PROGMEM redBtn = 3;
const byte PROGMEM greenBtn = 2;

volatile bool redBtnState = false;
unsigned int redBtnTime = 0;

volatile bool greenBtnState = false;
unsigned int greenBtnTime = 0;
void buttonRedInterrupt();
void buttonGreenInterrupt();

////////////////////horloge///////////////////////
RTC_DS1307 rtc;
String date();

//////////////////////LED///////////////////////////////
ChainableLED leds(7, 8, 1);

///////////////capteur lumiere//////////////////
void CLumiere(int8_t n);
int lightSensor = A3;

////////////////capteur bme/////////////////////
BME280I2C bme;

void BMETemp(Stream* client, int8_t n);
void BMEhum(Stream* client, int8_t n);
void BMEpres(Stream* client, int8_t n);

///////////////////mode///////////////////////////////
uint8_t mode = 0;
uint8_t previous_mode = 0;
/////////////////////gps///////////////////
/*
void gps(int8_t n);
// Rx: pin 10 (à connecter au Tx du GPS)
// Tx: pin 11 (à connecter au Rx du GPS)
SoftwareSerial gpsSerial(5, 6);
*/


void setup() {
  Serial.begin(9600);
  pinMode(redBtn, INPUT_PULLUP);
  pinMode(greenBtn, INPUT_PULLUP);
  Wire.begin();


 //////////////////Initialisation Horloge RTC avec erreur/////////////////////////////////

  if (!rtc.begin()) {
    Serial.println(F("Not find RTC!"));
    while (1){  // Boucle infinie si le RTC n'est pas trouvé
      leds.setColorRGB(0, 0, 0, 255);
      delay(500);
      leds.setColorRGB(0, 255, 0, 0);
      delay(500);
    }
  }

  // Si l'initialisation du RTC réussit, ajustez l'heure à la date et à l'heure de compilation
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

 //////////////////Initialisation carte SD avec erreur/////////////////////////////////
  pinMode(chipSelect, OUTPUT);

  if (SD.begin(4)) {
    Serial.println(F("Carte SD initialisée."));
  } else {
    Serial.println(F("Erreur initialisation carte SD."));
    while (1)
    {
      leds.setColorRGB(0, 255, 0, 0);
      delay(500);
      leds.setColorRGB(0, 255, 255, 255);
      delay(1000);
    }
    return;
  }

  attachInterrupt(digitalPinToInterrupt(redBtn), buttonRedInterrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(greenBtn), buttonGreenInterrupt, FALLING);

  while(!Serial) {} // Attendre

 //////////////////Initialisation BME avec erreur/////////////////////////////////

  while(!bme.begin())
  {
    Serial.println(F("Not find BME280"));
    delay(1000);
    while (1)
    {
      leds.setColorRGB(0, 255, 0, 0);
      delay(500);
      leds.setColorRGB(0, 0, 255, 0);
      delay(500);
    }
  }

  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println(F("BME280 success."));
       break;
     case BME280::ChipModel_BMP280:
       Serial.println(F("No Humidity available."));
       break;
     default:
       Serial.println(F("UNKNOWN sensor"));
  }

   //////////////////Initialisation mode au démarrage/////////////////////////////////
  delay(7000);
    if(digitalRead(redBtn) == LOW){
      mode = 2;
    }
    else{
      mode = 0;
    }

    //////////////////Initialisation GPS avec erreur/////////////////////////////////
/*
    gpsSerial.begin(9600);
  // Vérifie si l'initialisation du module GPS a échoué
  if (!gpsSerial) {
    Serial.println("Erreur lors de l'initialisation du module GPS.");
    while (1)
    {
      leds.setColorRGB(0, 255, 0, 0);
      delay(500);
      leds.setColorRGB(0, 255, 255, 0);
      delay(500);
    }
  }

  Serial.println("Module GPS initialisé avec succès !");
*/
}


void loop() {
   //////////////////Changement mode/////////////////////////////////

  if (redBtnState) {
    unsigned int currentTimeRed = millis();
    if (currentTimeRed - redBtnTime >= 5000) {
      if (mode == 0 || mode == 3){
        mode = 1;
        if (redBtnState) {
          redBtnState = false;
        }
      }
      if(mode == 1 && previous_mode == 1){
        mode = 0;
        if (redBtnState) {
          redBtnState = false;
        }
      }
    }
  }
  else if (greenBtnState) {
    unsigned int currentTimeGreen = millis();
    if (currentTimeGreen - greenBtnTime >= 5000) {
      if (mode == 0 || mode == 1){
        mode = 3;
        if (greenBtnState) {
          greenBtnState = false;
        }
      }
      if(mode == 3 && previous_mode == 3){
        mode = 0;
        if (greenBtnState) {
          greenBtnState = false;
        }
      }
    }
  }

  previous_mode = mode;
  int Ncomande = 0;
  unsigned long currentMillis = millis();

 //////////////////Différent mode/////////////////////////////////
  switch (mode) {
    case 0: // Mode standard
    leds.setColorRGB(0, 0, 255, 0);
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        printSD();
      }
      break;
    case 1: // Mode Maintenance
      leds.setColorRGB(0, 255, 128, 0);
      Serial.print(date());
      //gps(0);
      if (LUMIN == 1){
        CLumiere(0);
      }
      if (TEMP_AIR == 1){
        BMETemp(&Serial, 0);
      }
      if (PRESSURE == 1){
        BMEpres(&Serial, 0);
      }
      if (HYGR == 1){
        BMEhum(&Serial, 0);
      }
      fichier.println();

      Serial.println();
      break;
    case 2: // Mode configuration
      leds.setColorRGB(0, 255, 255, 0);
      Serial.println(F("comande disponible"));
      Serial.println(F("1 : LOG_INTERVALL"));
      Serial.println(F("2 : FILE_MAX_SIZE"));
      Serial.println(F("3 : RESET"));
      Serial.println(F("4 : LUMIN"));
      Serial.println(F("5 : LUMIN_LOW"));
      Serial.println(F("6 : LUMIN_HIGH"));
      Serial.println(F("7 : TEMP_AIR"));
      Serial.println(F("8 : MIN_TEMP_AIR"));
      Serial.println(F("9 : MAX_TEMP_AIR"));
      Serial.println(F("10 : HYGR"));
      Serial.println(F("11 : HYGR_MINT"));
      Serial.println(F("12 : HYGR_MAXT"));
      Serial.println(F("13 : PRESSURE"));
      Serial.println(F("14 : PRESSURE_MIN"));
      Serial.println(F("15 : PRESSURE_MAX"));
      Serial.println(F("16 : recap"));
      Serial.println(F("17 : exit"));
      Serial.println(F("Saisir numero commande"));
      while (Serial.available() == 0) {
      }
      Ncomande = Serial.parseInt(); 
      if (Ncomande == 1){
        Serial.print(F("LOG_INTERVALL = "));
        while (Serial.available() == 0 ) {
        }
        LOG_INTERVALL = Serial.parseInt();
        Serial.println();
        interval = LOG_INTERVALL * 60UL * 1000UL;
      }

      if (Ncomande == 2){
        Serial.print(F("FILE_MAX_SIZE = "));
        while (Serial.available() == 0 ) {
        }
        FILE_MAX_SIZE = Serial.parseInt();
        Serial.println();
      }

      if (Ncomande == 3){
        Serial.println(F("RESET"));
        LUMIN_LOW = 200;
        LUMIN_HIGH = 700;
        MIN_TEMP_AIR = -5;
        MAX_TEMP_AIR = 30;
        HYGR_MINT = 0;
        HYGR_MAXT = 50;
        PRESSURE_MIN = 450;
        PRESSURE_MAX = 1030;
        LOG_INTERVALL = 10;
        FILE_MAX_SIZE = 2048;
        LUMIN = 1;
        TEMP_AIR = 1;
        HYGR = 1;
        PRESSURE = 1;
      }

      if (Ncomande == 4){
        Serial.print(F("LUMIN = "));
        while (Serial.available() == 0 ) {
        }
        LUMIN = Serial.parseInt();
        Serial.println();
      }

      if (Ncomande == 5){
        Serial.print(F("LUMIN_LOW = "));
        while (Serial.available() == 0 ) {
        }
        LUMIN_LOW = Serial.parseInt();
        Serial.println();
      }
      if (Ncomande == 6){
        Serial.print(F("LUMIN_HIGH = "));
        while (Serial.available() == 0 ) {
        }
        LUMIN_HIGH = Serial.parseInt();
        Serial.println();
      }
      if (Ncomande == 7){
        Serial.print(F("TEMP_AIR = "));
        while (Serial.available() == 0 ) {
        }
        TEMP_AIR = Serial.parseInt();
        Serial.println();
      }
      if (Ncomande == 8){
        Serial.print(F("MIN_TEMP_AIR = "));
        while (Serial.available() == 0 ){
        }
        MIN_TEMP_AIR = Serial.parseInt();
        Serial.println();
      }
      if (Ncomande == 9){
        Serial.print(F("MAX_TEMP_AIR = "));
        while (Serial.available() == 0 ) {
        }
        MAX_TEMP_AIR = Serial.parseInt();
        Serial.println();
      }
      if (Ncomande == 10){
        Serial.print(F("HYGR = "));
        while (Serial.available() == 0 ) {
        }
        HYGR = Serial.parseInt();
        Serial.println();
      }
      if (Ncomande == 11){
        Serial.print(F("HYGR_MINT = "));
        while (Serial.available() == 0 ) {
        }
        HYGR_MINT = Serial.parseInt();
        Serial.println();
      }
      if (Ncomande == 12){
        Serial.print(F("HYGR_MAXT = "));
        while (Serial.available() == 0 ) {
        }
        HYGR_MAXT = Serial.parseInt();
        Serial.println();
      }
      if (Ncomande == 13){
        Serial.print(F("PRESSURE = "));
        while (Serial.available() == 0 ) {
        }
        PRESSURE = Serial.parseInt();
        Serial.println();
      }

      if (Ncomande == 14){
        Serial.print(F("PRESSURE_MIN = "));
        while (Serial.available() == 0 ) {
        }
        PRESSURE_MIN = Serial.parseInt();
        Serial.println();
      }
      if (Ncomande == 15){
        Serial.print(F("PRESSURE_MAX = "));
        while (Serial.available() == 0 ) {
        }
        PRESSURE_MAX = Serial.parseInt();
        Serial.println();
      }
      if (Ncomande == 16){
        Serial.println(F("recap :"));
        Serial.print(F("LOG_INTERVALL = "));
        Serial.println(LOG_INTERVALL);
        Serial.print(F("FILE_MAX_SIZE = "));
        Serial.println(FILE_MAX_SIZE);
        Serial.print(F("LUMIN = "));
        Serial.println(LUMIN);
        Serial.print(F("LUMIN_LOW = "));
        Serial.println(LUMIN_LOW);
        Serial.print(F("LUMIN_HIGH = "));
        Serial.println(LUMIN_HIGH);
        Serial.print(F("TEMP_AIR = "));
        Serial.println(TEMP_AIR);
        Serial.print(F("MIN_TEMP_AIR = "));
        Serial.println(MIN_TEMP_AIR);
        Serial.print(F("MAX_TEMP_AIR = "));
        Serial.println(MAX_TEMP_AIR);
        Serial.print(F("HYGR = "));
        Serial.println(HYGR);
        Serial.print(F("HYGR_MINT = "));
        Serial.println(HYGR_MINT);
        Serial.print(F("HYGR_MAXT = "));
        Serial.println(HYGR_MAXT);
        Serial.print(F("PRESSURE = "));
        Serial.println(PRESSURE);
        Serial.print(F("PRESSURE_MIN = "));
        Serial.println(PRESSURE_MIN);
        Serial.print(F("PRESSURE_MAX = "));
        Serial.println(PRESSURE_MAX);
      }
      if (Ncomande == 17){
        previous_mode=0;
        mode=0;
      }
      break;
    case 3: //Mode économique
      leds.setColorRGB(0, 0, 0, 255);
      if (currentMillis - previousMillis >= 2 * interval) {
        previousMillis = currentMillis;
        printSD();
      }
      break;
    default:
      break;
  }
}

////////////////////lumiere/////////////////////

void CLumiere(int8_t n) {
  if (LUMIN_LOW <= analogRead(lightSensor) && analogRead(lightSensor) <= LUMIN_HIGH) {
    if (n == 1) {
      fichier.print(" luminosité : ");
      fichier.print(map(analogRead(lightSensor), 0, 1023, 0, 100));
      fichier.print(",");
    } else {
      Serial.print(F(" luminosité : "));
      Serial.print(map(analogRead(lightSensor), 0, 1023, 0, 100));
      Serial.print(F(", "));
    }
  } else {
    if (n == 1) {
      fichier.print(" luminosité : erreur,");
    } else {
      Serial.print(F(" luminosité : erreur,"));
    }
    if (ereurLum==0){
      leds.setColorRGB(0,255,0,0);
      delay(500);
    }else{
      leds.setColorRGB(0,0,255,0);
      delay(500);
    }
    ereurLum = !ereurLum;
  }
}

///////////////////////capteur BME///////////////////////////////////////

///////////////////////capteur BME temperature///////////////////////////////////////

float temp(NAN), hum(NAN), pres(NAN);

void BMETemp(Stream* client, int8_t n) {
  bme.read(pres, temp, hum);
  if (MIN_TEMP_AIR <= temp && temp <= MAX_TEMP_AIR) {
    if (n == 1) {
      fichier.print(" température : ");
      fichier.print(temp);
      fichier.print(",");
    } else {
      Serial.print(F(" température : "));
      Serial.print(temp);
      Serial.print(F(","));
    }
  } else {
    if (n == 1) {
      fichier.print(" température : erreur,");
    } else {
      Serial.print(F(" température : erreur,"));
    }
    if (ereurtemp==0){
      leds.setColorRGB(0,255,0,0);
      delay(500);
    }else{
      leds.setColorRGB(0,0,255,0);
      delay(500);
    }
    ereurtemp = !ereurtemp;
  }
}

///////////////////////capteur BME humidite///////////////////////////////////////

void BMEhum(Stream* client, int8_t n) {
  bme.read(pres, temp, hum);
  if (HYGR_MINT <= hum && hum <= HYGR_MAXT) {
    if (n == 1) {
      fichier.print(" humidité :");
      fichier.print(hum);
      fichier.print(",");
    } else {
      Serial.print(F(" humidité :"));
      Serial.print(hum);
      Serial.print(F(","));
    }
  } else {
    if (n == 1) {
      fichier.print(" humidité : erreur,");
    } else {
      Serial.print(F(" humidité : erreur,"));
    }
    if (ereurhum==0){
      leds.setColorRGB(0,255,0,0);
      delay(500);
    }else{
      leds.setColorRGB(0,0,255,0);
      delay(500);
    }
    ereurhum = !ereurhum;
  }
}

///////////////////////capteur BME pression///////////////////////////////////////

void BMEpres(Stream* client, int8_t n) {
  bme.read(pres, temp, hum);
  if (PRESSURE_MIN <= pres && pres <= PRESSURE_MAX) {
    if (n == 1) {
      fichier.print(" pression : ");
      fichier.print(pres);
      fichier.print(",");
    } else {
      Serial.print(F(" pression : "));
      Serial.print(pres);
      Serial.print(F(","));
    }
  } else {
    if (n == 1) {
      fichier.print(" pression : erreur,");
    } else {
      Serial.print(F(" pression : erreur,"));
    }
    if (ereurpres==0){
      leds.setColorRGB(0,255,0,0);
      delay(500);
    }else{
      leds.setColorRGB(0,0,255,0);
      delay(500);
    }
    ereurpres = !ereurpres;
  }
}




///////////////////////////////////date////////////////////////////////////////////
String date(){
  DateTime now = rtc.now();
  return String(now.day()) + "/" + now.month() + "/"+now.year()+" "+ now.hour()+":"+now.minute();
}

//////////////////////////////////////essai de la fonction print SD complete///////////////////////////////////////////

/*
boucle qui parcoure les fichier de la carte sd 

  si docier actuel == date_0
    docier d'ecriture == docier actuel
  
si docier d'ecriture >= taille choisie 
  boucle qui parcoure les fichier de la carte sd 
    si titre du tocier contien un "_" regarder le chifer apres lui 
    chifre = 1+chifre

    cree un nouveau fichier date_0
sinon 
  ouvrei fichier d'ecriture 
  texte a ecrire
  fermer fichier 
*/


/*

File openRootDirectory() {
  File root = SD.open("/");
  if (!root) {
    Serial.println(F("Erreur lors de l'ouverture du répertoire racine"));
  }
  return root;
}

void closeFileSafely(File& file) {
  if (file) {
    file.close();
  }
}





void creerFichier() {
  DateTime now = rtc.now();
  String nomFichier = String(now.year(), DEC) + String(now.month(), DEC) + String(now.day(), DEC) + "_0.LOG";
  if (File fichier = SD.open(nomFichier, FILE_WRITE)) {
    closeFileSafely(fichier);
    Serial.println(F("Fichier créé avec succès"));
  } else {
    Serial.println(F("Erreur lors de la création du fichier"));
  }
}

void incrementerEtCreerFichier() {
  File root = openRootDirectory();

  while (File entry = root.openNextFile()) {
    if (entry.isDirectory()) {
      continue;
    }

    String nomFichier = entry.name();

    if (nomFichier.endsWith(".LOG")) {
      int indiceUnderscore = nomFichier.indexOf("_");

      if (indiceUnderscore != -1) {
        int chiffre = nomFichier.substring(indiceUnderscore + 1).toInt();
        chiffre++;

        String nouveauNomFichier = nomFichier.substring(0, indiceUnderscore + 1) + String(chiffre) + ".LOG";
        copierEtRenommerFichier(entry, nouveauNomFichier);
      }
    }

    entry.close();
  }

  root.close();
}

void ajouterContenuFichier(File& fichier) {
  if (File fichier = SD.open(fichier.name(), FILE_WRITE)) {
    Serial.println(F("Écriture dans le fichier "));
    fichier.print(date());
    if (LUMIN == 1) {
      CLumiere(1);
    }
    if (TEMP_AIR == 1) {
      BMETemp(&Serial, 1);
    }
    if (PRESSURE == 1) {
      BMEpres(&Serial, 1);
    }
    if (HYGR == 1) {
      BMEhum(&Serial, 1);
    }
    fichier.println();
    closeFileSafely(fichier);
  } else {
    Serial.println(F("Erreur lors de l'ouverture du fichier "));
  }
}

void copierEtRenommerFichier(File& ancienFichier, const String& nouveauNomFichier) {
  File nouveauFichier = SD.open(nouveauNomFichier.c_str(), FILE_WRITE);

  if (nouveauFichier && ancienFichier) {
    while (ancienFichier.available()) {
      nouveauFichier.write(ancienFichier.read());
    }
    closeFileSafely(nouveauFichier);

    SD.remove(ancienFichier.name());
    Serial.println("Le fichier " + String(ancienFichier.name()) + " a été renommé en " + nouveauNomFichier);
  } else {
    Serial.println("Erreur lors de la copie vers le nouveau fichier " + nouveauNomFichier);
    closeFileSafely(nouveauFichier);
  }
}


void printSD() {
  File root = openRootDirectory();
  while (File entry = root.openNextFile()) {
    if (entry.isDirectory()) {
      continue;
    }

    String nomFichier = entry.name();

    if (nomFichier.endsWith(".LOG")) {
      int indiceUnderscore = nomFichier.indexOf("_");

      if (indiceUnderscore != -1) {
        int chiffre = nomFichier.substring(indiceUnderscore + 1).toInt();

        if (chiffre == 0) {
          if (entry.size() >= FILE_MAX_SIZE) {
            incrementerEtCreerFichier();
            creerFichier();
          } else {
            ajouterContenuFichier(entry);
          }
        }
      }
    }

    entry.close();
  }
  root.close();
}


*/

///////////////////////PRINT SD fonctionnelle///////////////////////////////////////

void printSD() {
  
  if (fichier = SD.open("donner.txt", FILE_WRITE)) {
    Serial.println(F("Écriture dans fichier "));
    fichier.print(date());
    //gps(1);
    if (LUMIN == 1) {
      CLumiere(1);
    }
    if (TEMP_AIR == 1) {
      BMETemp(&Serial, 1);
    }
    if (PRESSURE == 1) {
      BMEpres(&Serial, 1);
    }
    if (HYGR == 1) {
      BMEhum(&Serial, 1);
    }
    fichier.println();
    fichier.close();
  } else {
    Serial.println(F("Erreur ouverture fichier "));
    int i = 0;
      while ( i >= 5)
      {
        leds.setColorRGB(0, 255, 0, 0);
        delay(500);
        leds.setColorRGB(0, 255, 255, 255);
        delay(1000);
        i=+1;
      }
  }
}



////////////////////////gps/////////////////
/*
void gps(int8_t n){
  if (gpsSerial.available()) {
    String data = gpsSerial.readStringUntil('\n');
    if (n == 1){
      fichier.print(data);
    }else{
      Serial.print(data);
    }
  }
}
*/




//////////////Interruption bouton//////////////
void buttonRedInterrupt() {
  if (!redBtnState) {
    redBtnState= true;
    redBtnTime= millis();
    if (greenBtnState) {
      greenBtnState= false;
    }
  }
}

void buttonGreenInterrupt() {
  if (!greenBtnState) {
    greenBtnState= true;
    greenBtnTime= millis();
    if (redBtnState) {
      redBtnState= false;
    }
  }
}