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
    // Slider relays — playback controls
    juce::WebSliderRelay speedRelay  { "speed" };
    juce::WebSliderParameterAttachment speedAttachment;

    juce::WebSliderRelay startRelay  { "start" };
    juce::WebSliderParameterAttachment startAttachment;

    juce::WebSliderRelay lengthRelay { "length" };
    juce::WebSliderParameterAttachment lengthAttachment;

    juce::WebSliderRelay grainSizeRelay  { "grain_size" };
    juce::WebSliderParameterAttachment grainSizeAttachment;

    juce::WebSliderRelay densityRelay { "density" };
    juce::WebSliderParameterAttachment densityAttachment;

    juce::WebSliderRelay scatterRelay { "scatter" };
    juce::WebSliderParameterAttachment scatterAttachment;

    juce::WebSliderRelay pitchScatterRelay { "pitch_scatter" };
    juce::WebSliderParameterAttachment pitchScatterAttachment;

    juce::WebSliderRelay shapeRelay { "shape" };
    juce::WebSliderParameterAttachment shapeAttachment;

    // Toggle relays for capture and monitor
    juce::WebToggleButtonRelay captureRelay { "capture" };
    juce::WebToggleButtonParameterAttachment captureAttachment;

    juce::WebToggleButtonRelay monitorRelay { "monitor" };
    juce::WebToggleButtonParameterAttachment monitorAttachment;

    juce::WebToggleButtonRelay loopRelay { "loop" };
    juce::WebToggleButtonParameterAttachment loopAttachment;

    juce::WebToggleButtonRelay reverseRelay { "reverse" };
    juce::WebToggleButtonParameterAttachment reverseAttachment;

    std::unique_ptr<juce::WebBrowserComponent> webBrowser;

    /** Send the available source list to the JS frontend. */
    void refreshAndEmitSources();
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WiredMemoryAudioProcessorEditor)
};
