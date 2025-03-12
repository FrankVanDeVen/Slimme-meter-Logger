# Slimme-meter-Logger
Logt de data van de P1 poort van een slimme meter en slaat die op een micro SD kaart op.

Op de micro SD kaart komt voor iedere maand een nieuw *.CSV bestand te staan.

De software staat om 00.00u het electra verbruik (laag en normaal tarief) en gas verbruik op.
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
