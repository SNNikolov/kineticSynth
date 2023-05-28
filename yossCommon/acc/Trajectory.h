#pragma once

#include "../common/Math.h"
#include "../structs/CircularBuffer.h"
#include "../structs/CircularSummedBuffer.h"

namespace yoss
{
    class AccEngine;
    
    namespace trajectories
    {
        using yoss::CircularBuffer;
        using yoss::math::Vector3D;
        using yoss::math::Coo;
        using yoss::math::Time;
        using yoss::math::Velocity;
        
        //-----------------------------------------------------------------------
        // Structs and classes:
        struct Point;
        struct Segment;
        class Trajectory;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Types:
        typedef CircularBuffer<Vector3D>::BackPos BufferBackPos;
        typedef CircularBuffer<Vector3D>::Timestamp BufferTimestamp;
        enum SegmentType
        {
            SegmentType_Undefined,
            SegmentType_Idle, // Vibrating in the same place (only noise, no movement)
            SegmentType_Drift, // Slowly moving (noise that accumulates to movement)
            SegmentType_Move, // Moving with steps bigger than noise
        };
        enum SegmentDetectionType
        {
            SegmentDetectionType_None,
            SegmentDetectionType_MovingAgainstSegment,
            SegmentDetectionType_MovingAgainstSegmentInZ,
            SegmentDetectionType_MovingAgainstPast,
            SegmentDetectionType_MovingAgainstPeak
        };
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Constants:
        const int TRAJECTORY_BUFFERS_SIZE = 10000; // In samples
        
        const Coo ACC_NOISE_SIZE = 0.07; // [acc]
        
        const int LOCAL_VELOCITY_MAX_SPREAD = 2; // Max number of adjacent points to spread over the averaging of velocity
        
        const Coo MAX_IDLE_DPOS = ACC_NOISE_SIZE * 2.0; // [acc] Max displacement relative to idle state start pos
        const Time MIN_IDLE_DURATION = 0.08; // [sec] Minimal time without any (inc. drifting) movement for an "idle" state to be detected
        const BufferTimestamp MAX_IDLE_SEGMENT_LEN = TRAJECTORY_BUFFERS_SIZE / 3;
        //const Time MIN_IDLE_SEGMENT_DURATION = 0.05; // [sec] Minimal duration of an SegmentType_Idle segment
        
        const Velocity MAX_DRIFT_SPEED = 3.0; // [acc/sec]
        const Time MIN_DRIFT_DURATION = 0.08; // [sec] Minimal time without movement for a "drift" state to be detected
        //const Time MIN_DRIFT_SEGMENT_DURATION = 0.05; // [sec] Minimal duration of a SegmentType_Drift segment
        //-----------------------------------------------------------------------
        
        
        //-----------------------------------------------------------------------
        class Trajectory
        {
            friend class yoss::AccEngine;
            
        public:
            Trajectory(SegmentDetectionType detection_type);
            ~Trajectory();
            
            void FeedNewPosition(const Vector3D& new_pos, Time timestamp);
            void Update();
            bool SegmentJustEnded() { return _segmentJustEnded; }
            
            CircularBuffer<Segment>&        GetSegments()      { return _segments; }
            Segment*                        GetSegmentAt(BufferTimestamp timestamp);
            
            CircularBuffer<Time>&           GetTimestamps()    { return _timestamps; }
            CircularSummedBuffer<Vector3D>& GetPositions()     { return _pos; }
            CircularSummedBuffer<Vector3D>& GetVelocities()    { return _velocity; }
            CircularSummedBuffer<Vector3D>& GetAccelerations() { return _acc; }
            
        protected:
            SegmentDetectionType        _detectionType;
            CircularBuffer<Point>       _points;
            CircularBuffer<Segment>     _segments;
            
            CircularBuffer<Time>           _timestamps;
            CircularSummedBuffer<Vector3D> _pos;
            CircularSummedBuffer<Vector3D> _velocity;
            CircularSummedBuffer<Vector3D> _acc;
            
