# Proposta di Nuova Struttura JSON per la Pagina /setup

## Analisi della Struttura Attuale

### Problemi Identificati

1. **JSON Piatto**: Il config.json attuale è a un singolo livello, con chiavi che hanno significati diversi in base al loro nome:
   - Chiavi con prefisso `param-box` = creano una nuova sezione nella UI
   - Chiavi con prefisso `raw-html-`, `raw-css-`, `raw-javascript-` = caricano file CSS/JS/HTML
   - Chiavi con prefisso `img-` e `name-` = vengono ignorate nel rendering
   - Chiavi normali = diventano input, checkbox, slider, select a seconda del tipo

2. **Logica Complessa nel Frontend**: La funzione `createOptionsBox()` deve interpretare molteplici pattern di denominazione

3. **Mancanza di Metadati**: Non c'è modo di descrivere un'opzione senza avere il dato stesso (etichette, intervalli, lista di valori)

4. **Difficile da Mantenere**: Aggiungere nuove tipologie di elementi richiede modifiche sia al backend che al frontend

### Esempio Struttura Attuale
```json
{
  "port": 80,
  "host": "myserver",
  "param-box1": "My Options",
  "The LED pin number": 2,
  "A bool variable": true,
  "A float variable": 15.5,
  "A String variable": "Test",
  "A dropdown listbox": {
    "values": ["Item1", "Item2", "Item3"],
    "selected": "Item1"
  },
  "raw-html-fetch-test": "/config/raw-html-fetch-test.htm",
  "param-box2": "ThingsBoard",
  "Device Name": "ESP Sensor"
}
```

---

## Proposta: Nuova Struttura JSON Gerarchica

### Principi di Design

1. **Versione della Libreria**: Identificare il formato JSON per gestire migrazioni future
2. **Metadati Espliciti**: Ogni elemento UI ha una struttura chiara con proprietà ben definite
3. **Separazione di Responsabilità**: 
   - `_version` e `_meta` = informazioni di sistema
   - `_state` = opzioni che non mostrano UI (variabili di stato persistenti)
   - `sections` = array di sezioni UI
4. **Tipo Esplicito**: Ogni elemento specifica il suo tipo
5. **Estensibilità**: Facile aggiungere nuovi tipi di controlli

### Struttura Proposta

```json
{
  "_version": "2.0",
  "_meta": {
    "app_title": "Custom HTML Web Server",
    "logo": "/config/logo.svg",
    "port": 80,
    "host": "myserver"
  },
  "_state": {
    "internal_flag1": true,
    "api_key": "secret_value",
    "last_updated": "2025-02-09T10:30:00Z"
  },
  "sections": [
    {
      "id": "my-options",
      "title": "My Options",
      "elements": [
        {
          "id": "led-pin",
          "type": "number",
          "label": "The LED pin number",
          "value": 2,
          "min": 0,
          "max": 40
        },
        {
          "id": "bool-var",
          "type": "boolean",
          "label": "A bool variable",
          "value": true
        },
        {
          "id": "float-var",
          "type": "number",
          "label": "A float variable",
          "value": 15.5,
          "min": 0.0,
          "max": 100.0,
          "step": 0.01
        },
        {
          "id": "string-var",
          "type": "text",
          "label": "A String variable",
          "value": "Test option String"
        },
        {
          "id": "dropdown-test",
          "type": "select",
          "label": "A dropdown listbox",
          "value": "Item1",
          "options": ["Item1", "Item2", "Item3"]
        },
        {
          "id": "slider-example",
          "type": "slider",
          "label": "Slider Value",
          "value": 50,
          "min": 0,
          "max": 100,
          "step": 1
        }
      ]
    },
    {
      "id": "custom-html",
      "title": "Custom HTML",
      "elements": [
        {
          "id": "html-content",
          "type": "raw-html",
          "src": "/config/custom-html.htm"
        }
      ]
    },
    {
      "id": "thingsboard",
      "title": "ThingsBoard",
      "elements": [
        {
          "id": "tb-device-name",
          "type": "text",
          "label": "Device Name",
          "value": "ESP Sensor"
        },
        {
          "id": "tb-latitude",
          "type": "number",
          "label": "Device Latitude",
          "value": 41.88505,
          "min": -180.0,
          "max": 180.0,
          "step": 0.00001
        },
        {
          "id": "tb-longitude",
          "type": "number",
          "label": "Device Longitude",
          "value": 12.50050,
          "min": -180.0,
          "max": 180.0,
          "step": 0.00001
        },
        {
          "id": "tb-server",
          "type": "text",
          "label": "ThingsBoard server address",
          "value": "thingsboard.cloud"
        },
        {
          "id": "tb-port",
          "type": "number",
          "label": "ThingsBoard server port",
          "value": 80,
          "min": 1,
          "max": 65535
        },
        {
          "id": "tb-device-token",
          "type": "text",
          "label": "ThingsBoard device token",
          "value": "xxxxxxxxxxxxxxxxxxx"
        },
        {
          "id": "tb-device-key",
          "type": "text",
          "label": "Provisioning device key",
          "value": "xxxxxxxxxxxxxxxxxxx"
        },
        {
          "id": "tb-secret-key",
          "type": "text",
          "label": "Provisioning secret key",
          "value": "xxxxxxxxxxxxxxxxxxx"
        }
      ]
    }
  ],
  "_assets": {
    "css": ["/config/custom-style.css"],
    "javascript": ["/config/custom-script.js", "/config/thingsboard-script.js"]
  }
}
```

