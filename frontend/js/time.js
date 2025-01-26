export function initTimeFeature(BACKEND_URL) {
  const fetchTimeButton = document.getElementById('fetchTimeButton');
  const timeDisplay = document.getElementById('timeDisplay');

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

  fetchTimeButton.addEventListener('click', fetchTime);
}
