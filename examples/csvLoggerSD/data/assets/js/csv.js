
// Default file to be loaded with no parameter in url
var filename = '';

// JQuery-like selector
var $ = function(el) {
	return document.getElementById(el);
};


/**
 * @returns {getUserInput.userInput} an object
 * containing all the user input at the time 
 * of the method call.
 */
function getUserInput() {
  var userInput = {};
  userInput.fileName = filename;
  userInput.maxRows = "0";
  userInput.encoding = 'UTF-8';
  userInput.columnSeparator = ',';
  userInput.useQuotes = true;
  userInput.firstRowHeaders = true;
  userInput.firstRowInlcude = false;
  return userInput;
}

/* Tables */

var tableCount = 1;

/**
 * Creates a table holder with the 
 * given table in it.
 * 
 * @param {type} title
 * @param {type} tableHtml
 * @returns {String|getTableUnit.tableHolder}
 */

function getTableUnit(title, tableHtml){
  var id = "table-" + tableCount;
  var tableHolder = "<div class='table-holder' id='" + id + "'><b contenteditable>{@name}</b>{@table}</div>";
  tableHolder = tableHolder.replace("{@name}", title);
  tableHolder = tableHolder.replace("{@table}", tableHtml);
  tableCount++;
  return tableHolder;
}

/**
 * Clears all tables from the page.
 */
function clearTables(){
  tableCount = 1;
  $('csv-table').innerHTML = '';
}

/**
 * Adds (appends) a table holder to the page.
 * 
 * @param {type} unit
 * @returns {undefined}
 */
function addTableUnit(unit){
  $('csv-table').innerHTML = unit ;
}


function saveTable(filename, text) {
  var myblob = new Blob([text]);
  var formData = new FormData();
  formData.append("data", myblob, filename);

  // POST data using the Fetch API
  fetch('/edit', {
    method: 'POST',
    headers: {
      'Access-Control-Allow-Origin': '*',
      'Access-Control-Max-Age': '600',
      'Access-Control-Allow-Methods': 'PUT,POST,GET,OPTIONS',
      'Access-Control-Allow-Headers': '*',
      'filename': filename
   },
    body: formData
  })
  // Handle the server response
  .then(response => response.text())
  .then(text => {
    console.log(text);
  });
}


/* Table downloading */

/**
 * Download the specified table to 
 * the users computer.
 * 
 * @param {type} table table number
 * @param {save} save file to host memory
 * @returns {undefined}
 */

function downloadTable(table, save = false) {
  var tableId = "table-" + table;
   
	var csvArray = [];
	var rows = document.querySelectorAll("#" + tableId + " > table tr");
			
	for (var i = 0; i < rows.length; i++) {
		var row = [], cols = rows[i].querySelectorAll("td, th");
		for (var j = 0; j < cols.length; j++) {
		  
		  var value = cols[j].innerText;
		  
		  if(customParseFloat(value) ) {
        row.push(value);
		  }
		  else {
		     row.push('"' + value +'"');
		  }
    }
		csvArray.push(row.join(",")); 		
		csvArray.push("\n");
	}

  var csvString = csvArray.join("");
  if (save === false)
    download(filename, csvString);
  else {
    saveTable(filename, csvString);
  }
}


/**
 * Creates and downloads a file to the users computer.
 * 
 * @param {type} filename
 * @param {type} text
 * @returns {undefined}
 */
function download(filename, text) {
  var element = document.createElement('a');
  element.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(text));
  element.setAttribute('download', filename);

  element.style.display = 'none';
  document.body.appendChild(element);
  element.click();
  document.body.removeChild(element);
}

/**
 * Gets the users input and creates
 * a set of tables from it.
 */
function populate(csv){
  var ui = getUserInput();
  addTableUnit(
    getTableUnit(
      ui.fileName,
      getTable(
        csvTo2DArray(
          csv,
          ui.columnSeparator,
          ui.useQuotes,
          ui.maxRows
        ),
        ui.firstRowHeaders, 
        ui.firstRowInlcude
      )
    )
  );

  // Prevents to ad a new line inside cell
  var cells =  document.querySelectorAll('td');
  
  cells.forEach(item => {
    item.addEventListener('keypress', event => {
      if ( event.keyCode === 13 ){
        if (window.event) {
          window.event.returnValue = false;
        }
      }
    });
  });
}

