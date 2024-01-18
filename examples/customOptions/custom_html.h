
static const char save_btn_htm[] PROGMEM = R"EOF(
<div class="btn-bar">
  <a class="btn" id="reload-btn">Reload options</a>
</div>
)EOF";

static const char button_script[] PROGMEM = R"EOF(
/* Add click listener to button */
document.getElementById('reload-btn').addEventListener('click', reload);
function reload() {
  console.log('Reload configuration options');
  fetch('/reload')
  .then((response) => {
    if (response.ok) {
      openModalMessage('Options loaded', 'Options was reloaded from configuration file');
      return;
    }
    throw new Error('Something goes wrong with fetch');
  })
  .catch((error) => {
    openModalMessage('Error', 'Something goes wrong with your request');
  });
}
)EOF";
