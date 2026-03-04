#include "PluginProcessor.h"
#include "PluginEditor.h"

MyPluginNameAudioProcessorEditor::MyPluginNameAudioProcessorEditor (MyPluginNameAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);
}

MyPluginNameAudioProcessorEditor::~MyPluginNameAudioProcessorEditor() {}

void MyPluginNameAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("My Plugin Name", getLocalBounds(), juce::Justification::centred, 1);
}

void MyPluginNameAudioProcessorEditor::resized() {}
