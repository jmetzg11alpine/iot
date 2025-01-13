// const BACKEND_URL = 'http://localhost:8080';
const BACKEND_URL = 'http://192.168.1.245:8080';

document.addEventListener('DOMContentLoaded', () => {
  const timeDisplay = document.getElementById('timeDisplay');
  const fetchButton = document.getElementById('fetchButton');

  async function fetchTime() {
    console.log('i was called');
    const response = await fetch(`${BACKEND_URL}/time`);
    const data = await response.json();
    timeDisplay.textContent = `Current time: ${data.time}`;
  }

  fetchButton.addEventListener('click', fetchTime);
});