---

## Mappatura Tipi di Elementi

| Tipo | Descrizione | Proprietà | Note |
|------|-------------|-----------|------|
| `text` | Input testuale | `label`, `value`, `placeholder` | Stringa semplice |
| `number` | Input numerico | `label`, `value`, `min`, `max`, `step` | Float o int |
| `boolean` | Toggle checkbox | `label`, `value` | true/false |
| `select` | Dropdown/Select | `label`, `value`, `options[]` | Array di stringhe |
| `slider` | Range slider | `label`, `value`, `min`, `max`, `step` | Numero con readout opzionale |
| `raw-html` | HTML personalizzato | `src` | Path a file HTML |
| `raw-css` | Foglio di stile | `src` | Path a file CSS (preferibilmente in `_assets`) |
| `raw-javascript` | Script JS | `src` | Path a file JS (preferibilmente in `_assets`) |

### Proprietà Comuni Opzionali
- `id`: identificatore univoco (obbligatorio per il binding)
- `label`: etichetta UI (obbligatorio per elementi interattivi)
- `description`: tooltip/help text
- `placeholder`: testo di esempio (per text input)
- `readonly`: se true, disabilita l'edit
- `hidden`: se true, nasconde l'elemento

---

## Sezione _state (Opzioni di Sistema)

La sezione `_state` contiene valori che NON devono creare elementi UI ma che devono essere persistiti nel JSON:

```json
"_state": {
  "internal_config_flag": true,
  "api_authentication_key": "secret",
  "last_sync_timestamp": "2025-02-09T10:30:00Z",
  "database_connection_string": "server=localhost;user=admin"
}
```

**Vantaggi:**
- Chiaramente separati dagli elementi UI
- Non inquinano lo spazio dei nomi degli elementi
- Facili da gestire con un unico loop nel backend

---

## Migrazione dal Formato v1 al v2

### Strategy di Upgrade Automatico

Nel backend (SetupConfig.hpp e FSWebServer.h):

