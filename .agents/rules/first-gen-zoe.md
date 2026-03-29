---
trigger: always_on
---

It is a first generation Renault Zoe Q210.

*Fontos OBD infó ehhez a modellhez:*
- A Phase 1-es (Q210/R240, 22 kWh) akkumulátor vezérlő (LBC - 79B) **nem támogatja** az újabb ZE50 beépített UDS cellafeszültség lekérdezéseit (mint a 221417 vagy 221419). Ezek `7F 22 11` (Service Not Supported) hibát fognak dobni.
- Ehelyett a régebbi KWP2000-es paramétertármazékot, a **`2103`**-at kell használni többrészes `ISO-TP` (Multi-Frame) választ feldolgozva, amiből kiolvasható a Max és a Min cellafeszültség egyszerre (a `10C0` Extended Session inicializálása kötelező előtte).