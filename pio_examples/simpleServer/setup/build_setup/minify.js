// Finalize Nodejs Script
// 1 - Append JS in HTML Document
// 2 - Gzip HTML
// 3 - Covert to Raw Bytes
// 4 - ( Save to File: webpage.h ) in dist Folder

const fs = require('fs');
const minify = require('@node-minify/core');
const terser = require('@node-minify/terser');
const cssnano = require('@node-minify/cssnano');
const converter = require('./stringConverter');
const { createGzip, constants } = require('zlib');
const { pipeline } = require('stream');
const { createReadStream, createWriteStream } = require('fs');

async function build() {
  try {
    console.log('Starting build...');

    // 1. Minify JS (App)
    await minify({
      compressor: terser,
      input: '../app.js',
      output: './min/app.js'
    });
    console.log('App JS minified');

    // 2. Minify JS (Creds)
    await minify({
      compressor: terser,
      input: '../creds.js',
      output: './min/creds.js'
    });
    console.log('Creds JS minified');

    // Copy creds to data
    try {
        fs.copyFileSync('./min/creds.js', '../../data/creds.js');
        console.log('creds.js copied to data/');
    } catch(e) { console.error('Copy creds failed', e); }

    // 3. Minify CSS
    await minify({
      compressor: cssnano,
      input: '../style.css',
      output: './min/style.css'
    });
    console.log('CSS minified');

    // 4. Inline into HTML
    let html = fs.readFileSync('../setup.htm').toString();
    const css = fs.readFileSync('./min/style.css'); 
    const appjs = fs.readFileSync('./min/app.js');

    // Convert buffers to string for template literal
    const cssStr = css.toString();
    const jsStr = appjs.toString();

    console.log(`CSS Size: ${css.length} -> ${cssStr.length}`);
    console.log(`JS Size: ${appjs.length} -> ${jsStr.length}`);

    html = html.replace('<link href=style.css rel=stylesheet>', `<style>${cssStr}</style>`);
    html = html.replace('<script src=app.js></script>', `<script>${jsStr}</script>`);

    fs.writeFileSync('./min/all.htm', html);
    console.log('all.htm created');

    // 5. Gzip HTML -> setup_htm.h
    await new Promise((resolve, reject) => {
        const gzip = createGzip({ level: constants.Z_BEST_COMPRESSION });
        const source = createReadStream('./min/all.htm');
        const destination = createWriteStream('./min/all.htm.gz');

        pipeline(source, gzip, destination, (err) => {
            if (err) return reject(err);
            try {
                const c_array = converter.toString(fs.readFileSync('./min/all.htm.gz'), 16, '_acsetup_min_htm');
                fs.writeFileSync('setup_htm.h', c_array, 'utf8');
                console.log('setup_htm.h created');
                resolve();
            } catch(e) { reject(e); }
        })
    });

    // 6. Gzip Creds -> creds_js.h
    await new Promise((resolve, reject) => {
        const gzip = createGzip({ level: constants.Z_BEST_COMPRESSION });
        const source = createReadStream('./min/creds.js');
        const destination = createWriteStream('./min/creds.js.gz');

        pipeline(source, gzip, destination, (err) => {
            if (err) return reject(err);
            try {
                const c_array = converter.toString(fs.readFileSync('./min/creds.js.gz'), 16, '_accreds_js');
                fs.writeFileSync('creds_js.h', c_array, 'utf8');
                console.log('creds_js.h created');
                resolve();
            } catch(e) { reject(e); }
        })
    });

  } catch (err) {
    console.error('Build failed:', err);
  }
}

build();