#pragma once

#include <atomic>
#include <cstring>
#include <memory>

/**
 * Lock-free single-producer / single-consumer ring buffer for audio.
 *
 * - Writer: SCK callback thread (pushes captured PCM frames)
 * - Reader: JUCE audio thread (pulls into processBlock)
 *
 * Capacity is rounded up to a power of two so index wrapping is a bitmask.
 */
class AudioRingBuffer
{
public:
    AudioRingBuffer() = default;

    /** Allocate storage. Call from main thread before streaming starts. */
    void allocate (int numChannels, int capacitySamples)
    {
        numChannels_ = numChannels;
        capacity_    = nextPowerOfTwo (capacitySamples);
        mask_        = capacity_ - 1;

        data_ = std::make_unique<float[]> (sz (numChannels_) * sz (capacity_));
        std::memset (data_.get(), 0, sizeof (float) * sz (numChannels_) * sz (capacity_));

        writePos_.store (0, std::memory_order_relaxed);
        readPos_.store  (0, std::memory_order_relaxed);
    }

    /** Reset head/tail. Only call when neither thread is active (e.g. prepareToPlay). */
    void reset()
    {
        writePos_.store (0, std::memory_order_relaxed);
        readPos_.store  (0, std::memory_order_relaxed);

        if (data_)
            std::memset (data_.get(), 0, sizeof (float) * sz (numChannels_) * sz (capacity_));
    }

    /**
     * Write samples into the ring buffer.  Called from the SCK callback thread.
     * Returns the number of samples actually written (may be less than numSamples
     * if the buffer is nearly full).
     */
    int write (const float* const* src, int srcChannels, int numSamples)
    {
        const int wr = writePos_.load (std::memory_order_relaxed);
        const int rd = readPos_.load  (std::memory_order_acquire);

        const int available = capacity_ - (wr - rd);
        const int toWrite   = (numSamples < available) ? numSamples : available;

        if (toWrite <= 0)
            return 0;

        const int chans = (srcChannels < numChannels_) ? srcChannels : numChannels_;

        for (int ch = 0; ch < chans; ++ch)
        {
            float* dst = data_.get() + ch * capacity_;
            const float* s = src[ch];
            const int startIdx = wr & mask_;
            const int firstChunk = capacity_ - startIdx;

            if (toWrite <= firstChunk)
            {
                std::memcpy (dst + startIdx, s, sizeof (float) * sz (toWrite));
            }
            else
            {
                std::memcpy (dst + startIdx, s, sizeof (float) * sz (firstChunk));
                std::memcpy (dst, s + firstChunk, sizeof (float) * sz (toWrite - firstChunk));
            }
        }

        // Zero any extra channels in the ring buffer that the source doesn't provide
        for (int ch = chans; ch < numChannels_; ++ch)
        {
            float* dst = data_.get() + ch * capacity_;
            const int startIdx = wr & mask_;
            const int firstChunk = capacity_ - startIdx;

            if (toWrite <= firstChunk)
            {
                std::memset (dst + startIdx, 0, sizeof (float) * sz (toWrite));
            }
            else
            {
                std::memset (dst + startIdx, 0, sizeof (float) * sz (firstChunk));
                std::memset (dst, 0, sizeof (float) * sz (toWrite - firstChunk));
            }
        }

        writePos_.store (wr + toWrite, std::memory_order_release);
        return toWrite;
    }

    /**
     * Read samples from the ring buffer.  Called from the audio thread.
     * Returns the number of samples actually read (may be less than numSamples
     * if the buffer doesn't have enough data — caller should zero the remainder).
     */
    int read (float* const* dst, int dstChannels, int numSamples)
    {
        const int rd = readPos_.load (std::memory_order_relaxed);
        const int wr = writePos_.load (std::memory_order_acquire);

        const int available = wr - rd;
        const int toRead    = (numSamples < available) ? numSamples : available;

        if (toRead <= 0)
            return 0;

        const int chans = (dstChannels < numChannels_) ? dstChannels : numChannels_;

        for (int ch = 0; ch < chans; ++ch)
        {
            const float* src = data_.get() + ch * capacity_;
            float* d = dst[ch];
            const int startIdx = rd & mask_;
            const int firstChunk = capacity_ - startIdx;

            if (toRead <= firstChunk)
            {
                std::memcpy (d, src + startIdx, sizeof (float) * sz (toRead));
            }
            else
            {
                std::memcpy (d, src + startIdx, sizeof (float) * sz (firstChunk));
                std::memcpy (d + firstChunk, src, sizeof (float) * sz (toRead - firstChunk));
            }
        }

        // Zero any extra destination channels
        for (int ch = chans; ch < dstChannels; ++ch)
            std::memset (dst[ch], 0, sizeof (float) * sz (toRead));

        readPos_.store (rd + toRead, std::memory_order_release);
        return toRead;
    }

    /** How many samples can be read right now.  Safe from any thread. */
    int getNumAvailable() const noexcept
    {
        const int wr = writePos_.load (std::memory_order_acquire);
        const int rd = readPos_.load  (std::memory_order_relaxed);
        return wr - rd;
    }

    int getCapacity() const noexcept { return capacity_; }
    int getNumChannels() const noexcept { return numChannels_; }

private:
    static constexpr size_t sz (int v) noexcept { return static_cast<size_t> (v); }

    static int nextPowerOfTwo (int v)
    {
        int p = 1;
        while (p < v) p <<= 1;
        return p;
    }

    std::unique_ptr<float[]> data_;
    int numChannels_ = 0;
    int capacity_    = 0;
    int mask_        = 0;

    // Monotonically increasing positions — wrapping handled via bitmask.
    // This avoids the classic "is it full or empty?" ambiguity.
    std::atomic<int> writePos_ { 0 };
    std::atomic<int> readPos_  { 0 };
};
