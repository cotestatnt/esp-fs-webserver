# Config Upgrade Test

Test per verificare la migrazione automatica da config v1.0 a v2.0

## Cosa Fa

1. Carica un file `config.json` v1.0 (formato flat) embeddato nel filesystem
2. Utilizza `ConfigUpgrader` per convertire automaticamente a v2.0
3. **Salva il file aggiornato in `/config/config_v2.json`** (il file originale rimane intatto)
4. Stampa sia il JSON v1.0 che il v2.0 formattati su serial per facile confronto

## File

- **`/config/config.json`** (originale, v1.0 - non modificato)
- **`/config/config_v2.json`** (nuovo, v2.0 - generato dal test)

## Come Eseguire

### 1. Build

```bash
cd pio_examples/config_upgrade_test
platformio run --environment esp32-s3-devkitc1-n4r2
```

### 2. Upload

```bash
platformio run --target upload --environment esp32-s3-devkitc1-n4r2
```

### 3. Monitor Serial

```bash
platformio device monitor --baud 115200
```

## Output Atteso sul Serial

```
========================================
CONFIG UPGRADE TEST - v1 to v2.0
========================================

✓ LittleFS mounted

--- ORIGINAL CONFIG (v1.0) ---
/config/config.json
{
  "page-title": "Config Upgrade Test",
  "img-logo": "/config/logo.png",
  "param-box1": "My Options",
  "A bool variable": true,
  ...
}

--- UPGRADING CONFIG ---
[INFO] ConfigUpgrader: Upgrading config from v1 to v2.0
✓ Config upgraded and saved to /config/config_v2.json

--- UPGRADED CONFIG (v2.0) ---
/config/config_v2.json
{
  "_version": "2.0",
  "_meta": {
    "app_title": "Config Upgrade Test",
    "logo": "/config/logo.png",
    "port": 80,
    "host": "myserver"
  },
  "_state": {},
  "_assets": {
    "css": [],
    "javascript": []
  },
  "sections": [
    {
      "id": "my-options",
      "title": "My Options",
      "elements": [
        {
          "id": "a-bool-variable",
          "type": "boolean",
          "label": "A bool variable",
          "value": true
        },
        ...
      ]
    }
  ]
}

--- UPGRADE TEST COMPLETED ---
Original file: /config/config.json (unchanged)
Upgraded file: /config/config_v2.json (new)
```

## Struttura Test

```
config_upgrade_test/
├── platformio.ini           # Configurazione PlatformIO
├── partitions.csv           # Partizioni Flash
├── README.md               # Questo file
├── src/
│   └── config_upgrade_test.ino  # Sketch di test
└── data/
    └── config.json         # File config v1.0 embeddato
```

## Note Importanti

- Il file v1.0 è **embeddato nel filesystem** durante la build
- La cartella `/config/` viene creata automaticamente se non esiste
- L'upgrade è **non-distruttivo**: il file originale rimane intatto
- Il nuovo file v2.0 viene salvato con estensione `_v2.json`
- La conversione gestisce automaticamente:
  - Metadati (`page-title`, `img-logo`, `port`, `host` → `_meta`)
  - Sezioni (`param-box*` → `sections`)
  - Tipi di dati (boolean, number, text, select, slider)
  - Raw content (HTML, CSS, JavaScript)

## Verifica

Dopo l'upload, controlla il serial monitor per:
1. ✓ Messaggio di mounting LittleFS
2. ✓ Visualizzazione del config v1.0 originale
3. ✓ Messaggio di upgrade
4. ✓ Visualizzazione del config v2.0 convertito
5. ✓ Conferma dei percorsi file
