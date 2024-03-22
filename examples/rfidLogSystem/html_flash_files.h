static const char login_htm[] PROGMEM = R"string_literal(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Login</title>
    <style>
    #about {color:lightgray};
    body {background-color: #f5f5f5; font-family: Arial, sans-serif;}
    header, footer {padding: 10px; background-color: #024c5b; color: #fff; width: 50%; text-align: center;}
    header {margin-top: 40px;}
    label {display: block; margin: 5px;}
    input[type="text"], input[type="password"], input[type="email"] {width: 80%; padding: 8px; border: 1px solid #ddd; border-radius: 3px;}
    button[type="submit"] {margin-top: 20px; width: 50%; min-width: 60px; padding: 10px; background-color: #607D8B; color: white; border: none; cursor: pointer;}
    button[type="submit"]:hover, .nav {background-color: #16729F;}
    .container {display: flex; flex-direction: column; align-items: center; min-height: 100vh;}
    .custom-container {padding: 10px; width: 50%; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); background-color: #ffffff;}  
    .content {padding: 20px; display: flex; flex-direction: column;}
    .center {text-align: center};
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>ESP32 RFID Logs - Login</h1> 
    </header>
    <div class="custom-container">
      <div class="content">
        <form id="loginForm" class="center">
          <div>
            <label for="username">Username:</label>
            <input type="text" id="username" name="username" required>
          </div>
          <div>
            <label for="password">Password:</label>
            <input type="password" id="password" name="password" required>
          </div>
          <button type="submit">Login</button>
        </form>
      </div>
    </div>
    <footer class="footer">
      <div class="has-text-centered">
        <p> RFID Log &copy; 2024. All rights reserved.</p>
        <a id=about target=_blank rel=noopener href="https://github.com/cotestatnt/esp-fs-webserver/">Created with https://github.com/cotestatnt/esp-fs-webserver/</a>
      </div>
    </footer>
  </div>

  <script>
    // To avoid sharing plain text between client and server, 
    // send SHA256 of password input text (based on https://geraintluff.github.io/sha256/)
    function sha256(ascii) {
      var rightRotate = (value, amount) => (value >>> amount) | (value << (32 - amount));
      var mathPow = Math.pow, maxWord = mathPow(2, 32), lengthProperty = 'length', result = '', words = [], asciiBitLength = ascii[lengthProperty] * 8;
      var hash = sha256.h = sha256.h || [], k = sha256.k = sha256.k || [], primeCounter = k[lengthProperty];
      var isComposite = {};
      for (var candidate = 2; primeCounter < 64; candidate++) {
        if (!isComposite[candidate]) {
          for (i = 0; i < 313; i += candidate) isComposite[i] = candidate;
          hash[primeCounter] = (mathPow(candidate, .5) * maxWord) | 0;
          k[primeCounter++] = (mathPow(candidate, 1 / 3) * maxWord) | 0;
        }
      }
      ascii += '\x80';
      while (ascii[lengthProperty] % 64 - 56) ascii += '\x00';
      for (i = 0; i < ascii[lengthProperty]; i++) {
        j = ascii.charCodeAt(i);
        if (j >> 8) return;
        words[i >> 2] |= j << ((3 - i) % 4) * 8;
      }
      words[words[lengthProperty]] = ((asciiBitLength / maxWord) | 0);
      words[words[lengthProperty]] = (asciiBitLength);
      for (j = 0; j < words[lengthProperty];) {
        var w = words.slice(j, j += 16);
        var oldHash = hash.slice(0, 8);
        for (i = 0; i < 64; i++) {
          var i2 = i + j, w15 = w[i - 15], w2 = w[i - 2];
          var a = hash[0], e = hash[4];
          var temp1 = hash[7] + (rightRotate(e, 6) ^ rightRotate(e, 11) ^ rightRotate(e, 25)) + ((e & hash[5]) ^ ((~e) & hash[6])) + k[i] +
              (w[i] = (i < 16) ? w[i] : (w[i - 16] + (rightRotate(w15, 7) ^ rightRotate(w15, 18) ^ (w15 >>> 3)) + w[i - 7] +
                  (rightRotate(w2, 17) ^ rightRotate(w2, 19) ^ (w2 >>> 10))) | 0);

          var temp2 = (rightRotate(a, 2) ^ rightRotate(a, 13) ^ rightRotate(a, 22)) + ((a & hash[1]) ^ (a & hash[2]) ^ (hash[1] & hash[2]));
          hash = [(temp1 + temp2) | 0].concat(hash);
          hash[4] = (hash[4] + temp1) | 0;
        }
        for (i = 0; i < 8; i++) hash[i] = (hash[i] + oldHash[i]) | 0;
      }
      for (i = 0; i < 8; i++)
        for (j = 3; j + 1; j--)
          result += ((hash[i] >> (j * 8)) & 255).toString(16).padStart(2, '0');
      return result;
   };

    document.getElementById('loginForm').addEventListener('submit', function(event) {
      event.preventDefault();
      const hash = sha256(document.getElementById('password').value);
      const username = document.getElementById('username').value;
      var formData = new FormData();
      formData.append("username", document.getElementById('username').value);
      formData.append("hash", hash);
      
      fetch('/rfid', {
        method: 'POST',
        body: formData
      })
      .then(response => {
        if (response.ok) {
            var url = '/rfid?username=' + username + '&hash=' + hash;
            window.location.href = url;
        } else {
            alert("Wrong password");
        };
      });
    });
  </script>
</body>
</html>
)string_literal";





static const char index_htm[] PROGMEM = R"string_literal(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>RFID Log</title>
  <style>
    #about {color:lightgray};
    details,main,summary{display: block;}
    body {background-color: #f5f5f5; margin: 0; font-family: Arial, sans-serif; margin-top: 20px;}
    header, footer {padding: 10px; background-color: #024c5b; color: #fff; width: 90%; text-align: center;}
    label {display: block; margin-bottom: 5px;}
    input[type="text"], input[type="password"], input[type="email"] {width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 3px;}
    button.submit {margin: 0 20px -10px 20px; width: 80%; min-width: 60px; padding: 10px; background-color: #607D8B; color: white; border: none; cursor: pointer;}
    button.submit:hover {background-color: #16729F;}
    button[disabled]:hover, button[disabled] {background-color: #ccc; color: #666;}
  
    .container {display: flex; flex-direction: column; align-items: center; min-height: 100vh;}
    .custom-container {padding: 10px; width: 90%; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); background-color: #ffffff;}
    .sidebar {background-color: #f8f9fa; flex: 0 0 10%; transition: all 0.3s ease;}
    .tab {padding: 10px 20px; cursor: pointer;}
    .tab.active {background-color: #e9ecef;}
    .content {flex-grow: 1; display: flex; justify-content: center; align-items: flex-start;}
    .main-content {width: 100%; padding: 0 0 20px 20px; min-height: 70vh; }
    .table-container {display: flex; flex-direction: column; border: 1px solid #ddd; border-radius: 5px; overflow: hidden;}
    .table-header {display: flex; background-color: #f2f2f2; padding: 10px 15px; font-weight: bold; justify-content: space-around;}
    .table-body {display: flex; flex-direction: column;}
    .table-body > div {display: flex; padding: 4px 10px; border-bottom: 1px solid #ddd; justify-content: space-around;}
    .table-body > div:nth-child(even) {background-color: #f9f9f9;}
    .table-body > div.selected {background-color: #d3d3d3; border: 1px solid orange; color: blue;}
    .section {position: relative;}
    .collapse-container {margin-bottom: 20px;}
    .collapse-content {display: none; padding: 10px; border: 1px solid #ddd; border-radius: 5px; margin-bottom: 20px;}
    .collapse-content.show {display: block;}
    .form-row {display: flex; flex-wrap: wrap; margin-bottom: 10px; align-items: flex-end;}
    .form-column {flex: 1; margin: 0 20px 0 20px;}
    .form-column:last-child { margin-right: 40px; }
    .but-row {display: flex; margin: 20px 0 10px 0;}
    .button:hover {cursor: pointer; background-color: #16729F;}
    .button:active {transform: scale(0.8);}
    
    .floating {position: absolute; float: right; z-index: 1; right: -4px;}
    .floating.top {top: -8px; }
    .floating.bottom {bottom: -8px;}
    .floating.inline {position: sticky;}
    .floating .button {width: 24px; height: 24px; border-radius: 50%; box-shadow: 0 2px 4px rgba(0, 0, 0, 0.8);
                      border: 1px solid transparent; font-family: monospace; font-size: larger; color: cornsilk;}

    .green {background-color: #008000ba;}
    .blue {background-color: #607D8B;}
    .w05 {width: 5%;} .w10 {width: 10%;} .w20 {width: 20%;} .w30 {width: 30%;}
    
    .d-modal{
      width: -webkit-fill-available; border-radius: 10px; border-style: solid; border-width: 1px; border-color: #3333336e;
      box-shadow: rgba(0, 0, 0, 0.24) 0 3px 8px;left: 50%; position: absolute; top: 60%; transform: translate(-50%, -50%);
      flex-direction: column; background-color: hsl(199.53deg 18.3% 46.08% / 90%); z-index: 999;
    }
    .d-modal-title{color:#111827;padding:1.5em;position:relative;width:calc(100% - 4.5em);}
    .d-modal-content{border-top:1px solid #e0e0e0;padding:1.5em;overflow:auto;}
    .btn{display:inline-flex ;padding:10px 15px; background-color:#2E8BC0;color:#fff;border:0;cursor:pointer;min-width:40%;border-radius:5px;justify-content:center;}
    .btn:hover{ filter: brightness(85%);}
    .btn-bar{display:flex;padding:20px;justify-content:center;flex-wrap:wrap;grid-column-gap:30px;grid-row-gap:20px; order:998;}
    
    /*Must be the last rule*/
    .hide {display: none; }
    .loader { width: 120px; height: 120px; border: 24px solid #dedbdba8; border-top: 24px solid #3498dbe3;  border-radius: 50%;
       animation: spin 2s linear infinite;  position: fixed; top: 50%; left: 50%; margin: -60px; display: none;}
    @keyframes spin { 0% { transform: rotate(0deg); }  100% { transform: rotate(360deg); } }

  </style>
</head>
<body>
  <div class="loader" id="loader"></div>
  
  <details id=modal-message>
    <summary style="display: block"></summary>
    <div class=d-modal>
      <div class=d-modal-title><h2 id=message-title>t</h2></div>
      <div class=d-modal-content><p id=message-body></p></div>
      <div class=btn-bar>
          <a id=ok-modal class="btn hide" onclick=closeModal(true)><span>OK</span></a>
          <a id=close-modal class="btn" onclick=closeModal(false)><span>Close</span></a>
      </div>
    </div>
  </details>
          
  <div id="main-box" class="container">
    <header>
      <h1 class="title is-3">ESP32 RFID Logs</h1> 
    </header>
    <div class="custom-container">
      <div class="content">
        <div class="sidebar">
          <div class="tab" id="logsTab" data-target="logsContent">Logs</div>
          <div class="tab" id="usersTab" data-target="usersContent">Users</div>
          <div class="tab"><a id="setup" style="color: inherit; text-decoration: none;" href="#" disabled>Setup</a></div>
        </div>
        <div class="main-content">
          <div id="logsContent" class="section">
            
            <div class="floating top">
              <button class="button green" onclick="toggleCollapse('collapse-logs')"><b id='toggle-logs'>+</b></button>
            </div>
            
            <div class="floating bottom">
              <button class="button blue" id="prev-log"><b>&lt;</b></button>
              <button class="button blue" id="next-log"><b>&gt;</b></button>
            </div>
            
            <div class="collapsible">
              <div id="collapse-logs" class="collapse-content">
               <div id="insertForm">
                  <div class="form-row">
                    <div class="form-column">
                      <input type="datetime-local" id="start">
                      <input type="datetime-local" id="end">
                    </div>
                    <div class="form-column">
                      <label for="username-log">Username:</label>
                      <input type="text" id="username-log" name="username-log" placeholder="Username">
                    </div>
                    <div class="form-column">
                      <label for="reader-log">Reader:</label>
                      <input type="text" id="reader-log" name="reader-log" placeholder="Reader number">
                    </div>
                    
                    <div class="but-row">
                      <button class="submit" id="filter-log">Filter</button>
                      <button class="submit" id="export-log">Export</button>
                    </div>
                  </div>
                </div>
              </div>
            </div>
    
            <div class="table-container">
              <div class="table-header">
                <div class="w05">ID</div>
                <div class="w30">Timestamp</div>
                <div class="w30">Username</div>
                <div class="w10">Tag Code</div>
                <div class="w10">Reader</div>
              </div>
              <div id="logsTable" class="table-body">
                <!-- //// -->
              </div>
            </div>
          </div>
        
          <div id="usersContent" class="section hide">
            <div class="floating top">
              <button class="button green" id="handle-users" onclick="toggleCollapse('collapse-user')" disabled><b id='toggle-user'>+</b></button>
            </div>
            <div class="collapsible">
              <div id="collapse-user" class="collapse-content">
                <!-- Form per l'inserimento di un record nella tabella -->
                <div id="insertForm">
                  <div class="form-row">
                    <div class="form-column">
                      <label for="tagCode">Tag Code:</label>
                      <span style="display: inline-flex;">
                        <input type="text" id="tagCode" name="tagCode" placeholder="Tag Code">
                        
                        <div class="floating inline">
                          <button id="get-tag" class="button blue" title="Read RFID tag code"><b>@</b></button>
                        
                        </div>
                      </span>
                    </div>
                    <div class="form-column">
                      <label for="name">Name:</label>
                      <input type="text" id="name" name="name" placeholder="Name">
                    </div>
                    <div class="form-column">
                      <label for="level">Level:</label>
                      <input type="text" id="level" name="level" placeholder="Level">
                    </div>
                   
                  </div>
                  <div class="form-row">
                    <div class="form-column">
                      <label for="username">Username:</label>
                      <input type="text" id="username" name="username" placeholder="Username">
                    </div>
                    <div class="form-column">
                      <label for="password">Password:</label>
                      <input type="password" id="password" name="password" placeholder="password">
                    </div>
                    <div class="form-column">
                      <label for="email">Email:</label>
                      <input type="email" id="email" name="email" placeholder="Email">
                    </div>
                  </div>
                  
                  <div class="but-row">
                    <button class="submit" id="add-user">Insert</button>
                    <button class="submit" id="delete-user" disabled>Delete</button>
                  </div>
                </div>
              </div>
            </div>
            
            <div class="table-container">
              <div class="table-header">
                <div class="w05">ID</div>
                <div class="w20">Username</div>
                <div class="w05">Role</div>
                <div class="w30">Name</div>
                <div class="w30">Email</div>
                <div class="w10">Tag Code</div>
              </div>
              <div id="usersTable" class="table-body">
                 <!-- //// -->
              </div>
            </div>
          </div>
          
        </div>
      </div>
    </div>
    <footer class="footer">
      <div class="has-text-centered">
        <p> RFID Log &copy; 2024. All rights reserved.</p>
        <a id=about target=_blank rel=noopener href="https://github.com/cotestatnt/esp-fs-webserver/">Created with https://github.com/cotestatnt/esp-fs-webserver/</a>
      </div>
    </footer>
  </div>

  <script>
    var userLevel = 0;
   
    // Callback function called on modal box close
    var closeCb = function(){};
  
    // ID Element selector shorthands
    var $ = function(el) {
    	return document.getElementById(el);
    };
    
    // Switch active page
    function tabClick(el) {
      const tabs = document.querySelectorAll('.tab');
      const sections = document.querySelectorAll('.section');
      tabs.forEach(t => t.classList.remove('active'));
      sections.forEach(s => s.classList.add('hide'));
      const target = this.dataset.target;
      $(target).classList.remove('hide');
      this.classList.remove('hide');
      this.classList.add('active');
    }
    
    // Toggle the collapsible user section 
    function toggleCollapse(id, keep) {
      if (keep)
        $(id).classList.add('show');
      else
        $(id).classList.toggle('show');
        
      const allRows = document.querySelectorAll('.table-body > div');
      allRows.forEach(row => row.classList.remove('selected'));
      const allInput = $('insertForm').querySelectorAll('input');
      allInput.forEach(inp => inp.value = '');
      $('add-user').innerHTML = 'Insert';
      $('delete-user').disabled = true;
      $('add-user').disabled = true;
      const btnId = id.split('-')[1];
      $('toggle-' + btnId).innerHTML = $(id).classList.contains('show') ? '-' : '+';
    }
      
    // Get logs records
    function getLogs(filter) {
      var formData = new FormData();
      if (filter != undefined) 
        formData.append("filter", filter);
      else
        formData.append("filter", "");
      const option = {
        method: 'POST',
        body: formData
      };
      
      const logs = $('logsTable');
      fetch('/logs', option)
      .then(response => {
        if (!response.ok) {
          throw new Error('Requesr error');
        }
        return response.json();
      })
      .then(data => {
        logs.innerHTML = '';
        data.forEach(log => {
          const logEntry = document.createElement('div');
          logEntry.innerHTML = 
           `<div class="w05">${log.id}</div>
            <div class="w30">${new Date(log.epochTime * 1000).toLocaleString()}</div>
            <div class="w30">${log.username}</div>
            <div class="w10">${log.tagCode}</div>
            <div class="w10">${log.readerID}</div>`;
          logs.appendChild(logEntry);
        });
        $('loader').style.display = "none";
      })
      .catch(error => {
        console.error('Error:', error);
        logs.innerHTML = '';
        $('loader').style.display = "none";
      });
    }
    
    // Read the RFID code for current user
    function getTagCode() {
      fetch('/getCode')
      .then(response => {
        if (!response.ok) {
          throw new Error('Request error');
        }
        return response.json();
      })
      .then(data => {
        $('tagCode').value = data.tagCode;
        $('add-user').disabled = false;
      })
      .catch(error => {
        alert('Error:' + error);
      });
    }
    
    // Get list of registered users
    function getUsers() {
      const usersTable = $('usersTable');
      fetch('/users')
      .then(response => {
        if (!response.ok) {
          throw new Error('Requesr error');
        }
        return response.json();
      })
      .then(data => {
        data.forEach(user => {
          const userEntry = document.createElement('div');
          userEntry.innerHTML = 
             `<div class="w05" id="user">${user.id}</div>
              <div class="w20" id="username">${user.username}</div>
              <div class="w05" id="level">${user.level}</div>
              <div class="w30" id="name">${user.name}</div>
              <div class="w30" id="email">${user.email}</div>
              <div class="w10" id="tagCode">${user.tagCode}</div>`;
          usersTable.appendChild(userEntry);
          
          userEntry.addEventListener('click', function(ev) {
            const allRows = document.querySelectorAll('.table-body > div');
            allRows.forEach(row => row.classList.remove('selected'));
            
            if(userLevel >= 5) {
              toggleCollapse('collapse-user', true);
              this.classList.add('selected');
              const cols = Array.from(ev.target.parentNode.querySelectorAll('div')).map(el => {
                return {
                  id: el.id,
                  value: el.innerHTML
                };
              });
              if (cols.length === 6) {
                cols.forEach(item => {
                  $(item.id).value = item.value; 
                  $(item.id).addEventListener('input', function() {
                    $('add-user').disabled = false;
                  });
                });
              }
              $('delete-user').disabled = false;
              $('add-user').innerHTML = 'Update';
            }
          });
        });
      })
      .catch(error => {
        console.error('Error:', error);
      });
    }
    
    // Send command to MCU before the readings in order to handle properly logs record
    function readTagCode() {
      fetch('/waitCode')
      .then(response => {
        openModal('Read new TAG', "<br>Please hold your tag close to the RFID reader", getTagCode);
      })
    }
    
    // Insert or update a new user record
    function sendUserForm(url) {
      var formData = new FormData();
      formData.append("username", $('username').value);
      formData.append("password", $('password').value);
      formData.append("name", $('name').value);
      formData.append("email", $('email').value);    
      formData.append("tagCode", $('tagCode').value);
      formData.append("level", $('level').value);
      const option = {
        method: 'POST',
        body: formData
      };
      fetch(url, option)
        .then(response => {
          if (!response.ok) {
            throw new Error('Request error');
          }
          return response.text();
        })
        .then(result => {
          openModal('Users', "<br>New record inserted or updated");
        })
        .catch(error => {
          openModal('Error', error);
        });
    }
    
        // Send command to MCU before the readings in order to handle properly logs record
    function deleteUser() {
      $('delete-user').disabled = true;
      sendUserForm('/delUser');
    }
    
    // Show a message, if fn != undefinded run as callback on OK button press
    function openModal(title, msg, fn) {
      $('message-title').innerHTML = title;
      $('message-body').innerHTML = msg;
      $('modal-message').open = true;
      $('main-box').style.filter = "blur(3px)";
      if (typeof fn != 'undefined') {
        closeCb = fn;
        $('ok-modal').classList.remove('hide');
      }
      else
        $('ok-modal').classList.add('hide');
    }
    
    // Clode modal box
    function closeModal(do_cb) {
      $('modal-message').open = false;
      $('main-box').style.filter = "";
      if (typeof closeCb != 'undefined' && do_cb)
        closeCb();
    }
    
    function getRowTimestamp(id) {
      // get last row
      var divs = Array.from($(id).querySelectorAll('div:not([class^="w"])'));
      //Get timestamp value
      const [day, month, year, hour, minute, second] = Array.from(divs[divs.length - 1].querySelectorAll('div'))[1].innerHTML.split(/[\s,:\/]+/);
      return (new Date(year, month - 1, day, hour, minute, second)).getTime()/1000;
    }
    
    function customFilter() {
      let filter = ' WHERE ';
      function includeAND(rule) {
        return (filter != ' WHERE ') ? (' AND ' + rule) : rule;
      }
      if ( $('start').value)
        filter += includeAND('epoch >= ' + (new Date($('start').value)) / 1000);
      if ( $('end').value)
        filter += includeAND('epoch <= ' + (new Date($('end').value)) / 1000);
      if ( $('reader-log').value)
        filter += includeAND('reader = ' + $('reader-log').value);
      if ( $('username-log').value)
        filter += includeAND("username = '" + $('username-log').value + "'");
      getLogs(filter);
    }
    
    function exportCSV() {
      // Create CSV data
      const divs = Array.from($('logsTable').querySelectorAll('div:not([class^="w"])'));
      let table = [];
      
      divs.forEach(item => {
        const cols = Array.from(item.querySelectorAll('div'));
        let row = [];
        cols.forEach(field => {
          row.push(field.innerHTML)
        });
        table.push(row);
      })
  
      let csvString = 'id, timestamp, username, tagCode, reader\n'
      for (let i = 1; i < table.length; i++) {
        csvString += table[i].join(', ') + '\n';
      }
      
      // Download as CSV file
      const blob = new Blob([csvString], { type: 'text/csv' });
      const url = window.URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = 'data.csv';
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      window.URL.revokeObjectURL(url);
    }
    
    function getUserLevel() {
      fetch('/userLevel?username=cotestatnt', {
        method: 'GET'
      })
      .then(response => {
        console.log(response);
      })
    }
    
    // Event listeners
    $('add-user').addEventListener('click', function(event) {
      event.preventDefault(); 
      $('add-user').disabled = true;
      $('delete-user').disabled = true;
      sendUserForm('/addUser');
    });
    
    $('delete-user').addEventListener('click', function(event) {
      event.preventDefault(); 
      openModal('Delete user', "Do you really want to drop current user record?", deleteUser);
    });
      
    document.addEventListener('DOMContentLoaded', function() {   
      $('loader').style.display = "block";
      getUsers();
      getLogs();
      
      $('logsTab').addEventListener('click', tabClick);
      $('usersTab').addEventListener('click', tabClick);
      $('get-tag').addEventListener('click', readTagCode);
      $('filter-log').addEventListener('click', customFilter);
      $('export-log').addEventListener('click', exportCSV);
        
      $('next-log').addEventListener('click', function(){
        $('loader').style.display = "block";
        var ts = getRowTimestamp('logsTable');
        var filter = ` WHERE epoch <= ${ts}`;
        getLogs(filter);
      });
      
      $('prev-log').addEventListener('click', function(){
        $('loader').style.display = "block";
        var ts = getRowTimestamp('logsTable');
        var filter = ` WHERE epoch >= ${ts}`;
        getLogs(filter);
      });
      
      // Check if user has admin level (>= 5)
      var usernameValue = document.cookie.replace(/(?:(?:^|.*;\s*)username\s*=\s*([^;]*).*$)|^.*$/, "$1");
      userLevel = usernameValue.split(',')[1];
      if(userLevel >= 5) {
        console.log(usernameValue.split(',')[0], "is admin");
        $('handle-users').disabled = false;
        $('setup').href = '/setup';
      }

    });

  </script>
</body>
</html>
)string_literal";
