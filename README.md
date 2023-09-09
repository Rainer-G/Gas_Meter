# Gas Verbrauch messen und verarbeiten 

## Funktion

Manche Gaszähler geben pro vebrauchte 10 Liter einen magnetischen Impuls ab.
Dieser Impuls wird mit einem Hall Sensor in elektrische Impulse gewandelt, ein RTC Baustein PCF8583 arbeitet als Zähler und speichert die Zahl der Impulse im internen RAM ab.

PCB Eagle Dateien und Stückliste anbei.

Die Daten werden mit Hilfe eines ESP8266 über die I²C SAchnittstelle ausgelesen und verarbeitet. 
Über das MQTT Protokoll werden die Daten an einen lokalen Broker weitergeleitet, Daten werden nicht ins Internet hochgeladen.
Eine 3 Volt Knopfzelle versorgt den Baustein mit Energie und erhält die Daten bei Stromausfall.


* Diese Daten werden im RTC RAM gespeichert

1. Zählimpulse
2. Zählerstand zu Beginn der Messung ( Counts = 0 )
3. Zählerstand zu Beginn der Abrechnungsperiod
4. monatlicherVerbrauch in m³
5. Datum und Uhrzeit des letzen Auslesens des RTC Bausteins

* Diese Daten werden über MQTT an den Broker gesendet

1. Uhrzeit 
2. Gesamtverbrauch in kWh
3. Verbrauch zwischen zwei Zugriffen auf den RTC Speicher in kWh
4. Anzahl der Impulse seit Reset
5. Gesamtverbrauch in m³
6. Zählerstand zum Begin der Messung ( Counts = 0 ) in Liter
7. Zählerstand zum Begin der Abrechnungsperiode in Liter
8. nach Bedarf monatlicher Verbrauch
9. Status ( OK wenn Zugriff auf den RTC Baustein möglich )

* Diese Daten/Kommandos können vom Broker an den ESP8266 gesendet werden

1. Publish consumption of a single month or month 1 to 12 
 * SYNTAX  : "Energy/Test/command"
 * PAYLOAD : "Month, x"
 * value for x : either 1 to 12 for a single month, x > 12 shows all 12 months
  
 2. Set consumption for the start of a heating period 
 * SYNTAX  : "Energy/Test/command"
 * PAYLOAD : "InitP, x"
 * value for x : consumption in liter

 3. Set initial consumption value for a count value of zero 
 * SYNTAX  : "Energy/Test/command"
 * PAYLOAD : "InitS, x"
 * value for x : consumption in liter

 4. Set consumption for a single month to init data  
 * SYNTAX  : "Energy/Test/command"
 * PAYLOAD : "InitM, x ; y"
 * value for x : selected month
 * value for y : consumption in liter
 
 5. Set consumption for the start of a counting period 
 * Initialyse RTC chip and set initial consumption value for count = 0 of a counting period 
 * SYNTAX  : "Energy/Test/command"
 * PAYLOAD : "InitC, x"
 * value for x : consumption in liter

## Vorbereitung

* in Gas_WiFi SSID und Passwort eintragen
* in Gas_MQTT IP Adresse des MQTT Broker eintragen
* ggfs. Thinkspeak aktivieren und Channel und Key eintragen
* Topic für MQTT an die eigenen Wünsche anpassen
  
