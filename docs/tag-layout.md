# Tag Layout

The order of the UID table is the source of truth for the logical door position.

## Mapping

```text
raw_percent = round(index * 100 / (TAG_COUNT - 1))
percent = INDEX_INCREASES_WHEN_OPENING ? raw_percent : (100 - raw_percent)
```

Current physical layout:

| Tag | Lamella | Index | Opening |
| ---: | ------: | ----: | ------: |
| 1 | 1 | 0 | 0 % |
| 2 | 5 | 1 | 11 % |
| 3 | 10 | 2 | 22 % |
| 4 | 14 | 3 | 33 % |
| 5 | 19 | 4 | 44 % |
| 6 | 23 | 5 | 56 % |
| 7 | 28 | 6 | 67 % |
| 8 | 32 | 7 | 78 % |
| 9 | 37 | 8 | 89 % |
| 10 | 41 | 9 | 100 % |

Current UID order:

```text
0  -> 04-F5-33-16-C9-2A-81 ->   0 %
1  -> 04-16-34-16-C9-2A-81 ->  11 %
2  -> 04-15-34-16-C9-2A-81 ->  22 %
3  -> 04-14-34-16-C9-2A-81 ->  33 %
4  -> 04-FE-33-16-C9-2A-81 ->  44 %
5  -> 04-FD-33-16-C9-2A-81 ->  56 %
6  -> 04-FC-33-16-C9-2A-81 ->  67 %
7  -> 04-FB-33-16-C9-2A-81 ->  78 %
8  -> 04-F6-33-16-C9-2A-81 ->  89 %
9  -> 04-17-34-16-C9-2A-81 -> 100 %
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
