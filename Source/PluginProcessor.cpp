#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "capture/SCKAudioCapture.h"

WiredMemoryAudioProcessor::WiredMemoryAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    formatManager_.registerBasicFormats();
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

    // Amplitude ADSR envelope, applied to the entire playback voice. Times in
    // seconds with a skew that gives more resolution at low values.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "env_attack", 1 },
        "Env Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.5f),
        0.005f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "env_decay", 1 },
        "Env Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.5f),
        0.001f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "env_sustain", 1 },
        "Env Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "env_release", 1 },
        "Env Release",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.5f),
        0.05f));

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
        juce::ParameterID { "glide", 1 },
        "Glide",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "fine_tune", 1 },
        "Fine Tune",
        juce::NormalisableRange<float> (-100.0f, 100.0f),
        0.0f));

    // Make-up gain applied to the recorded sample during playback.
    // Range 0–4 with skew 0.5 puts unity gain (1.0) at the knob centre.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sample_gain", 1 },
        "Sample Gain",
        juce::NormalisableRange<float> (0.0f, 4.0f, 0.0f, 0.5f),
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

    // Apply any sample restore that arrived before prepareToPlay
    if (! pendingRestoreSamples_.empty())
    {
        const int copyLen = juce::jmin (static_cast<int> (pendingRestoreSamples_.size()),
                                        recordBufferCapacity_);
        std::memcpy (recordBuffer_.get(), pendingRestoreSamples_.data(),
                     sizeof (float) * static_cast<size_t> (copyLen));
        recordBufferPos_ = copyLen;
        sampleLength_.store (copyLen);
        snapshotRebuildPending_.store (true);
        pendingRestoreSamples_.clear();
    }

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

    triggeringNote_   = -1;
    samplerEnvState_  = EnvState::Idle;
    samplerEnvLevel_  = 0.0f;
    stopRequested_.store (false);

    // Reset polyphonic voices
    for (auto& v : voices_)
        v = Voice {};
    voiceAllocCounter_ = 0;
    wasSynthMode_ = false;
}
void WiredMemoryAudioProcessor::releaseResources() {}