/**
 * Load the csv from webserver location and fit table with data
*/
function loadCsv(path) {
  clearTables();
  filename = path;
  
  fetch(path)
  .then(response => response.text()) 
  .then(textString => {
      populate(textString);
  });
}


/**
 * Standard parseFloat don't handle properly "0", "0.0", "0.00" etc strings
 * @param {strNumber} the string representing a number to be parsed
 * @returns {float number} or NaN
*/
function customParseFloat(strNumber){
  if (isNaN(parseFloat(strNumber)) === false){
    let toFixedLength = 0;

    let arr = strNumber.split('.');
    if (arr.length === 2 ){
      toFixedLength = arr[1].length;
    }
    return parseFloat(strNumber).toFixed(toFixedLength);
  }
  return NaN; // Not a number
}

/**
 * Creates an unstyled, bare-bones html table
 * from the provided 2D(Multidimentional Array).
 * 
 * @param {type} tableArray the 2D array.
 * @param {type} useHeaders will make the first row
 * in the table bold if true.
 * @param {type} dupeHeaders will duplicate the first row
 * in the table if true and useHeaders is true.
 * @param {type} tableId HTML ID for the table.
 * @returns {String} the constructed html table as text.
 */
function getTable(tableArray, useHeaders, dupeHeaders, tableId){
    var tableOpen = "<table contenteditable id=\"" + tableId + "\">";
    var tableClose = "</table>";
    
    var headerCell = "<th>{@val}</th>";
    var cell = "<td>{@val}</td>";
    
    var rowOpen = "<tr>";
    var rowClose = "</tr>";
    
    var table = tableOpen;
    
    for(i = 0; i < tableArray.length; i++){
        //Row
        if(i === 1 && useHeaders && dupeHeaders){
            i = 0;
            useHeaders = false;
            dupeHeaders = false;
        }
        
        table += rowOpen;
        for(j = 0; j < tableArray[i].length; j++){
            //Cell
            if(i === 0 && useHeaders){
                table += headerCell.replace("{@val}", tableArray[i][j]);
            } else {
                table += cell.replace("{@val}", tableArray[i][j]);
            }
        }
        
        table += rowClose;
    }
    
    return table + tableClose;
}

/**
 * Creates a 2D (Multidimentional) array from
 * CSV data in string form.
 * 
 * @param {type} csv the CSV data.
 * @param {type} separator the character used
 * to separate the columns/cells.
 * @param {type} quotes ignores the separator
 * in quoted text.
 * @param {type} maxRows the maximum rows
 * to scan.
 * @returns {Array|csvTo2DArray.table} the CSV data
 * as a 2D (Multidimentional) array.
 */
function csvTo2DArray(csv, separator, quotes, maxRows){
    var table = [];
    var rows = 0;
    
    csv.split("\n").map(function(row){
        if(maxRows !== "0")
            if(rows >= maxRows)
                return;
        
        var tableRow = getRow(row, separator,  quotes);
        
        if(tableRow === null)
            return table;
        
        table.push(tableRow);
        rows++;
    });
    
    return table;
}

/**
 * Creates an array from a CSV row (line)
 * 
 * @param {type} row the CSV row.
 * @param {type} separator character used to separate
 * cells/columns
 * @param {type} quotes ignores the separator
 * in quoted text.
 * @returns {Array|getRow.trow} the CSV row as an array.
 */
function getRow(row, separator, quotes){
    if(row.length === 0)
        return null;
        
    isQuoted = false;
    var trow = [];
    var cell = "";
        
    for(var i = 0; i < row.length; i++){
        var char = row.charAt(i);
            
        if(quotes){
            if(char === '\"' || char === '\''){
                isQuoted = !isQuoted;
                continue;
            }
        }
            
        if(char === separator && !isQuoted){
            trow.push(cell);
            cell = "";
            continue;
        }
            
        cell += char;
    }
    
    trow.push(cell);
    return trow;
}