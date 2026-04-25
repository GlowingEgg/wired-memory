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
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.0f, 0.3f),
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

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "scatter", 1 },
        "Scatter",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pitch_scatter", 1 },
        "Pitch",
        juce::NormalisableRange<float> (-1.0f, 1.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "shape", 1 },
        "Shape",
        juce::NormalisableRange<float> (0.0f, 3.0f, 1.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "freeze", 1 },
        "Freeze",
        false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drift", 1 },
        "Drift",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "smear", 1 },
        "Smear",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "speed_lock_pitch", 1 },
        "Speed Lock Pitch",
        true));

    // ── Hybrid sampler/synth mode (MIDI 1/4) ─────────────────────────────
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "synth_mode", 1 },
        "Synth Mode",
        false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "trigger_mode", 1 },
        "Trigger Mode",
        false));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "root_note", 1 },
        "Root Note",
        juce::NormalisableRange<float> (0.0f, 127.0f, 1.0f),
        60.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "density_track", 1 },
        "Density Track",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "velocity_sens", 1 },
        "Velocity Sensitivity",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.7f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "amp_attack", 1 },
        "Amp Attack",
        juce::NormalisableRange<float> (0.001f, 2.0f, 0.0f, 0.5f),
        0.005f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "amp_release", 1 },
        "Amp Release",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.5f),
        0.15f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "glide", 1 },
        "Glide",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "fine_tune", 1 },
        "Fine Tune",
        juce::NormalisableRange<float> (-100.0f, 100.0f),
        0.0f));

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

    // Pre-compute Hann window
    for (int i = 0; i < kFFTSize; ++i)
        hannWindow_[i] = 0.5f * (1.0f - std::cos (2.0f * juce::MathConstants<float>::pi
                                                    * static_cast<float> (i)
                                                    / static_cast<float> (kFFTSize)));

    // Zero FFT buffers
    fftInputBuffer_.fill (0.0f);
    fftInputWritePos_ = 0;
    olaBuffer_.fill (0.0f);
    olaReadPos_  = 0;
    olaWritePos_ = 0;
    currentMagnitude_.fill (0.0f);
    currentPhase_.fill (0.0f);
    frozenMagnitude_.fill (0.0f);
    frozenPhase_.fill (0.0f);
    driftVelocity_.fill (0.0f);
    fftWorkBuffer_.fill (0.0f);
    wasFrozen_ = false;
    hasFrozenFrame_ = false;
    freezeHopCounter_ = 0;
    driftRngState_ = 0xDEADBEEF;

    triggeringNote_              = -1;
    releaseFadeActive_           = false;
    releaseFadeSamplesRemaining_ = 0;
    releaseFadeTotal_            = 0;

    // Reset polyphonic voices
    for (auto& v : voices_)
        v = Voice {};
    voiceAllocCounter_ = 0;
    wasSynthMode_ = false;
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
                                               juce::MidiBuffer& midiMessages)
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
    const float scatter      = apvts.getRawParameterValue ("scatter")->load();
    const float pitchScatter = apvts.getRawParameterValue ("pitch_scatter")->load();
    const int   shapeType    = static_cast<int> (apvts.getRawParameterValue ("shape")->load());
    const bool  freezeOn     = apvts.getRawParameterValue ("freeze")->load() >= 0.5f;
    const float drift        = apvts.getRawParameterValue ("drift")->load();
    const float smear        = apvts.getRawParameterValue ("smear")->load();
    const bool  speedLockPitch = apvts.getRawParameterValue ("speed_lock_pitch")->load() >= 0.5f;
    const bool  synthMode    = apvts.getRawParameterValue ("synth_mode")->load() >= 0.5f;
    const bool  gateMode     = apvts.getRawParameterValue ("trigger_mode")->load() >= 0.5f;
    const float rootNote     = apvts.getRawParameterValue ("root_note")->load();
    const float densityTrack = apvts.getRawParameterValue ("density_track")->load();
    const float velocitySens = apvts.getRawParameterValue ("velocity_sens")->load();
    const float ampAttack    = apvts.getRawParameterValue ("amp_attack")->load();
    const float ampRelease   = apvts.getRawParameterValue ("amp_release")->load();
    const float glide        = apvts.getRawParameterValue ("glide")->load();
    const float fineTune     = apvts.getRawParameterValue ("fine_tune")->load();

    // ── Mode-switch detection: reset both engines on transition ───────────
    if (synthMode != wasSynthMode_)
    {
        playbackActive_.store (false);
        playbackPending_.store (false);
        playbackProgress_.store (0.0f);
        for (auto& g : grainPool_)
            g.active = false;
        grainSpawnAccum_ = 0.0;
        for (auto& v : voices_)
            v = Voice {};
        voiceAllocCounter_ = 0;
        triggeringNote_              = -1;
        releaseFadeActive_           = false;
        releaseFadeSamplesRemaining_ = 0;
        wasSynthMode_ = synthMode;
    }

    // ── MIDI handling (block-quantised) ────────────────────────────────────
    if (! synthMode)
    {
        // Sampler-mode MIDI (Ticket 2)
        for (const auto meta : midiMessages)
        {
            const auto msg = meta.getMessage();

            if (msg.isNoteOn())
            {
                if (sampleLength_.load() > 0)
                {
                    playbackPending_.store (true);
                    triggeringNote_     = msg.getNoteNumber();
                    releaseFadeActive_  = false;
                }
            }
            else if (msg.isNoteOff() && gateMode)
            {
                const int note = msg.getNoteNumber();
                const bool willPlay = playbackActive_.load() || playbackPending_.load();
                if (willPlay && note == triggeringNote_)
                {
                    releaseFadeTotal_            = juce::jmax (1, static_cast<int> (currentSampleRate_ * 0.030));
                    releaseFadeSamplesRemaining_ = releaseFadeTotal_;
                    releaseFadeActive_           = true;
                }
            }
            else if (msg.isAllNotesOff() && gateMode)
            {
                if (playbackActive_.load() || playbackPending_.load())
                {
                    releaseFadeTotal_            = juce::jmax (1, static_cast<int> (currentSampleRate_ * 0.030));
                    releaseFadeSamplesRemaining_ = releaseFadeTotal_;
                    releaseFadeActive_           = true;
                }
            }
        }
    }
    else
    {
        // Synth-mode MIDI: voice allocation + envelope state changes
        const int totalLen   = sampleLength_.load();
        const int startFrame = static_cast<int> (startNorm * totalLen);
        const int regionLen  = juce::jmax (1, static_cast<int> (lenNorm * totalLen));
        const int endFrame   = juce::jmin (startFrame + regionLen, totalLen);

        for (const auto meta : midiMessages)
        {
            const auto msg = meta.getMessage();

            if (msg.isNoteOn() && totalLen > 0)
            {
                const int   note     = msg.getNoteNumber();
                const float velocity = msg.getFloatVelocity();

                // Allocate voice: prefer idle, then oldest Release, else oldest of any
                int alloc = -1;
                for (int v = 0; v < kNumVoices; ++v)
                    if (voices_[v].envState == EnvState::Idle) { alloc = v; break; }

                if (alloc < 0)
                {
                    uint64_t oldest = UINT64_MAX;
                    for (int v = 0; v < kNumVoices; ++v)
                        if (voices_[v].envState == EnvState::Release
                            && voices_[v].allocOrder < oldest)
                        {
                            oldest = voices_[v].allocOrder;
                            alloc = v;
                        }
                }
                if (alloc < 0)
                {
                    uint64_t oldest = UINT64_MAX;
                    for (int v = 0; v < kNumVoices; ++v)
                        if (voices_[v].allocOrder < oldest)
                        {
                            oldest = voices_[v].allocOrder;
                            alloc = v;
                        }
                }
                if (alloc < 0) continue;

                auto& voice = voices_[alloc];
                const bool wasIdle = (voice.envState == EnvState::Idle);

                voice.pitchMul = std::exp2 ((static_cast<double> (note - rootNote)) / 12.0
                                            + static_cast<double> (fineTune) / 1200.0);

                if (glide <= 0.0f || wasIdle)
                    voice.currentPitchMul = voice.pitchMul;
                // else: keep currentPitchMul, glide ramps it in the sample loop

                // Density tracking (auto-shrink grain if density would exceed 32)
                const double targetFreq = 440.0 * std::exp2 ((note - 69.0) / 12.0);
                const double freqGrainProduct = targetFreq * static_cast<double> (grainSizeSec);
                const double targetDensity = juce::jlimit (1.0, 32.0, freqGrainProduct);
                voice.effectiveDensity = (1.0f - densityTrack) * density
                                       + densityTrack * static_cast<float> (targetDensity);
                voice.effectiveDensity = juce::jlimit (1.0f, 32.0f, voice.effectiveDensity);

                if (freqGrainProduct > 32.0)
                    voice.effectiveGrainSize = static_cast<float> (
                        std::min (static_cast<double> (grainSizeSec), 32.0 / targetFreq));
                else
                    voice.effectiveGrainSize = grainSizeSec;

                voice.velocityGain = (1.0f - velocitySens) + velocitySens * velocity;

                voice.playbackPos = reverseOn
                    ? static_cast<double> (endFrame - 1)
                    : static_cast<double> (startFrame);
                voice.grainSpawnAccum = 1e9; // spawn first grain immediately

                // Clear any stale grains owned by this voice slot
                for (auto& g : grainPool_)
                    if (g.voiceIndex == alloc)
                        g.active = false;

                voice.envState   = EnvState::Attack;
                voice.envLevel   = 0.0f;
                voice.midiNote   = note;
                voice.allocOrder = ++voiceAllocCounter_;
                voice.sustainHeld = false;
            }
            else if (msg.isNoteOff())
            {
                const int note = msg.getNoteNumber();
                for (auto& voice : voices_)
                {
                    if (voice.midiNote == note && voice.envState != EnvState::Idle)
                    {
                        // Sustain pedal logic deferred to Ticket 4
                        voice.envState = EnvState::Release;
                    }
                }
            }
            else if (msg.isAllNotesOff())
            {
                for (auto& voice : voices_)
                    if (voice.envState != EnvState::Idle)
                        voice.envState = EnvState::Release;
            }
        }
    }

    // Start with silence — playback will write into the buffer below
    buffer.clear();

    if (! synthMode)
    {
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
        grainSpawnAccum_ = 1e9;

        playbackActive_.store (true);
    }

    // ── Sample playback (grain engine) ──────────────────────────────────
    if (playbackActive_.load() && recordBuffer_ && ! captureOn)
    {
        const int totalLen    = sampleLength_.load();
        const int startFrame  = static_cast<int> (startNorm * totalLen);
        const int regionLen   = juce::jmax (1, static_cast<int> (lenNorm * totalLen));
        const int endFrame    = juce::jmin (startFrame + regionLen, totalLen);

        // Deterministic pitch multiplier
        const double pitchMul = (std::abs (pitchScatter) > 0.001f)
                              ? std::exp2 (static_cast<double> (pitchScatter))
                              : 1.0;

        if (density <= 1.0f)
        {
            // ── Direct playback — clean varispeed, no grain engine ──
            const double effectiveSpeed = static_cast<double> (speed) * pitchMul;

            for (int i = 0; i < numSamples; ++i)
            {
                if (reverseOn)
                {
                    if (playbackPosFrac_ < static_cast<double> (startFrame))
                    {
                        if (loopOn)
                            playbackPosFrac_ = static_cast<double> (endFrame - 1);
                        else
                        {
                            playbackActive_.store (false);
                            playbackProgress_.store (0.0f);
                            break;
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
                            playbackActive_.store (false);
                            playbackProgress_.store (0.0f);
                            break;
                        }
                    }
                }

                const int idx0 = juce::jlimit (0, totalLen - 1, static_cast<int> (playbackPosFrac_));
                const int idx1 = juce::jmin (idx0 + 1, totalLen - 1);
                const float frac = static_cast<float> (playbackPosFrac_ - idx0);
                const float samp = recordBuffer_[idx0] * (1.0f - frac)
                                 + recordBuffer_[idx1] * frac;

                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    buffer.getWritePointer (ch)[i] = samp;

                playbackPosFrac_ += reverseOn ? -effectiveSpeed : effectiveSpeed;
            }
        }
        else
        {
        // ── Grain engine ──
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
                            // Apply scatter: offset grain start by random amount
                            double grainStart = playbackPosFrac_;
                            if (scatter > 0.0f)
                            {
                                const float rnd = nextRandom() - 0.5f; // [-0.5, 0.5]
                                grainStart += static_cast<double> (rnd * scatter) * regionLen;
                                grainStart = juce::jlimit (static_cast<double> (startFrame),
                                                           static_cast<double> (endFrame - 1),
                                                           grainStart);
                            }
                            g.startPos  = grainStart;
                            g.phase     = grainStart;

                            // Grain speed: varispeed or pitch-independent
                            double grainSpeed = speedLockPitch
                                ? static_cast<double> (speed) : 1.0;
                            grainSpeed *= pitchMul;
                            g.speed     = grainSpeed;
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

                // Window function selected by shape parameter
                float window;
                switch (shapeType)
                {
                    default:
                    case 0: // Hann
                        window = 0.5f * (1.0f - std::cos (6.283185307f * envPhase));
                        break;
                    case 1: // Triangle
                        window = 1.0f - std::abs (2.0f * envPhase - 1.0f);
                        break;
                    case 2: // Trapezoid (15% ramp, 70% hold, 15% ramp)
                        if (envPhase < 0.15f)
                            window = envPhase / 0.15f;
                        else if (envPhase > 0.85f)
                            window = (1.0f - envPhase) / 0.15f;
                        else
                            window = 1.0f;
                        break;
                    case 3: // Rectangle
                        window = 1.0f;
                        break;
                }

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
        } // end grain engine

        // Update progress for UI
        if (playbackActive_.load() && regionLen > 0)
        {
            const double elapsed = reverseOn
                ? static_cast<double> (endFrame - 1) - playbackPosFrac_
                : playbackPosFrac_ - static_cast<double> (startFrame);
            playbackProgress_.store (static_cast<float> (elapsed / regionLen));
        }

        // Write grain snapshot for UI visualisation
        {
            const juce::SpinLock::ScopedLockType lock (grainSnapshotLock_);
            int count = 0;
            for (int gi = 0; gi < kMaxGrains && count < kMaxGrains; ++gi)
            {
                if (grainPool_[gi].active)
                {
                    grainSnapshot_[count].position = static_cast<float> (grainPool_[gi].phase / totalLen);
                    grainSnapshot_[count].active   = true;
                    ++count;
                }
            }
            grainSnapshotCount_ = count;
            grainSnapshotReady_ = true;
        }
    }
    } // end sampler-mode engine
    else if (recordBuffer_ && ! captureOn && sampleLength_.load() > 0)
    {
        // ── Synth-mode 6-voice polyphonic granular engine ────────────────
        const int totalLen   = sampleLength_.load();
        const int startFrame = static_cast<int> (startNorm * totalLen);
        const int regionLen  = juce::jmax (1, static_cast<int> (lenNorm * totalLen));
        const int endFrame   = juce::jmin (startFrame + regionLen, totalLen);

        // Pre-compute envelope/glide coefficients (per-block constants)
        const float attackInc   = 1.0f / juce::jmax (1.0f, ampAttack  * static_cast<float> (currentSampleRate_));
        const float releaseDec  = std::exp (-1.0f / juce::jmax (1.0f, ampRelease * static_cast<float> (currentSampleRate_)));
        const float glideAlpha  = (glide > 0.0f)
            ? 1.0f - std::exp (-1.0f / juce::jmax (1.0f, glide * static_cast<float> (currentSampleRate_)))
            : 1.0f;

        // Per-voice spawn intervals & grain sizes (samples)
        std::array<int, kNumVoices>    voiceGrainSizeSamples {};
        std::array<double, kNumVoices> voiceSpawnInterval    {};
        for (int v = 0; v < kNumVoices; ++v)
        {
            voiceGrainSizeSamples[v] = juce::jmax (1, static_cast<int> (
                voices_[v].effectiveGrainSize * currentSampleRate_));
            voiceSpawnInterval[v] = static_cast<double> (voiceGrainSizeSamples[v])
                                  / static_cast<double> (juce::jmax (1.0f, voices_[v].effectiveDensity));
        }

        const double speedAbs = static_cast<double> (speed);
        const float twoPi = juce::MathConstants<float>::twoPi;

        for (int i = 0; i < numSamples; ++i)
        {
            // ── Per-voice update: glide, envelope, playhead, spawn ──
            for (int v = 0; v < kNumVoices; ++v)
            {
                auto& voice = voices_[v];
                if (voice.envState == EnvState::Idle)
                    continue;

                // Glide
                if (glide > 0.0f)
                    voice.currentPitchMul += (voice.pitchMul - voice.currentPitchMul) * glideAlpha;
                else
                    voice.currentPitchMul = voice.pitchMul;

                // Envelope
                switch (voice.envState)
                {
                    case EnvState::Attack:
                        voice.envLevel += (1.0f - voice.envLevel) * attackInc;
                        if (voice.envLevel >= 0.999f)
                        {
                            voice.envLevel = 1.0f;
                            voice.envState = EnvState::Sustain;
                        }
                        break;
                    case EnvState::Sustain:
                        voice.envLevel = 1.0f;
                        break;
                    case EnvState::Release:
                        voice.envLevel *= releaseDec;
                        if (voice.envLevel < 0.001f)
                        {
                            voice.envLevel = 0.0f;
                            voice.envState = EnvState::Idle;
                            voice.midiNote = -1;
                            for (auto& g : grainPool_)
                                if (g.voiceIndex == v)
                                    g.active = false;
                        }
                        break;
                    case EnvState::Idle:
                    default: break;
                }

                if (voice.envState == EnvState::Idle)
                    continue;

                // Playhead bounds
                if (reverseOn)
                {
                    if (voice.playbackPos < static_cast<double> (startFrame))
                    {
                        if (loopOn) voice.playbackPos = static_cast<double> (endFrame - 1);
                    }
                    if (voice.playbackPos >= static_cast<double> (endFrame))
                        voice.playbackPos = static_cast<double> (endFrame - 1);
                }
                else
                {
                    if (voice.playbackPos >= static_cast<double> (endFrame))
                    {
                        if (loopOn) voice.playbackPos = static_cast<double> (startFrame);
                    }
                    if (voice.playbackPos < static_cast<double> (startFrame))
                        voice.playbackPos = static_cast<double> (startFrame);
                }

                const bool inRange = reverseOn
                    ? (voice.playbackPos >= static_cast<double> (startFrame))
                    : (voice.playbackPos <  static_cast<double> (endFrame));

                // Spawn grains for this voice
                if (inRange)
                {
                    voice.grainSpawnAccum += 1.0;
                    while (voice.grainSpawnAccum >= voiceSpawnInterval[v])
                    {
                        voice.grainSpawnAccum -= voiceSpawnInterval[v];

                        // Find inactive slot, else steal oldest grain owned by this voice
                        Grain* slot = nullptr;
                        for (auto& g : grainPool_)
                        {
                            if (! g.active) { slot = &g; break; }
                        }
                        if (slot == nullptr)
                        {
                            int oldest = INT_MAX;
                            for (auto& g : grainPool_)
                            {
                                if (g.voiceIndex == v && g.lifetime < oldest)
                                {
                                    oldest = g.lifetime;
                                    slot = &g;
                                }
                            }
                        }
                        if (slot == nullptr)
                            break; // pool full, no stealable slot for this voice

                        double grainStart = voice.playbackPos;
                        if (scatter > 0.0f)
                        {
                            const float rnd = nextRandom() - 0.5f;
                            grainStart += static_cast<double> (rnd * scatter) * regionLen;
                            grainStart = juce::jlimit (static_cast<double> (startFrame),
                                                       static_cast<double> (endFrame - 1),
                                                       grainStart);
                        }
                        slot->startPos = grainStart;
                        slot->phase    = grainStart;

                        double grainSpeed = speedLockPitch ? speedAbs : 1.0;
                        grainSpeed *= voice.currentPitchMul;
                        if (std::abs (pitchScatter) > 0.001f)
                            grainSpeed *= std::exp2 (static_cast<double> (pitchScatter));
                        slot->speed      = grainSpeed;
                        slot->lifetime   = voiceGrainSizeSamples[v];
                        slot->totalLife  = voiceGrainSizeSamples[v];
                        slot->active     = true;
                        slot->reverse    = reverseOn;
                        slot->voiceIndex = v;
                    }
                }

                voice.playbackPos += reverseOn ? -speedAbs : speedAbs;
            }

            // ── Sum active grains, scaled by owning voice's env*velocity ──
            float mix = 0.0f;
            for (auto& g : grainPool_)
            {
                if (! g.active)
                    continue;

                const float envPhase = 1.0f - static_cast<float> (g.lifetime)
                                              / static_cast<float> (g.totalLife);
                float window;
                switch (shapeType)
                {
                    default:
                    case 0:
                        window = 0.5f * (1.0f - std::cos (twoPi * envPhase));
                        break;
                    case 1:
                        window = 1.0f - std::abs (2.0f * envPhase - 1.0f);
                        break;
                    case 2:
                        if (envPhase < 0.15f)        window = envPhase / 0.15f;
                        else if (envPhase > 0.85f)   window = (1.0f - envPhase) / 0.15f;
                        else                         window = 1.0f;
                        break;
                    case 3:
                        window = 1.0f;
                        break;
                }

                const int idx0 = juce::jlimit (0, totalLen - 1, static_cast<int> (g.phase));
                const int idx1 = juce::jmin (idx0 + 1, totalLen - 1);
                const float frac = static_cast<float> (g.phase - idx0);
                const float samp = recordBuffer_[idx0] * (1.0f - frac)
                                 + recordBuffer_[idx1] * frac;

                float voiceGain = 1.0f;
                if (g.voiceIndex >= 0 && g.voiceIndex < kNumVoices)
                {
                    const auto& vref = voices_[g.voiceIndex];
                    const float dens = juce::jmax (1.0f, vref.effectiveDensity);
                    voiceGain = vref.envLevel * vref.velocityGain / std::sqrt (dens);
                }

                mix += samp * window * voiceGain;

                g.phase += g.reverse ? -g.speed : g.speed;
                if (--g.lifetime <= 0)
                    g.active = false;
            }

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.getWritePointer (ch)[i] = mix;
        }

        // Grain snapshot for UI
        {
            const juce::SpinLock::ScopedLockType lock (grainSnapshotLock_);
            int count = 0;
            for (int gi = 0; gi < kMaxGrains && count < kMaxGrains; ++gi)
            {
                if (grainPool_[gi].active)
                {
                    grainSnapshot_[count].position = static_cast<float> (grainPool_[gi].phase / totalLen);
                    grainSnapshot_[count].active   = true;
                    ++count;
                }
            }
            grainSnapshotCount_ = count;
            grainSnapshotReady_ = true;
        }
    }

    // ── Spectral freeze / drift + smear crossfade ────────────────────────
    // Dual-path architecture:
    //   1. Grain engine output (already in buffer above)
    //   2. Spectral resynthesis from last frozen frame
    // Smear crossfades between them. Freeze toggle controls capture only.
    {
        const float twoPi = juce::MathConstants<float>::twoPi;
        float* outL = buffer.getWritePointer (0);

        // Detect freeze on-edge → capture spectral frame
        const bool freezeEdge = freezeOn && ! wasFrozen_;

        // Feed grain output into FFT input buffer for analysis (only when not frozen,
        // so we always have a recent frame ready when freeze is triggered)
        if (! freezeOn)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                fftInputBuffer_[fftInputWritePos_] = outL[i];
                fftInputWritePos_ = (fftInputWritePos_ + 1) % kFFTSize;
            }
        }

        if (freezeEdge)
        {
            // Capture current spectral frame into frozen storage
            std::copy (fftInputBuffer_.begin(), fftInputBuffer_.end(),
                       fftWorkBuffer_.begin());
            for (int i = 0; i < kFFTSize; ++i)
                fftWorkBuffer_[i] *= hannWindow_[i];

            fft_.performRealOnlyForwardTransform (fftWorkBuffer_.data(), true);

            for (int bin = 0; bin <= kFFTSize / 2; ++bin)
            {
                const float re = fftWorkBuffer_[bin * 2];
                const float im = fftWorkBuffer_[bin * 2 + 1];
                frozenMagnitude_[bin] = std::sqrt (re * re + im * im);
                frozenPhase_[bin]     = std::atan2 (im, re);
            }

            driftVelocity_.fill (0.0f);
            olaBuffer_.fill (0.0f);
            olaReadPos_  = 0;
            olaWritePos_ = 0;
            freezeHopCounter_ = 0;
            hasFrozenFrame_ = true;
        }

        // Run spectral resynthesis when needed:
        // - Always when freeze is on (full spectral output)
        // - When smear > 0 and we have a frozen frame (blend with grain)
        const bool needSpectral = hasFrozenFrame_ && (freezeOn || smear > 0.0f);

        if (needSpectral)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                if (freezeHopCounter_ <= 0)
                {
                    freezeHopCounter_ = kHopSize;

                    fftWorkBuffer_.fill (0.0f);
                    for (int bin = 0; bin <= kFFTSize / 2; ++bin)
                    {
                        const float baseAdvance = twoPi * static_cast<float> (bin)
                                                * static_cast<float> (kHopSize)
                                                / static_cast<float> (kFFTSize);

                        if (drift > 0.0f)
                        {
                            const float rndNorm = nextDriftRandom() * 2.0f - 1.0f;
                            driftVelocity_[bin] += (rndNorm - driftVelocity_[bin]) * drift * 0.1f;
                            frozenPhase_[bin] += baseAdvance
                                               + drift * driftVelocity_[bin] * juce::MathConstants<float>::pi;
                        }
                        else
                        {
                            frozenPhase_[bin] += baseAdvance;
                        }

                        const float mag   = frozenMagnitude_[bin];
                        const float phase = frozenPhase_[bin];
                        fftWorkBuffer_[bin * 2]     = mag * std::cos (phase);
                        fftWorkBuffer_[bin * 2 + 1] = mag * std::sin (phase);
                    }

                    fft_.performRealOnlyInverseTransform (fftWorkBuffer_.data());

                    for (int j = 0; j < kFFTSize; ++j)
                    {
                        const int pos = (olaWritePos_ + j) % kOLABufferSize;
                        olaBuffer_[pos] += fftWorkBuffer_[j] * hannWindow_[j];
                    }
                    olaWritePos_ = (olaWritePos_ + kHopSize) % kOLABufferSize;
                }

                const float spectralSample = olaBuffer_[olaReadPos_] * (2.0f / 3.0f);
                olaBuffer_[olaReadPos_] = 0.0f;
                olaReadPos_ = (olaReadPos_ + 1) % kOLABufferSize;
                freezeHopCounter_--;

                // Determine effective smear: when freeze is on, force full spectral
                const float effectiveSmear = freezeOn ? 1.0f : smear;

                // Crossfade: grain (in buffer) vs spectral
                const float grainSample = outL[i];
                const float mixed = (1.0f - effectiveSmear) * grainSample
                                  + effectiveSmear * spectralSample;

                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    buffer.getWritePointer (ch)[i] = mixed;
            }
        }
        // else: smear=0 and not frozen — grain output stays as-is (no spectral cost)

        wasFrozen_ = freezeOn;
    }

    // ── Release fade (sampler mode only — synth uses voice envelopes) ─────
    if (! synthMode && releaseFadeActive_)
    {
        const int numCh = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            if (releaseFadeSamplesRemaining_ <= 0)
            {
                for (int j = i; j < numSamples; ++j)
                    for (int ch = 0; ch < numCh; ++ch)
                        buffer.getWritePointer (ch)[j] = 0.0f;

                playbackActive_.store (false);
                playbackProgress_.store (0.0f);
                for (auto& g : grainPool_)
                    g.active = false;
                grainSpawnAccum_ = 0.0;
                releaseFadeActive_ = false;
                triggeringNote_    = -1;
                break;
            }

            const float gain = static_cast<float> (releaseFadeSamplesRemaining_)
                             / static_cast<float> (releaseFadeTotal_);
            for (int ch = 0; ch < numCh; ++ch)
                buffer.getWritePointer (ch)[i] *= gain;
            --releaseFadeSamplesRemaining_;
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
    releaseFadeActive_ = false;
    triggeringNote_    = -1;
}

float WiredMemoryAudioProcessor::getPlaybackProgress() const
{
    if (! playbackActive_.load())
        return 0.0f;
    return playbackProgress_.load();
}

float WiredMemoryAudioProcessor::getSampleDuration() const
{
    const int len = sampleLength_.load();
    if (len <= 0 || currentSampleRate_ <= 0.0)
        return 0.0f;
    return static_cast<float> (len) / static_cast<float> (currentSampleRate_);
}

bool WiredMemoryAudioProcessor::readGrainSnapshot (GrainSnapshot* dest, int& count)
{
    const juce::SpinLock::ScopedLockType lock (grainSnapshotLock_);
    if (! grainSnapshotReady_)
        return false;
    count = grainSnapshotCount_;
    std::memcpy (dest, grainSnapshot_.data(), sizeof (GrainSnapshot) * static_cast<size_t> (count));
    grainSnapshotReady_ = false;
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
