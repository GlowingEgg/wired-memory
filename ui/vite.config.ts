import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  // Build outputs to dist/ — this directory gets embedded as BinaryData by CMake.
  build: {
    outDir: "dist",
    // Inline all assets so the plugin doesn't need to serve separate chunks.
    assetsInlineLimit: 1024 * 1024,
  },
  // Allow the dev server to be reached from the plugin's WebBrowserComponent.
  server: {
    host: "localhost",
    port: 5173,
    cors: true,
  },
});
