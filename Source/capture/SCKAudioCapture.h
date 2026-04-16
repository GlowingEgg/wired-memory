#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

/**
 * Describes a running application that can be targeted for audio capture.
 */
struct AudioSourceInfo
{
    std::string bundleId;
    std::string displayName;
};

/**
 * Captures audio from a macOS application via ScreenCaptureKit.
 *
 * Ownership: lives in the AudioProcessor (outlives the editor).
 * Threading:
 *   - readSamples / isStreamReady  → audio thread (lock-free)
 *   - setSource / stopCapture      → main thread only
 *   - getAvailableSources          → any thread; callback on main thread
 *   - prepareForPlayback           → main thread (between audio callbacks)
 */
class SCKAudioCapture
{
public:
    SCKAudioCapture();
    ~SCKAudioCapture();

    // Non-copyable, non-movable (due to PIMPL + ObjC pointers)
    SCKAudioCapture (const SCKAudioCapture&) = delete;
    SCKAudioCapture& operator= (const SCKAudioCapture&) = delete;

    // ── Source enumeration ───────────────────────────────────────────────

    /** Callback receives available audio sources on the main thread. */
    using SourcesCallback = std::function<void (std::vector<AudioSourceInfo>)>;

    /** Enumerate running applications. Callback fires on the main thread. */
    void getAvailableSources (SourcesCallback cb);

    // ── Stream control (main thread only) ────────────────────────────────

    /** Start capturing from the app with the given bundle ID.
        Tears down any existing stream first. */
    void setSource (const std::string& bundleId);

    /** Stop the current capture stream. */
    void stopCapture();

    // ── Audio thread interface (lock-free) ───────────────────────────────

    /** Pull captured samples into dst buffers.
        Returns number of samples actually read; caller zeros the remainder. */
    int readSamples (float* const* dst, int numChannels, int numSamples);

    /** True once a stream is actively delivering audio. */
    bool isStreamReady() const noexcept;

    /** True if the user has denied Screen Recording permission. */
    bool isPermissionDenied() const noexcept;

    // ── Lifecycle ────────────────────────────────────────────────────────

    /** Called from prepareToPlay. Resizes the ring buffer. */
    void prepareForPlayback (double sampleRate, int maxBlockSize);

    // ── State ────────────────────────────────────────────────────────────

    /** Returns the bundle ID of the currently selected source (empty if none). */
    std::string getSelectedBundleId() const;

    /** How many samples are available in the ring buffer right now. */
    int getRingBufferAvailable() const noexcept;

    // Impl is public so the ObjC helper can reference it.
    // Only the .mm translation unit sees its definition.
    struct Impl;

private:
    void startStreamForBundleId (const std::string& bundleId);

    std::unique_ptr<Impl> impl_;
};