bool WiredMemoryAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono()
        || out == juce::AudioChannelSet::stereo();
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
    const float envAttackSec  = apvts.getRawParameterValue ("env_attack")->load();
    const float envDecaySec   = apvts.getRawParameterValue ("env_decay")->load();
    const float envSustain    = apvts.getRawParameterValue ("env_sustain")->load();
    const float envReleaseSec = apvts.getRawParameterValue ("env_release")->load();
    const bool  freezeOn     = apvts.getRawParameterValue ("freeze")->load() >= 0.5f;
    const float drift        = apvts.getRawParameterValue ("drift")->load();
    const float smear        = apvts.getRawParameterValue ("smear")->load();
    const bool  speedLockPitch = apvts.getRawParameterValue ("speed_lock_pitch")->load() >= 0.5f;
    const bool  synthMode    = apvts.getRawParameterValue ("synth_mode")->load() >= 0.5f;
    const bool  gateMode     = apvts.getRawParameterValue ("trigger_mode")->load() >= 0.5f;
    const float rootNote     = apvts.getRawParameterValue ("root_note")->load();
    const float densityTrack = apvts.getRawParameterValue ("density_track")->load();
    const float velocitySens = apvts.getRawParameterValue ("velocity_sens")->load();
    const float glide        = apvts.getRawParameterValue ("glide")->load();
    const float fineTune     = apvts.getRawParameterValue ("fine_tune")->load();
    const float sampleGain   = apvts.getRawParameterValue ("sample_gain")->load();

    // ── Synth-mode MIDI modulation (Ticket 4) ────────────────────────────
    // CC overrides are sticky once received (sentinel -1.0f = not yet received).
    // They only take effect in synth mode; sampler mode always reads the raw parameter.
    const float ccScatterVal      = ccScatter_.load();
    const float ccSmearVal        = ccSmear_.load();
    const float ccDensityTrackVal = ccDensityTrack_.load();
    const float effScatter      = (synthMode && ccScatterVal      >= 0.0f) ? ccScatterVal      : scatter;
    const float effSmear        = (synthMode && ccSmearVal        >= 0.0f) ? ccSmearVal        : smear;
    const float effDensityTrack = (synthMode && ccDensityTrackVal >= 0.0f) ? ccDensityTrackVal : densityTrack;

    // Pitch bend: hardcoded ±2 semitones, synth-mode only.
    const float pitchBendSemis = synthMode ? pitchBendSemitones_.load() : 0.0f;
    const double bendMul       = std::exp2 (static_cast<double> (pitchBendSemis) / 12.0);

    // ── File load (message thread → audio thread) ───────────────────────
    // Hold off until captureOn is false, otherwise the capture state
    // machine below would zero recordBufferPos_ and overwrite our buffer.
    if (loadPending_.load() && recordBuffer_ && ! captureOn)
    {
        const juce::SpinLock::ScopedTryLockType lock (pendingLoadLock_);
        if (lock.isLocked())
        {
            const int newLen = juce::jmin (static_cast<int> (pendingLoadBuffer_.size()),
                                           recordBufferCapacity_);
            if (newLen > 0)
            {
                std::memcpy (recordBuffer_.get(), pendingLoadBuffer_.data(),
                             sizeof (float) * static_cast<size_t> (newLen));
                if (recordBufferCapacity_ - newLen > 0)
                    std::memset (recordBuffer_.get() + newLen, 0,
                                 sizeof (float) * static_cast<size_t> (recordBufferCapacity_ - newLen));

                sampleLength_.store (newLen);
                recordBufferPos_ = newLen;
                wasCapturing_ = false;

                // Stop transport / reset all engines so playheads don't reference stale offsets
                playbackActive_ .store (false);
                playbackPending_.store (false);
                playbackProgress_.store (0.0f);
                playbackPosFrac_ = 0.0;
                for (auto& g : grainPool_)
                    g.active = false;
                grainSpawnAccum_ = 0.0;
                for (auto& v : voices_)
                    v = Voice {};
                samplerEnvState_   = EnvState::Idle;
                samplerEnvLevel_   = 0.0f;
                triggeringNote_    = -1;

                snapshotRebuildPending_.store (true);
            }
            pendingLoadBuffer_.clear();
            loadPending_.store (false);
        }
    }

    // ── Trim request (message thread → audio thread) ─────────────────────
    if (trimPending_.exchange (false) && recordBuffer_)
    {
        const int total = sampleLength_.load();
        if (total > 0)
        {
            const float ts = trimStartNorm_.load();
            const float tl = trimLenNorm_.load();
            const int s = juce::jlimit (0, total, static_cast<int> (ts * total));
            const int e = juce::jlimit (s, total, s + static_cast<int> (tl * total));
            const int newLen = e - s;
            if (newLen > 0)
            {
                if (s > 0)
                    std::memmove (recordBuffer_.get(),
                                  recordBuffer_.get() + s,
                                  sizeof (float) * static_cast<size_t> (newLen));
                std::memset (recordBuffer_.get() + newLen, 0,
                             sizeof (float) * static_cast<size_t> (recordBufferCapacity_ - newLen));
                sampleLength_.store (newLen);
                recordBufferPos_ = newLen;

                // Stop playback so the playhead doesn't reference stale offsets
                playbackActive_.store (false);
                playbackPending_.store (false);
                playbackProgress_.store (0.0f);
                playbackPosFrac_ = 0.0;
                for (auto& g : grainPool_)
                    g.active = false;
                grainSpawnAccum_ = 0.0;
                for (auto& v : voices_)
                    v = Voice {};

                snapshotRebuildPending_.store (true);
            }
        }
    }

    // ── Sample-snapshot rebuild (after restore or trim) ──────────────────
    if (snapshotRebuildPending_.exchange (false) && recordBuffer_)
    {
        const int srcLen = sampleLength_.load();
        if (srcLen > 0)
        {
            const juce::SpinLock::ScopedLockType lock (sampleLock_);
            const float* src = recordBuffer_.get();
            for (int i = 0; i < kSampleSnapshotSize; ++i)
            {
                const int s = i * srcLen / kSampleSnapshotSize;
                const int e = juce::jmin ((i + 1) * srcLen / kSampleSnapshotSize, srcLen);
                float peak = 0.0f;
                for (int j = s; j < e; ++j)
                    if (std::abs (src[j]) > std::abs (peak))
                        peak = src[j];
                sampleSnapshot_[i] = peak;
            }
            sampleReady_ = true;
        }
    }

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
        triggeringNote_   = -1;
        samplerEnvState_  = EnvState::Idle;
        samplerEnvLevel_  = 0.0f;
        pitchBendSemitones_.store (0.0f);
        sustainPedalDown_ = false;
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
                }
            }
            else if (msg.isNoteOff() && gateMode)
            {
                const int note = msg.getNoteNumber();
                const bool willPlay = playbackActive_.load() || playbackPending_.load();
                if (willPlay && note == triggeringNote_
                    && samplerEnvState_ != EnvState::Release
                    && samplerEnvState_ != EnvState::Idle)
                {
                    samplerEnvState_ = EnvState::Release;
                }
            }
            else if (msg.isAllNotesOff() && gateMode)
            {
                if ((playbackActive_.load() || playbackPending_.load())
                    && samplerEnvState_ != EnvState::Release
                    && samplerEnvState_ != EnvState::Idle)
                {
                    samplerEnvState_ = EnvState::Release;
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

                // If a sustain-held voice is already sounding this note, release
                // it before allocating a fresh slot (prevents stacking duplicates
                // of the same pitch when the pedal is down).
                for (auto& voice : voices_)
                {
                    if (voice.sustainHeld
                        && voice.midiNote == note
                        && voice.envState != EnvState::Idle)
                    {
                        voice.sustainHeld = false;
                        voice.envState    = EnvState::Release;
                    }
                }

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
                voice.effectiveDensity = (1.0f - effDensityTrack) * density
                                       + effDensityTrack * static_cast<float> (targetDensity);
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
                    if (voice.midiNote == note && voice.envState != EnvState::Idle
                        && voice.envState != EnvState::Release
                        && ! voice.sustainHeld)
                    {
                        if (sustainPedalDown_)
                            voice.sustainHeld = true;   // defer release until pedal up
                        else
                            voice.envState = EnvState::Release;
                    }
                }
            }
            else if (msg.isAllNotesOff())
            {
                for (auto& voice : voices_)
                {
                    if (voice.envState != EnvState::Idle)
                    {
                        voice.sustainHeld = false;
                        voice.envState    = EnvState::Release;
                    }
                }
            }
            else if (msg.isPitchWheel())
            {
                const int bendValue = msg.getPitchWheelValue();      // 0..16383
                const float semis = (static_cast<float> (bendValue - 8192) / 8192.0f) * 2.0f;
                pitchBendSemitones_.store (semis);
            }
            else if (msg.isController())
            {
                const int cc   = msg.getControllerNumber();
                const int val  = msg.getControllerValue();
                const float nv = static_cast<float> (val) / 127.0f;

                switch (cc)
                {
                    case 1:  ccScatter_.store      (nv); break;
                    case 11: ccSmear_.store        (nv); break;
                    case 74: ccDensityTrack_.store (nv); break;
                    case 64:
                    {
                        const bool down = val >= 64;
                        if (! down && sustainPedalDown_)
                        {
                            // Pedal released — release every sustain-held voice.
                            for (auto& voice : voices_)
                            {
                                if (voice.sustainHeld
                                    && voice.envState != EnvState::Idle
                                    && voice.envState != EnvState::Release)
                                {
                                    voice.sustainHeld = false;
                                    voice.envState    = EnvState::Release;
                                }
                            }
                        }
                        sustainPedalDown_ = down;
                        break;
                    }
                    default: break;
                }
            }
        }
    }

    // ── Voice ADSR coefficients (per-block, used by sampler+synth engines) ──
    const float sr           = static_cast<float> (currentSampleRate_);
    const float envAttackInc = 1.0f / juce::jmax (1.0f, envAttackSec  * sr);
    const float envDecayInc  = 1.0f / juce::jmax (1.0f, envDecaySec   * sr);
    const float envReleaseDec = std::exp (-1.0f / juce::jmax (1.0f, envReleaseSec * sr));

    // Honour any stop request from the message thread by triggering envelope Release.
    if (stopRequested_.exchange (false))
    {
        playbackPending_.store (false);
        if (! synthMode
            && samplerEnvState_ != EnvState::Release
            && samplerEnvState_ != EnvState::Idle)
        {
            samplerEnvState_ = EnvState::Release;
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

        // Trigger global ADSR envelope
        samplerEnvState_ = EnvState::Attack;
        samplerEnvLevel_ = 0.0f;

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
                            if (samplerEnvState_ != EnvState::Release
                                && samplerEnvState_ != EnvState::Idle)
                                samplerEnvState_ = EnvState::Release;
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
                            if (samplerEnvState_ != EnvState::Release
                                && samplerEnvState_ != EnvState::Idle)
                                samplerEnvState_ = EnvState::Release;
                            break;
                        }
                    }
                }

                const int idx0 = juce::jlimit (0, totalLen - 1, static_cast<int> (playbackPosFrac_));
                const int idx1 = juce::jmin (idx0 + 1, totalLen - 1);
                const float frac = static_cast<float> (playbackPosFrac_ - idx0);
                const float samp = (recordBuffer_[idx0] * (1.0f - frac)
                                  + recordBuffer_[idx1] * frac) * sampleGain;

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
                        if (samplerEnvState_ != EnvState::Release
                            && samplerEnvState_ != EnvState::Idle)
                            samplerEnvState_ = EnvState::Release;
                        bool anyActive = false;
                        for (const auto& g : grainPool_)
                            if (g.active) { anyActive = true; break; }
                        if (! anyActive)
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
                        if (samplerEnvState_ != EnvState::Release
                            && samplerEnvState_ != EnvState::Idle)
                            samplerEnvState_ = EnvState::Release;
                        bool anyActive = false;
                        for (const auto& g : grainPool_)
                            if (g.active) { anyActive = true; break; }
                        if (! anyActive)
                            break;
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

            // Write to output with gain compensation + user make-up gain
            mix *= gainComp * sampleGain;
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
        const float glideAlpha  = (glide > 0.0f)
            ? 1.0f - std::exp (-1.0f / juce::jmax (1.0f, glide * static_cast<float> (currentSampleRate_)))
            : 1.0f;

        // Per-block: recompute each active voice's effective density + grain size
        // so that mid-note changes to the grain_size parameter, density_track CC,
        // or pitch bend immediately track. Matches the formula used at note-on but
        // with the bent note substituted for midiNote.
        for (auto& voice : voices_)
        {
            if (voice.envState == EnvState::Idle || voice.midiNote < 0)
                continue;
            const double bentNote = static_cast<double> (voice.midiNote) + pitchBendSemis;
            const double targetFreq = 440.0 * std::exp2 ((bentNote - 69.0) / 12.0);
            const double freqGrainProduct = targetFreq * static_cast<double> (grainSizeSec);
            const double targetDensity = juce::jlimit (1.0, 32.0, freqGrainProduct);
            voice.effectiveDensity = (1.0f - effDensityTrack) * density
                                   + effDensityTrack * static_cast<float> (targetDensity);
            voice.effectiveDensity = juce::jlimit (1.0f, 32.0f, voice.effectiveDensity);

            if (freqGrainProduct > 32.0)
                voice.effectiveGrainSize = static_cast<float> (
                    std::min (static_cast<double> (grainSizeSec), 32.0 / targetFreq));
            else
                voice.effectiveGrainSize = grainSizeSec;
        }

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

                // ADSR envelope
                switch (voice.envState)
                {
                    case EnvState::Attack:
                        voice.envLevel += (1.0f - voice.envLevel) * envAttackInc;
                        if (voice.envLevel >= 0.999f)
                        {
                            voice.envLevel = 1.0f;
                            voice.envState = EnvState::Decay;
                        }
                        break;
                    case EnvState::Decay:
                        voice.envLevel -= envDecayInc;
                        if (voice.envLevel <= envSustain)
                        {
                            voice.envLevel = envSustain;
                            voice.envState = EnvState::Sustain;
                        }
                        break;
                    case EnvState::Sustain:
                        voice.envLevel = envSustain;
                        break;
                    case EnvState::Release:
                        voice.envLevel *= envReleaseDec;
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
                        if (effScatter > 0.0f)
                        {
                            const float rnd = nextRandom() - 0.5f;
                            grainStart += static_cast<double> (rnd * effScatter) * regionLen;
                            grainStart = juce::jlimit (static_cast<double> (startFrame),
                                                       static_cast<double> (endFrame - 1),
                                                       grainStart);
                        }
                        slot->startPos = grainStart;
                        slot->phase    = grainStart;

                        double grainSpeed = speedLockPitch ? speedAbs : 1.0;
                        grainSpeed *= voice.currentPitchMul * bendMul;
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
                const float window = 0.5f * (1.0f - std::cos (6.283185307f * envPhase));

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

            mix *= sampleGain;
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
        const bool needSpectral = hasFrozenFrame_ && (freezeOn || effSmear > 0.0f);

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
                const float effectiveSmear = freezeOn ? 1.0f : effSmear;

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

    // ── Sampler-mode global ADSR envelope, applied to every output sample ──
    // Runs whenever the envelope is non-Idle so a Release tail keeps audible
    // even after grain audio has been cut.
    if (! synthMode && samplerEnvState_ != EnvState::Idle)
    {
        const int numCh = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            switch (samplerEnvState_)
            {
                case EnvState::Attack:
                    samplerEnvLevel_ += (1.0f - samplerEnvLevel_) * envAttackInc;
                    if (samplerEnvLevel_ >= 0.999f)
                    {
                        samplerEnvLevel_ = 1.0f;
                        samplerEnvState_ = EnvState::Decay;
                    }
                    break;
                case EnvState::Decay:
                    samplerEnvLevel_ -= envDecayInc;
                    if (samplerEnvLevel_ <= envSustain)
                    {
                        samplerEnvLevel_ = envSustain;
                        samplerEnvState_ = EnvState::Sustain;
                    }
                    break;
                case EnvState::Sustain:
                    samplerEnvLevel_ = envSustain;
                    break;
                case EnvState::Release:
                    samplerEnvLevel_ *= envReleaseDec;
                    if (samplerEnvLevel_ < 0.001f)
                    {
                        samplerEnvLevel_ = 0.0f;
                        samplerEnvState_ = EnvState::Idle;
                        playbackActive_.store (false);
                        playbackProgress_.store (0.0f);
                        for (auto& g : grainPool_)
                            g.active = false;
                        grainSpawnAccum_ = 0.0;
                        triggeringNote_    = -1;
                        for (int j = i; j < numSamples; ++j)
                            for (int ch = 0; ch < numCh; ++ch)
                                buffer.getWritePointer (ch)[j] = 0.0f;
                        break;
                    }
                    break;
                case EnvState::Idle:
                default:
                    break;
            }

            for (int ch = 0; ch < numCh; ++ch)
                buffer.getWritePointer (ch)[i] *= samplerEnvLevel_;

            if (samplerEnvState_ == EnvState::Idle)
                break;
        }
    }
    else if (! synthMode && samplerEnvState_ == EnvState::Idle)
    {
        // Envelope idle — silence any residual grain audio (no playback voice)
        buffer.clear();
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
    // Audio thread converts this to an envelope Release transition so
    // stop yields a smooth fade rather than a click.
    stopRequested_.store (true);
}

