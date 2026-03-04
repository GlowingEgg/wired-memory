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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyPluginNameAudioProcessorEditor)
};
