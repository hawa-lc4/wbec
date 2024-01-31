**wbec** - WLAN-Anbindung der Heidelberg **W**all**B**ox **E**nergy **C**ontrol über ESP8266  

Die Heidelberg Wallbox Energy Control ist eine hochwertige Ladestation, bietet aber nur Modbus RTU als Schnittstelle.  
Ziel des Projekts ist es, eine WLAN-Schnittstelle zu entwickeln um zusätzliche Funktionen (z.B. PV-Überschussladen) zu ermöglichen.  

[wbec Homepage](https://steff393.github.io/wbec-site/)  
[Empfehlung im Heidelberg Amperfied Blog](https://www.amperfied.de/de/clever-laden/blog/wbec-fuer-heidelberg-wallbox-energy-control-blog/)  
  
![GitHub all releases](https://img.shields.io/github/downloads/steff393/wbec/total?color=blue&style=flat-square)  

## Funktionen
- Anbindung an openWB, EVCC, Solaranzeige
- MQTT-Kommunikation mit openWB und EVCC (ideal für mehrere Ladestationen)
- Steuerbar per Android App [Wallbox Steuerung](https://android.chk.digital/ecar-charger-control/) 
- PV-Überschussladen, Zielladen, etc. mit den o.g. Steuerungen
- Abfrage von Shelly 3EM, powerfox, Solaredge, Fronius, ...
- RFID-Kartenleser zur Freischaltung der Wallbox mit gültiger Karte/Chip (spezielle HW nötig, s. Wiki)  
- Ansteuerung aller verbundenen Ladestationen (bis zu 16 Follower am Modbus, bis zu 8 openWB-Ladepunkte)
- Lokales Lastmanagement für zwei Wallboxen
- Softwareupdate per WLAN (Over The Air), z.B. mit PlatformIO oder einfach per Browser (s. Wiki)
- Weniger als 1W Strombedarf (trotz Ansteuerung von bis zu 16 Ladestationen)
- [-> Neue Funktionen](https://steff393.github.io/wbec-site/features.html)

## hawa-lc4: Anpassungen wall-E
## ACHTUNG: Baustelle, work in progress!   Das Ganze wird noch überarbeitet!

- Abstürze bei Zugriff auf Web-Seiten; Ursache: ESP8266 Platform Version!!!  NUR v4.0.1 verwenden; v4.1.0 verursacht den Fehler.
- AsyncElegantOTA.h mit Benutzer und Passwort (globalConfig.h & .cpp, AsyncElegantOTA.h); wegen Fehlern in Version 2.2.7 nur noch fix Version 2.2.5
    (https://github.com/ayushsharma82/ElegantOTA/issues/67)
- - (edit: sieht nicht so aus als ob dieser Fehler in AsyncElegantOTA v2.2.8 behoben wäre; bleibt nur die neue v3.x.x)
- ArduinoOTA entfernt (main.cpp)
- Komponenten entfernt: rfid, powerfox, shelly, inverter, button
- WiFi immer im Station-Mode (main.cpp) und Verbindung in mein lokales WLAN (globalConfig.cpp & include/wlan_key.h)
- Anpassung Versionsnummer (globalConfig.cpp)
- NTP Server auf meine Fritz.Box umgestellt (globalConfig.cpp)
- Anbindung RFID (benötigt SPI) in rfid.cpp in Routine "rfid_setup" komplett deaktiviert. SPI ist jetzt verfügbar
- Mit den folgenden Aktionen ist jetzt der I2C Bus wieder verfügbar für Display und andere Erweiterungen
- - UART0 (Serial) wird nicht mehr für Terminal-Kommunikation initialisiert, umgestellt auf UART1 (Serial1) nur für debug Ausgaben an GPIO2 (D4 = TX)
- - UART0 (Serial) wird jetzt für die ModBus Kommunikation (RS485) auf GPIO1 & GPIO3 verwendet mit HW-Serial (Pins RX/TX des UART0)
- - PIN_DE_RE deaktiviert (-1) da beim derzeit verwendeten RS485 Modul nicht benötigt

## geänderte Dateien
- main.cpp:         includes und Ablauf; alles raus was ich nicht brauche
- globalConfig.h:   
- globalConfig.cpp: 
- goEmulator.cpp:   
- phaseCtrl.h:      
- phaseCtrl.cpp:    
- logger.cpp:       
- mbComm.h:         
- mbComm.cpp:       
- 

hawa-v0.6a: 
- Kleinere Änderungen und Erweiterungen: uptime; Anzeige der powerfox Ladestati sowie Leistung im Web-GUI (wird vom DD3 geliefert)
- 
hawa-v0.61c: lief sehr stabil, ohne automatische restarts
- ArduinoOTA entfernt; ist unnötig wegen ElegantOTA; spart eine Menge Speicherplatz für code.    Scheint zu funktionieren: check.
- pvSoladin.cpp sendet jetzt den PWM Wert an den Arduino; bisher keine Abstürze.       Scheint zu funktionieren: check.
- Regelalgorythmus zur Ermittlung des PWM Wertes ist noch verbesserungswürdig, besonders zur Reduzierung unnötiger Einspeisung.
- Anpassungen am Regelalgorythmus in pvSoladin.cpp; etwas weniger "agressiv".
- Einbau der phaseCtrl mit Ziel, Erweiterung der Wallbox um ein Relais das L2 und L3 abschaltet um mit 1 oder 3(2) Phasen zu laden.
- Weitere PIN Definitionen die nicht genutzt werden entfernt. Pin für das Schalten des Phasen-Relais hinzugefügt.
- Prüfung in pvSoladin.cpp auf völlig überhöhte Werte der PV-Leistung; werden einfach ignoriert.
- 
hawa-v0.62: 
- Wegen Fehler in AsyncElegantOTA v2.2.7 zurück auf v2.2.5 gesetzt. (https://github.com/ayushsharma82/ElegantOTA/issues/67)
- Prüfung in smDD3.cpp auf einzelne ungültige Werte von "0 Watt"; werden einfach ausgeblendet.      Scheint nicht ganz zu funktionieren!
- Einbau der Funktionen für die Phasen-Umschaltung in phaseCtrl.cpp und goEmulator.cpp
- 
hawa-v0.63:
- Derzeit in Betrieb mit kleineren Änderungen. (b)
- Erweiterung webSocket.cpp zur Prüfung auf "openWB alive" über HTTP Abfrage
  Wenn openWB antwortet keine Änderungen der wbec GUI übernehmen um Kollisionen der Anforderungen zu vermeiden.
- Ladestatus "PV_OFF" wird verwendet als Ersatzfunktion für "Sofortladen".
  Alle Ladefunktionen für PV derzeit auf 1-phasig (PV-Anlage zu klein), Sofortladen auf 3-phasig
- 
hawa-v0.64:
- Freigabe der Ladung an LP1 (alw=1) wenn openWB fehlt und wbec alleine verfügbar ist in webSocket.cpp eingebaut (v0.64b)
- 
hawa-v0.65:
- Phasenumschalter hat jetzt eine web-Passwort bekommen; eingetragen in phaseCtrl.cpp
- webServer.cpp hat einen Eintrag erhalten um WB in standby schalten zu können; zulässige Werte sind 0 (standby aktiv) und 4 (standby deaktiviert)
- <break>

hawa-v0.70:
- Adaption von wbec v0.4.9; wbec Modul wird jetzt in direkter Nahe der WB montiert.
  - DD3 und Soladin wieder entfernt; werden für neue PV-Anlage nicht mehr benötigt.
  - RS485 Schnittstelle jetzt über SoftwareSerial; Details zu den PINs siehe globalConfig.
  - Steuerung des Schütz für phaseCtrl jetzt über lokalen PIN (siehe globalConfig).
  - mbComm.cpp: bei Comm-Failure der RS485 Schnittstelle blinkt die rote LED (PIN_openWB) ca. alle 10 Sekunden kurz auf.
  - wenn openWB nicht erreichbar soll die rote LED (PIN_openWB) dauerhaft leuchten.   ODER UMGEKEHRT: RS485 Fehler dauerhaft und openWB nur blinken??
  - 

hawa-v0.71:
- Rückkehr zur alten HW und Konzept. Systeme wbec und phase13 bleiben getrennt. Das hat mehrere Gründe:
  - Beim Start von wbec schaltet das Schütz gelegentlich ohne erkennbaren Grund; HW? Das ist ein No-Go!
  - Trennung der System wegen Betriebssicherheit; wenn wbec korrekte http Kommandos an phase 13 senden kann dann ist anzunehmen das alles korrekt läuft.
    Bei direkt angeschlossenem Schütz könnte es bei Programmfehlern in wbec dazu komen daß das Schütz während des Ladevorgangs unkontrolliert schaltet; No-Go!
- Aufbau der Programme auf Basis wbec 0.4.9 unter Verwendung der Änderungen aus hawa-v0.65.

hawa-v0.72:
- Einbau vergessener Funktion für Steuerng der Phasenumschaltung durch openWB (goEmulator.cpp)
- Der wbec im Keller hat immer wieder Probleme mit dem ModBus; der MAX485 Chip im Adapter stirbt immer wieder, vermutlich durch Überspannung. Daher: ...
- Erweiterung des Programms zur dynamischen Anpassung des Programms an unterschiedliche Hardware über die platformio.ini:
  - v0.72.1 / wbec_v1 / [env:d1_mini]: klassischer wbec im Keller mit Wemos D1-mini Board.
  - v0.72.2 / wbec_v2 / [env:nodemcuv2]: wbec auf Basis ESP8266 Chip im Hutschienengehäuse; jetzt näher an der WB (Garage).
    diese Version hat auch wieder die Vorbereitung zum direkten Schalten des Phasen-Relais.

hawa-vx.73.0 & hawa-vx.73.1:
- Änderung der Versionsbezeichnung auf allgemein gültige Regeln (Major.Minor.BugFix)
  - v1.73.x / WALLE_VERSION_MAJOR=1 / [env:d1_mini]: klassischer wbec im Keller mit Wemos D1-mini Board.
  - v2.73.x / WALLE_VERSION_MAJOR=2 / [env:nodemcuv2]: wbec auf Basis ESP8266 Chip im Hutschienengehäuse; jetzt näher an der WB (Garage).
  - Die beiden Versionen sind wegen unterschiedlicher Hardware nicht kompatibel.
- Update auf Basis von wbec 0.5.0
- Ausnahme: das data-Verzeichnis ist noch auf Stand von wbec 0.4.9; der wbec-0.5.0_data_fix ist seit heute verfügbar.
- Verlagerung von header-Dateien in das include Verzeichnis.
- Private Daten werden in include/wlan_key.h gepflegt und bei einer Übertragung zu GitHub nicht veröffentlicht.

hawa-vx.74.0 & hawa-vx.74.1:
- erneut den zusätzlichen Summen-Fehlerzähler für modbus-Error "mbErrCnt" eingebaut.
- kleine Änderungen am "mbErrCnt".
- Umstellung AsyncElegantOTA v2.2.5 auf ElegantOTA v3.1.0 im Async-Mode unter ESP8266 Platform Version 4.2.1.
  - compilieren fehlerfrei
  - OTA upload fehlerfrei
  - auf den ersten Blick keine Fehler beim laden der web-Seiten
  - testen ........

hawa-vx.74.2:
- Anpassungen zur Veröffentlichung auf github.
- 

  
hawa ToDo:
- Anzeige über LED wenn openWB nicht erreichbar, dann Umschaltung Phasen per Taster(?)/WebUI erlauben
- Wenn openWB nicht erreichbar Freigabe des WebUI zur lokalen Steuerung
- Uhrzeit im Winter falsch, Uhr geht 1 Stunde vor; vermutlich liegt es an der Zeitbasis der FritzBox bzw. meines Linux mit openWB
- und wie sieht's im Sommer aus?
- ElegantOTA v3.x.x testen (https://docs.elegantota.pro/async-mode/) || läuft nur mit ESP8266 Platform Version 4.1.0, nicht mit 4.0.1.
  Damit scheidet das vorerst aus, falls nicht die Web-Seiten Probleme vom Anfang zufällig auch behoben sind.
  Allerdings ist jetzt die ESP8266 Platform Version 4.2.1 verfügbar: testen!
- wbec-0.5.0_data_fix testen ()
- 
- Baustellen:
  - webServer.cpp => /chargelog
  - 

(ab hier wieder Original von steff393)
## Kontakt
Bei Fragen oder wenn ihr Unterstützung braucht gerne einfach eine Mail schicken (wbec393@gmail.com).    
Bitte schaut auch ins [Wiki](https://github.com/steff393/wbec/wiki) und in meine anderen Projekte, z.B. den [SmartUploader](https://github.com/steff393/SmartUploader) zum Auslesen von Wechselrichtern und [hgdo](https://github.com/steff393/hgdo) zur Steuerung von Torantrieben.  

## Beispiele
Einfaches Web-Interface (geeignet für alle Browser, Smartphone, PC, etc.):  
`http://wbec.local/`  
<p align="center"> 
  <img src="https://i.ibb.co/3sg0YdL/wbec-web3.png"> 
</p>

JSON API Schnittstelle:  
`http://wbec.local/json`  
```c++
{
  "wbec": {
    "version": "v0.3.0"         // wbec version
    "bldDate": "2021-06-10"     // wbec build date
  },
  "box": [
    {                           // s. also https://wallbox.heidelberg.com/wp-content/uploads/2021/04/EC_ModBus_register_table_20210222.pdf
      "busId": 1,               // Modbus bus id (as configured by DIP switches)
      "version": "108",         // Modbus Register-Layouts Version, e.g. 1.0.8
      "chgStat": 2,             // Charging State
      "currL1": 0,              // L1 - Current RMS (in 0.1A)
      "currL2": 0,              // L2 - Current RMS (in 0.1A)
      "currL3": 0,              // L3 - Current RMS (in 0.1A)
      "pcbTemp": 333,           // PCB-Temperatur (in 0.1°C)
      "voltL1": 232,            // Voltage L1 - N rms in Volt
      "voltL2": 9,              // Voltage L2 - N rms in Volt
      "voltL3": 9,              // Voltage L3 - N rms in Volt
      "extLock": 1,             // extern lock state
      "power": 0,               // Power (L1+L2+L3) in VA
      "energyP": 0,             // Energy since PowerOn (in kWh)
      "energyI": 0.003,         // Energy since Installation (in kWh)
      "currMax": 16,            // Hardware configuration maximal current (in 0.1A)
      "currMin": 6,             // Hardware configuration minimal current (in 0.1A)
      "logStr": "<item no> <mfgDate> <serial>",
      "wdTmOut": 15000,         // ModBus-Master WatchDog Timeout (in ms)
      "standby": 4,             // Standby Function Control 
      "remLock": 1,             // Remote lock (only if extern lock unlocked) 
      "currLim": 130,           // Maximal current command
      "currFs": 0,              // FailSafe Current configuration 
      "load": 0,                // wbec load management
      "resCode": "0"            // Result code of last Modbus message (0 = ok)
    },
    {                           // Values of 2nd box ...
      "busId": 2,
      "version": "0",
      "chgStat": 0,
      ...
      "load": 0,
      "resCode": "e4"
    }
  ],
  "modbus": {
    "state": {
      "lastTm": 2852819,        // Timestamp of last Modbus message (in ms)
      "millis": 2855489         // Time since start of wbec (in ms)
    }
  },
  "rfid": {
    "enabled": true,
    "release": false,
    "lastId": "0cb6a781"
  },
  "wifi": {
    "mac": "00:1F:3F:15:29:7E", // wbec MAC address
    "rssi": -76,                // WiFi signal
    "signal": 48,               // WiFi signal quality (in %)
    "channel": 11               // WiFi channel
  }
}
```

Maximalen Ladestrom einstellen:
```c++
http://192.168.xx.yy/json?currLim=120      --> set current limit to 12A (on the box with id=0, i.e. ModBus Bus-ID=1)
http://192.168.xx.yy/json?currLim=60&id=2  --> set current limit to 6A on the box with id=2 (i.e. ModBus Bus-ID=3)
```

## Danksagung
Folgende Projekte wurden in wbec genutzt/angepasst:  
- [modbus-esp8266](https://github.com/emelianov/modbus-esp8266)
- [ESP Async WebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- [PubSubClient](https://github.com/knolleary/PubSubClient)
- [NTPClient](https://github.com/arduino-libraries/NTPClient)
- [MFRC522](https://github.com/miguelbalboa/MFRC522)
- [RTCVars](https://github.com/highno/RTCVars)
- [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [Web Interface](https://RandomNerdTutorials.com)
- [A Beginner's Guide to the ESP8266 - article](https://github.com/tttapa/ESP8266)
- [AsyncElegantOTA](https://github.com/ayushsharma82/AsyncElegantOTA)

Ein besonderer Dank ergeht an die frühen Tester und Unterstützer: mli987, profex1337, Clanchef und viele mehr!

## Unterstützung des Projektes
wbec gefällt dir? Dann gib dem Projekt [einen Stern auf GitHub](https://github.com/steff393/wbec/stargazers)!  

[![Star History Chart](https://api.star-history.com/svg?repos=steff393/wbec&type=Date)](https://star-history.com/#steff393/wbec&Date)
