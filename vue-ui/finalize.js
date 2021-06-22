// Finalize Nodejs Script
// 1 - Append JS in HTML Document
// 2 - Gzip HTML
// 3 - Covert to Raw Bytes
// 4 - ( Save to File: webpage.h ) in dist Folder

const fs = require('fs');
const { gzip } = require('@gfx/zopfli');

function getByteArray(file){
    let fileData = file.toString('hex');
    let result = [];
    for (let i = 0; i < fileData.length; i+=2)
      result.push('0x'+fileData[i]+''+fileData[i+1]);
    return result;
}

let js = fs.readFileSync(__dirname+'/dist/js/app.js');
let html = `
<!DOCTYPE html>
<html lang="">
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width,initial-scale=1.0">
    <link rel="shortcut icon" href="data:image/x-icon;base64,AAABAAEAEBAAAAEAIABoBAAAFgAAACgAAAAQAAAAIAAAAAEAIAAAAAAAAAQAABILAAASCwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIaGhgAAAAAAQ0NDEDs7OzA4ODgrPj4+DFJSUgMoKCgsISEhZSIiInsaGhpfHx8fIZSUlABGRkYAAAAAAAAAAAAAAAAAPDw8QTExMcErKyvtJycn6SQkJL0iIiKiGhoa4xUVFf4RERH/Dg4O/Q0NDdcWFhZU8fHxAUBAQAAAAAAAPz8/HzExMc0qKir/JSUl/yAgIP8cHBz/FxcX/xUVFf8RERH/Dw8P/wsLC/8DAwP/BQUF4xMTEzkJCQkAAAAAADc3N1crKyv3IyMj/0pKSv+IiIj/MjIy/zExMf8WFhb/CwsL/xUVFf9sbGz/VVVV/yUlJf8pKSmhU1NTD+jo6AAyMjJ7JiYm/iEhIf+UlJT/4+Pj/3l5ef+9vb3/Nzc3/1BQUP8pKSn/i4uL/9zc3P/Kysr/uLi4+KKiorh1dXUqODg4qiAgIP84ODj/ycnJ/9TU1P+6urr/2dnZ/01NTf+bm5v/MjIy/4eHh/+ampr/0NDQ/+np6f/R0dH8p6enaygoKKsbGxv/dnZ2/8jIyP+urq7/zc3N/7y8vP9mZmb/jo6O/yUlJf+srKz/cHBw/5ubm/+3t7f/vLy8+MDAwF4pKSlaGxsb8VlZWf9cXFz/Y2Nj/25ubv+BgYH/cHBw/1xcXP8+Pj7/urq6/3h4eP+zs7P/s7Oz/83NzdqsrKwlMjIyCB0dHXoSEhLnDAwM/QsLC/8ICAj/DAwM/x4eHv8zMzP/TExM/729vf+0tLT/vLy8/8LCwv/GxsaQDw8PAzc3NwA/Pz8EICAgLBEREVUODg5xCAgI5QMDA/8VFRX/NTU1/0xMTP9zc3P/m5ub87e3t6+zs7N+kpKSGJ6engAAAAAAAAAAAAAAAAAvLy8Af39/ARcXF1UZGRnQNTU1+UxMTP5ZWVn4YGBg0WNjY2BUVFQGs7OzAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHNzcwD19fUAR0dHGk9PT01XV1diZWVlSHFxcRnY2NgAl5eXAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA//8AAP//AADABwAAgAMAAAADAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAQAA8AcAAPgPAAD//wAA//8AAA==" />
    <title>ESP FS Webserver</title>
  </head>
  <body>
    <noscript>
      <strong>We're sorry but <b>ESP FS Webserver</b> doesn't work properly without JavaScript enabled. Please enable it to continue.</strong>
    </noscript>
    <div id="app"></div>
    <script>${js}</script>
  </body>
</html>
`;
// Gzip the index.html file with JS Code.
// const gzippedIndex = zlib.gzipSync(html, {'level': zlib.constants.Z_BEST_COMPRESSION});
// let indexHTML = getByteArray(gzippedIndex);

// let source =
// `
// const uint32_t WEBPAGE_HTML_SIZE = ${indexHTML.length};
// const char WEBPAGE_HTML[] PROGMEM = { ${indexHTML} };
// `;
// fs.writeFileSync(__dirname+'/dist/webpage.h', source, 'utf8');

// Produce a second variant with zopfli (a improved zip algorithm by google)
// Takes much more time and maybe is not available on every machine
const input =  html;
gzip(input, {numiterations: 15}, (err, output) => {
    indexHTML = output;
    let source =
`
const uint32_t WEBPAGE_HTML_SIZE = ${indexHTML.length};
const char WEBPAGE_HTML[] PROGMEM = { ${indexHTML} };
`;

fs.writeFileSync(__dirname + '/dist/webpage.h', source, 'utf8');

});