void WiredMemoryAudioProcessor::requestTrim (float startNorm, float lenNorm)
{
    trimStartNorm_.store (juce::jlimit (0.0f, 1.0f, startNorm));
    trimLenNorm_  .store (juce::jlimit (0.0f, 1.0f, lenNorm));
    trimPending_  .store (true);
}

bool WiredMemoryAudioProcessor::loadSampleFromBytes (const void* data, size_t numBytes)
{
    if (data == nullptr || numBytes == 0)
        return false;

    auto stream = std::make_unique<juce::MemoryInputStream> (data, numBytes, false);
    std::unique_ptr<juce::AudioFormatReader> reader (
        formatManager_.createReaderFor (std::move (stream)));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;

    const double srcRate = reader->sampleRate > 0.0 ? reader->sampleRate : currentSampleRate_;
    const double dstRate = currentSampleRate_ > 0.0 ? currentSampleRate_ : srcRate;
    const int    srcCh   = juce::jmax (1, (int) reader->numChannels);
    const auto   srcLen  = (juce::int64) reader->lengthInSamples;

    // Decode into a temporary multichannel buffer
    juce::AudioBuffer<float> srcBuf (srcCh, (int) juce::jmin (srcLen,
                                            (juce::int64) (kMaxRecordSeconds * srcRate * 2 + srcRate)));
    if (! reader->read (&srcBuf, 0, srcBuf.getNumSamples(), 0, true, srcCh > 1))
        return false;

    // Mono mix-down
    const int srcFrames = srcBuf.getNumSamples();
    std::vector<float> mono (static_cast<size_t> (srcFrames));
    if (srcCh == 1)
    {
        std::memcpy (mono.data(), srcBuf.getReadPointer (0), sizeof (float) * static_cast<size_t> (srcFrames));
    }
    else
    {
        const float invCh = 1.0f / static_cast<float> (srcCh);
        for (int i = 0; i < srcFrames; ++i)
        {
            float sum = 0.0f;
            for (int c = 0; c < srcCh; ++c)
                sum += srcBuf.getReadPointer (c)[i];
            mono[(size_t) i] = sum * invCh;
        }
    }

    // Resample to host sample rate (mono LagrangeInterpolator)
    std::vector<float> resampled;
    const float ratio = static_cast<float> (srcRate / dstRate);
    if (std::abs (ratio - 1.0f) < 1.0e-4f)
    {
        resampled = std::move (mono);
    }
    else
    {
        const int outFrames = static_cast<int> (std::ceil (srcFrames / ratio));
        resampled.assign ((size_t) outFrames, 0.0f);
        juce::LagrangeInterpolator interp;
        interp.process (ratio, mono.data(), resampled.data(), outFrames,
                        srcFrames, 0);
    }

    // Truncate to capacity (kMaxRecordSeconds at host rate)
    const int capFrames = static_cast<int> (dstRate * kMaxRecordSeconds);
    if (capFrames > 0 && (int) resampled.size() > capFrames)
        resampled.resize ((size_t) capFrames);

    if (resampled.empty())
        return false;

    {
        const juce::SpinLock::ScopedLockType lock (pendingLoadLock_);
        pendingLoadBuffer_ = std::move (resampled);
    }
    loadPending_.store (true);
    return true;
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

    // Persist the recorded sample buffer
    const int len = sampleLength_.load();
    if (recordBuffer_ && len > 0)
    {
        juce::ValueTree sampleState ("RecordedSample");
        sampleState.setProperty ("length", len, nullptr);
        sampleState.setProperty ("sampleRate", currentSampleRate_, nullptr);
        juce::MemoryBlock mb (recordBuffer_.get(), sizeof (float) * static_cast<size_t> (len));
        sampleState.setProperty ("samples", mb, nullptr);
        state.addChild (sampleState, -1, nullptr);
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

        // Restore recorded sample
        auto sampleState = tree.getChildWithName ("RecordedSample");
        if (sampleState.isValid())
        {
            const int len = static_cast<int> (sampleState.getProperty ("length", 0));
            const auto* mb = sampleState.getProperty ("samples").getBinaryData();
            if (len > 0 && mb != nullptr
                && mb->getSize() >= sizeof (float) * static_cast<size_t> (len))
            {
                pendingRestoreSamples_.assign (len, 0.0f);
                std::memcpy (pendingRestoreSamples_.data(), mb->getData(),
                             sizeof (float) * static_cast<size_t> (len));

                // If prepareToPlay has already run, apply immediately.
                if (recordBuffer_ && recordBufferCapacity_ > 0)
                {
                    const int copyLen = juce::jmin (len, recordBufferCapacity_);
                    std::memcpy (recordBuffer_.get(), pendingRestoreSamples_.data(),
                                 sizeof (float) * static_cast<size_t> (copyLen));
                    recordBufferPos_ = copyLen;
                    sampleLength_.store (copyLen);
                    snapshotRebuildPending_.store (true);
                    pendingRestoreSamples_.clear();
                }
            }
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WiredMemoryAudioProcessor();
}
