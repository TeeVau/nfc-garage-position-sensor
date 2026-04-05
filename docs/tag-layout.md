# Tag Layout

The order of the UID table is the source of truth for the logical door position.

## Mapping

```text
raw_percent = index * 100 / (TAG_COUNT - 1)
percent = INDEX_INCREASES_WHEN_OPENING ? raw_percent : (100 - raw_percent)
```

Current UID order:

```text
0  -> 04-29-CB-3E-D4-2A-81 ->   0 %
1  -> 04-61-C2-3E-D4-2A-81 ->  12 %
2  -> 04-AD-BC-3E-D4-2A-81 ->  25 %
3  -> 04-2D-B7-3E-D4-2A-81 ->  37 %
4  -> 04-DC-AE-3E-D4-2A-81 ->  50 %
5  -> 04-21-A7-3E-D4-2A-81 ->  62 %
6  -> 04-2F-A3-3E-D4-2A-81 ->  75 %
7  -> 04-11-9E-3E-D4-2A-81 ->  87 %
8  -> 04-96-9A-3E-D4-2A-81 -> 100 %
```

This reflects the current firmware setting `INDEX_INCREASES_WHEN_OPENING = true`, i.e. the UID table is ordered from closed to open and the reported cover percentage follows the SDK/Zigbee2MQTT convention directly: `0 = closed`, `100 = open`.

That means:

- the tag labeled `0 %` must represent the closed position
- the tag labeled `100 %` must represent the fully open position

When publishing to the Zigbee Window Covering endpoint, this opening percentage is converted to the cluster-facing lift percentage so that external consumers such as Zigbee2MQTT still receive `0 = closed` and `100 = open`.

## Runtime Logic

- `stableIndex`: last confirmed position
- `pendingIndex`: next candidate position
- `pendingCount`: how often the candidate was seen in a row

A new position is accepted only after the same candidate tag was read `INDEX_CONFIRM_COUNT` times.

This keeps the sketch responsive while reducing false transitions near the boundary between two neighboring tags.
