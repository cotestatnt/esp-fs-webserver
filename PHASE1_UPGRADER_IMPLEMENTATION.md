# Implementazione ConfigUpgrader - Phase 1

## Overview

È stata creata una nuova classe `ConfigUpgrader` dedicata alla migrazione del formato JSON da v1 (flat) a v2 (hierarchical). La classe implementa il pattern **Single Responsibility**: una sola classe, una sola responsabilità.

## Struttura della Classe

### File: `src/ConfigUpgrader.hpp`

```cpp
class ConfigUpgrader {
public:
    ConfigUpgrader(fs::FS* filesystem, const char* configFile);
    
    /**
     * Auto-detects version and upgrades if needed
     * - Se _version == "2.0" → passa oltre (no-op)
     * - Se _version mancante o diversa → esegui upgrade
     */
    bool upgrade();
    
private:
    bool upgradeFromV1(CJSON::Json& oldDoc, CJSON::Json& newDoc);
    void convertV1Option(CJSON::Json& oldDoc, const String& key, std::vector<CJSON::Json>& elements);
    std::vector<String> getAllKeys(CJSON::Json& doc);
    std::vector<String> getArrayAsStrings(CJSON::Json& doc, const char* objKey, const char* arrayKey);
    String generateId(const String& label);
};
```

## Flusso di Utilizzo

### In SetupConfig.hpp

```cpp
void upgradeConfigIfNeeded() {
    ConfigUpgrader upgrader(m_filesystem, ESP_FS_WS_CONFIG_FILE);
    upgrader.upgrade();  // Crea istanza → converte se v1 → distrugge automaticamente
}
```

Viene chiamato in `openConfiguration()` prima di leggere il file:

```cpp
bool openConfiguration() {
    if (checkConfigFile()) {
        upgradeConfigIfNeeded();  // Auto-upgrade se v1
        // ... continua normalmente
    }
}
```

## Logica di Conversione v1 → v2

### 1. Rilevamento Versione
```cpp
String version;
if (oldDoc.getString("_version", version) && version.equals("2.0")) {
    return true;  // Nulla da fare
}
```

### 2. Estrazione Metadati
- `page-title` → `_meta.app_title`
- `img-logo` → `_meta.logo`
- `port` → `_meta.port`
- `host` → `_meta.host`

### 3. Gestione Sezioni
- Chiavi `param-box*` → nuove sezioni
- Gli elementi tra una sezione e l'altra vengono raggruppati automaticamente
- Ogni sezione ha `id`, `title`, e array di `elements`

### 4. Conversione Tipi di Elemento

| Pattern v1 | Tipo v2 | Proprietà |
|-----------|---------|-----------|
| `"value": bool` | `boolean` | `value: bool` |
| `"value": number` | `number` | `value: number, min, max, step` |
| `"value": string` | `text` | `value: string` |
| `{selected, values}` | `select` | `value: string, options[]` |
| `{type: "slider", value, min, max, step}` | `slider` | `value, min, max, step` |
| `raw-html-*` | `raw-html` | `src: path` |
| `raw-css-*` | `raw-css` | `src: path` |
| `raw-javascript-*` | `raw-javascript` | `src: path` |

### 5. Chiavi Ignorate
- `page-title` (già estratto in `_meta`)
- `img-logo` (già estratto in `_meta`)
- `port`, `host` (già estratti in `_meta`)
- `img-*`, `name-*` (inutilizzate, scartate)

## Esempio Conversione

### Input (v1)
```json
{
  "page-title": "Custom Options Example",
  "img-logo": "/config/logo.png",
  "param-box2": "My Options",
  "A bool variable": true,
  "The LED pin number": 97,
  "A float variable": {
    "value": 15.51,
    "min": 1,
    "max": 100,
    "step": 0.01,
    "type": "number"
  },
  "Days of week": {
    "selected": "Wednesday",
    "values": ["Monday", "Tuesday", ..., "Sunday"]
  },
  "Brightness": {
    "value": 50,
    "min": 0,
    "max": 100,
    "step": 1,
    "type": "slider"
  },
  "raw-html-buttons": "/config/raw-html-buttons.htm",
  "port": 80,
  "host": "myserver"
}
```

### Output (v2.0)
```json
{
  "_version": "2.0",
  "_meta": {
    "app_title": "Custom Options Example",
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
        {
          "id": "the-led-pin-number",
          "type": "number",
          "label": "The LED pin number",
          "value": 97
        },
        ...
      ]
    }
  ]
}
```

## Gestione della Memoria

La classe segue il pattern **RAII** (Resource Acquisition Is Initialization):

```cpp
// Utilizzo
{
    ConfigUpgrader upgrader(fs, "/config/config.json");
    upgrader.upgrade();  // Istanza utilizzata
}  // Distruttore automatico, nulla lasciato in memoria
```

- Non allocazioni dinamiche interne
- Istanza locale dello stack
- Automaticamente distrutta quando esce dallo scope

## Prossimi Passi

### Phase 2: Frontend (app.js)
Aggiungere parser per v2.0:

```javascript
// In setup.js / app.js
async function loadConfig() {
    const config = await fetch('/api/config').then(r => r.json());
    
    if (!config._version || config._version !== "2.0") {
        console.warn("Config v1 detected, using legacy parser");
        return createLegacyUI(config);
    }
    
    return createModernUI(config);
}
```

### Phase 3: Nuovi Metodi in SetupConfig.hpp (v2.0 API)

Per la prossima fase, aggiungeremo nuovi metodi helper per costruire la struttura v2.0:

```cpp
class SetupConfigurator {
    // ... metodi vecchi mantengono compatibilità
    
    // Nuovi metodi per v2.0
    Section* createSection(const char* id, const char* title);
    void addNumberOption(const char* id, const char* label, double val, double min, double max);
    void addBooleanOption(const char* id, const char* label, bool val);
    void addSelectOption(const char* id, const char* label, const char** options, size_t size);
};
```

## Test

File di test creati:
- `test_config_v1.json` - Esempio completo v1
- `test_config_v2_expected.json` - Risultato atteso dopo conversione
- `test_upgrade.cpp` - Programma di verifica della struttura

## Validazione

Per verificare che funziona correttamente:

1. Compilare il progetto (compilerà la nuova classe)
2. Eseguire l'esempio `customOptions`
3. Verificare nel serial log:
   ```
   [INFO] ConfigUpgrader: Upgrading config from v1 to v2.0
   [INFO] ConfigUpgrader: Config upgraded and saved successfully
   ```
4. Controllare che `/config/config.json` sia stato convertito a v2.0

## Design Pattern Utilizzati

1. **Single Responsibility**: ConfigUpgrader fa una cosa: l'upgrade
2. **RAII**: Gestione automatica della memoria
3. **Facade**: Interfaccia semplice `upgrade()` nasconde la complessità
4. **Automatic Detection**: Auto-rileva versione e agisce di conseguenza
