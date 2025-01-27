import { initTimeFeature } from './time.js';
import { initDistanceFeature } from './distance.js';
import { initLightFeature } from './lights.js';

// const BACKEND_URL = 'http://localhost:8080';
const BACKEND_URL = 'https://iot-white-pond-1937.fly.dev';

async function loadComponent(componentPath, targetId) {
  const response = await fetch(componentPath);
  const html = await response.text();
  document.getElementById(targetId).innerHTML += html;
}

document.addEventListener('DOMContentLoaded', async () => {
  const app = document.getElementById('app');
  app.innerHTML = `
  <div class="first-row">
    <div id="time"></div>
    <div id="distance"></div>
  </div>
  <div class="second-row">
    <div id="lights"></div>
  </div>
  `;

  await loadComponent('/html/time.html', 'time');
  await loadComponent('/html/distance.html', 'distance');
  await loadComponent('/html/lights.html', 'lights');

  initTimeFeature(BACKEND_URL);
  initDistanceFeature(BACKEND_URL);
  initLightFeature(BACKEND_URL);
});
