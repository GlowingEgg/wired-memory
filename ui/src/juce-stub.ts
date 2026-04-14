// Dev-mode stub for juce-framework-frontend.
// The real module is injected by the JUCE WebView at runtime.
// plugin-bridge.ts guards all calls behind an isInsidePlugin() check,
// so these stubs are never actually called.

export function getSliderState(_name: string) {
  return null;
}

export function getToggleState(_name: string) {
  return null;
}
