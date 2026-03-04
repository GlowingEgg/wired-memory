#include "PluginProcessor.h"
#include "PluginEditor.h"

MyPluginNameAudioProcessor::MyPluginNameAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
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

    // Audio processing goes here.
    // buffer contains interleaved input samples; write your output into the same buffer.
}

bool MyPluginNameAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* MyPluginNameAudioProcessor::createEditor()
{
    return new MyPluginNameAudioProcessorEditor (*this);
}

void MyPluginNameAudioProcessor::getStateInformation (juce::MemoryBlock& /*destData*/) {}
void MyPluginNameAudioProcessor::setStateInformation (const void* /*data*/, int /*sizeInBytes*/) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MyPluginNameAudioProcessor();
}
