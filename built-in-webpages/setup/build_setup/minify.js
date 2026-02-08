// Finalize Nodejs Script
// 1 - Append JS in HTML Document
// 2 - Gzip HTML
// 3 - Convert to Raw Bytes
// 4 - ( Save to File: webpage.h ) in dist Folder

const fs = require('fs');
const path = require('path');
const readline = require('readline');
// node-minify v10+: CommonJS usage requires named exports
const { minify } = require('@node-minify/core');
const { terser } = require('@node-minify/terser');
const { cssnano } = require('@node-minify/cssnano');
const { htmlMinifier } = require('@node-minify/html-minifier');
const converter = require('./stringConverter');
const { createGzip, constants } = require('zlib');
const { pipeline } = require('stream');
const { createReadStream, createWriteStream } = require('fs');

// Output folder for the generated .h files
// Path: repo-root/src/assets (relative to the location of this script)
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

    console.log('App JS minified');

    // 2. Minify JS (Creds)
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

    console.log('CSS minified');

    // 4. Inline into HTML
    let html = fs.readFileSync('../setup.htm', 'utf8');

    const css = fs.readFileSync('./min/style.css');
    const appjs = fs.readFileSync('./min/app.js');

    // Convert buffers to string for template literal
    const cssStr = css.toString();
    const jsStr = appjs.toString();

    console.log(`CSS Size: ${css.length} -> ${cssStr.length}`);
    console.log(`JS Size: ${appjs.length} -> ${jsStr.length}`);

    // Replace the external CSS/JS references in setup.htm with inlined, minified content.
    // Use regex to be robust to attribute ordering, quotes and extra attributes (e.g. defer).
    html = html.replace(/<link[^>]*href=["']style\.css["'][^>]*>/i, `<style>${cssStr}</style>`);
    html = html.replace(/<script[^>]*src=["']app\.js["'][^>]*><\/script>/i, `<script>${jsStr}</script>`);

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

    // 7. Gzip Logo -> logo_svg.h
    await new Promise((resolve, reject) => {
        const gzip = createGzip({ level: constants.Z_BEST_COMPRESSION });
        const source = createReadStream('../logo.svg');
        const destination = createWriteStream('./min/logo.svg.gz');

      pipeline(source, gzip, destination, async (err) => {
            if (err) return reject(err);
            try {
                const c_array = converter.toString(fs.readFileSync('./min/logo.svg.gz'), 16, '_aclogo_svg');
          const targetPath = path.join(outputDir, 'logo_svg.h');
          await writeWithConfirm(targetPath, c_array);
                console.log('Logo SVG processed');
                resolve();
            } catch(e) { reject(e); }
        })
    });

  } catch (err) {
    console.error('Build failed:', err);
  }
}

build();