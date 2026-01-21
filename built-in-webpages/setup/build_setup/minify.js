// Finalize Nodejs Script
// 1 - Append JS in HTML Document
// 2 - Gzip HTML
// 3 - Covert to Raw Bytes
// 4 - ( Save to File: webpage.h ) in dist Folder

const fs = require('fs');
const path = require('path');
const readline = require('readline');
const minify = require('@node-minify/core');
const terser = require('@node-minify/terser');
const cssnano = require('@node-minify/cssnano');
const htmlMinifier = require('@node-minify/html-minifier');
const converter = require('./stringConverter');
const { createGzip, constants } = require('zlib');
const { pipeline } = require('stream');
const { createReadStream, createWriteStream } = require('fs');

// Directory di output per i file .h generati
// Percorso: repo-root/src/assets (relativo alla posizione di questo script)
const outputDir = path.resolve(__dirname, '../../../src/assets');

function askOverwrite(filePath) {
  return new Promise((resolve) => {
    const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
    rl.question(`File ${filePath} esiste già. Sovrascrivere? [y/N]: `, (answer) => {
      rl.close();
      const ans = String(answer).trim().toLowerCase();
      resolve(ans === 'y' || ans === 'yes' || ans === 's' || ans === 'si');
    });
  });
}

async function writeWithConfirm(filePath, data) {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });

  if (fs.existsSync(filePath)) {
    const ok = await askOverwrite(filePath);
    if (!ok) {
      console.log(`Scrittura annullata per ${filePath}`);
      return false;
    }
  }

  fs.writeFileSync(filePath, data, 'utf8');
  console.log(`File scritto: ${filePath}`);
  return true;
}

// MAPPING FOR AGGRESSIVE RENAMING
// ATTENZIONE: viene applicato a HTML, CSS e JS come semplice sostituzione di stringa.
// Per evitare di rompere i tag HTML o nomi troppo generici, qui usiamo SOLO classi/ID
// "parlanti" con trattino che non coincidono con nomi di tag o parole comuni.
const mangleMap = {
  // Classi layout / contenitori più lunghe e specifiche
  'slider-wrapper': 'sw',
  'slider-readout': 'sr',
  'tf-wrapper': 'tw',
  'opt-box': 'ob',
  'heading-2': 'h2',
  'row-wrapper': 'rw',
  'btn-bar': 'bb',
  'inline-creds': 'ic',
  'show-hide-wrap': 'shw',
  'fw-upload': 'fwu',
  'progress-wrap': 'pw1',
  'esp-info': 'ei',
  'svg-menu-icon': 'smi',
  'chevron-networks': 'cn',
  'hide-tiny': 'ht',
  'upload-note': 'un',
  'raw-html': 'rh',

  // Classi di input/toggle più specifiche
  'input-label': 'il',
  'opt-input': 'oi',
  'toggle-switch': 'ts',
  'toggle-label': 'tl',

  // IDs usati solo come riferimenti DOM (NON config JSON)
  // NOTA: 'img-logo' e 'name-logo' sono chiavi di configurazione speciali
  // nel file JSON e NON vanno manglate, altrimenti la logica in app.js
  // non le riconosce più e smette di aggiornare logo/titolo.
  'main-box': 'mb',
  'top-nav': 'tn',
  'nav-link': 'nl',
  'wifi-table': 'wt',
  'show-pass': 'sp',
  'hide-pass': 'hp',
  'conf-wifi': 'cw',
  'save-wifi': 'swi',

  // ID contenitori per le icone SVG (solo DOM)
  'svg-menu': 'sm',
  'svg-eye': 'sy',
  'svg-no-eye': 'sne',
  'svg-scan': 'ssc',
  'svg-connect': 'sc',
  'svg-save': 'ssa',
  'svg-save2': 'ss2',
  'svg-restart': 'sr',
  'svg-delete': 'sdel',
  'svg-clear': 'sclr'
};

function applyMangle(content) {
  let res = content;
  // Ordina per lunghezza decrescente per evitare conflitti tra chiavi simili (es. btn / btn-bar)
  const entries = Object.entries(mangleMap).sort((a, b) => b[0].length - a[0].length);
  for (const [key, val] of entries) {
    res = res.split(key).join(val);
  }
  return res;
}

