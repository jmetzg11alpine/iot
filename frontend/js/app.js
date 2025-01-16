// const BACKEND_URL = 'http://localhost:8080';
const BACKEND_URL = 'https://iot-white-pond-1937.fly.dev';

document.addEventListener('DOMContentLoaded', () => {
  const timeDisplay = document.getElementById('timeDisplay');
  const fetchButton = document.getElementById('fetchButton');

  async function fetchTime() {
    console.log('fetch time was called');
    try {
      const response = await fetch(`${BACKEND_URL}/time`);
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      const data = await response.json();
      timeDisplay.textContent = `Current time: ${data.time}`;
    } catch (error) {
      console.error('failed to fetch time:', error);
      timeDisplay.textContent = 'code is broken';
    }
  }

  fetchButton.addEventListener('click', fetchTime);
});
