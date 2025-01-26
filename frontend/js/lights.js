export function initLightFeature(BACKEND_URL) {
  const lightsDisplay = document.getElementById('lights-display');

  document.querySelectorAll('.light-button').forEach((button) => {
    button.addEventListener('click', () => {
      const choice = button.dataset.choice;
      console.log(choice);
      if (lightsDisplay.textContent) {
        lightsDisplay.textContent += `-${choice}`;
      } else {
        lightsDisplay.textContent = choice;
      }
    });
  });
}
