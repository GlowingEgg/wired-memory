#import "SCKAudioCapture.h"
#import "AudioRingBuffer.h"

#import <ScreenCaptureKit/ScreenCaptureKit.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreAudio/CoreAudioTypes.h>

#include <atomic>
#include <mutex>
#include <string>
#include <os/log.h>

static os_log_t wmLog() {
    static os_log_t log = os_log_create ("com.khan.wiredmemory", "capture");
    return log;
}

// ── Forward-declare the ObjC helper ──────────────────────────────────────────

@class SCKCaptureHelper;

// ── PIMPL: holds all ObjC pointers and the ring buffer ───────────────────────

struct SCKAudioCapture::Impl
{
    AudioRingBuffer ringBuffer;

    std::atomic<bool> streamReady   { false };
    std::atomic<bool> permissionDenied { false };

    double processorSampleRate = 44100.0;
    int    maxBlockSize        = 512;

    std::string selectedBundleId;

    // ObjC objects — must be __strong so ARC retains them in this C++ struct
    __strong SCStream*           stream         = nil;
    __strong SCKCaptureHelper*   captureHelper  = nil;
    __strong dispatch_queue_t    audioQueue     = nil;

    // Cache the shareable content for source switching without re-enumerating
    __strong SCShareableContent* cachedContent  = nil;

    Impl()
    {
        audioQueue = dispatch_queue_create ("com.khan.wiredmemory.sckcapture",
                                            DISPATCH_QUEUE_SERIAL);
    }

    ~Impl()
    {
        stopStream();
    }

    void stopStream()
    {
        streamReady.store (false, std::memory_order_seq_cst);

        if (stream != nil)
        {
            // Fire-and-forget stop — we're tearing down
            [stream stopCaptureWithCompletionHandler:^(NSError*) {}];
            stream = nil;
        }

        captureHelper = nil;
    }
};

// ── ObjC helper: receives SCStreamOutput callbacks ───────────────────────────

@interface SCKCaptureHelper : NSObject <SCStreamOutput, SCStreamDelegate>
@property (nonatomic, assign) SCKAudioCapture::Impl* impl;
@end

@implementation SCKCaptureHelper
{
    int _callbackCount;
}

- (void)stream:(SCStream *)stream
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
                   ofType:(SCStreamOutputType)type
{
    if (type != SCStreamOutputTypeAudio || !_impl)
        return;

    // Log first few callbacks for diagnostics
    if (_callbackCount < 5)
    {
        CMItemCount n = CMSampleBufferGetNumSamples (sampleBuffer);
        os_log (wmLog(), "SCStream audio callback #%{public}d: %{public}ld frames", _callbackCount, (long) n);
        ++_callbackCount;
    }

    // Get the audio buffer list from the sample buffer
    CMBlockBufferRef blockBuffer = nil;
    AudioBufferList audioBufferList;
    memset (&audioBufferList, 0, sizeof (audioBufferList));

    OSStatus status = CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer (
        sampleBuffer,
        nullptr,                    // buffer list size out
        &audioBufferList,
        sizeof (audioBufferList),
        nullptr,                    // allocator
        nullptr,                    // block allocator
        kCMSampleBufferFlag_AudioBufferList_Assure16ByteAlignment,
        &blockBuffer);

    if (status != noErr || audioBufferList.mNumberBuffers == 0)
    {
        if (blockBuffer) CFRelease (blockBuffer);
        return;
    }

    CMItemCount numFrames = CMSampleBufferGetNumSamples (sampleBuffer);

    // SCK delivers non-interleaved float32 by default.
    // Each AudioBuffer in the list is one channel (or interleaved pair).
    // Handle both cases.

    const int numBuffers = (int) audioBufferList.mNumberBuffers;

    if (numBuffers == 1 && audioBufferList.mBuffers[0].mNumberChannels >= 2)
    {
        // Interleaved stereo — deinterleave into temp buffers
        const float* interleaved = (const float*) audioBufferList.mBuffers[0].mData;
        const int channels = (int) audioBufferList.mBuffers[0].mNumberChannels;
        const int frames = (int) numFrames;

        // Stack-allocate for typical block sizes; heap-allocate for large ones
        constexpr int kStackLimit = 4096;
        float stackL[kStackLimit], stackR[kStackLimit];
        std::unique_ptr<float[]> heapL, heapR;
        float* left  = stackL;
        float* right = stackR;

        if (frames > kStackLimit)
        {
            heapL = std::make_unique<float[]> (static_cast<size_t> (frames));
            heapR = std::make_unique<float[]> (static_cast<size_t> (frames));
            left  = heapL.get();
            right = heapR.get();
        }

        for (int i = 0; i < frames; ++i)
        {
            left[i]  = interleaved[i * channels];
            right[i] = interleaved[i * channels + 1];
        }

        const float* ptrs[2] = { left, right };
        _impl->ringBuffer.write (ptrs, 2, frames);
    }
    else
    {
        // Non-interleaved: each buffer is one channel
        const int channels = (numBuffers < 2) ? numBuffers : 2;
        const float* ptrs[2] = { nullptr, nullptr };

        for (int ch = 0; ch < channels; ++ch)
            ptrs[ch] = (const float*) audioBufferList.mBuffers[ch].mData;

        _impl->ringBuffer.write (ptrs, channels, (int) numFrames);
    }

    if (blockBuffer)
        CFRelease (blockBuffer);
}

