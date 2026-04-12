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
    // Relay objects expose APVTS parameters to the JS bridge.
    // The name string must match what plugin-bridge.ts uses.
    juce::WebSliderRelay gainRelay { "gain" };

    // Attachments connect relays to APVTS parameters so that UI changes
    // actually update the processor.  Without these, the relay talks to JS
    // but the parameter keeps its default value.
    juce::WebSliderParameterAttachment gainAttachment;

    std::unique_ptr<juce::WebBrowserComponent> webBrowser;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyPluginNameAudioProcessorEditor)
};