async function build() {
  try {
    console.log('Starting build...');

    // 1. Minify JS (App)
    await minify({
      compressor: terser,
      input: '../app.js',
      output: './min/app.js',
      options: {
        toplevel: false, // Must be false so global vars (wifiCredentials, etc) are accessible by creds.js
        mangle: { properties: false }
      }
    });
    
    // Apply Mangle to App JS
    let appVal = fs.readFileSync('./min/app.js', 'utf8');
    appVal = applyMangle(appVal);
    fs.writeFileSync('./min/app.js', appVal);
    console.log('App JS minified & mangled');

    // 2. Minify JS (Creds)
    // IMPORTANTE: non applichiamo applyMangle qui, per non rompere
    // le funzioni globali e la logica condivisa con app.js.
    await minify({
      compressor: terser,
      input: '../creds.js',
      output: './min/creds.js',
      options: {
        toplevel: false // KEEP FALSE to satisfy dynamic loading requirement!
      }
    });

    console.log('Creds JS minified (no extra mangle)');

    // 3. Minify CSS
    await minify({
      compressor: cssnano,
      input: '../style.css',
      output: './min/style.css'
    });
    
    // Apply Mangle to CSS
    let cssVal = fs.readFileSync('./min/style.css', 'utf8');
    cssVal = applyMangle(cssVal);
    fs.writeFileSync('./min/style.css', cssVal);
    console.log('CSS minified & mangled');

    // 4. Inline into HTML
    let html = fs.readFileSync('../setup.htm').toString();
    
    // Apply Mangle to HTML Source
    html = applyMangle(html);

    const css = fs.readFileSync('./min/style.css'); 
    const appjs = fs.readFileSync('./min/app.js');

    // Convert buffers to string for template literal
    const cssStr = css.toString();
    const jsStr = appjs.toString();

    console.log(`CSS Size: ${css.length} -> ${cssStr.length}`);
    console.log(`JS Size: ${appjs.length} -> ${jsStr.length}`);

    html = html.replace('<link href=style.css rel=stylesheet>', `<style>${cssStr}</style>`);
    html = html.replace('<script src=app.js></script>', `<script>${jsStr}</script>`);

    /*
    // Minify HTML
    await minify({
      compressor: htmlMinifier,
      content: html,
      options: {
        collapseWhitespace: true,
        removeComments: true,
        removeAttributeQuotes: true,
        minifyCSS: true,
        minifyJS: true
      }
    }).then(min => {
        html = min;
        console.log('HTML Minified');
    });
    */

    fs.writeFileSync('./min/all.htm', html);
    console.log('all.htm created');

    // 5. Gzip HTML -> setup_htm.h
    await new Promise((resolve, reject) => {
        const gzip = createGzip({ level: constants.Z_BEST_COMPRESSION });
        const source = createReadStream('./min/all.htm');
        const destination = createWriteStream('./min/all.htm.gz');

      pipeline(source, gzip, destination, async (err) => {
            if (err) return reject(err);
            try {
                const c_array = converter.toString(fs.readFileSync('./min/all.htm.gz'), 16, '_acsetup_min_htm');
          const targetPath = path.join(outputDir, 'setup_htm.h');
          await writeWithConfirm(targetPath, c_array);
                resolve();
            } catch(e) { reject(e); }
        })
    });

    // 6. Gzip Creds -> creds_js.h
    await new Promise((resolve, reject) => {
        const gzip = createGzip({ level: constants.Z_BEST_COMPRESSION });
        const source = createReadStream('./min/creds.js');
        const destination = createWriteStream('./min/creds.js.gz');

      pipeline(source, gzip, destination, async (err) => {
            if (err) return reject(err);
            try {
                const c_array = converter.toString(fs.readFileSync('./min/creds.js.gz'), 16, '_accreds_js');
          const targetPath = path.join(outputDir, 'creds_js.h');
          await writeWithConfirm(targetPath, c_array);
                resolve();
            } catch(e) { reject(e); }
        })
    });

  } catch (err) {
    console.error('Build failed:', err);
  }
}

build();