export function initLightFeature(BACKEND_URL) {
  const lightsDisplay = document.getElementById('lights-display');
  const lightsButton = document.getElementById('send-lights');

  document.querySelectorAll('.light-button').forEach((button) => {
    button.addEventListener('click', () => {
      const choice = button.dataset.choice;
      if (
        lightsDisplay.textContent === 'error sending lights' ||
        lightsDisplay.textContent === 'lights activated' ||
        lightsDisplay.textContent.trim() === ''
      ) {
        lightsDisplay.textContent = choice;
      } else {
        lightsDisplay.textContent += `-${choice}`;
      }
    });
  });

  async function sendLights() {
    let message = '';
    try {
      const response = await fetch(`${BACKEND_URL}/lights`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ lights: lightsDisplay.textContent.trim() }),
      });
      if (!response.ok) {
        message = 'error sending lights';
      } else {
        const data = await response.json();
        message = data.message;
      }
    } catch (error) {
      console.error('Failed to sends lights');
      message = 'error sending lights';
    }
    lightsDisplay.textContent = message;
  }

  lightsButton.addEventListener('click', sendLights);
}
