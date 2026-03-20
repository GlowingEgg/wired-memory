#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MyPluginNameAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MyPluginNameAudioProcessorEditor (MyPluginNameAudioProcessor&);
    ~MyPluginNameAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MyPluginNameAudioProcessor& audioProcessor;

#if JUCE_WEB_BROWSER
    // Relay objects expose AudioProcessorValueTreeState parameters to JS.
    // Add one per parameter; the name string must match what plugin-bridge.ts uses.
    // juce::WebSliderRelay gainRelay { "gain" };

    std::unique_ptr<juce::WebBrowserComponent> webBrowser;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyPluginNameAudioProcessorEditor)
};
