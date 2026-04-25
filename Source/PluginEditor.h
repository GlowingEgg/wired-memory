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

    // Spectral freeze / drift / smear
    juce::WebSliderRelay driftRelay { "drift" };
    juce::WebSliderParameterAttachment driftAttachment;

    juce::WebSliderRelay smearRelay { "smear" };
    juce::WebSliderParameterAttachment smearAttachment;

    juce::WebToggleButtonRelay freezeRelay { "freeze" };
    juce::WebToggleButtonParameterAttachment freezeAttachment;

    juce::WebToggleButtonRelay speedLockPitchRelay { "speed_lock_pitch" };
    juce::WebToggleButtonParameterAttachment speedLockPitchAttachment;

    // Hybrid sampler/synth mode (MIDI 1/4)
    juce::WebToggleButtonRelay synthModeRelay { "synth_mode" };
    juce::WebToggleButtonParameterAttachment synthModeAttachment;

    juce::WebToggleButtonRelay triggerModeRelay { "trigger_mode" };
    juce::WebToggleButtonParameterAttachment triggerModeAttachment;

    juce::WebSliderRelay rootNoteRelay { "root_note" };
    juce::WebSliderParameterAttachment rootNoteAttachment;

    juce::WebSliderRelay densityTrackRelay { "density_track" };
    juce::WebSliderParameterAttachment densityTrackAttachment;

    juce::WebSliderRelay velocitySensRelay { "velocity_sens" };
    juce::WebSliderParameterAttachment velocitySensAttachment;

    juce::WebSliderRelay ampAttackRelay { "amp_attack" };
    juce::WebSliderParameterAttachment ampAttackAttachment;

    juce::WebSliderRelay ampReleaseRelay { "amp_release" };
    juce::WebSliderParameterAttachment ampReleaseAttachment;

    juce::WebSliderRelay glideRelay { "glide" };
    juce::WebSliderParameterAttachment glideAttachment;

    juce::WebSliderRelay fineTuneRelay { "fine_tune" };
    juce::WebSliderParameterAttachment fineTuneAttachment;

    std::unique_ptr<juce::WebBrowserComponent> webBrowser;

    /** Send the available source list to the JS frontend. */
    void refreshAndEmitSources();
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WiredMemoryAudioProcessorEditor)
};
