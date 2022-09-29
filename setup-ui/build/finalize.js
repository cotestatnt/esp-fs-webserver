// Finalize Nodejs Script
// 1 - Append JS in HTML Document
// 2 - Gzip HTML
// 3 - Covert to Raw Bytes
// 4 - ( Save to File: webpage.h ) in dist Folder

const fs = require('fs');
const { gzip } = require('@gfx/zopfli');
const minify = require('@node-minify/core');
const uglifyES = require('@node-minify/uglify-es');
const cssnano = require('@node-minify/cssnano');
const htmlMinifier = require('@node-minify/html-minifier');

var dir = 'min/pico/css';
if (!fs.existsSync(dir)){
  fs.mkdirSync(dir, { recursive: true });
}

minify({
  compressor: uglifyES,
  input: '../data/app.js',
  output: 'min/app.js',
  callback: function(err, min) {}
});

minify({
  compressor: cssnano,
  input: '../data/style.css',
  output: 'min/style.css',
  callback: function(err, min) {}
});

minify({
  compressor: cssnano,
  input: '../data/pico/css/pico.fluid.classless.css',
  output: 'min/pico/css/pico.fluid.classless.css',
  callback: function(err, min) {}
});

let html = fs.readFileSync('../data/setup.htm').toString();
let pico = fs.readFileSync('min/pico/css/pico.fluid.classless.css');
let css = fs.readFileSync('min/style.css');
let appjs = fs.readFileSync('min/app.js');

html = html.replace('<link type="text/css" rel="stylesheet" href="/pico/css/pico.fluid.classless.css" />', `<style>${pico}</style>` );
html = html.replace('<link type="text/css" rel="stylesheet" href="/style.css" />', `<style>${css}</style>`);
html = html.replace('<script src="app.js"></script>', `<script>${appjs}</script>` );

fs.writeFile('min/all.htm', html, function (err) {
  if (err) return console.log(err);
});

// Gzip the result html file
const input =  fs.readFileSync('min/all.htm');
gzip(input, {numiterations: 15}, (err, output) => {
  indexHTML = output;
  let source =
  `
  const uint32_t WEBPAGE_HTML_SIZE = ${indexHTML.length};
  const char WEBPAGE_HTML[] PROGMEM = { ${indexHTML} };
  `;
  fs.writeFileSync('setup_htm.h', source, 'utf8');
});