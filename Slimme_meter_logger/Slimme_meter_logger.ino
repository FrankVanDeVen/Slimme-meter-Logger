/*
Slimme meter logger.

Logt de data van de P1 poort van een slimme meter en slaat die op een micro SD kaart op.

Op de micro SD kaart komt voor iedere maand een nieuw *.CSV bestand te staan.

De software staat om 00.00u het elektra verbruik (laag en normaal tarief) en gas verbruik op.
Vervolgens wordt periodiek (deze tijd is instelbaar in instellingen.h) het opgenomen vermogen
op het moment van loggen (in Watt) of het verbruik tussen de laatste 2 log momenten (in W/h)
opgeslagen. Ook deze keuze is instelbaar in instellingen.h.

Het gegenereerde *.CSV bestand is te openen in bijvoorbeeld Excel of LibreOffice Calc.

De software is geschreven voor een slimme meter die de DSMR 5 standaard hanteert.
Mocht u een andere slimme meter hebben dan kunnen de OBIS scan codes indien noodzakelijk in
het bestand instellingen.h aangepast worden om ook uw slimme meter uit te kunnen lezen.

Als de micro SD kaart niet aanwezig is tijdens een log moment worden de eerste 12 registraties
tijdelijk opgeslagen. Bij het terugplaatsen van de micro SD kaart zullen deze gegevens tijdens
het volgende log moment alsnog op de micro SD kaart gezet worden zodat je niets mist.

De schakeling wordt gevoed vanuit de slimme meter. Een aparte adapter is niet nodig.

Op ESP32-C3 SuperMini module zit een blauwe led die aangaat als de meter logt
(uitgezonderd de 1e keer dat de led aangaat bij aansluiten van de logger op de slimme meter).
Als de led niet brand is ESP32-C3 in Deep-sleep mode zodat het stroomverbruik minimaal is.

Benodigdheden:
- ESP32-C3 SuperMini (micro controller).
- Micro SD kaart module (kleine versie zonder level shifters).
- Weerstand 10K.
- Aansluitkabel met RJ12 stekker (voor het aansluiten op de P1 poort).
- Voor de schakeling is een print ontworpen (de Gerber files zitten bij de download).

Auteur software / hardware: Frank van de Ven.
License: MIT.
*/
//-------------------------------------------------------------------------------------------


// Laden benodigde bibliotheken / bestanden:
 #include "SPI.h"                           // voor communicatie over SPI bus
 #include "SD.h"                            // voor schrijven en lezen van een SD kaartje
 #include "instellingen.h"                  // bevat instellingen Slimme meter + zoek termen

// Instellingen pin nummers:
 #define pin_led 8                          // GPIO pin nr. ledje (brand als ESP32 actief is
 #define pin_P1 2                           // GPIO pin nr. uitgang P1 poort (vraag om data)
 #define pin_RXD2 3                         // GPIO pin nr. uart ingang (RXD2)
 #define pin_TXD2 10                        // GPIO pin nr. uart uitgang (niet gebruikt)
 #define pin_MISO 4                         // GPIO pin nr. SD kaart SPI MISO pin
 #define pin_CLK  5                         // GPIO pin nr. SD kaart SPI CLK pin
 #define pin_MOSI 6                         // GPIO pin nr. SD kaart SPI MOSI pin
 #define pin_CS   7                         // GPIO pin nr. SD kaart SPI CS pin

// Variabelen telegram:
 char dataP1[maxP1];                        // array waar telegram data in komt
 byte lees;                                 // bevat gelezen byte uit telegram
 char regel[50];                            // array waar 1 regel uit telegram in komt
 int nr = 0;                                // teller voor doorlopen telegram
 int nr2 = 0;                               // teller voor doorlopen regel
 byte test = 0;                             // "test" byte kijkt of alle data binnen is