- (void)stream:(SCStream *)stream didStopWithError:(NSError *)error
{
    os_log_error (wmLog(), "SCStream stopped with error: %{public}@", error.localizedDescription);
    if (_impl)
        _impl->streamReady.store (false, std::memory_order_seq_cst);
}

@end

// ── SCKAudioCapture public implementation ────────────────────────────────────

SCKAudioCapture::SCKAudioCapture()
    : impl_ (std::make_unique<Impl>())
{
}

SCKAudioCapture::~SCKAudioCapture() = default;

void SCKAudioCapture::getAvailableSources (SourcesCallback cb)
{
    os_log (wmLog(), "getAvailableSources called");

    auto* impl = impl_.get();

    [SCShareableContent getShareableContentExcludingDesktopWindows:NO
                                                onScreenWindowsOnly:NO
                                                  completionHandler:^(SCShareableContent* _Nullable content, NSError* _Nullable error)
    {
        if (error)
        {
            os_log_error (wmLog(), "SCShareableContent error (permission denied?): %{public}@", error.localizedDescription);
            // Check for permission denial
            impl->permissionDenied.store (true, std::memory_order_relaxed);

            // Dispatch empty list back to main thread
            dispatch_async (dispatch_get_main_queue(), ^{
                cb ({});
            });
            return;
        }

        impl->permissionDenied.store (false, std::memory_order_relaxed);
        impl->cachedContent = content;

        std::vector<AudioSourceInfo> sources;
        for (SCRunningApplication* app in content.applications)
        {
            // Filter to apps that likely produce audio (browsers, media players, etc.)
            // For now, include all apps — let the UI filter
            if (app.bundleIdentifier != nil && app.applicationName != nil)
            {
                sources.push_back ({
                    std::string ([app.bundleIdentifier UTF8String]),
                    std::string ([app.applicationName UTF8String])
                });
            }
        }

        // Sort alphabetically by display name
        std::sort (sources.begin(), sources.end(),
                   [] (const auto& a, const auto& b) { return a.displayName < b.displayName; });

        dispatch_async (dispatch_get_main_queue(), ^{
            cb (std::move (sources));
        });
    }];
}

void SCKAudioCapture::setSource (const std::string& bundleId)
{
    os_log (wmLog(), "setSource called with bundleId: %{public}s", bundleId.c_str());

    auto* impl = impl_.get();

    // Tear down existing stream first
    impl->stopStream();
    impl->selectedBundleId = bundleId;

    if (bundleId.empty())
        return;

    // If we don't have cached content, enumerate first then set source
    if (impl->cachedContent == nil)
    {
        getAvailableSources ([this, bundleId] (std::vector<AudioSourceInfo>) {
            // Re-enter on main thread with content now cached
            if (impl_->cachedContent != nil)
                startStreamForBundleId (bundleId);
        });
        return;
    }

    startStreamForBundleId (bundleId);
}

void SCKAudioCapture::stopCapture()
{
    impl_->stopStream();
    impl_->selectedBundleId.clear();
}

int SCKAudioCapture::readSamples (float* const* dst, int numChannels, int numSamples)
{
    return impl_->ringBuffer.read (dst, numChannels, numSamples);
}

bool SCKAudioCapture::isStreamReady() const noexcept
{
    return impl_->streamReady.load (std::memory_order_acquire);
}

bool SCKAudioCapture::isPermissionDenied() const noexcept
{
    return impl_->permissionDenied.load (std::memory_order_relaxed);
}

