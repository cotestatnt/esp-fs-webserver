
/*
* This HTML code will be injected in /setup webpage using a <div></div> element as parent
* The parent element will hhve the HTML id properties equal to 'raw-html-<id>'
* where the id value will be equal to the id parameter passed to the function addHTML(html_code, id).
*/
static const char custom_html[] = R"EOF(
<label for=url class=input-label>Endpoint</label>
<input type=text placeholder='https://httpbin.org/' id=url value='https://httpbin.org/' />
<br>
<div class=row-wrapper>
  <input type="radio" id="get" name="httpmethod" value="GET" checked>
  <label for="html">GET</label><br>
  <input type="radio" id="post" name="httpmethod" value="POST">
  <label for="css">POST</label>
  <a id=fetch class='btn'>
  <span>Fecth url</span>
  </a>
</div>
<pre id=payload></pre>
)EOF";


/*
* In this example, a style sections is added in order to render properly the new
* <select> and <pre> elements introduced. Since this section will be added at the end of the body,
* it is also possible to override the style of the elements already present:
* for example the background color of body will be ovverrided with a different color
*/
static const char custom_css[] = R"EOF(
pre{
    font-family: Monaco,Menlo,Consolas,'Courier New',monospace;
    color: #333;
    line-height: 20px;
    background-color: #f5f5f5;
    border: 1px solid rgba(0,0,0,0.15);
    border-radius: 6px;
    overflow-y: scroll;
    min-height: 350px;
    font-size: 85%;
    width: 95%;
}
)EOF";


/*
* Also the JavaScript will be added at the bottom of body
* In this example a simple 'click' event listener will be added for the button
* with id='fetch' (added as HTML). The listener will execute the function 'fetchEndpoint'
* in order to fetch a remote resource and show the response in a text box.
*
* The instruction $('<id-name>') is a "Jquery like" selector already defined
* so you can use for your purposes:
*      var $ = function(el) {
*        return document.getElementById(el);
*      };
*/
static const char custom_script[] = R"EOF(
function fetchEndpoint() {
  var mt;
  document.getElementsByName('httpmethod').forEach(el => {
    if (el.checked)
      mt = el.value;
  })

  var url = $('url').value + mt.toLowerCase();
  var bd = (mt != 'GET') ? 'body: ""' : '';
  var options = {
    method: mt,
    bd
  };
  fetch(url, options)
  .then(response => response.text())
  .then(txt => {
    $('payload').innerHTML = txt;
  });
}

$('fetch').addEventListener('click', fetchEndpoint);
)EOF";


