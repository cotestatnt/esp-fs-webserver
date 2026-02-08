// Finalize Nodejs Script for Edit page
// 1 - Inline CSS and JS in HTML
// 2 - Minify HTML (optional)
// 3 - Gzip HTML
// 4 - Convert to Raw Bytes
// 5 - Save to File: edit_htm.h in src/assets folder

const fs = require('fs');
const path = require('path');
const readline = require('readline');
const { minify } = require('@node-minify/core');
const { htmlMinifier } = require('@node-minify/html-minifier');
const converter = require('./stringConverter');
const { createGzip, constants } = require('zlib');
const { pipeline } = require('stream');
const { createReadStream, createWriteStream } = require('fs');

// Output folder for the generated .h files
const outputDir = path.resolve(__dirname, '../../../src/assets');

function askOverwrite(filePath) {
  return new Promise((resolve) => {
    const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
    rl.question(`File ${filePath} already exists. Overwrite? [y/N]: `, (answer) => {
      rl.close();
      const ans = String(answer).trim().toLowerCase();
      resolve(ans === 'y' || ans === 'yes');
    });
  });
}

async function writeWithConfirm(filePath, data) {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });

  if (fs.existsSync(filePath)) {
    const ok = await askOverwrite(filePath);
    if (!ok) {
      console.log(`Write cancelled for ${filePath}`);
      return false;
    }
  }

  fs.writeFileSync(filePath, data, 'utf8');
  console.log(`File written: ${filePath}`);
  return true;
}

async function build() {
  try {
    console.log('Starting edit page build...');

    // Create min directory if it doesn't exist
    if (!fs.existsSync('./min')) {
      fs.mkdirSync('./min');
    }

    // 1. Read HTML and inline all content (CSS and JS are already inline in edit.htm)
    let html = fs.readFileSync('../edit.htm', 'utf8');
    
    console.log(`Original HTML size: ${html.length} bytes`);

    // 2. Optional: Minify HTML (commented out for safety - enable if desired)
    /*
    html = await minify({
      compressor: htmlMinifier,
      content: html,
      options: {
        collapseWhitespace: true,
        removeComments: true,
        minifyCSS: true,
        minifyJS: true,
        // Keep attribute quotes for compatibility
        removeAttributeQuotes: false
      }
    });
    console.log(`Minified HTML size: ${html.length} bytes`);
    */

    // Write intermediate file
    fs.writeFileSync('./min/edit.htm', html);
    console.log('edit.htm prepared');

    // 3. Gzip HTML -> edit_htm.h
    await new Promise((resolve, reject) => {
      const gzip = createGzip({ level: constants.Z_BEST_COMPRESSION });
      const source = createReadStream('./min/edit.htm');
      const destination = createWriteStream('./min/edit.htm.gz');

      pipeline(source, gzip, destination, async (err) => {
        if (err) return reject(err);
        try {
          const gzipped = fs.readFileSync('./min/edit.htm.gz');
          console.log(`Gzipped size: ${gzipped.length} bytes`);
          
          const c_array = converter.toString(gzipped, 16, '_acedit_htm');
          const targetPath = path.join(outputDir, 'edit_htm.h');
          await writeWithConfirm(targetPath, c_array);
          console.log('Edit page processed successfully');
          resolve();
        } catch(e) { 
          reject(e); 
        }
      });
    });

  } catch (err) {
    console.error('Build failed:', err);
  }
}

build();
