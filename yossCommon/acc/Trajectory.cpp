#include "Trajectory.h"
#include "../common/Log.h"

using namespace std;
using namespace yoss::math;
using namespace yoss::trajectories;

#define DEBUG_SEGMENTS(str) //Log::LogText(str, true, 3)
#define DEBUG_SEG_END_REASON(str_reason) { _debugSegEndReason = str_reason; }
#define DEBUG_ISMOVINGAGAINST_BACKPOS(past_backpos) { _debugIsMovingAgainstPastBackPos = past_backpos; }

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// struct Point
//-----------------------------------------------------------------------
Point::Point() :
    absPos(Vector3D(0, 0, 0)),
    timestamp(0.0),
    bufferTimestamp((BufferTimestamp)0)
{
}

//-----------------------------------------------------------------------
yoss::trajectories::Point::Point(const Vector3D& abs_pos,
             const Time timestamp,
             const BufferTimestamp buffer_timestamp) :
    absPos(abs_pos),
    timestamp(timestamp),
    bufferTimestamp(buffer_timestamp)
{
}

//-----------------------------------------------------------------------
Coo yoss::trajectories::Point::GetDistanceTo(const Vector3D& pos)
{
    Vector3D dpos = pos - absPos;
    return dpos.Size();
}


//-----------------------------------------------------------------------
Coo yoss::trajectories::Point::GetDistanceTo(const Point& another_point)
{
    Vector3D dpos = another_point.absPos - absPos;
    return dpos.Size();
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// struct Segment
//-----------------------------------------------------------------------
Segment::Segment(Point* start_point, Point* end_point) :
    type(SegmentType_Undefined),
    startPoint(start_point),
    endPoint(end_point),
    peakPoint(nullptr),
    widePeakPoint(nullptr),
    samplesNum(0),
    detectedGrav(0, 0, 0),
    traveledLength(0),
    avgVelocity(0, 0, 0),
    topVelocity(0, 0, 0),
    weightCenter(0, 0, 0),
    middleOfStartEnd(0, 0, 0)
{
}

//-----------------------------------------------------------------------
void Segment::MergePrevSegment(Segment* prev_segment)
{
    ASSERT(prev_segment);
    ASSERT(prev_segment->endPoint == startPoint);
    
    startPoint = prev_segment->startPoint;
}

//-----------------------------------------------------------------------
void Segment::MergeNextSegment(Segment* next_segment)
{
    ASSERT(next_segment);
    ASSERT(next_segment->startPoint == endPoint);
    
    endPoint = next_segment->endPoint;
}

//-----------------------------------------------------------------------
Vector3D Segment::GetDPos()
{
    ASSERT(startPoint && endPoint);
    return (endPoint->absPos - startPoint->absPos);
}

//-----------------------------------------------------------------------
Time Segment::GetDuration()
{
    ASSERT(startPoint && endPoint);
    return (endPoint->timestamp - startPoint->timestamp);
}

//-----------------------------------------------------------------------
Time Segment::GetDurationToPeak()
{
    ASSERT(startPoint && peakPoint);
    return (peakPoint->timestamp - startPoint->timestamp);
}

//-----------------------------------------------------------------------
Time Segment::GetDurationAfterPeak()
{
    ASSERT(endPoint && peakPoint);
    return (endPoint->timestamp - peakPoint->timestamp);
}

//-----------------------------------------------------------------------
Velocity Segment::GetAvgVelocitySize()
{
    Time duration = GetDuration();
    return (Velocity)(duration > 0 ? (traveledLength / duration) : 0);
}

//-----------------------------------------------------------------------
bool Segment::ContainsPoint(Point* point, bool include_start_and_end)
{
    ASSERT(startPoint && endPoint && point);
    ASSERT(point->timestamp != 0.0);
    
    if (include_start_and_end)
        return (point->timestamp >= startPoint->timestamp && point->timestamp <= endPoint->timestamp);
    else
        return (point->timestamp > startPoint->timestamp && point->timestamp < endPoint->timestamp);
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// class Trajectory
//-----------------------------------------------------------------------
Trajectory::Trajectory(SegmentDetectionType detection_type) :
    _detectionType(detection_type),
    _points(TRAJECTORY_BUFFERS_SIZE * 4, false),
    _segments(TRAJECTORY_BUFFERS_SIZE, false),
    _feedsCounter(0),
    _processedFeedsCounter(0),
    _lastFeedTimestamp(0),
    _segmentJustEnded(false),
    _timestamps(TRAJECTORY_BUFFERS_SIZE, false),
    _pos(TRAJECTORY_BUFFERS_SIZE, true),
    _velocity(TRAJECTORY_BUFFERS_SIZE, true),
    _acc(TRAJECTORY_BUFFERS_SIZE, true),
    _debugSegEndReason(""),
    _debugIsMovingAgainstPastBackPos(-33),
    _debugIsMovingAgainstSinceBackPos(-33),
    _isIdlingSince(0),
    _isDriftingSince(0)
{
    FeedNewPosition(ZERO_VECTOR, 1.0);
    FeedNewPosition(ZERO_VECTOR, 2.0);
    FeedNewPosition(ZERO_VECTOR, 3.0);
    
    // Create inital Point
    Point point_zero(ZERO_VECTOR, _timestamps.Get(), GetTimestamp());
    _points.Push(point_zero);
    
    // Create inital Segment
    Segment segment_zero(&_points.Get());
    _segments.Push(segment_zero);
}

//-----------------------------------------------------------------------
Trajectory::~Trajectory()
{
}

//-----------------------------------------------------------------------
void Trajectory::FeedNewPosition(const Vector3D& new_pos, Time timestamp)
{
    Time dt = timestamp - _lastFeedTimestamp;
    if (dt == 0) dt = 0.001;
    ASSERT(dt > 0);
    _lastFeedTimestamp = timestamp;
    
    _timestamps.Push(timestamp);
    
    Vector3D prev_pos = _pos.Get();
    _pos.Push(new_pos);
    
    Vector3D v = (new_pos - prev_pos) * (1.0 / dt);
    Vector3D prev_v = _velocity.Get();
    _velocity.Push(v);
    
    Vector3D acc = (v - prev_v) * (1.0 / dt);
    _acc.Push(acc);
    
    _feedsCounter++;
}

//-----------------------------------------------------------------------
Segment* Trajectory::GetSegmentAt(BufferTimestamp timestamp)
{
    for (BufferBackPos i = 0; i < _segments.GetOccupiedSize(); i++)
    {
        auto& seg = _segments.Get(i);
        if (seg.startPoint->bufferTimestamp <= timestamp)
            return &seg;
    }
    
    return nullptr;
}

//-----------------------------------------------------------------------
void Trajectory::Update()
{
    int feeds_to_process = (_feedsCounter - _processedFeedsCounter);
    _processedFeedsCounter = _feedsCounter;
    
    _segmentJustEnded = false;
    
    if (_detectionType == SegmentDetectionType_None) return;

    while (feeds_to_process > 0)
    {
        feeds_to_process--;
        
        const auto buffer_pos = (BufferBackPos)feeds_to_process;
        //const auto buffer_timestamp = GetTimestamp(buffer_pos);
        const Vector3D current_pos = _pos.Get(buffer_pos);
        //const Vector3D current_vel = _velocity.Get(buffer_pos);
        //const Time     current_time = _timestamps.Get(buffer_pos);

        Segment& segment = _segments.Get();
        //BufferBackPos segment_start_back_pos = GetBackPos(segment.startPoint->bufferTimestamp);
        
        bool end_detected = false;
        std::string debug_end_str;
        BufferBackPos end_back_pos = buffer_pos;
        
        Vector3D seg_vec = current_pos - segment.startPoint->absPos;
        
        BufferBackPos drifting_start_backpos;
        bool is_drifting = IsDrifting(buffer_pos, &drifting_start_backpos, segment);
        
        BufferBackPos idling_start_backpos;
        bool is_idling = IsIdling(buffer_pos, &idling_start_backpos, segment);
        
        if (is_idling)
        {
            if (segment.type == SegmentType_Undefined)
            {
                segment.type = SegmentType_Idle;
            }
            else if (segment.type == SegmentType_Move)
            {
                end_back_pos = idling_start_backpos;
                end_detected = true;
                DEBUG_SEG_END_REASON("idling");
            }
        }
        else if (is_drifting)
        {
            if (segment.type == SegmentType_Undefined)
            {
                segment.type = SegmentType_Drift;
            }
            else if (segment.type == SegmentType_Idle)
            {
                if ((1))
                {
                    segment.type = SegmentType_Drift;
                    DEBUG_SEGMENTS("switching segment type: idle -> drift");
                }
                else
                {
                    end_back_pos = drifting_start_backpos;
                    end_detected = true;
                    DEBUG_SEG_END_REASON("drifting when idle");
                }
            }
            else if (segment.type == SegmentType_Move)
            {
                end_back_pos = drifting_start_backpos;
                end_detected = true;
                DEBUG_SEG_END_REASON("drifting");
            }
        }
        else
        {
            if (segment.type == SegmentType_Idle)
            {
                end_back_pos = buffer_pos + 1;
                end_detected = true;
                DEBUG_SEG_END_REASON("not idling");
            }
            else if (segment.type == SegmentType_Drift)
            {
                end_back_pos = buffer_pos + 1;
                end_detected = true;
                DEBUG_SEG_END_REASON("not drifting");
            }
            else
            {
                segment.type = SegmentType_Move;
            }
        }
        
        if (!end_detected && (segment.type == SegmentType_Move || segment.type == SegmentType_Drift))
        {
            switch (_detectionType)
            {
                case SegmentDetectionType_MovingAgainstSegment:
                case SegmentDetectionType_MovingAgainstSegmentInZ:
                    if (IsMovingAgainstSegment(buffer_pos, &end_back_pos, segment))
                    {
                        end_detected = true;
                        DEBUG_SEG_END_REASON("agnst seg at " + Log::ToStr(_debugIsMovingAgainstPastBackPos - buffer_pos) + " since " + Log::ToStr(end_back_pos - buffer_pos));
                    }
                    break;
                    
                case SegmentDetectionType_MovingAgainstPeak:
                    if (IsMovingAgainstPeak(buffer_pos, &end_back_pos, segment))
                    {
                        end_detected = true;
                        DEBUG_SEG_END_REASON("agnst peak at " + Log::ToStr(_debugIsMovingAgainstPastBackPos - buffer_pos) + " since " + Log::ToStr(end_back_pos - buffer_pos));
                    }
                    break;
                
                case SegmentDetectionType_MovingAgainstPast:
                    if (IsMovingAgainstPast(buffer_pos, &end_back_pos, segment))
                    {
                        end_detected = true;
                        DEBUG_SEG_END_REASON("agnst past at " + Log::ToStr(_debugIsMovingAgainstPastBackPos - buffer_pos) + " since " + Log::ToStr(end_back_pos - buffer_pos));
                    }
                    break;
                    
                default:
                    ASSERT(false);
            }
        }
        
        UpdateDetectedGrav(buffer_pos, segment);
        UpdateSegmentPeaks(buffer_pos, segment);
        
        if (end_detected)
        {
            BufferBackPos seg_start_back_pos = GetBackPos(segment.startPoint->bufferTimestamp);
            if (end_back_pos > seg_start_back_pos)
                end_back_pos = seg_start_back_pos;
            
            // Create end Point
            Point& end_point = PushNewPoint(end_back_pos);
            segment.endPoint = &end_point;
            
            //if (end_back_pos != buffer_pos)
            seg_vec = segment.endPoint->absPos - segment.startPoint->absPos;
            
            // Create next Segment
            Segment next_segment(&end_point);
            _segments.Push(next_segment);
            
            // Calculate ended segment's parameters
            CalcSegmentParams(segment);
            
            _segmentJustEnded = true;
            
            if ((true) || seg_vec.z < 0)
            {
                static const std::vector<std::string> type_names = {"undefined", "idle", "drift", "move"};
                std::string type_str = type_names[(int)segment.type];
                
                DEBUG_SEGMENTS("SEG_END: " + _debugSegEndReason +
                    ", type=" + type_str +
                    ", dur=" + Log::ToStr(segment.GetDuration()) +
                    "=" + Log::ToStr(segment.samplesNum) +
                    ", amp=" + Log::ToStr(segment.GetDPos().Size()) +
                    //", seg_freq=" + Log::ToStr(PointToDeg(-segment.middleOfStartEnd.z, -segment.middleOfStartEnd.y) / 220.0) +
                    ", avg_vel=" + Log::ToStr(segment.GetAvgVelocitySize()) +
                    ", a_to_Z=" + Log::ToStr(RadToDeg(segment.GetDPos().AngleTo(Z_AXIS)))
                );
            }
        }
    }
}

//-----------------------------------------------------------------------
void Trajectory::CalcSegmentParams(Segment& segment)
{
    segment.samplesNum = 1 + segment.endPoint->bufferTimestamp - segment.startPoint->bufferTimestamp;
    double one_div_samples_num = (double)1 / (double)segment.samplesNum;
    
    segment.avgVelocity = 0;
    segment.weightCenter *= 0;
    segment.traveledLength = 0;
    
    for (auto t = segment.startPoint->bufferTimestamp; t <= segment.endPoint->bufferTimestamp; t++)
    {
        if (!_pos.IsStillInBuffer(t)) break;
        
        auto back_t = GetBackPos(t);
        auto pos = _pos.Get(back_t);
        auto v = _velocity.Get(back_t);
        
        segment.avgVelocity += v;
        segment.weightCenter += pos;
        segment.traveledLength += _pos.GetDiff(back_t).Size();
    }
    
    segment.avgVelocity *= one_div_samples_num;
    segment.weightCenter *= one_div_samples_num;
    segment.middleOfStartEnd = (segment.startPoint->absPos + segment.endPoint->absPos) * 0.5;
}

//-----------------------------------------------------------------------
Point& Trajectory::PushNewPoint(BufferBackPos when)
{
    Point point(_pos.Get(when), _timestamps.Get(when), GetTimestamp(when));
    _points.Push(point);
    
    return _points.Get();
}

//-----------------------------------------------------------------------
BufferBackPos Trajectory::GetBackPos(BufferTimestamp buffer_timestamp)
{
    return _timestamps.GetBackPos(buffer_timestamp);
}

//-----------------------------------------------------------------------
BufferTimestamp Trajectory::GetTimestamp(BufferBackPos back_pos)
{
    return _timestamps.GetTimestamp(back_pos);
}

//-----------------------------------------------------------------------
bool Trajectory::IsBackPosInBuffer(BufferBackPos back_pos)
{
    return _timestamps.IsBackPosOccupied(back_pos);
}

//-----------------------------------------------------------------------
bool Trajectory::IsPointDataInBuffer(Point* point)
{
    return IsBackPosInBuffer(GetBackPos(point->bufferTimestamp));
}

//-----------------------------------------------------------------------
Vector3D Trajectory::GetWideVelocity(BufferBackPos when)
{
    ASSERT(when >= 1 && IsBackPosInBuffer(when + 2));
    
    auto wide_dpos = _pos.Get(when - 1) - _pos.Get(when + 2);
    auto wide_dur = _timestamps.Get(when - 1) - _timestamps.Get(when + 2);
    auto wide_vel = wide_dpos * (1.0 / wide_dur);
    
    return wide_vel;
}

//-----------------------------------------------------------------------
Vector3D Trajectory::GetLocalVelocity(BufferBackPos when)
{
    ASSERT(when >= 0 && IsBackPosInBuffer(when));
    
    auto dpos = _pos.GetDiff(when);
    auto duration = _timestamps.GetDiff(when);
    
    BufferBackPos spread = 1;
    BufferBackPos from_when = when + 1;
    BufferBackPos to_when = when;
    
    while (dpos.Size() < ACC_NOISE_SIZE &&
           spread <= LOCAL_VELOCITY_MAX_SPREAD)
    {
        if (IsBackPosInBuffer(from_when + 1))
            from_when++;
        if (to_when - 1 >= 0)
            to_when--;
        spread++;
        
        dpos = _pos.Get(to_when) - _pos.Get(from_when);
        duration = _timestamps.Get(to_when) - _timestamps.Get(from_when);
    }
    
    auto velocity = dpos * (1.0 / duration);
    return velocity;
}

//-----------------------------------------------------------------------
BufferBackPos Trajectory::GetLastPeakOrStart(Segment& in_segment)
{
    Point* point = in_segment.startPoint;
    if (in_segment.peakPoint)
        point = in_segment.peakPoint;
    if (in_segment.widePeakPoint && in_segment.widePeakPoint->bufferTimestamp > point->bufferTimestamp)
        point = in_segment.widePeakPoint;
    
    return GetBackPos(point->bufferTimestamp);
}

//-----------------------------------------------------------------------
void Trajectory::UpdateSegmentPeaks(BufferBackPos when, Segment& in_segment)
{
    if (_pos.GetDiff(when).Size() <= ACC_NOISE_SIZE) return;
    
    auto when_vel_size = _velocity.Get(when).Size();

    // Update segment.peakPoint
    if (!in_segment.peakPoint)
    {
        // Create peak Point
        Point& peak_point = PushNewPoint(when);
        in_segment.peakPoint = &peak_point;
        
        in_segment.debugPeaksTimestamps.push_back(GetTimestamp(when));
    }
    else
    {
        // Update existing peak Point
        auto peak_vel_size = IsPointDataInBuffer(in_segment.peakPoint) ?
            _velocity.GetByTimestamp(in_segment.peakPoint->bufferTimestamp).Size() : 0;
        if (when_vel_size >= peak_vel_size)
        {
            in_segment.peakPoint->absPos = _pos.Get(when);
            in_segment.peakPoint->timestamp = _timestamps.Get(when);
            in_segment.peakPoint->bufferTimestamp = GetTimestamp(when);
            
            in_segment.debugPeaksTimestamps.push_back(GetTimestamp(when));
        }
    }
    
    // Update segment.widePeakPoint
    auto wide_when = when + 1;
    auto wide_when2 = when + 2;
    
    if (!in_segment.widePeakPoint)
    {
        // Create peak Point
        Point& peak_point = PushNewPoint(wide_when);
        in_segment.widePeakPoint = &peak_point;
        
        in_segment.debugWidePeaksTimestamps.push_back(GetTimestamp(wide_when));
    }
    else
    {
        // Update wide peak Point
        auto wide_when_vel_size = GetWideVelocity(wide_when).Size();
        auto wide_when2_vel_size = GetWideVelocity(wide_when2).Size();
        
        auto when_wide_peak = GetBackPos(in_segment.widePeakPoint->bufferTimestamp);
        auto wide_peak_vel_size = IsPointDataInBuffer(in_segment.widePeakPoint) ?
            GetWideVelocity(when_wide_peak).Size() : 0;
        
        if (wide_when2_vel_size >= wide_peak_vel_size &&
            wide_when2_vel_size > wide_when_vel_size)
        {
            in_segment.widePeakPoint->absPos = _pos.Get(wide_when2);
            in_segment.widePeakPoint->timestamp = _timestamps.Get(wide_when2);
            in_segment.widePeakPoint->bufferTimestamp = GetTimestamp(wide_when2);

            if (in_segment.debugWidePeaksTimestamps.size() == 0 ||
                in_segment.debugWidePeaksTimestamps.back() != wide_when2)
                in_segment.debugWidePeaksTimestamps.push_back(GetTimestamp(wide_when2));
        }
        else if (wide_when_vel_size >= wide_peak_vel_size)
        {
            in_segment.widePeakPoint->absPos = _pos.Get(wide_when);
            in_segment.widePeakPoint->timestamp = _timestamps.Get(wide_when);
            in_segment.widePeakPoint->bufferTimestamp = GetTimestamp(wide_when);
            
            in_segment.debugWidePeaksTimestamps.push_back(GetTimestamp(wide_when));
        }
    }
}

//-----------------------------------------------------------------------
void Trajectory::UpdateDetectedGrav(BufferBackPos when, Segment& in_segment)
{
    Vector3D new_pos = _pos.Get(when);
    Vector3D prev_pos = _pos.Get(when + 1);
    
    Coo new_pos_size = new_pos.Size();
    Coo prev_pos_size = prev_pos.Size();
    
    if (new_pos.Size() == 1)
    {
        in_segment.detectedGrav = new_pos;
    }
    else if ((new_pos_size > 1) != (prev_pos_size > 1))
    {
        Coo dsize = (new_pos_size - prev_pos_size);
        in_segment.detectedGrav = (new_pos * ((1 - prev_pos_size) / dsize)) +
                                (prev_pos * ((new_pos_size - 1) / dsize));
        
        auto& grav = in_segment.detectedGrav;
        if ((false)) Log::LogText("grav: z=" + Log::ToStr(grav.z) +
                     ", x=" + Log::ToStr(grav.x) +
                     ", y=" + Log::ToStr(grav.y) +
                     ", size=" + Log::ToStr(grav.Size()) +
                     "(prev=" + Log::ToStr(prev_pos_size) +
                     ", next=" + Log::ToStr(new_pos_size) +
                     ", " + Log::ToStr((int)(100 * (1 - prev_pos_size) / dsize)) + "% of next)" +
                     ", atan=" + Log::ToStr(math::RadToDeg(atan(grav.y / grav.z)), 0));
        
        if ((false)) Log::LogText("new_pos_size=" + Log::ToStr(new_pos_size) +
                                  ", new_pos_size=" + Log::ToStr(new_pos_size) +
                                  ", dsize=" + Log::ToStr(dsize) +
                                  ", gx=" + Log::ToStr(in_segment.detectedGrav.x) +
                                  ", gy=" + Log::ToStr(in_segment.detectedGrav.y) +
                                  ", gz=" + Log::ToStr(in_segment.detectedGrav.z));
    }
}

//-----------------------------------------------------------------------
//bool Trajectory::IsMovingFast(BufferBackPos when)
//{
//    ASSERT(false);
//    return false;
//}

//-----------------------------------------------------------------------
bool Trajectory::IsMovingAgainst(Vector3D against_velocity, BufferBackPos when, BufferBackPos* since_when)
{
    BufferBackPos since = when;
    
    bool use_x = true;
    bool use_y = (_detectionType != SegmentDetectionType_MovingAgainstSegmentInZ);
    bool use_z = true;

    while (IsBackPosInBuffer(since + 1) &&
           RadToDeg(GetLocalVelocity(since).
                    Filtered(use_x, use_y, use_z).
                    AngleTo(against_velocity)) >= 110)
    {
        since++;
    }
    
    if (since > when)
    {
        (*since_when) = since;
        return true;
    }
    
    return false;
}

//-----------------------------------------------------------------------
bool Trajectory::IsMovingAgainst(BufferBackPos against_when, BufferBackPos when, BufferBackPos* since_when)
{
    bool is_against = IsMovingAgainst(GetLocalVelocity(against_when), when, since_when);
    
    if (is_against) DEBUG_ISMOVINGAGAINST_BACKPOS(against_when);
    
    return is_against;
}

//-----------------------------------------------------------------------
bool Trajectory::IsMovingAgainstPeak(BufferBackPos when, BufferBackPos* since_when, Segment& in_segment)
{
    bool is_against = false;
    auto peak = in_segment.peakPoint;
    if (peak && IsPointDataInBuffer(peak))
    {
        is_against = IsMovingAgainst(_velocity.GetByTimestamp(peak->bufferTimestamp), when, since_when);
        
        if (is_against) DEBUG_ISMOVINGAGAINST_BACKPOS(GetBackPos(peak->bufferTimestamp));
    }
    
    auto wide_peak = in_segment.widePeakPoint;
    if (!is_against && wide_peak && IsPointDataInBuffer(wide_peak))
    {
        auto wide_peak_back_pos = GetBackPos(wide_peak->bufferTimestamp);
        auto wide_peak_vel = GetWideVelocity(wide_peak_back_pos);
        is_against = IsMovingAgainst(wide_peak_vel, when, since_when);
        
        if (is_against) DEBUG_ISMOVINGAGAINST_BACKPOS(wide_peak_back_pos);
    }
    
    return is_against;
}

//-----------------------------------------------------------------------
bool Trajectory::IsMovingAgainstPast(BufferBackPos when, BufferBackPos* since_when, Segment& in_segment)
{
    auto up_to_past = GetLastPeakOrStart(in_segment);
    //auto up_to_past = GetBackPos(in_segment.startPoint->bufferTimestamp);
    //up_to_past = min(up_to_past, when + 4);
    
    for (BufferBackPos past = when + 1; past < up_to_past; past++)
    {
        if (IsMovingAgainst(past, when, since_when)) return true;
    }
    
    return false;
}

//-----------------------------------------------------------------------
bool Trajectory::IsMovingAgainstSegment(BufferBackPos when, BufferBackPos* since_when, Segment& in_segment)
{
    BufferBackPos seg_start = GetBackPos(in_segment.startPoint->bufferTimestamp);
    BufferBackPos seg_end = when + 1;
    if (seg_start - seg_end < 4) return false;
    
    Vector3D start_pos = _pos.Get(_pos.IsBackPosOccupied(seg_start) ? seg_start : _pos.GetOccupiedSize() - 1);
    Vector3D seg_vec = _pos.Get(seg_end) - start_pos;
    
    bool is_against = IsMovingAgainst(seg_vec, when, since_when);
    return is_against;
}

//-----------------------------------------------------------------------
bool Trajectory::IsIdling(BufferBackPos when, BufferBackPos* since_when, Segment& in_segment)
{
    auto when_pos = _pos.Get(when);
    auto when_time = _timestamps.Get(when);
    
    if (GetTimestamp(when) < 5) return false;
    
    auto is_idling_since_back_pos = (_isIdlingSince > 0 ? GetBackPos(_isIdlingSince) : when);
    if (!_pos.IsBackPosOccupied(is_idling_since_back_pos))
        is_idling_since_back_pos = _pos.GetOccupiedSize() - 1;
    auto is_idling_since_time = _timestamps.Get(is_idling_since_back_pos);
    bool is_idling = (when_pos - _pos.Get(is_idling_since_back_pos)).Size() <= MAX_IDLE_DPOS;
    
    if (is_idling &&
        when_time - is_idling_since_time >= MIN_IDLE_DURATION)
    {
        *since_when = is_idling_since_back_pos;
        return true;
    }
    else if (!is_idling)
    {
        _isIdlingSince = 0;
    }
    else if (_isIdlingSince == 0)
    {
        _isIdlingSince = GetTimestamp(is_idling_since_back_pos);
    }
    
    return false;
}

//-----------------------------------------------------------------------
bool Trajectory::IsDrifting(BufferBackPos when, BufferBackPos* since_when, Segment& in_segment)
{
    auto when_time = _timestamps.Get(when);
    
    if (GetTimestamp(when) < 5) return false;
    
    auto is_drifting_since_back_pos = (_isDriftingSince > 0 ? GetBackPos(_isDriftingSince) : when);
    if (!_timestamps.IsBackPosOccupied(is_drifting_since_back_pos))
        is_drifting_since_back_pos = _timestamps.GetOccupiedSize() - 1;
    
    auto is_drifting_since_time = _timestamps.Get(is_drifting_since_back_pos);
    bool is_drifting = (_velocity.Get(when).Size() <= MAX_DRIFT_SPEED);
    
    if (is_drifting &&
        when_time - is_drifting_since_time >= MIN_DRIFT_DURATION)
    {
        *since_when = is_drifting_since_back_pos;
        return true;
    }
    else if (!is_drifting)
    {
        _isDriftingSince = 0;
    }
    else if (_isDriftingSince == 0)
    {
        _isDriftingSince = GetTimestamp(is_drifting_since_back_pos);
    }
    
    return false;
}

//-----------------------------------------------------------------------
bool Trajectory::IsAccelerating(BufferBackPos when)
{
    Vector3D v = GetLocalVelocity(when);
    Vector3D prev_v = GetLocalVelocity(when + 1);
    
    if (_pos.GetDiff(when).Size() <= ACC_NOISE_SIZE) return false;
    if (v.Size() <= prev_v.Size()) return false;
    //if (v.DotProduct(prev_v) < 0.0) return false;
    
    return true;
    
    //Vector3D prev2_v = _velocity.Get(when + 2);
    //return (v.Size() <= prev_v.Size() && prev_v.Size() <= prev2_v.Size());
}

//-----------------------------------------------------------------------
bool Trajectory::IsDeccelerating(BufferBackPos when)
{
    Vector3D v = GetLocalVelocity(when);
    Vector3D prev_v = GetLocalVelocity(when + 1);
    
    return (v.Size() < prev_v.Size());
    
    //Vector3D prev2_v = _velocity.Get(when + 2);
    //return (v.Size() <= prev_v.Size() && prev_v.Size() <= prev2_v.Size());
}

//-----------------------------------------------------------------------
bool Trajectory::IsDecceleratingToStop(BufferBackPos when)
{
    ASSERT(false);
    return false;
}


