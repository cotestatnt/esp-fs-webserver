// Expand or collapse the content according to current state
function expandCollapse() {
  // Get the HTML element immediately following the button (content)
  var content = this.nextElementSibling; 
  if (content.style.maxHeight) 
    content.style.maxHeight = null;
  else 
    content.style.maxHeight = content.scrollHeight + "px";
}

// Select all "+" HTML buttons in webpage and add a listener for each one
const btnList = document.querySelectorAll(".collapsible");
btnList.forEach(elem => {
  elem.addEventListener("click", expandCollapse ); // Set callback function linked to the button click
});

// This listener will execute an "arrow" function once the page was fully loaded
document.addEventListener("DOMContentLoaded", () => {
  console.log('Webpage is fully loaded');
  
  // At first, get the default values for form input elements from ESP
  fetch('/getDefault')
    .then(response => response.json())  // Parse the server response
    .then(jsonObj => {                     // Do something with parsed response
      console.log(jsonObj);
      document.getElementById('cars').value = jsonObj.car;
      document.getElementById('fname').value = jsonObj.firstname;
      document.getElementById('lname').value = jsonObj.lastname;
      document.getElementById('age').value = jsonObj.age;
    });
});

// This listener will prevent each form to reload page after submitting data
document.addEventListener("submit", (e) => {
  const form = e.target;        // Store reference to form to make later code easier to read
  fetch(form.action, {          // Send form data to server using the Fetch API
      method: form.method,
      body: new FormData(form),
    })
 
    .then(response => response.text())  // Parse the server response
    .then(text => {                     // Do something with parsed response
      console.log(text);
      const resEl = document.getElementById(form.dataset.result).innerHTML= text;
    });
  
  e.preventDefault();                 // Prevent the default form submit wich reload page
});
////////////////////////////////////////////////////////////////////////////////////////////