static const char base64_logo[] = R"EOF(
iVBORw0KGgoAAAANSUhEUgAAAGAAAABgCAMAAADVRocKAAAABGdBTUEAALGPC/xhBQAAAAFzUkdC
AK7OHOkAAAIKUExURQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAIhNjGoAAACtdFJOUwBl/fwyM2b+Bfn7mRAHBgEa+veYfx4C
PNsD6Lbr3T49T6zLhOYOzN7uWmx671yOjR0PU1EJTtTzaj8x9hP1wFDwfrg5gC1fIXeTYrIEVQry
+I+crg3XFoE1lFnR07HShyVdbW7f1cdo5aPxq8n0cO1xxJsRW2kM52QYwRkLYa0v4LNYb2BEUpd5
1siCFIaVpbAm7FbkiJKmF2ODxmfqmldJNn0gG0rCRtpNxUVMXLiLfgAABc5JREFUaN7tWflXE0kQ
bifAQAgJBAiBhAQMEATCfYsKoigIKIKioogHgnjggfe967Guq3vf933O/7gz1T0zfUyGwA4/7Hup
N/BSNT319VfTXd1TjVBa0pKWtPx/xHtkYTh0vGyrJG0tOx4aXvjR66T34jM7uxRFZq5LO88Ur91T
fZFoyzu13cV7Vy/1n2v7qTyxfVF9cv/dsryLM1W/UybbSFl9Ne9flruT+e9wKRxC9tthse8GA7jC
l7NZ/4ri7rD2v3u/9hiNUNKpKKsBKPJ8lOm/atm/28q/J4hZN+TrwW9yySmJ1Ki/ivwGbAl6LAAq
cb+2lRB9Tw10kO24VFAg8QzUq2aPznkbtlwQ/Y+AL9nVRvRIWGYA3Ocahwabb3q9N5sHh5q+LmAD
Fo6Qx9rIiBsRhvoSJreX6K1+Ogg5J461sO1b3vogh27hbyU39mK9n58kT3BnK4m6xa33Tv2rjeVb
vbT8r2opEu4txPwQW/axjStk8HWWvJwdZgQU/0lPsnHtOek3o1SwgxjP4ndUwWQaPIJcPU4klh48
+IJ0tjqNO3vUmdR1FHOqMy3tVWC6Ve0MQMstAKhqNyx1+M0/cCr7PsD+DAql5fBaZkudAiidBYfl
usMKPAxanVtAWvHQ0gdSCAjNB5wDCMyDyxCZMC5gkEUytCg+/blR2qry91k3UyULGLjxBL0CSv99
7P9SjiC9cKd5IuinrW8Q6mWahff9agDc74dOXwHlL6BTSAjkiOk4Q7sx1sVZ1Qcy+ZafG0toIehX
tZ+HcdqJGgBshlb/NIDf3fyCk6sCCOvQ+zpABCeow1peg3tVAVuA7LiSEoD8gz5Sq8CiZcCDQOYg
sg3Rn6LVKkSybCRR7Paa+usQQH2MbBm8llNjoHykA3wP6iGErrtgKbzDApzPouQ97Z1p1se0NWoA
xEA/D79zRomje6C6rqMeoBJHbIg2cTMnF6ybOWsmNcpQFl77jE1MHPQI6oZeVCKWgQigWQUAzEAH
AAYGwEMI2g00AUDTzjOYBr0RTUIvEs4zSACDSXQRGkWdB4gAwLvkXcxxIYplUDKuz/yXWH/kYUO0
AtYYF6IPQS9HeOvRh+znQRO7v/vtGc2AvpaMnNoHlmV0AG7krQKQ4KzSqyQAd83vCrAcQBIw8SH7
VDFVyxuzkGWq+NIA8OGNMcJ7Wd8qDLRXyN6onbJicHHUBMBUUwwRQq8kLhetWADM3ENCiJaBSd8q
IVLl2V2JsTYJIfrkp3ZqDPeBbRnFAXyOYxDLpGRc34o+wvpLYFBoMFgBa8bPPmaSzAGD8hQnGiM1
XLrOsGylT7RJYJJYJRcxsplbcKwBEnBvEk1AL3Y5z2AaGDSiGyml63UAVJJ03UNShuMhKicLDlky
v+CWzE2UqEtmlNYfc6PIEuCOBEumR1/0x+wnWq646L+2BxjTF32yv/jOfqLlitZ/7EN0zdi24I3X
YmCNDOLZtgwCi8bGi2wdS9YG4P4b2QKUmFtHdBXIvFhTiLrGhEWflRfm5pds3xvG7bbvb2jd/8dE
MzTupbf3jIw3AIPL+AMEb5xv232AlNK6kfN94pcHkdv0BwgKAUCnk59QnQAQMusImrQ5B/Ace6xg
P2NnHKMQmGE/Y9UPcWD01CmAp3j41lGlBGBU5VApoRq7W2wXiiHDG1UMQd4gALgizpRzAIAp5+gD
KW4WpMxi2ad2BSmzHVWQAumwLakdY0tqv1iW1GKWJbVKy5IaKl7CVUe9KPjcr1BVR6ui4AkmLZpF
QWwRioJoBBMzyprRMJvecFlzyuudKh4cajxXwN79NmqUNbFhRGR8YYMLs8gTxI8xpeVUateK1GSW
lrHFsrSsFcc1KaKL46mUrjtLzCcGwGJdHEeoQ83bctF/K+8P2JT34YCiSDygsAMQDygGlOQHFEmO
WBLbk5T5rY9YBurXcUh0Wjwk6lrXIZHdMdc3n+nHXLOh4YUjjh5zpSUtaUnLBsu/XvNl5HUuawwA
AAAASUVORK5CYII=
)EOF";