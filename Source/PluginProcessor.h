#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <memory>

class SCKAudioCapture;

// ── Grain state ──────────────────────────────────────────────────────────────
struct Grain
{
    double startPos   = 0.0;   // start position in record buffer (samples)
    double speed      = 1.0;   // playback speed (inherited from master)
    double phase      = 0.0;   // current fractional sample position
    int    lifetime   = 0;     // remaining samples before deactivation
    int    totalLife  = 0;     // original lifetime (for envelope calc)
    bool   active     = false;
    bool   reverse    = false; // grain plays in reverse
    int    voiceIndex = -1;    // owning voice in synth mode (-1 = sampler)
};

static constexpr int kMaxGrains = 64;
static constexpr int kNumVoices = 6;

// ── Polyphonic voice state (synth mode) ──────────────────────────────────────
enum class EnvState { Idle, Attack, Sustain, Release };

struct Voice
{
    int      midiNote          = -1;     // -1 = idle slot
    double   pitchMul          = 1.0;    // target pitch multiplier
    double   currentPitchMul   = 1.0;    // current pitch (for glide)
    double   playbackPos       = 0.0;    // independent playhead in record buffer
    double   grainSpawnAccum   = 0.0;
    float    velocityGain      = 1.0f;
    float    effectiveDensity  = 1.0f;
    float    effectiveGrainSize = 0.1f;  // seconds
    EnvState envState          = EnvState::Idle;
    float    envLevel          = 0.0f;
    bool     sustainHeld       = false;  // for sustain pedal logic (Ticket 4)
    uint64_t allocOrder        = 0;      // for voice-stealing oldest-first
};

class WiredMemoryAudioProcessor : public juce::AudioProcessor
{
public:
    WiredMemoryAudioProcessor();
    ~WiredMemoryAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parameter tree — add parameters in createParameterLayout().
    juce::AudioProcessorValueTreeState apvts;

    /** Non-owning access to the capture engine (for the editor). */
    SCKAudioCapture* getCapture() noexcept { return capture_.get(); }

    /** Waveform snapshot for UI visualization.
        Written by the audio thread in processBlock, read by the editor timer. */
    static constexpr int kWaveformSnapshotSize = 128;
    bool readWaveformSnapshot (float* dest);

    /** Captured sample snapshot — peak-envelope downsampled for UI display.
        Written by the audio thread when capture stops, read by editor timer. */
    static constexpr int kSampleSnapshotSize = 512;
    bool readSampleSnapshot (float* dest);

    /** Start/stop sample playback from the recorded buffer. */
    void startPlayback();
    void stopPlayback();

    /** Returns normalised playback progress [0, 1], or 0 if not playing. */
    float getPlaybackProgress() const;

    /** Returns the sample duration in seconds, or 0 if no sample is loaded. */
    float getSampleDuration() const;

    /** Grain snapshot for UI visualisation.
        Each entry holds a normalised position within the full sample [0, 1].
        Written by the audio thread, read by the editor timer. */
    struct GrainSnapshot
    {
        float position = 0.0f;  // normalised position in sample [0, 1]
        bool  active   = false;
    };
    static constexpr int kGrainSnapshotSize = kMaxGrains;
    bool readGrainSnapshot (GrainSnapshot* dest, int& count);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::unique_ptr<SCKAudioCapture> capture_;

    // -- Live waveform snapshot (128 samples, ~30fps) --
    juce::SpinLock waveformLock_;
    std::array<float, kWaveformSnapshotSize> waveformSnapshot_ {};
    bool waveformReady_ = false;

    // -- Sample accumulation (audio thread only during recording) --
    static constexpr int kMaxRecordSeconds = 30;
    std::unique_ptr<float[]> recordBuffer_;   // mono, pre-allocated
    int recordBufferCapacity_ = 0;
    int recordBufferPos_      = 0;
    bool wasCapturing_        = false;

    // -- Sample playback (grain engine) --
    std::atomic<bool>  playbackActive_  { false };
    std::atomic<bool>  playbackPending_ { false };  // message thread signals start
    double             playbackPosFrac_ = 0.0;      // master playhead (audio thread only)
    std::atomic<float> playbackProgress_ { 0.0f };  // normalised progress for UI
    std::atomic<int>   sampleLength_    { 0 };       // length of recorded sample in frames

