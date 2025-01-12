import adapter from '@sveltejs/adapter-static';

export default {
	kit: {
		adapter: adapter({
			// Serve `index.html` for all routes (SPA behavior)
			fallback: 'index.html'
		}),
		// Ensure proper paths for static files
		paths: {
			base: '',
			assets: ''
		}
	}
};
