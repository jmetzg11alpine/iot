<script>
	let time = '';
	let loading = false;

	const BACKEND_URL = import.meta.env.VITE_BACKEND_URL;

	async function fetchTime() {
		loading = true;
		const response = await fetch(`${BACKEND_URL}/time`);
		const data = await response.json();
		time = data.time;
		loading = false;
	}
</script>

<div class="m-4"></div>
<button on:click={fetchTime} class="bg-slate-600">{loading ? 'Loading...' : 'Get Time'}</button>
<p>{time ? `Current time: ${time}` : 'click button to get time'}</p>
