/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MOZILLA_MEDIASOURCEREADER_H_
#define MOZILLA_MEDIASOURCEREADER_H_

#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsString.h"
#include "nsTArray.h"
#include "MediaDecoderReader.h"

namespace mozilla {

class MediaSourceDecoder;
class SourceBufferDecoder;
class TrackBuffer;

namespace dom {

class MediaSource;

} // namespace dom

class MediaSourceReader : public MediaDecoderReader
{
public:
  explicit MediaSourceReader(MediaSourceDecoder* aDecoder);

  nsresult Init(MediaDecoderReader* aCloneDonor) MOZ_OVERRIDE
  {
    // Although we technically don't implement anything here, we return NS_OK
    // so that when the state machine initializes and calls this function
    // we don't return an error code back to the media element.
    return NS_OK;
  }

  // Indicates the point in time at which the reader should consider
  // registered TrackBuffers essential for initialization.
  void PrepareInitialization();

  bool IsWaitingMediaResources() MOZ_OVERRIDE;

  void RequestAudioData() MOZ_OVERRIDE;

  void OnAudioDecoded(AudioData* aSample);

  void RequestVideoData(bool aSkipToNextKeyframe, int64_t aTimeThreshold) MOZ_OVERRIDE;

  void OnVideoDecoded(VideoData* aSample);

  void OnNotDecoded(MediaData::Type aType, RequestSampleCallback::NotDecodedReason aReason);

  void OnSeekCompleted(nsresult aResult);

  bool HasVideo() MOZ_OVERRIDE
  {
    return mInfo.HasVideo();
  }

  bool HasAudio() MOZ_OVERRIDE
  {
    return mInfo.HasAudio();
  }

  void NotifyTimeRangesChanged();

  // We can't compute a proper start time since we won't necessarily
  // have the first frame of the resource available. This does the same
  // as chrome/blink and assumes that we always start at t=0.
  virtual int64_t ComputeStartTime(const VideoData* aVideo, const AudioData* aAudio) MOZ_OVERRIDE { return 0; }

  bool IsMediaSeekable() { return true; }

  nsresult ReadMetadata(MediaInfo* aInfo, MetadataTags** aTags) MOZ_OVERRIDE;
  void Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime,
            int64_t aCurrentTime) MOZ_OVERRIDE;

  already_AddRefed<SourceBufferDecoder> CreateSubDecoder(const nsACString& aType);

  void AddTrackBuffer(TrackBuffer* aTrackBuffer);
  void RemoveTrackBuffer(TrackBuffer* aTrackBuffer);
  void OnTrackBufferConfigured(TrackBuffer* aTrackBuffer, const MediaInfo& aInfo);

  void Shutdown();

  virtual void BreakCycles();

  bool IsShutdown()
  {
    ReentrantMonitorAutoEnter decoderMon(mDecoder->GetReentrantMonitor());
    return mDecoder->IsShutdown();
  }

  // Return true if all of the active tracks contain data for the specified time.
  bool TrackBuffersContainTime(int64_t aTime);

  // Mark the reader to indicate that EndOfStream has been called on our MediaSource
  void Ended();

  // Return true if the Ended method has been called
  bool IsEnded();

#ifdef MOZ_EME
  nsresult SetCDMProxy(CDMProxy* aProxy);
#endif

private:
  bool SwitchAudioReader(int64_t aTarget);
  bool SwitchVideoReader(int64_t aTarget);

  // Return a reader from the set available in aTrackDecoders that has data
  // available in the range requested by aTarget.
  already_AddRefed<MediaDecoderReader> SelectReader(int64_t aTarget,
                                                    const nsTArray<nsRefPtr<SourceBufferDecoder>>& aTrackDecoders);

  void AttemptSeek();

  nsRefPtr<MediaDecoderReader> mAudioReader;
  nsRefPtr<MediaDecoderReader> mVideoReader;

  nsTArray<nsRefPtr<TrackBuffer>> mTrackBuffers;
  nsTArray<nsRefPtr<TrackBuffer>> mEssentialTrackBuffers;
  nsRefPtr<TrackBuffer> mAudioTrack;
  nsRefPtr<TrackBuffer> mVideoTrack;

#ifdef MOZ_EME
  nsRefPtr<CDMProxy> mCDMProxy;
#endif

  // These are read and written on the decode task queue threads.
  int64_t mLastAudioTime;
  int64_t mLastVideoTime;

  // Temporary seek information while we wait for the data
  // to be added to the track buffer.
  int64_t mPendingSeekTime;
  int64_t mPendingStartTime;
  int64_t mPendingEndTime;
  int64_t mPendingCurrentTime;
  bool mWaitingForSeekData;

  // Number of outstanding OnSeekCompleted notifications
  // we're expecting to get from child decoders, and the
  // result we're going to forward onto our callback.
  uint32_t mPendingSeeks;
  nsresult mSeekResult;

  int64_t mTimeThreshold;
  bool mDropAudioBeforeThreshold;
  bool mDropVideoBeforeThreshold;

  bool mEnded;

  // For a seek to complete we need to send a sample with
  // the mDiscontinuity field set to true once we have the
  // first decoded sample. These flags are set during seeking
  // so we can detect when we have the first decoded sample
  // after a seek.
  bool mAudioIsSeeking;
  bool mVideoIsSeeking;

  bool mHasEssentialTrackBuffers;
};

} // namespace mozilla

#endif /* MOZILLA_MEDIASOURCEREADER_H_ */
