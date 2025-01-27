export function initDistanceFeature(BACKEND_URL) {
  const fetchDistanceButton = document.getElementById('fetchDistanceButton');
  const distanceDisplay = document.getElementById('distanceDisplay');

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

  fetchDistanceButton.addEventListener('click', fetchDistance);
}
