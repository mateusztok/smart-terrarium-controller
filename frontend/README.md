# Terrarium Dashboard

The React dashboard uses the Vite development server. During local
development, Vite proxies `/api` requests to the backend on port `9080`.

Install dependencies with `npm ci`, then start the development server with
`npm run dev`. Run `npm run lint` and `npm run build` to validate the frontend.

Set `VITE_API_BASE_URL` when the API is hosted at a different base URL.
