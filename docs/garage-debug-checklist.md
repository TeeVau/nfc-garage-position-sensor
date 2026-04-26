# Garage Debug Checklist

Diese Checkliste ist fuer den Vor-Ort-Test in der Garage gedacht, wenn der Sensor zwar sichtbar wird, aber Zigbee2MQTT kein sauberes Interview abschliesst.

## Ziel

Wir wollen drei Dinge sauber voneinander trennen:

1. Bootet die Firmware sauber?
2. Joint oder rejoind das ESP32-C6-Endgeraet wirklich ins Zigbee-Netz?
3. Scheitert erst danach das Zigbee2MQTT-Interview bzw. die Converter-Zuordnung?

## 1. Debug-Firmware mit BLE bauen

```powershell
.\tools\firmware\build.ps1 -EnableBleDebug
.\tools\firmware\flash.ps1 -Port COM3 -EnableBleDebug
```

Erwartete BLE-Logzeile kurz nach dem Boot:

```text
BLE adv garage-sensor
```

Das BLE-Debug nutzt den Nordic UART Service.

- Device-Name: `garage-sensor`
- iOS-Apps: `nRF Connect`, `BLESerial nRF`

## 2. Vor dem Garagentest in Zigbee2MQTT aufraeumen

Wenn Zigbee2MQTT das Geraet schon unter der IEEE-Adresse kennt, kommt oft nur ein Rejoin statt eines frischen Interviews zustande.

Vor einem echten Interview-Test:

1. Geraet in Zigbee2MQTT loeschen.
2. Sicherstellen, dass der externe Converter aktiv ist:
   `zigbee2mqtt/external_converters/nfc-garage-position-sensor.js`
3. Zigbee2MQTT neu starten.
4. `Permit join` erst direkt vor dem Einschalten oder Reset aktivieren.

## 3. Beim Einschalten gleichzeitig beobachten

Parallel drei Beobachtungen sammeln:

1. LED am ESP32-C6
2. BLE-Log auf dem Handy
3. Zigbee2MQTT-Log bzw. Frontend

Fuer reproduzierbare Mitschnitte im Repo:

```powershell
.\tools\firmware\serial-capture.ps1 -Port COM3 -DurationSeconds 120 -OutputFile .\tmp\garage-serial.log
.\tools\mqtt-capture.ps1 -BrokerHost 192.168.178.2 -DeviceId 0x58e6c5fffedd775c -DurationSeconds 120 -OutputFile .\tmp\garage-mqtt.log
```

Optional fuer bessere Vergleichbarkeit:

```powershell
.\tools\firmware\serial-capture.ps1 -Port COM3 -DurationSeconds 120 -OutputFile .\tmp\garage-serial.log -Timestamp
```

Wichtige BLE-/Serial-Logzeilen:

```text
PN532 0x...
NFC wait
ZB mm 1
ZB appv ok
ZB swid 1
ZB pwr 1
ZB ep ok
ZB dbg on
ZB cfg mask=...
ZB join...
ZB started
ZB up
ZB st role=zed ch=15 pan=0x... short=0x... conn=1
DBG up=...
```

Interpretation:

- `ZB join...` bedeutet: Zigbee-Stack wurde gestartet.
- `ZB started` bedeutet: der Arduino-Zigbee-Stack laeuft bereits, aber das Geraet ist eventuell noch nicht im Netz angekommen.
- `ZB up` bedeutet: Das Device ist aus Sicht der Firmware wirklich verbunden.
- Fehlt `ZB up`, liegt das Problem noch vor Zigbee2MQTT.
- Kurzer Tastendruck auf dem Board erzeugt zusaetzlich `DBG ...`-Zeilen plus den aktuellen Zigbee-Status.

## 4. Wenn Zigbee2MQTT das Device sieht, aber nicht interviewt

Dann bitte genau unterscheiden:

1. Neues Geraet mit neuem Interview?
2. Oder nur bekanntes Geraet mit Rejoin/Announce?

Wenn in Zigbee2MQTT nur die bekannte IEEE-Adresse wieder auftaucht, aber kein neues Interview startet:

- altes Device wirklich loeschen
- danach erneut `permit join`
- dann Board neu starten oder per langem Tastendruck Factory Reset ausloesen

Die bisherige Testlage im Repo spricht dafuer, dass dieses Problem nicht immer Firmware-seitig ist.

## 5. Parent-/Router-Thema in der Garage mitpruefen

Im Repo ist bereits dokumentiert, dass bestimmte `eWeLink SA-003-Zigbee`-Router vermutlich keine brauchbare `End Device Capacity` fuer ESP32-C6-Endgeraete announcen.

Referenz:

- `docs/eWeLink-SA-003-Zigbee-router-capacity-issue.md`

Wenn das Interview in der Garage wieder haengt:

1. Test moeglichst nahe am Coordinator oder an `USB_Repeater3`
2. Nicht primaer ueber `USB_Repeater1` oder `USB_Repeater2` debuggen
3. Wenn es nahe an einem anderen Parent ploetzlich klappt, ist das ein starkes Indiz fuer ein Parent-/Router-Problem statt eines Sketch-Fehlers

## 6. Minimaler Schritt-fuer-Schritt-Test

1. BLE-Debug-Firmware flashen
2. Geraet in Zigbee2MQTT loeschen
3. External Converter pruefen
4. Zigbee2MQTT neu starten
5. `Permit join` aktivieren
6. Handy mit `garage-sensor` per BLE verbinden
7. Board neu starten
8. BLE-Logs bis mindestens `ZB up` mitschneiden
9. Pruefen, ob Zigbee2MQTT danach wirklich ein frisches Interview startet
10. Erst danach NFC-Tags bewegen und `TAG ...` / `ZB open=...` pruefen

## 7. Was fuer den naechsten Debug-Lauf am hilfreichsten ist

Wenn du vom Garagentest zurueckkommst, sind diese drei Artefakte ideal:

1. BLE-Log vom Boot bis mindestens 30 Sekunden nach `ZB up`
2. Zigbee2MQTT-Frontend- oder Bridge-Log fuer denselben Zeitraum
3. Info, ueber welchen Router bzw. an welchem Ort der Test lief