// Variabelen voor de data uit telegram
 char jaar[3];                              // opslag jaar gegevens uit telegram
 char maand[3];                             // opslag maand gegevens uit telegram
 char dag[3];                               // opslag dag gegevens uit telegram
 int uur;                                   // opslag uur gegevens uit telegram (INT)
 int minuut;                                // opslag minuut gegevens uit telegram (INT)
 int sec;                                   // opslag sec gegevens uit telegram (INT)
 unsigned int verbruik_L1;                  // actueel verbruik fase L1
 unsigned int verbruik_L2;                  // actueel verbruik fase L2
 unsigned int verbruik_L3;                  // actueel verbruik fase L3
 unsigned int verbruik_tot;                 // actueel verbruik totaal (L1 + L2 + L3)
 unsigned long normaal;                     // meterstand normaal tarief (in KW) voor punt
 unsigned long normaal_nu;                  // meterstand normaal tarief (in W) achter punt
 RTC_DATA_ATTR unsigned long normaal_old;   // meterstand normaal tarief (in W) achter punt
 unsigned long laag;                        // meterstand laag tarief (in KW) voor punt
 unsigned long laag_nu;                     // meterstand laag tarief (in W) achter punt
 RTC_DATA_ATTR unsigned long laag_old= 1111;// meterstand laag tarief (in W) achter punt
 unsigned long gas;                         // meterstand gas verbruik

// Variabelen SD kaart
 char bestand[15];                          // bestandsnaam op SD kaart
 char meterstand[60];                       // voor opslag meterstand gegevens
 char koptekst[] = "Datum:,Dal (kWh):,Normaal (kWh):,Gas (m³):,"; // tekst in bestand op SD
                
// Variabelen Deep-sLeep
 #define uS_sec 1000000                     // vermenigvuldigingsfactor voor uS naar sec
 int slaap;                                 // tijd die ESP32 slaapt in seconden

// variabele onthouden verbruiksgegevens als SD kaart er niet in zit
 RTC_DATA_ATTR byte tel = 0;                // teller die onthoud hoe vaak je verbruik opslaat
 RTC_DATA_ATTR unsigned int verbruik[12];   // array met verbruiksgegevens

void setup() {     
 Serial.begin(115200);                            // setup seriele monitor
 Serial1.begin(baud, protocol, pin_RXD2, pin_TXD2, inverteer); // setup 2e seriele poort 
 SPI.begin(pin_CLK, pin_MISO, pin_MOSI, pin_CS);  // setup SPI poort voor SD kaart
 pinMode(pin_led, OUTPUT);                        // maak pin_led als output pin
 pinMode(pin_P1, OUTPUT);                         // maak pin_P1 als output pin
 digitalWrite(pin_led,LOW);                       // zet led aan
 delay(2000);                                     // wacht op seriele verbindingen
 if(!SD.begin(pin_CS)){                           // Zit de SD kaart er in?
  lees_P1();                                      // Nee: lees een datablok van P1 poort
  wis_oude_data();                                // wis de eerder opgehaalde data
  analyseer_telegram();                           // haal data regel voor regel uit telegram
  if (meting == 1) {verbruik_Wh();}               // bereken verbruik tussen 2 sample tijden
  sla_verbruik_op();                              // sla verbruik op
 }                    
 else {
  lees_P1();                                      // Ja: lees een datablok van P1 poort
  wis_oude_data();                                // wis de eerder opgehaalde data
  analyseer_telegram();                           // haal data regel voor regel uit telegram
  sprintf(bestand,"/20%-2s%-2sP1.CSV",jaar,maand);// bestandsnaam maken uit telegram datum
  controle_file_aanwezig();                       // controleer of bestand op SD staat.
  controle_kaart_eruit();                         // is kaart eruit geweest? ja: save data 
  controle_12u();                                 // is het 00.00u? ja: datum + meterstanden
  if (meting == 1) {verbruik_Wh();}               // bereken verbruik tussen 2 sample tijden
  save_verbruik();                                // zet huidig verbruik in bestand op SD
 }
 bereken_delay();                                 // bereken de delay tijd voor deep sleep
 delay(1000);                                     // wacht voor zekerheid dat alles klaar is
 digitalWrite(pin_led,HIGH);                      // zet led uit
 esp_deep_sleep_start();                          // ga naar Deep-sleep mode
}
//-------------------------------------------------------------------------------------------

void loop() {}
//===========================================================================================

