If you want customize **/setup** webpage (headers, logo etc etc):
* edit the source files as your needs (setup.htm, app.js, style.css)
* open a terminal in ***build_setup*** folder and run `npm i` to install all nodejs modules needed
* run `node minify.js`
* overwrite the content of `setup_htm.h` inside `/src` folder with the new generated file


#
As alternative on Windows system, is possible to use the tool [SEGGER Bin2C](https://www.segger.com/free-utilities/bin2c/).

`all.htm.gz` file need to be prepared manually (for example you could use online tools for minify CSS, JavaScript and HTML)

This Bin2C program will create a file named `all.htm.c`, simply copy & past the content and overwrite `setup_htm.h` file