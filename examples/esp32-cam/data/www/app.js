const imgFolder = '/img/';
var fileList = document.getElementById('file-list');
var intervalID;

// Load list of files every x milliseconds (used in order to get fresh list)
var myInterval = function(val){
  if (val !== 0)
    intervalID = setInterval(listFiles, val*1000 + 1000);
  else
    clearInterval(intervalID);
};

// Fetch the list of files and fill the filelist
function listFiles() {
  fetch('/list?dir=/img')      // Do the request
  .then(response => response.json())    // Parse the response
  .then(obj => {                        // DO something with response
    fileList.innerHTML = '';
    obj.forEach(function(entry, i) {
      addEntry(entry.name);
    });
  });
}

// Load selected image inside the preview content
function loadFile(filename) {
  clearInterval(intervalID);
  var name = document.getElementById("image-name");
  var content = document.getElementById("image-content");

  name.innerHTML = 'Image path: ' + filename;
  content.src = filename;
  content.alt = 'SD: ' + filename;

  // Get the modal
  var modal = document.getElementById("modal-container");

  // Get the image and insert it inside the modal - use its "alt" text as a caption
  var img = document.getElementById("image-content");
  var modalImg = document.getElementById("modal-image");
  var captionText = document.getElementById("caption");
  img.onclick = function(){
    modal.style.display = "block";
    modalImg.src = this.src;
    captionText.innerHTML = this.alt;
  };

  // Get the <span> element that closes the modal
  var span = document.getElementsByClassName("close")[0];

  // When the user clicks on <span> (x), close the modal
  span.onclick = function() {
    modal.style.display = "none";
  };
}

// Delete selected file in SD
async function deleteFile(filename) {
  console.log(filename);
  const data = new URLSearchParams();
  data.append('path', filename);
  fetch('/edit', {
      method: 'DELETE',
      body: data
  });

  // Update the file browser.
  listFiles();
}

async function deleteAll() {
  let isExecuted = confirm("Are you sure to delete all files in /img/ folder?");
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
  console.log(entryName);
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
    loadFile(imgFolder +  entryName);
  });

  // Setup an event listener that will delete the file when the delete link is clicked.
  delLink.addEventListener('click', function(e) {
    e.preventDefault();
    deleteFile(imgFolder + entryName);
  });

}

// Add the buttons event listeners
var getNew = document.getElementById('get-new');
var getInterval = document.getElementById('set-interval');

getNew.addEventListener('click', function(e) {
  e.preventDefault();
  fetch('/getPicture')                  // Do the request
  .then(response => response.text())    // Parse the response
  .then(txt => {                        // DO something with response
    console.log(txt);
    addEntry(txt);
    loadFile(imgFolder + txt);
  });
});

getInterval.addEventListener('click', function(e) {
  e.preventDefault();
  let val = document.getElementById('interval').value;

  fetch('/setInterval?val='+ val)       // Do the request
  .then(response => response.text())    // Parse the response
  .then(txt => {                        // DO something with response
    console.log('Set interval ' + txt);
    myInterval(val);
  });
});

// Start the web page
clearInterval(intervalID);
listFiles();
loadFile('espressif.jpg');