#pragma once

#include <iostream>

#include "../common/Math.h"
#include "../common/Input.h"
#include "../sound/SoundEngine.h"
#include "../graphics/common/Graphics.h"
#include "../graphics/common/Scene.h"
#include "Trajectory.h"

namespace yoss
{
    //-----------------------------------------------------------------------
    class AccEngine
    {
    public:
        typedef std::function<void (bool beat_is_detected)> FeedListener;
        
        static constexpr math::Angle AccSegmentIsABeatMaxAngleToZ = Config::AccSegmentIsABeatMaxAngleToZ;
        static const bool DETECT_BEATS = true;
        static constexpr math::Angle BEATS_SCALE_START_ANGLE = -20.0;
        static constexpr math::Angle BEATS_SCALE_END_ANGLE   = 120.0;
        static const bool DRAW_BEAT_SEGMENTS_IN_CONSOLE = false;
        
        AccEngine();
        ~AccEngine();
        void ResetInput();
        
        trajectories::Trajectory& GetAccTrajectory() { return _accTrajectory; }
        trajectories::Trajectory& GetMagTrajectory() { return _magTrajectory; }
        
        trajectories::Segment&    GetAccBeatSegment() { return *_prevBeatSegment; }
        math::Coo       GetAccBeatAmplitude();
        math::Frequency GetAccBeatFrequency();
        math::Frequency GetAccNextBeatFrequency();
        bool            IsExpectingBeat();
        void AddAccFeedListener(FeedListener listener);
        void RemoveAccFeedListener(FeedListener& listener);
        
        // Called regularly by native accelerometer functionality
        void FeedAcc(double acc_x, double acc_y, double acc_z,
                     double gyro_x, double gyro_y, double gyro_z,
                     double mag_x, double mag_y, double mag_z);
        
        // Debugging methods
        void DrawTrajectory(graphics::Graphics* graphics);
        void DrawSegmentInConsole(trajectories::Segment segment);
        
    private:
        bool IsSegmentABeat(trajectories::Segment& seg);
        
        const math::Coo  MIN_BEAT_AMPLITUDE = 0.3; // [g] Min amplitude of a segment for a beat to be detected
        const math::Coo  MAX_BEAT_AMPLITUDE = 4.0; // [g] Max amplitude of a beat (is clamped if more)
        const math::Time MIN_SMALL_BEAT_AVG_VELOCITY = 4.0; // [g/sec] Min average velocity of a segment for a beat with amplitude MIN_BEAT_AMPLITUDE to be detected
        const math::Time MIN_BIG_BEAT_AVG_VELOCITY = 25.0; // [g/sec] Min average velocity of a segment for a beat with amplitude MAX_BEAT_AMPLITUDE to be detected
        
        trajectories::Trajectory _accTrajectory;
        trajectories::Trajectory _magTrajectory;
        trajectories::Trajectory _gyroTrajectory;
        
        std::vector<FeedListener> _accFeedListeners;
        
        math::Time     _prevFeedTimestamp, _currentFeedTimestamp;
        math::Time     _prevSegEndTimestamp;
        math::Coo      _prevSegAmplitude, _prevSegAmplitude2;
        math::Velocity _prevSegAvgVelocity, _prevSegAvgVelocity2;
        bool           _prevSegIsAntiBeat;
        bool           _prevSegIsDrawn;
        math::Time     _prevBeatTimestamp;
        math::Coo      _prevBeatAmplitude;
        trajectories::Segment* _prevBeatSegment;
        trajectories::Segment* _prevSegment;
        trajectories::BufferTimestamp _freezeDrawingAt;
    };

}

