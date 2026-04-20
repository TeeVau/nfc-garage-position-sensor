# eWeLink SA-003-Zigbee Router Capacity Issue

## Kurzfassung

Im Zigbee2MQTT-Netz gibt es Hinweise, dass der USB-Router `eWeLink SA-003-Zigbee` mit Firmware `1.0.7 / 20210703` zwar als Zigbee-Router arbeitet und Permit-Join-Befehle erfolgreich annimmt, aber in Beacons keine nutzbare Parent-Kapazitaet fuer End Devices annonciert.

Das ist relevant, weil ein ESP32-C6 Zigbee End Device beim Join/Rejoin einen Parent braucht, dessen Beacon `End Device Capacity: Yes` meldet. Bei den eWeLink-SA-003-Routern wurde im ESP32-C6-Scan wiederholt `End Device Capacity: No` beobachtet, waehrend IKEA-Router im selben Netz `Router Capacity: Yes` und `End Device Capacity: Yes` melden.

## Betroffene Router

Aus `zigbee2mqtt/bridge/devices`:

```text
USB_Repeater1
ieee:         0x00124b000622de59
nwk:          0x92C8
network addr: 37576
manufacturer: eWeLink
model_id:     SA-003-Zigbee
firmware:     1.0.7
date_code:    20210703
power_source: Mains (single phase)

USB_Repeater2
ieee:         0x00124b00061ff2e5
nwk:          0xCDD5
manufacturer: eWeLink
model_id:     SA-003-Zigbee
firmware:     1.0.7
date_code:    20210703
power_source: Mains (single phase)
```

Vergleichsrouter:

```text
USB_Repeater3
ieee:         0xa4c138d294ab5f73
nwk:          0x3F91
manufacturer: HOBEIAN
model_id:     CK-BL702-ROUTER-01(7018)
firmware:     0122052017
power_source: DC Source
```

## Zigbee2MQTT Permit-Join-Test

Zigbee2MQTT hat den Permit Join gezielt ueber `USB_Repeater1` aktiviert:

```text
Zigbee: allowing new devices to join via USB_Repeater1.
ZDO PERMIT_JOINING_REQUEST UNICAST to=...:37576 payload=46fe01
SENT ZDO UNICAST ... status=OK
Received ZDO response: clusterId=PERMIT_JOINING_RESPONSE, status=SUCCESS
```

Interpretation:

```text
PERMIT_JOINING_RESPONSE: SUCCESS
```

bedeutet nur, dass der Router den Permit-Join-Befehl angenommen hat. Es beweist nicht, dass der Router freie Child-Slots fuer End Devices hat oder sich als Parent fuer neue End Devices eignet.

## ESP32-C6 Scan-Beobachtung

Das ESP32-C6-Scan-Beispiel sieht das Zigbee2MQTT-Netz:

```text
PAN ID:          0xaae1
Channel:         15
Extended PAN ID: 96:e3:c7:4d:11:90:05:aa
```

Bei aktivem Permit Join via Coordinator oder via `USB_Repeater1` wurde beobachtet:

```text
PAN ID | CH | Permit Joining | Router Capacity | End Device Capacity | Extended PAN ID
0xaae1 | 15 | Yes            | No              | No                  | 96:e3:c7:4d:11:90:05:aa
```

Bei einer IKEA-Leuchte als Router wurde dagegen beobachtet:

```text
Router Capacity:     Yes
End Device Capacity: Yes
```

Arbeitshypothese:

Der eWeLink `SA-003-Zigbee` mit Firmware `1.0.7 / 20210703` routet zwar im Mesh, annonciert aber fuer neue End Devices keine Parent-Kapazitaet. Dadurch wird er vom ESP32-C6-Zigbee-Stack beim Join/Rejoin als Parent-Kandidat verworfen.

## Nuance: Gleiche PAN, mehrere Beacon-Quellen

Der ESP32-C6-Scan listet Netzwerke nach PAN/Extended PAN, zeigt aber nicht eindeutig, welcher konkrete Router das Beacon geliefert hat. Wenn mehrere Router im gleichen PAN senden, kann dieselbe PAN-Zeile je nach gehoertem Beacon unterschiedliche Capacity-Werte zeigen.

Das erklaert Beobachtungen, bei denen `0xaae1` gelegentlich `Yes/Yes`, danach aber wieder `No/No` zeigt. Vermutlich stammen diese Scans von unterschiedlichen Routern im gleichen Netz.

## Relevanz fuer das eigentliche Problem

Das urspruengliche Problem:

Ein ESP32-C6 Zigbee End Device verbindet sich nach Ausfall seines bisherigen Parent-Routers nicht zuverlaessig ueber einen anderen Router neu.

Wenn der erreichbare eWeLink-SA-003-Router keine `End Device Capacity` annonciert, kann der ESP32-C6 ihn beim Rejoin nicht als neuen Parent nutzen. Das passt zu Logs wie:

```text
No dev for join
Can't find PAN to join to
Network steering was not successful
```

## Naechste Tests

1. ESP32-C6 direkt neben `USB_Repeater1` platzieren.
2. In Zigbee2MQTT gezielt `Permit Join via USB_Repeater1` aktivieren.
3. Andere starke Router, soweit praktikabel, kurz stromlos machen oder Abstand vergroessern.
4. Pruefen, ob der ESP-Scan bei `0xaae1` jemals `End Device Capacity: Yes` zeigt.
5. Denselben Test mit einer IKEA-Leuchte wiederholen.
6. Denselben Test mit `USB_Repeater3` wiederholen, da dieser ein anderes Modell/Firmware-Lineage ist.

## Entscheidungskriterium

Wenn `USB_Repeater1` bei gezieltem Permit Join und direkter Naehe weiterhin nur:

```text
Permit Joining:      Yes
Router Capacity:     No
End Device Capacity: No
```

meldet, dann ist dieser Router fuer neue ESP32-C6 End-Device-Children praktisch ungeeignet.

Wenn IKEA oder `USB_Repeater3` unter denselben Bedingungen:

```text
Permit Joining:      Yes
Router Capacity:     Yes
End Device Capacity: Yes
```

meldet, liegt das Problem sehr wahrscheinlich beim eWeLink `SA-003-Zigbee` bzw. dessen Firmware/Child-Table-Verhalten, nicht beim ESP32-C6-Sketch.

