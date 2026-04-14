#include "PluginProcessor.h"
#include "PluginEditor.h"

WiredMemoryAudioProcessor::WiredMemoryAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
WiredMemoryAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Example parameter — replace / extend with your own.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 },
        "Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.5f));

    return layout;
}

WiredMemoryAudioProcessor::~WiredMemoryAudioProcessor() {}

const juce::String WiredMemoryAudioProcessor::getName() const { return JucePlugin_Name; }

bool WiredMemoryAudioProcessor::acceptsMidi() const  { return false; }
bool WiredMemoryAudioProcessor::producesMidi() const { return false; }
bool WiredMemoryAudioProcessor::isMidiEffect() const { return false; }
double WiredMemoryAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int WiredMemoryAudioProcessor::getNumPrograms() { return 1; }
int WiredMemoryAudioProcessor::getCurrentProgram() { return 0; }
void WiredMemoryAudioProcessor::setCurrentProgram (int) {}
const juce::String WiredMemoryAudioProcessor::getProgramName (int) { return {}; }
void WiredMemoryAudioProcessor::changeProgramName (int, const juce::String&) {}

void WiredMemoryAudioProcessor::prepareToPlay (double /*sampleRate*/, int /*samplesPerBlock*/) {}
void WiredMemoryAudioProcessor::releaseResources() {}

bool WiredMemoryAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void WiredMemoryAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Example: apply gain parameter to all channels.
    auto gainValue = apvts.getRawParameterValue ("gain")->load();
    buffer.applyGain (gainValue);
}

bool WiredMemoryAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* WiredMemoryAudioProcessor::createEditor()
{
    return new WiredMemoryAudioProcessorEditor (*this);
}

void WiredMemoryAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void WiredMemoryAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WiredMemoryAudioProcessor();
}
