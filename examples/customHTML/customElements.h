
/*
* This HTML code will be injected in /setup webpage using a <div></div> element as parent
* The parent element will hhve the HTML id properties equal to 'raw-html-<id>'
* where the id value will be equal to the id parameter passed to the function addHTML(html_code, id).
*/
static const char custom_html[] PROGMEM = R"EOF(
<form class=form>
  <div class=tf-wrapper>
    <label for=httpmethod class=input-label>Method</label>
    <select class='select' id='httpmethod'>
      <option>GET</option>
      <option>POST</option>
    </select>
    <label for=url class=input-label>Endpoint</label>
    <input type=text placeholder='https://httpbin.org/' id=url value='https://httpbin.org/' />
  </div>
  <br>
  <a id=fetch class='btn'>
    <span>Fecth url</span>
  </a>
  <br>
  <pre id=payload></pre>
</form>
)EOF";


/*
* In this example, a style sections is added in order to render properly the new
* <select> and <pre> elements introduced. Since this section will be added at the end of the body,
* it is also possible to override the style of the elements already present:
* for example the background color of body will be ovverrided with a different color
*/
static const char custom_css[] PROGMEM = R"EOF(
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
}
.select{
  width: 25%;
  height:40px;
  padding-top: 10px;
  padding-left: 20px;
  border:1px solid #ccc;
  border-radius: 6px;
  box-shadow: 0 1px 2px 0 rgba(220, 220, 230, 0.5);
}
.body{
  background-color: cadetblue;
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
static const char custom_script[] PROGMEM = R"EOF(
function fetchEndpoint() {
  var mt = $('httpmethod').options[$('httpmethod').selectedIndex].text;
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