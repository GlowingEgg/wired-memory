import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import path from "path";

// https://vite.dev/config/
export default defineConfig(({ command }) => ({
  plugins: [react()],
  resolve: {
    alias: command === "serve"
      ? {
          // Dev only: stub juce-framework-frontend so Vite's dep scanner
          // doesn't choke when the JUCE native module isn't available.
          "juce-framework-frontend": path.resolve(__dirname, "src/juce-stub.ts"),
        }
      : {},
  },
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
}));