void SCKAudioCapture::prepareForPlayback (double sampleRate, int maxBlockSize)
{
    impl_->processorSampleRate = sampleRate;
    impl_->maxBlockSize        = maxBlockSize;

    // Size ring buffer for ~0.5s of audio to absorb bursty SCK callbacks
    const int minCapacity = maxBlockSize * 4;
    const int halfSecond  = static_cast<int> (sampleRate * 0.5);
    const int capacity    = (halfSecond > minCapacity) ? halfSecond : minCapacity;
    impl_->ringBuffer.allocate (2, capacity);  // stereo
}

std::string SCKAudioCapture::getSelectedBundleId() const
{
    return impl_->selectedBundleId;
}

// ── Private: start the SCK stream ────────────────────────────────────────────

void SCKAudioCapture::startStreamForBundleId (const std::string& bundleId)
{
    os_log (wmLog(), "startStreamForBundleId: %{public}s", bundleId.c_str());

    auto* impl = impl_.get();

    @try
    {
        NSString* targetBundleId = [NSString stringWithUTF8String:bundleId.c_str()];

        // Find the matching application
        SCRunningApplication* targetApp = nil;
        for (SCRunningApplication* app in impl->cachedContent.applications)
        {
            if ([app.bundleIdentifier isEqualToString:targetBundleId])
            {
                targetApp = app;
                break;
            }
        }

        if (targetApp == nil)
        {
            os_log_error (wmLog(), "startStream: target app not found for %{public}s", bundleId.c_str());
            return;
        }

        if (impl->cachedContent.displays.count == 0)
        {
            os_log_error (wmLog(), "startStream: no displays found");
            return;
        }

        SCDisplay* display = impl->cachedContent.displays.firstObject;

        // Build a filter scoped to the target app
        SCContentFilter* filter = nil;
        NSMutableArray<SCWindow*>* appWindows = [NSMutableArray array];
        for (SCWindow* window in impl->cachedContent.windows)
        {
            if (window.owningApplication != nil &&
                [window.owningApplication.bundleIdentifier isEqualToString:targetBundleId])
            {
                [appWindows addObject:window];
            }
        }

        if (appWindows.count > 0)
        {
            filter = [[SCContentFilter alloc] initWithDisplay:display
                                             includingWindows:appWindows];
        }
        else
        {
            filter = [[SCContentFilter alloc] initWithDisplay:display
                                        includingApplications:@[targetApp]
                                         exceptingWindows:@[]];
        }

        if (filter == nil)
            return;

        // Configure the stream — audio only (we don't need video frames)
        SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
        config.capturesAudio = YES;
        config.excludesCurrentProcessAudio = YES;
        config.channelCount  = 2;
        config.sampleRate    = (int) impl->processorSampleRate;

        // Minimize video overhead since we only want audio
        config.width  = 2;
        config.height = 2;
        config.minimumFrameInterval = CMTimeMake (1, 1); // 1 fps minimum
        config.showsCursor = NO;

        // Create the capture helper
        impl->captureHelper = [[SCKCaptureHelper alloc] init];
        impl->captureHelper.impl = impl;

        // Create and configure the stream
        impl->stream = [[SCStream alloc] initWithFilter:filter
                                         configuration:config
                                              delegate:impl->captureHelper];

        if (impl->stream == nil)
        {
            impl->captureHelper = nil;
            return;
        }

        NSError* addOutputError = nil;
        [impl->stream addStreamOutput:impl->captureHelper
                                 type:SCStreamOutputTypeAudio
                   sampleHandlerQueue:impl->audioQueue
                                error:&addOutputError];

        if (addOutputError)
        {
            impl->stream = nil;
            impl->captureHelper = nil;
            return;
        }

        // Reset the ring buffer before starting
        impl->ringBuffer.reset();

        // Start capture
        [impl->stream startCaptureWithCompletionHandler:^(NSError* error) {
            if (error == nil)
            {
                impl->streamReady.store (true, std::memory_order_release);
                os_log (wmLog(), "SCStream started successfully (rate=%{public}.0f)", impl->processorSampleRate);
            }
            else
            {
                os_log_error (wmLog(), "SCStream start failed: %{public}@", error.localizedDescription);
                dispatch_async (dispatch_get_main_queue(), ^{
                    impl->stream = nil;
                    impl->captureHelper = nil;
                });
            }
        }];
    }
    @catch (NSException* exception)
    {
        os_log_error (wmLog(), "SCStream exception: %{public}@ — %{public}@", exception.name, exception.reason);
        impl->stream = nil;
        impl->captureHelper = nil;
        impl->streamReady.store (false, std::memory_order_seq_cst);
    }
}
