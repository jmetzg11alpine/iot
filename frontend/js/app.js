// const BACKEND_URL = 'http://localhost:8080';
const BACKEND_URL = 'https://iot-white-pond-1937.fly.dev';

document.addEventListener('DOMContentLoaded', () => {
  const fetchTimeButton = document.getElementById('fetchTimeButton');
  const timeDisplay = document.getElementById('timeDisplay');

  const fetchDistanceButton = document.getElementById('fetchDistanceButton');
  const distanceDisplay = document.getElementById('distanceDisplay');

  async function fetchTime() {
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

  async function fetchDistance() {
    try {
      const response = await fetch(`${BACKEND_URL}/distance`);
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }
      const data = await response.json();
      distanceDisplay.textContent = `Current distance: ${data.distance}`;
    } catch (error) {
      console.error('failed to fetch time:', error);
      distanceDisplay.textContent = 'code is broken';
    }
  }

  fetchTimeButton.addEventListener('click', fetchTime);
  fetchDistanceButton.addEventListener('click', fetchDistance);
});