            int _feedsCounter;
            int _processedFeedsCounter;
            Time _lastFeedTimestamp;
            bool _segmentJustEnded;
            
            std::string   _debugSegEndReason;
            BufferBackPos _debugIsMovingAgainstPastBackPos;
            BufferBackPos _debugIsMovingAgainstSinceBackPos;
            
            BufferTimestamp _isIdlingSince;
            BufferTimestamp _isDriftingSince;
            
            bool IsIdling(BufferBackPos when, BufferBackPos* since_when, Segment& in_segment);
            bool IsDrifting(BufferBackPos when, BufferBackPos* since_when, Segment& in_segment);
            
            //bool IsMovingFast(BufferBackPos when, BufferBackPos* since_when, Segment& in_segment);
            bool IsMovingAgainst(Vector3D against_velocity, BufferBackPos when, BufferBackPos* since_when);
            bool IsMovingAgainst(BufferBackPos against_when, BufferBackPos when, BufferBackPos* since_when);
            bool IsMovingAgainstPast(BufferBackPos when, BufferBackPos* since_when, Segment& in_segment);
            bool IsMovingAgainstPeak(BufferBackPos when, BufferBackPos* since_when, Segment& in_segment);
            bool IsMovingAgainstSegment(BufferBackPos when, BufferBackPos* since_when, Segment& in_segment);
            
            bool IsAccelerating(BufferBackPos when);
            bool IsDeccelerating(BufferBackPos when);
            bool IsDecceleratingToStop(BufferBackPos when);
            
            void UpdateDetectedGrav(BufferBackPos when, Segment& in_segment);
            void UpdateSegmentPeaks(BufferBackPos when, Segment& in_segment);
            void CalcSegmentParams(Segment& segment);
            
            Point& PushNewPoint(BufferBackPos when);
            BufferBackPos GetLastPeakOrStart(Segment& in_segment);
            
            Vector3D GetWideVelocity(BufferBackPos when);
            Vector3D GetLocalVelocity(BufferBackPos when);
            
            BufferBackPos GetBackPos(BufferTimestamp buffer_timestamp);
            BufferTimestamp GetTimestamp(BufferBackPos back_pos = 0);
            bool IsBackPosInBuffer(BufferBackPos back_pos);
            bool IsPointDataInBuffer(Point* point);
        };
        
        //-----------------------------------------------------------------------
        struct Point
        {
            Vector3D absPos;
            Time timestamp;
            BufferTimestamp bufferTimestamp;
            
            Point();
            Point(const Vector3D& abs_pos,
                  const Time timestamp,
                  const BufferTimestamp buffer_timestamp);
            
            Coo GetDistanceTo(const Vector3D& pos);
            Coo GetDistanceTo(const Point& another_point);
        };
        
        //-----------------------------------------------------------------------
        struct Segment
        {
            SegmentType type;
            Point* startPoint;
            Point* endPoint;
            Point* peakPoint;
            Point* widePeakPoint;
            int samplesNum; // Number of samples in the buffer that the segment contains (including the start and end points)
            Vector3D detectedGrav;
            Coo      traveledLength;
            Vector3D avgVelocity;
            Vector3D topVelocity;
            Vector3D weightCenter;
            Vector3D middleOfStartEnd;
            
            std::vector<BufferTimestamp> debugPeaksTimestamps;
            std::vector<BufferTimestamp> debugWidePeaksTimestamps;
            
            Segment(Point* start = nullptr, Point* end = nullptr);
            void MergePrevSegment(Segment* prev_segment);
            void MergeNextSegment(Segment* next_segment);
            
            bool     IsFinished() { return (endPoint != nullptr); }
            Vector3D GetDPos();
            Time     GetDuration();
            Time     GetDurationToPeak();
            Time     GetDurationAfterPeak();
            Velocity GetAvgVelocitySize();
            bool     ContainsPoint(Point* point, bool include_start_and_end = true);
        };
        
    }
}