```cpp
// Funzione di upgrade del config.json
bool upgradeConfigFromV1() {
    if (!m_filesystem->exists(ESP_FS_WS_CONFIG_FILE)) return false;
    
    // Leggi il vecchio config
    File file = m_filesystem->open(ESP_FS_WS_CONFIG_FILE, "r");
    CJSON::Json oldDoc;
    oldDoc.parse(file.readString());
    
    // Crea il nuovo documento v2
    CJSON::Json newDoc;
    newDoc.createObject();
    newDoc.setString("_version", "2.0");
    
    // Copia metadati speciali
    newDoc.ensureObject("_meta");
    double port;
    if (oldDoc.getNumber("port", port)) {
        newDoc.setNumber("_meta", "port", port);
    }
    // ... copiare altri campi
    
    // Crea una sezione di default per le opzioni non categorizzate
    newDoc.ensureArray("sections");
    CJSON::Json defaultSection;
    defaultSection.createObject();
    defaultSection.setString("id", "legacy-options");
    defaultSection.setString("title", "Options");
    // ... convertire elementi uno per uno
    
    // Salva il nuovo config
    // ...
}
```

Nel frontend (app.js), prima di `createOptionsBox()`:

```javascript
// Controlla versione e esegui upgrade se necessario
const config = await fetch(...).then(r => r.json());
if (!config._version || config._version !== "2.0") {
    // Fallback al vecchio parser o mostra messaggio di errore
    console.warn("Config v1 detected, using legacy parser");
    createOptionsBoxLegacy(config);
} else {
    createOptionsBoxNew(config);
}
```

---

## Vantaggi della Nuova Struttura

✅ **Più Lineare**: Ogni elemento ha una struttura chiara e coerente  
✅ **Autodescrittivo**: Il JSON contiene tutti i metadati necessari  
✅ **Manutenibile**: Aggiungere nuovi tipi di controlli è semplice  
✅ **Scalabile**: Supporta sezioni illimitate e elementi nidificati in futuro  
✅ **Retrocompatibile**: È possibile rilevare la versione e convertire il vecchio formato  
✅ **Separazione Dati-UI**: Gli stati interni non inquinano la UI  
✅ **Meno Magia nel Codice**: Niente pattern di naming e interpretazione euristica  

---

## Implementazione Proposta

### Fase 1: Backend (SetupConfig.hpp)
- Aggiungere metodi per costruire la nuova struttura
- Mantener compatibilità con i vecchi metodi (deprecati)
- Aggiungere funzione di upgrade automatico

### Fase 2: Frontend (app.js)
- Creare parser per la nuova struttura
- Mantenere parser legacy per backward compatibility
- Rifattorizzare `createOptionsBox()` in versione moderna

### Fase 3: Documentazione
- Aggiornare API documentation
- Aggiungere esempi di migrazione
- Deprecare metodi v1

---

## Esempio di Transizione nel Codice C++

### Prima (v1 - ancora supportato)
```cpp
server.addOptionBox("My Options");
server.addOption("The LED pin number", ledPin);
server.addOption("A float variable", floatVar, 0.0, 100.0, 0.01);
```

### Dopo (v2 - nuovo approccio)
```cpp
// Opzione A: Manteniamo i metodi old-style ma internamente usano v2
server.addOptionBox("My Options");
server.addOption("The LED pin number", ledPin);

// Opzione B: Nuovo approccio strutturato (futuro)
auto myOptions = server.createSection("my-options", "My Options");
myOptions.addNumberOption("led-pin", "The LED pin number", ledPin, 0, 40, 1);
myOptions.addNumberOption("float-var", "A float variable", floatVar, 0.0, 100.0, 0.01);
myOptions.finalize();
```

---

## Considerazioni Aggiuntive

1. **ID Univoci**: Ogni elemento deve avere un `id` univoco per il binding JavaScript
2. **Validazione**: Aggiungere validazione schema JSON nel backend
3. **Performance**: Il JSON più strutturato è lievemente più grande, ma il frontend è più veloce
4. **Estensioni Future**: 
   - Elementi nidificati (array, oggetti complessi)
   - Validazione client-side
   - Condizionalità (mostra elemento se...)
5. **Localizzazione**: La struttura supporta facilmente i18n tramite chiavi

---

## Conclusioni

Questa proposta di nuova struttura JSON rende la pagina `/setup` molto più **lineare, manutenibile e scalabile**, eliminando la necessità di pattern di naming complessi e logica di interpretazione nel frontend.

La **retrocompatibilità** può essere mantenuta tramite rilevamento automatico della versione e upgrade del vecchio formato.

