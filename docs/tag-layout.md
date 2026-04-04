# Tag Layout

The order of the UID table is the source of truth for the logical door position.

## Mapping

```text
percent = index * 100 / (TAG_COUNT - 1)
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

## Runtime Logic

- `stableIndex`: last confirmed position
- `pendingIndex`: next candidate position
- `pendingCount`: how often the candidate was seen in a row

A new position is accepted only after the same candidate tag was read `INDEX_CONFIRM_COUNT` times.

This keeps the sketch responsive while reducing false transitions near the boundary between two neighboring tags.