// bereken het verbruik tussen de laatste en huidige sample tijd en pas "verbruik_tot" aan
void verbruik_Wh() {
 if (laag_old < 1111){                            // ESP32 ingeschakeld bij normaal gebruik?
  unsigned long laag2 = laag_nu;                  // tijdelijke opslag laag tarief
  unsigned long normaal2 = normaal_nu;            // tijdelijke opslag normaal tarief
  if (laag_nu < laag_old) {laag_nu = laag_nu + 1000;} // is er een overflow van laag_nu?
  if (normaal_nu < normaal_old) {normaal_nu = normaal_nu + 1000;} // overflow van normaal_nu
  verbruik_tot = (laag_nu - laag_old) + (normaal_nu - normaal_old); // bereken verbruik_tot
  laag_old = laag2;                               // sla het huidige verbruik op
  normaal_old = normaal2;                         // sla het huidige verbruik op
 }
 if (laag_old == 1112) {                          // is de ESP32 voor de 2e keer opgestart?
  laag_old = laag_nu;                             // zo ja, sla het huidige verbruik op
  normaal_old = normaal_nu;                       // zo ja, sla het huidige verbruik op
  verbruik_tot = 0;                               // registreer een verbruik_tot van 0
 }
 if (laag_old == 1111) {                          // is de ESP32 voor de 1e keer opgestart?
  laag_old++;                                     // zo ja, doe niets, wacht op sample tijd
  verbruik_tot = 0;                               // registreer een verbruik_tot van 0
 } 
}
//-------------------------------------------------------------------------------------------

// kijk of de SD kaart eruit is geweest. Zo ja: zet ontbrekende data alsnog op kaart.
void controle_kaart_eruit() {
 if (tel > 0) {                                   // kijk of er data in het array staat
  File file = SD.open(bestand, FILE_APPEND);      // open het bestand voor aanvullen
  do {
   tel--;                                         // vorige tellerstand
   file.print(verbruik[tel]);                     // schrijf verbruik op SD
   file.print(",");                               // schrijf een komma op SD
  }
  while (tel > 0);                                // ga door totdat alles is weggeschreven
  file.close();                                   // sluit het bestand
 } 
}
//-------------------------------------------------------------------------------------------

// Sla bij minder dan 12 registraties het verbruik op
void sla_verbruik_op() {
 if (tel < 12) {                                  // enkel bij minder dan 12 metingen opslaan
  verbruik[tel] = verbruik_tot;                   // zet totaal verbruik in het array
  tel++;                                          // volgende tellerstand
 }
}
//-------------------------------------------------------------------------------------------

// bereken het aantal seconden tot aan de volgende registratie (instelbaar met "sample")
void bereken_delay() {
 int var2 = minuut * 60 + sec;                    // tijd in sec. vanaf het hele uur
 int var3 = sample * 60;                          // tijd in sec. dat registratie herhaalt
 int var4 = var2 % var3;                          // rest tijd na 1 volledige sample tijd
 int var5 = var3 - var4;                          // tijd die over is tot volgende sample
 slaap = var5 + 20;                               // tel er 20 seconden bij op
 esp_sleep_enable_timer_wakeup(slaap * uS_sec);   // setup deep-sleep tijd
}
//-------------------------------------------------------------------------------------------

// kijk of het 00.00u is. zo ja: nieuwe Excel regel met datum en meterstanden aanmaken
void controle_12u() {
 if ((uur == 0) && (minuut == 0)) {                  // is het 00.00u? 
  File file = SD.open(bestand, FILE_APPEND);         // open het bestand voor aanvullen
  file.print("\n");                                  // print return (naar volgende regel)
  sprintf(meterstand,"%2s-%2s-%2s,%lu,%lu,%lu,",dag,maand,jaar,laag,normaal,gas);
  file.print(meterstand);                            // schrijf datum en meterstand op SD
  file.close();                                      // sluit het bestand
 }
}
//-------------------------------------------------------------------------------------------

