var dataFolder = document.getElementById("csv-path").value;
var fileList = document.getElementById('file-list');
var currentFile = "";

// Fetch the list of files and fill the filelist
function listFiles() {
  var url = '/list?dir=' + dataFolder;
  if (url.charAt(url.length - 1) === '/')
    url = url.slice(0, -1);             // Remove the last character
  fetch(url)                            // Do the request
  .then(response => response.json())    // Parse the response
  .then(obj => {                        // DO something with response
    fileList.innerHTML = '';
    obj.forEach(function(entry, i) {
      addEntry(entry.name);
    });
    // Load last file
    loadCsv(dataFolder + obj[obj.length -1].name);    
  });
}

// Load selected image inside the preview content
function loadFile(filename) {
  loadCsv(filename);
}

// Delete selected file in SD
async function deleteFile(filename) {
  var isExecuted = confirm("Are you sure to delete "+ filename + "?");
  if(isExecuted){
    const data = new URLSearchParams();
    data.append('path', filename);
    fetch('/edit', {
        method: 'DELETE',
        body: data
    });
    // Update the file browser.
    listFiles();
  }
}

async function deleteAll() {
  var isExecuted = confirm("Are you sure to delete all files in "+ dataFolder + " folder?");
  if(isExecuted){
    var ul = document.getElementById("file-list");
    var items = ul.getElementsByClassName("edit-file");
    for (var i=0; i<items.length; i++) {
      console.log("Delete " + items[i].innerHTML);
      await deleteFile(imgFolder + items[i].innerHTML);
    }
  }
}

// Add a single entry to the filelist
function addEntry(entryName) {
  var li = document.createElement('li');
  var link = document.createElement('a');
  link.innerHTML = entryName;
  link.className = 'edit-file';
  li.appendChild(link);

  var delLink = document.createElement('a');
  delLink.innerHTML = '<span class="delete">&times;</span>';
  delLink.className = 'delete-file';
  li.appendChild(delLink);
  fileList.insertBefore(li, fileList.firstChild);

  // Setup an event listener that will load the file when the link is clicked.
  link.addEventListener('click', function(e) {
    e.preventDefault();
    loadFile(dataFolder +  entryName);
    currentFile = dataFolder +  entryName;
  });

  // Setup an event listener that will delete the file when the delete link is clicked.
  delLink.addEventListener('click', function(e) {
    e.preventDefault();
    deleteFile(dataFolder + entryName);
  });

}

// Add the event listeners
document.getElementById('download-csv').addEventListener('click', function(e) {
  downloadTable(1);
});
document.getElementById('save-csv').addEventListener('click', function(e) {
  downloadTable(1, true);
});

document.getElementById('load-list').addEventListener('click', function(e) {
  listFiles();
});


// Start the web page
listFiles();