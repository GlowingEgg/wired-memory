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
        juce::ParameterID { "speed", 1 },
        "Speed",
        juce::NormalisableRange<float> (0.1f, 4.0f, 0.0f, 0.5f),
        1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "start", 1 },
        "Start",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "length", 1 },
        "Length",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        1.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "capture", 1 },
        "Capture",
        false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "monitor", 1 },
        "Monitor",
        false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "loop", 1 },
        "Loop",
        false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "reverse", 1 },
        "Reverse",
        false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "grain_size", 1 },
        "Grain Size",
        juce::NormalisableRange<float> (0.01f, 0.5f, 0.0f, 0.5f),
        0.1f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "density", 1 },
        "Density",
        juce::NormalisableRange<float> (1.0f, 32.0f, 1.0f),
        1.0f));

    return layout;
}

WiredMemoryAudioProcessor::~WiredMemoryAudioProcessor() {}

const juce::String WiredMemoryAudioProcessor::getName() const { return JucePlugin_Name; }

bool WiredMemoryAudioProcessor::acceptsMidi() const  { return true; }
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

    currentSampleRate_ = sampleRate;

    // Pre-allocate record buffer (mono, up to kMaxRecordSeconds)
    recordBufferCapacity_ = static_cast<int> (sampleRate * kMaxRecordSeconds);
    recordBuffer_ = std::make_unique<float[]> (static_cast<size_t> (recordBufferCapacity_));
    recordBufferPos_ = 0;
    wasCapturing_ = false;

    // Reset grain pool
    for (auto& g : grainPool_)
        g.active = false;
    grainSpawnAccum_ = 0.0;
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

    const bool captureOn    = apvts.getRawParameterValue ("capture")->load() >= 0.5f;
    const float speed       = apvts.getRawParameterValue ("speed")->load();
    const float startNorm   = apvts.getRawParameterValue ("start")->load();
    const float lenNorm     = apvts.getRawParameterValue ("length")->load();
    const bool  loopOn      = apvts.getRawParameterValue ("loop")->load() >= 0.5f;
    const bool  reverseOn   = apvts.getRawParameterValue ("reverse")->load() >= 0.5f;
    const float grainSizeSec = apvts.getRawParameterValue ("grain_size")->load();
    const float density      = apvts.getRawParameterValue ("density")->load();

    // Start with silence — playback will write into the buffer below
    buffer.clear();

    // ── Handle pending playback start (set by message thread) ───────────
    if (playbackPending_.exchange (false))
    {
        const int totalLen = sampleLength_.load();
        const int startFrame = static_cast<int> (startNorm * totalLen);
        const int regionLen  = juce::jmax (1, static_cast<int> (lenNorm * totalLen));
        const int endFrame   = juce::jmin (startFrame + regionLen, totalLen);

        playbackPosFrac_ = reverseOn
            ? static_cast<double> (endFrame - 1)
            : static_cast<double> (startFrame);

        // Reset grain state for fresh playback
        for (auto& g : grainPool_)
            g.active = false;
        grainSpawnAccum_ = 0.0;

        playbackActive_.store (true);
    }

    // ── Sample playback (grain engine) ──────────────────────────────────
    if (playbackActive_.load() && recordBuffer_ && ! captureOn)
    {
        const int totalLen    = sampleLength_.load();
        const int startFrame  = static_cast<int> (startNorm * totalLen);
        const int regionLen   = juce::jmax (1, static_cast<int> (lenNorm * totalLen));
        const int endFrame    = juce::jmin (startFrame + regionLen, totalLen);

        const int grainSizeSamples = juce::jmax (1, static_cast<int> (grainSizeSec * currentSampleRate_));
        const double spawnInterval = static_cast<double> (grainSizeSamples) / static_cast<double> (density);
        const float gainComp = 1.0f / std::sqrt (density);

        for (int i = 0; i < numSamples; ++i)
        {
            // ── Advance master playhead ──
            if (reverseOn)
            {
                if (playbackPosFrac_ < static_cast<double> (startFrame))
                {
                    if (loopOn)
                        playbackPosFrac_ = static_cast<double> (endFrame - 1);
                    else
                    {
                        // Check if any grains are still active before stopping
                        bool anyActive = false;
                        for (const auto& g : grainPool_)
                            if (g.active) { anyActive = true; break; }
                        if (! anyActive)
                        {
                            playbackActive_.store (false);
                            playbackProgress_.store (0.0f);
                            break;
                        }
                    }
                }
            }
            else
            {
                if (playbackPosFrac_ >= static_cast<double> (endFrame))
                {
                    if (loopOn)
                        playbackPosFrac_ = static_cast<double> (startFrame);
                    else
                    {
                        bool anyActive = false;
                        for (const auto& g : grainPool_)
                            if (g.active) { anyActive = true; break; }
                        if (! anyActive)
                        {
                            playbackActive_.store (false);
                            playbackProgress_.store (0.0f);
                            break;
                        }
                    }
                }
            }

            // ── Spawn grains ──
            bool masterInRange = reverseOn
                ? (playbackPosFrac_ >= static_cast<double> (startFrame))
                : (playbackPosFrac_ < static_cast<double> (endFrame));

            if (masterInRange)
            {
                grainSpawnAccum_ += 1.0;
                while (grainSpawnAccum_ >= spawnInterval)
                {
                    grainSpawnAccum_ -= spawnInterval;

                    // Find an inactive grain slot
                    for (auto& g : grainPool_)
                    {
                        if (! g.active)
                        {
                            g.startPos  = playbackPosFrac_;
                            g.phase     = playbackPosFrac_;
                            g.speed     = static_cast<double> (speed);
                            g.lifetime  = grainSizeSamples;
                            g.totalLife = grainSizeSamples;
                            g.active    = true;
                            g.reverse   = reverseOn;
                            break;
                        }
                    }
                }
            }

            // ── Sum active grains ──
            float mix = 0.0f;
            for (auto& g : grainPool_)
            {
                if (! g.active)
                    continue;

                // Envelope phase: 0 at start → 1 at end of grain
                const float envPhase = 1.0f - static_cast<float> (g.lifetime)
                                              / static_cast<float> (g.totalLife);
                // Hann window
                const float window = 0.5f * (1.0f - std::cos (6.283185307f * envPhase));

                // Read sample with linear interpolation
                const int idx0 = juce::jlimit (0, totalLen - 1, static_cast<int> (g.phase));
                const int idx1 = juce::jmin (idx0 + 1, totalLen - 1);
                const float frac = static_cast<float> (g.phase - idx0);
                const float samp = recordBuffer_[idx0] * (1.0f - frac)
                                 + recordBuffer_[idx1] * frac;

                mix += samp * window;

                // Advance grain phase
                g.phase += g.reverse ? -g.speed : g.speed;

                // Decrement lifetime
                if (--g.lifetime <= 0)
                    g.active = false;
            }

            // Write to output with gain compensation
            mix *= gainComp;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.getWritePointer (ch)[i] = mix;

            // Advance master playhead
            playbackPosFrac_ += reverseOn
                ? -static_cast<double> (speed)
                :  static_cast<double> (speed);
        }

        // Update progress for UI
        if (playbackActive_.load() && regionLen > 0)
        {
            const double elapsed = reverseOn
                ? static_cast<double> (endFrame - 1) - playbackPosFrac_
                : playbackPosFrac_ - static_cast<double> (startFrame);
            playbackProgress_.store (static_cast<float> (elapsed / regionLen));
        }
    }

    // Reset accumulation buffer when recording starts
    if (captureOn && ! wasCapturing_)
        recordBufferPos_ = 0;

    // Always drain the ring buffer so the waveform scope stays live
    float captureL[2048], captureR[2048];
    int samplesRead = 0;

    if (capture_ && capture_->isStreamReady())
    {
        float* capturePtrs[2] = { captureL, captureR };
        samplesRead = capture_->readSamples (capturePtrs, 2,
                                              juce::jmin (numSamples, 2048));

        // Accumulate into the record buffer only while recording (mono — left channel)
        if (captureOn && samplesRead > 0 && recordBuffer_)
        {
            const int space = recordBufferCapacity_ - recordBufferPos_;
            const int toWrite = juce::jmin (samplesRead, space);
            if (toWrite > 0)
            {
                std::memcpy (recordBuffer_.get() + recordBufferPos_,
                             captureL,
                             sizeof (float) * static_cast<size_t> (toWrite));
                recordBufferPos_ += toWrite;
            }
        }
    }

    // When capture stops, store the sample length and downsample into a peak-envelope snapshot
    if (wasCapturing_ && ! captureOn && recordBufferPos_ > 0)
    {
        sampleLength_.store (recordBufferPos_);

        const juce::SpinLock::ScopedLockType lock (sampleLock_);
        const float* src = recordBuffer_.get();
        const int srcLen = recordBufferPos_;

        for (int i = 0; i < kSampleSnapshotSize; ++i)
        {
            const int start = i * srcLen / kSampleSnapshotSize;
            const int end   = juce::jmin ((i + 1) * srcLen / kSampleSnapshotSize, srcLen);
            float peak = 0.0f;
            for (int j = start; j < end; ++j)
            {
                if (std::abs (src[j]) > std::abs (peak))
                    peak = src[j];
            }
            sampleSnapshot_[i] = peak;
        }
        sampleReady_ = true;
    }

    wasCapturing_ = captureOn;

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

bool WiredMemoryAudioProcessor::readSampleSnapshot (float* dest)
{
    const juce::SpinLock::ScopedLockType lock (sampleLock_);
    if (! sampleReady_)
        return false;
    std::memcpy (dest, sampleSnapshot_.data(), sizeof (float) * kSampleSnapshotSize);
    sampleReady_ = false;
    return true;
}

void WiredMemoryAudioProcessor::startPlayback()
{
    if (sampleLength_.load() > 0)
        playbackPending_.store (true);
}

void WiredMemoryAudioProcessor::stopPlayback()
{
    playbackActive_.store (false);
    playbackProgress_.store (0.0f);
    for (auto& g : grainPool_)
        g.active = false;
    grainSpawnAccum_ = 0.0;
}

float WiredMemoryAudioProcessor::getPlaybackProgress() const
{
    if (! playbackActive_.load())
        return 0.0f;
    return playbackProgress_.load();
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