    // -- MIDI gate / release fade (sampler mode) --
    int  triggeringNote_              = -1;
    bool releaseFadeActive_           = false;
    int  releaseFadeSamplesRemaining_ = 0;
    int  releaseFadeTotal_            = 0;

    // -- Grain pool --
    std::array<Grain, kMaxGrains> grainPool_ {};
    double grainSpawnAccum_ = 0.0;  // accumulator for grain spawn timing
    double currentSampleRate_ = 44100.0;

    // -- Polyphonic voices (synth mode) --
    std::array<Voice, kNumVoices> voices_ {};
    uint64_t voiceAllocCounter_ = 0;
    bool wasSynthMode_ = false;

    // -- MIDI modulation state (synth mode only; Ticket 4) --
    std::atomic<float> pitchBendSemitones_ { 0.0f };

    // CC overrides — sentinel -1.0f means "no CC received yet, use parameter".
    // Sticky once received: the CC override replaces the host parameter for the
    // lifetime of the plugin instance (no decay back to parameter control).
    std::atomic<float> ccScatter_       { -1.0f };
    std::atomic<float> ccSmear_         { -1.0f };
    std::atomic<float> ccDensityTrack_  { -1.0f };

    // CC64 sustain pedal — touched only on the audio thread.
    bool sustainPedalDown_ = false;

    // -- Grain snapshot for UI --
    juce::SpinLock grainSnapshotLock_;
    std::array<GrainSnapshot, kMaxGrains> grainSnapshot_ {};
    int grainSnapshotCount_ = 0;
    bool grainSnapshotReady_ = false;

    // -- Scatter RNG (xorshift32, no heap) --
    uint32_t rngState_ = 0x12345678;
    float nextRandom()
    {
        rngState_ ^= rngState_ << 13;
        rngState_ ^= rngState_ >> 17;
        rngState_ ^= rngState_ << 5;
        return static_cast<float> (rngState_) / static_cast<float> (0xFFFFFFFFu);
    }

    // -- Captured sample snapshot (read by editor timer) --
    juce::SpinLock sampleLock_;
    std::array<float, kSampleSnapshotSize> sampleSnapshot_ {};
    bool sampleReady_ = false;

    // -- Spectral freeze / drift (FFT) --
    static constexpr int kFFTOrder = 11;
    static constexpr int kFFTSize  = 1 << kFFTOrder;   // 2048
    static constexpr int kHopSize  = kFFTSize / 4;     // 512 (75% overlap)

    juce::dsp::FFT fft_ { kFFTOrder };

    // Pre-computed Hann analysis/synthesis window (2048 samples)
    std::array<float, kFFTSize> hannWindow_ {};

    // Circular input buffer: accumulates output samples for FFT analysis
    std::array<float, kFFTSize> fftInputBuffer_ {};
    int fftInputWritePos_ = 0;

    // Overlap-add output buffer for resynthesis (2x FFT size for safe overlap)
    static constexpr int kOLABufferSize = kFFTSize * 2;
    std::array<float, kOLABufferSize> olaBuffer_ {};
    int olaReadPos_  = 0;
    int olaWritePos_ = 0;

    // Current analysis frame: magnitude and phase from most recent FFT
    std::array<float, kFFTSize> currentMagnitude_ {};
    std::array<float, kFFTSize> currentPhase_ {};

    // Frozen frame: captured magnitude and running synthesis phase
    std::array<float, kFFTSize> frozenMagnitude_ {};
    std::array<float, kFFTSize> frozenPhase_ {};

    // Per-bin drift velocities (smoothly varying random offsets)
    std::array<float, kFFTSize> driftVelocity_ {};

    // Freeze state
    bool wasFrozen_ = false;
    bool hasFrozenFrame_ = false;  // true once at least one spectral frame has been captured
    int  freezeHopCounter_ = 0;    // counts samples until next resynthesis hop

    // Drift RNG (separate from scatter RNG to avoid correlation)
    uint32_t driftRngState_ = 0xDEADBEEF;
    float nextDriftRandom()
    {
        driftRngState_ ^= driftRngState_ << 13;
        driftRngState_ ^= driftRngState_ >> 17;
        driftRngState_ ^= driftRngState_ << 5;
        return static_cast<float> (driftRngState_) / static_cast<float> (0xFFFFFFFFu);
    }

    // FFT work buffer (needs 2*fftSize for JUCE's performRealOnlyForwardTransform)
    std::array<float, kFFTSize * 2> fftWorkBuffer_ {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WiredMemoryAudioProcessor)
};