// controleer of bestand op SD staat. Zo niet, maak deze aan en vul hem met data.
void controle_file_aanwezig(){            
 File file = SD.open(bestand);                     // probeer het bestand te openen
  if (!file) {                                     // is openen gelukt? nee:
   File file = SD.open(bestand, FILE_WRITE);       // maak een nieuw bestand
   file.print(koptekst);                           // zet hier de koptekst in
   for (int uren = 0; uren < 24; uren++) {         // loep voor uren
    for (int minuten = 0; minuten <60; minuten = minuten + sample) { // loep voor minuten
     if (uren < 10) {file.print("0");}             // indien maar 1 cijfer print een "0"
     file.print(uren);                             // print uren in kopregel bestand
     file.print(":");                              // print een : in kopregel bestand
     if (minuten < 10) {file.print("0");}          // indien maar 1 cijfer print een "0"
     file.print(minuten);                          // print minuten in kopregel bestand
      if (uren + minuten == 0) {                   // kijk of de cel inhoud 00.00 is (1e cel)
       if (meting == 0)                            // kijk wat voor meting je wilt
        {file.print(" (W)");} else                 // bij meting 0 print de eenheid "(W)"
        {file.print(" (Wh)");}                     // bij meting 1 print de eenheid "(Wh)"
      } 
     file.print(",");                              // print een komma als scheiding cellen
    }
   }
   if (uur + minuut > 0) {                         // kijk of het NIET 00.00u is. 
    file.print("\n");                              // print return (naar volgende regel)
    sprintf(meterstand,"%2s-%2s-%2s,,,,",dag,maand,jaar); // stel datum tijd komma's samen
    file.print(meterstand);                        // schrijf datum, tijd, komma's op SD
    int aantal = ((minuut+sample-1) / sample) + (uur * (60 / sample)); // bereken aantal ,
    for (int tel = 0; tel < aantal; tel++) {       // loep om aantal komma's te plaatsen
     file.print(",");                              // zet komma (schuif 1 cel naar rechts)
    }
   } 
  }
 file.close();                                     // sluit het bestand
}
//-------------------------------------------------------------------------------------------

// zet verbruik_totaal in bestand met de komma erachter voor volgende meting
void save_verbruik() {
 if (minuut % sample == 0) {                       // is minuten een veelvoud van sample?
  File file = SD.open(bestand, FILE_APPEND);       // ja: open het bestand op SD
  sprintf(meterstand,"%u,",verbruik_tot);          // stel char samen uit verbruik en komma
  file.print(meterstand);                          // schrijf verbruik naar SD
  file.close();                                    // sluit het bestand
 }
}
//-------------------------------------------------------------------------------------------

// laad de volledige telegram buffer vol met data van de afkomstig van de Slimme meter:
void lees_P1() {
 memset(dataP1, 0, sizeof(dataP1));    // wis de telegram buffer
 digitalWrite(pin_P1,HIGH);            // Geef P1 meter opdracht data te sturen
 if (Serial1.available()) {            // wacht op seriele verbinding
  Serial1.readBytes(dataP1, maxP1);    // lees volledige buffer vol met telegram data
 } 
 digitalWrite(pin_P1,LOW);             // Geef P1 meter opdracht te stoppen met data sturen
}
//-------------------------------------------------------------------------------------------

// wis alle variabele met eerder binnengehaalde data van het telegram:
void wis_oude_data() {                                     
 memset(jaar, 0, sizeof(jaar));        // wis jaar gegevens uit telegram
 memset(maand, 0, sizeof(maand));      // wis maand gegevens uit telegram
 memset(dag, 0, sizeof(dag));          // wis dag gegevens uit telegram
 uur = 0;                              // wis uur (INT) gegevens uit telegram
 minuut = 0;                           // wis minuut (INT) gegevens uit telegram
 sec = 0;                              // wis sec (INT) gegevens uit telegram
 verbruik_L1 = 0;                      // wis actueel verbruik fase L1
 verbruik_L2 = 0;                      // wis actueel verbruik fase L2
 verbruik_L3 = 0;                      // wis actueel verbruik fase L3
 verbruik_tot = 0;                     // wis actueel verbruik totaal
 normaal = 0;                          // wis meterstand normaal tarief
 normaal_nu = 0;                       // wis meterstand normaal tarief 
 laag = 0;                             // wis meterstand laag tarief
 laag_nu = 0;                          // wis meterstand laag tarief 
 gas = 0;                              // wis meterstand gas verbruik
}
//-------------------------------------------------------------------------------------------

