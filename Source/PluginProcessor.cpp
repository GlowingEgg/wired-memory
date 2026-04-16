#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "capture/SCKAudioCapture.h"

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

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 },
        "Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.5f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "capture", 1 },
        "Capture",
        false));

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

void WiredMemoryAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    if (! capture_)
        capture_ = std::make_unique<SCKAudioCapture>();

    capture_->prepareForPlayback (sampleRate, samplesPerBlock);
}
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

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    const bool captureOn = apvts.getRawParameterValue ("capture")->load() >= 0.5f;

    // Plugin output is always silent during capture — Chrome's own audio output
    // serves as the monitor, so we never double it through the DAW.
    buffer.clear();

    // Drain the ring buffer so it doesn't fill up, and grab samples for waveform vis
    float captureL[2048], captureR[2048];
    int samplesRead = 0;

    if (captureOn && capture_ && capture_->isStreamReady())
    {
        float* capturePtrs[2] = { captureL, captureR };
        samplesRead = capture_->readSamples (capturePtrs, 2,
                                              juce::jmin (numSamples, 2048));
    }

    // Write waveform snapshot for UI visualisation (from captured audio, not output)
    {
        const float* waveformSrc = (samplesRead > 0) ? captureL
                                                       : buffer.getReadPointer (0);
        const int waveformLen = (samplesRead > 0) ? samplesRead : numSamples;
        const int step = juce::jmax (1, waveformLen / kWaveformSnapshotSize);

        const juce::SpinLock::ScopedLockType lock (waveformLock_);
        for (int i = 0; i < kWaveformSnapshotSize; ++i)
            waveformSnapshot_[i] = waveformSrc[juce::jmin (i * step, waveformLen - 1)];
        waveformReady_ = true;
    }
}

bool WiredMemoryAudioProcessor::readWaveformSnapshot (float* dest)
{
    const juce::SpinLock::ScopedLockType lock (waveformLock_);
    if (! waveformReady_)
        return false;
    std::memcpy (dest, waveformSnapshot_.data(), sizeof (float) * kWaveformSnapshotSize);
    waveformReady_ = false;
    return true;
}

bool WiredMemoryAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* WiredMemoryAudioProcessor::createEditor()
{
    return new WiredMemoryAudioProcessorEditor (*this);
}

void WiredMemoryAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Persist the selected capture source alongside APVTS state
    if (capture_)
    {
        juce::ValueTree captureState ("CaptureState");
        captureState.setProperty ("sourceBundleId",
            juce::String (capture_->getSelectedBundleId()), nullptr);
        state.addChild (captureState, -1, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void WiredMemoryAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        apvts.replaceState (tree);

        // Restore capture source
        auto captureState = tree.getChildWithName ("CaptureState");
        if (captureState.isValid() && capture_)
        {
            auto bundleId = captureState.getProperty ("sourceBundleId").toString().toStdString();
            if (! bundleId.empty())
                capture_->setSource (bundleId);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WiredMemoryAudioProcessor();
}
