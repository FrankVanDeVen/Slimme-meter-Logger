// Instellingen "Slimme meter logger":
 #define baud            115200         // baudrate Slimme meter
 #define inverteer       true           // true = inverteren. false = niet inverteren 
 #define protocol        SERIAL_8N1     // serieel protocol meter: 8 bits + 1 stopbit
                                        // (software werkt enkel met het SERIAL_8N1 protocol)

// Instellingen telegram bericht slimme meter:
 #define maxP1           1056           // max. aantal bytes telegram: 1KB + 32 bytes

// Scan codes (OBIS ref.) waarmee telegram doorzocht wordt:
 #define OBIS_tijd       "0-0:1.0.0("   // OBIS referentie: tijdstempel (tijd en datum)
 #define OBIS_laag       "1-0:1.8.1("   // OBIS referentie: laag tarief
 #define OBIS_normaal    "1-0:1.8.2("   // OBIS referentie: normaal tarief
 #define OBIS_gas1       "0-1:24.2.1("  // OBIS referentie: gasmeter (aansluiting 1)
 #define OBIS_gas2       "0-2:24.2.1("  // OBIS referentie: gasmeter (aansluiting 2)
 #define OBIS_gas3       "0-3:24.2.1("  // OBIS referentie: gasmeter (aansluiting 3)
 #define OBIS_gas4       "0-4:24.2.1("  // OBIS referentie: gasmeter (aansluiting 4)
 #define OBIS_verbruikL1 "1-0:21.7.0("  // OBIS referentie: actueel verbruik (fase L1)
 #define OBIS_verbruikL2 "1-0:41.7.0("  // OBIS referentie: actueel verbruik (fase L2)
 #define OBIS_verbruikL3 "1-0:61.7.0("  // OBIS referentie: actueel verbruik (fase L3)

// Kies de sample tijd (de sample tijd is de tijd tussen 2 registraties):
// Opties zijn 5 minuten, 10 minuten, 15 minuten, 20 minuten of 30 minuten.
 #define sample 5                       // meet om de 5 minuten (dus 12x per uur)

// Kies wat je wilt registreren:
// 0 = Meter registreert het actueel vermogen op het moment van het sample tijdstip (in W)
// 1 = Meter registreert het verbruik tussen het vorige en het huidige sample tijdstip (in Wh)
 #define meting          1              // registreer het verbruik tussen 2 samples
