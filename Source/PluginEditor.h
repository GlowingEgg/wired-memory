#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class WiredMemoryAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit WiredMemoryAudioProcessorEditor (WiredMemoryAudioProcessor&);
    ~WiredMemoryAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    WiredMemoryAudioProcessor& audioProcessor;

#if JUCE_WEB_BROWSER
    // Slider relays
    juce::WebSliderRelay gainRelay { "gain" };
    juce::WebSliderParameterAttachment gainAttachment;

    // Toggle relays for capture and monitor
    juce::WebToggleButtonRelay captureRelay { "capture" };
    juce::WebToggleButtonParameterAttachment captureAttachment;

    juce::WebToggleButtonRelay monitorRelay { "monitor" };
    juce::WebToggleButtonParameterAttachment monitorAttachment;

    std::unique_ptr<juce::WebBrowserComponent> webBrowser;

    /** Send the available source list to the JS frontend. */
    void refreshAndEmitSources();
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WiredMemoryAudioProcessorEditor)
};