// splits telegram in losse regels:
void analyseer_telegram(){
 memset(regel, 0, sizeof(regel));      // wis de regel buffer
 nr = 0;                               // wis telegram teller
 nr2 = 0;                              // wis regel teller
 test = 0;                             // wis controle byte die kijkt of alle data binnen is
 while (nr < maxP1 && test != 0x7F) {  // doorloop telegram tot eind of tot data binnen is
 lees = dataP1[nr];                    // lees 1 karakter uit de telegram buffer
  if (lees == 0x0A) {                  // Kijk of je een Line-Feed (0A) ziet, zo ja:
   memset(regel, 0, sizeof(regel));    // wis de regel buffer
   nr2 = 0;                            // wis regel teller
  }
  if (lees == 0x0D) {                  // Kijk of je een Return (0D) ziet, zo ja:
   analyseer_regel();                  // 1 regel is binnen. Ga er nu de data uit halen
  }
  if (lees != 0x0A && lees !=0x0D) {   // in alle andere gevallen verwerk karakter
  regel[nr2] = lees;                   // vul de regel aan met het laatst gelezen karakter
  if (nr2 < 48) {nr2++;}               // als einde regel niet is bereikt: volgende karakter
  }
  nr++;                                // volgende karakter uit telegram
 }
}
//-------------------------------------------------------------------------------------------

// haal de data uit 1 telegram regel
// (indien 1e keer niet gelukt en data komt nog een 2e keer voor in telegram opnieuw lezen)
void analyseer_regel() {
 int var1, var2; char gasWeg[16];                                // tijdelijke variabelen
 if (bitRead(test,0) == 0){                                      // is tijdstempel al binnen?
 if (sscanf(regel,OBIS_tijd"%2c%2c%2c%2d%2d%2d",jaar,maand,dag,&uur,&minuut,&sec) == 6){ 
  bitSet(test,0);} }                                             // tijdstempel gevonden     
 if (bitRead(test,1) == 0){                                      // is laag tarief al binnen?
 if (sscanf(regel,OBIS_laag "%lu.%lu*", &laag, &laag_nu) == 2){  // scan laag tarief
  bitSet(test,1);} }                                             // laag tarief gevonden
 if (bitRead(test,2) == 0){                                      // is normaal al binnen?
 if (sscanf(regel,OBIS_normaal "%lu.%lu*", &normaal, &normaal_nu) == 2){//scan normaal tarief
  bitSet(test,2);} }                                             // normaal tarief gevonden
 if (bitRead(test,3) == 0){                                      // is gas al binnen?
 if (sscanf(regel,OBIS_gas1 "%15c%lu.", gasWeg, &gas) == 2){     // scan gas aansluiting 1
  bitSet(test,3);}                                               // verbruik gas gevonden
 if (sscanf(regel,OBIS_gas2 "%15c%lu.", gasWeg, &gas) == 2){     // scan gas aansluiting 2
  bitSet(test,3);}                                               // verbruik gas gevonden
 if (sscanf(regel,OBIS_gas3 "%15c%lu.", gasWeg, &gas) == 2){     // scan gas aansluiting 3
  bitSet(test,3);}                                               // verbruik gas gevonden
 if (sscanf(regel,OBIS_gas4 "%15c%lu.", gasWeg, &gas) == 2){     // scan gas aansluiting 4
  bitSet(test,3);}                                               // verbruik gas gevonden
 }
 if (bitRead(test,4) == 0){                                      // is verbruik L1 al binnen?
 if (sscanf(regel,OBIS_verbruikL1 "%2d.%3d" ,&var1,&var2) == 2){ // scan op verbruik L1
  verbruik_L1 = 1000 * var1 + var2;                              // bereken verbruik L1
  bitSet(test,4);} }                                             // verbruik L1 gevonden
 if (bitRead(test,5) == 0){                                      // is verbruik L2 al binnen?
 if (sscanf(regel,OBIS_verbruikL2 "%2d.%3d" ,&var1,&var2) == 2){ // scan op verbruik L2 
  verbruik_L2 = 1000 * var1 + var2;                              // bereken verbruik L2
  bitSet(test,5);} }                                             // verbruik L2 gevonden
 if (bitRead(test,6) == 0){                                      // is verbruik L3 al binnen?
 if (sscanf(regel,OBIS_verbruikL3 "%2d.%3d" ,&var1,&var2) == 2){ // scan op verbruik L3
  verbruik_L3 = 1000 * var1 + var2;                              // bereken verbruik L3
  bitSet(test,6);} }                                             // verbruik L3 gevonden
 verbruik_tot = verbruik_L1 + verbruik_L2 + verbruik_L3;         // bereken totaal verbruik
}
//-------------------------------------------------------------------------------------------
