#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>

class SCKAudioCapture;

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

    // -- Sample playback --
    std::atomic<bool>  playbackActive_ { false };
    std::atomic<int>   playbackPos_    { 0 };
    std::atomic<int>   sampleLength_   { 0 };   // length of recorded sample in frames

    // -- Captured sample snapshot (read by editor timer) --
    juce::SpinLock sampleLock_;
    std::array<float, kSampleSnapshotSize> sampleSnapshot_ {};
    bool sampleReady_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WiredMemoryAudioProcessor)
};
