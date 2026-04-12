#include "PluginProcessor.h"
#include "PluginEditor.h"

MyPluginNameAudioProcessor::MyPluginNameAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
MyPluginNameAudioProcessor::createParameterLayout()
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

MyPluginNameAudioProcessor::~MyPluginNameAudioProcessor() {}

const juce::String MyPluginNameAudioProcessor::getName() const { return JucePlugin_Name; }

bool MyPluginNameAudioProcessor::acceptsMidi() const  { return false; }
bool MyPluginNameAudioProcessor::producesMidi() const { return false; }
bool MyPluginNameAudioProcessor::isMidiEffect() const { return false; }
double MyPluginNameAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int MyPluginNameAudioProcessor::getNumPrograms() { return 1; }
int MyPluginNameAudioProcessor::getCurrentProgram() { return 0; }
void MyPluginNameAudioProcessor::setCurrentProgram (int) {}
const juce::String MyPluginNameAudioProcessor::getProgramName (int) { return {}; }
void MyPluginNameAudioProcessor::changeProgramName (int, const juce::String&) {}

void MyPluginNameAudioProcessor::prepareToPlay (double /*sampleRate*/, int /*samplesPerBlock*/) {}
void MyPluginNameAudioProcessor::releaseResources() {}

bool MyPluginNameAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void MyPluginNameAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Example: apply gain parameter to all channels.
    auto gainValue = apvts.getRawParameterValue ("gain")->load();
    buffer.applyGain (gainValue);
}

bool MyPluginNameAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* MyPluginNameAudioProcessor::createEditor()
{
    return new MyPluginNameAudioProcessorEditor (*this);
}

void MyPluginNameAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MyPluginNameAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MyPluginNameAudioProcessor();
}
