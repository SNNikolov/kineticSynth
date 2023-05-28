#include <iostream>

#include "AccEngine.h"
#include "../common/Config.h"
#include "../common/Log.h"
#include "../common/System.h"
#include "../bridge/CppBridge.h"

using namespace std;
using namespace yoss;
using namespace yoss::math;
using namespace yoss::trajectories;
using namespace yoss::input;
using namespace yoss::graphics;
using namespace yoss::sound;

//-----------------------------------------------------------------------
AccEngine::AccEngine() :
    _accTrajectory(SegmentDetectionType_MovingAgainstSegmentInZ),
    _magTrajectory(SegmentDetectionType_None),
    _gyroTrajectory(SegmentDetectionType_None),
    _prevFeedTimestamp(system::GetCurrentTimestamp()),
    _currentFeedTimestamp(system::GetCurrentTimestamp()),
    _prevSegEndTimestamp(0),
    _prevSegAmplitude(0),
    _prevSegAmplitude2(0),
    _prevSegAvgVelocity(0),
    _prevSegAvgVelocity2(0),
    _prevSegIsAntiBeat(false),
    _prevBeatTimestamp(0),
    _prevBeatAmplitude(0),
    _prevBeatSegment(nullptr),
    _prevSegment(nullptr),
    _freezeDrawingAt(0)
{
    Log::ClearGr();
    
    Log::LogText("AccEngine constructed");
}

//-----------------------------------------------------------------------
AccEngine::~AccEngine()
{
}

//-----------------------------------------------------------------------
void AccEngine::AddAccFeedListener(FeedListener listener)
{
    _accFeedListeners.push_back(listener);
}

//-----------------------------------------------------------------------
void AccEngine::RemoveAccFeedListener(FeedListener& listener)
{
    ASSERT(false); // ToDo: implement how to compare lambdas
    
    auto it = _accFeedListeners.begin();
    while (it != _accFeedListeners.end())
        //if (*it == listener)
    {
        _accFeedListeners.erase(it);
        return;
    }
    
    ASSERT(false); // Listener was not found
}

//-----------------------------------------------------------------------
void AccEngine::ResetInput()
{
    ::OrderResetAcc();
}

//-----------------------------------------------------------------------
void AccEngine::FeedAcc(double acc_x, double acc_y, double acc_z, double gyro_x, double gyro_y, double gyro_z, double mag_x, double mag_y, double mag_z)
{
    _prevFeedTimestamp = _currentFeedTimestamp;
    _currentFeedTimestamp = system::GetCurrentTimestamp();
    
    //auto dt = (_currentFeedTimestamp - _prevFeedTimestamp);
    //Time dt_microseconds = dt * 1000000.0;
    //Log::LogText("dt=" + Log::ToStr((double)dt_microseconds));
    
    //static int ccc = 0;
    //if ((ccc = ccc++ % 20) == 0)
    //    Log::LogText("pitch=" + Log::ToStr(mag_x) + ", roll=" + Log::ToStr(mag_y) + ", yaw=" + Log::ToStr(mag_z));
    
    //double fake_scale = .3;
    //acc_x = gyro_x; acc_y = gyro_y; acc_z = gyro_z;
    //acc_x = mag_y * fake_scale; acc_y = mag_x * fake_scale; acc_z = mag_z * fake_scale;
    
    Vector3D acc(acc_x, acc_y, acc_z);
    _accTrajectory.FeedNewPosition(acc, _currentFeedTimestamp);
    _accTrajectory.Update();
    
    Vector3D mag(mag_x, mag_y, mag_z);
    _magTrajectory.FeedNewPosition(mag, _currentFeedTimestamp);
    _magTrajectory.Update();
    
    Vector3D gyro(gyro_x, gyro_y, gyro_z);
    _gyroTrajectory.FeedNewPosition(gyro, _currentFeedTimestamp);
    _gyroTrajectory.Update();

    //Vector3D d_acc = _accTrajectory.GetVelocities().Get(0);
    //Vector3D prev_d_acc = _accTrajectory.GetVelocities().Get(1);
    //Vector3D dd_acc = _accTrajectory.GetAccelerations().Get(0);
    //acc = _accBuffer.GetAverage(0, 1);
    //d_acc = _dAccBuffer.GetAverage(0, 2);
    //prev_d_acc = _dAccBuffer.GetAverage(1, 3);
    //d_acc.Normalize();
    //prev_d_acc.Normalize();
    //double d_acc_angle = d_acc.AngleTo(prev_d_acc) * 180.0 / M_PI;
    //if (d_acc.Size() < 0.1 || prev_d_acc.Size() < 0.1) d_acc_angle = 90;
    
    if ((0))
    Log::LogValues({
        //d_acc_angle,
        //d_acc.Size(),
        //(acc.Size() - 1.0) * 30.0 + 90.0,
        acc.y * 30.0 + 90.0,
        acc.z * 30.0 + 90.0
    }, 0, 180);
    //Log::LogText(Log::ToStr(_dAcc.Angle(_dAccPrev)));

    //Log::LogValues({
    //     (acc_size - 1),
    //     acc_z
         //acc_y
    //}, -4, 4);
    
    //Log::LogText("acc.z=" + Log::ToStr(acc.z) + ",    prev_acc.z=" + Log::ToStr(prev_acc.z) + ",    d_acc.z=" + Log::ToStr(d_acc.z));
 
    bool beat_is_detected = false;
    
    if (DETECT_BEATS)
    {
        if (_accTrajectory.SegmentJustEnded())
        {
            auto& seg = _accTrajectory.GetSegments().Get(1);
            
            beat_is_detected = IsSegmentABeat(seg);
            
            auto seg_vec = seg.GetDPos();
            Coo beat_amplitude = seg_vec.Size();
            Time beat_duration = seg.GetDuration();
            Coo beat_avg_velocity = seg.GetAvgVelocitySize();
            
            if (beat_is_detected)
            {
                _prevBeatSegment = &seg;
                _prevBeatAmplitude = beat_amplitude;
                _prevBeatTimestamp = _currentFeedTimestamp;
                _prevSegIsDrawn = false;
                
                if ((false))
                Log::LogText("beat_dur=" + Log::ToStr(beat_duration) +
                             "=" + Log::ToStr(seg.endPoint->bufferTimestamp - seg.startPoint->bufferTimestamp) +
                             ", beat_amp=" + Log::ToStr(beat_amplitude) +
                             //", beat_freq=" + Log::ToStr(freq) +
                             ", beat_avg_vel=" + Log::ToStr(beat_avg_velocity) +
                             //", min amp=" + Log::ToStr(min_amplitude_debug) +
                             //", seg_angle_to_z=" + Log::ToStr(seg_angle_to_z) +
                             ", prev_amp=" + Log::ToStr(_prevSegAmplitude) +
                             ", prev_antiB=" + Log::ToStr(_prevSegIsAntiBeat ? 1 : 0) +
                             ", prev_dur=" + Log::ToStr(_prevSegment->endPoint->bufferTimestamp - _prevSegment->startPoint->bufferTimestamp)
                );
            }
            
            _prevSegAmplitude2 = _prevSegAmplitude;
            _prevSegAvgVelocity2 = _prevSegAvgVelocity;
            
            _prevSegment = &seg;
            _prevSegAmplitude = beat_amplitude;
            _prevSegAvgVelocity = beat_avg_velocity;
            _prevSegEndTimestamp = _currentFeedTimestamp;
            _prevSegIsAntiBeat = (seg_vec.z > 0);
            //_prevSegIsDrawn = false;
        }
        
        if (DRAW_BEAT_SEGMENTS_IN_CONSOLE)
        {
            Segment* draw_seg = _prevBeatSegment; //_prevBeatSegment;
            if (draw_seg && !_prevSegIsDrawn) // && !_prevSegIsAntiBeat)
            {
                if (draw_seg->endPoint->bufferTimestamp <= _accTrajectory.GetPositions().GetTimestamp() - 6)
                {
                    //DrawSegmentInConsole(*draw_seg);
                    
                    _prevSegIsDrawn = true;
                }
            }
        }
    }
    
    for (auto listener : _accFeedListeners)
        listener(beat_is_detected);
}

//-----------------------------------------------------------------------
// Note: Might be potentially expensive to call on every frame (due to calling _accTrajectory.CalcSegmentParams)
bool AccEngine::IsSegmentABeat(trajectories::Segment& seg)
{
    trajectories::Point seg_end_point(_accTrajectory.GetPositions().Get(), _accTrajectory.GetTimestamps().Get(), _accTrajectory.GetTimestamps().GetTimestamp());
    bool temp_end_point = (seg.endPoint == nullptr);
    if (temp_end_point)
    {
        seg.endPoint = &seg_end_point;
        
        _accTrajectory.CalcSegmentParams(seg);
    }
    
    auto seg_vec = seg.GetDPos();
    auto seg_angle_to_z = RadToDeg(seg_vec.AngleTo(Z_AXIS));
    
    auto seg_start_backpos = _gyroTrajectory.GetBackPos(seg.startPoint->bufferTimestamp);
    auto seg_end_backpos = _gyroTrajectory.GetBackPos(seg.endPoint->bufferTimestamp);
    auto gyro_vec = _gyroTrajectory.GetPositions().GetSum(seg_end_backpos, seg_start_backpos);
    Ratio gyro_y_strength = (gyro_vec.x == 0 ? 1000000 : abs(gyro_vec.y) / abs(gyro_vec.x));
    
    Coo beat_amplitude = seg_vec.Size();
    Time beat_duration = seg.GetDuration();
    Coo beat_avg_velocity = seg.GetAvgVelocitySize();
    //Coo beat_dy = (seg.endPoint->absPos.y + seg.startPoint->absPos.y) * 0.5 - seg.weightCenter.y;
    
    if (temp_end_point)
        seg.endPoint = nullptr;
    
    Coo min_avg_velocity = Interpolate(MIN_SMALL_BEAT_AVG_VELOCITY, MIN_BIG_BEAT_AVG_VELOCITY,
                                       MIN_BEAT_AMPLITUDE, MAX_BEAT_AMPLITUDE, beat_amplitude);
    Coo min_amplitude = MIN_BEAT_AMPLITUDE;
    Coo min_amplitude_debug = min_amplitude;
    if (_prevSegIsAntiBeat)
    {
        //min_amplitude = _prevSegAmplitude * 0.7; // / ((_currentFeedTimestamp - _prevSegEndTimestamp) * 10);
        min_amplitude_debug = min_amplitude;
        min_amplitude = max(min_amplitude, MIN_BEAT_AMPLITUDE);
        
        min_avg_velocity = max(min_avg_velocity, _prevSegAvgVelocity * 0.75f);
    }
    
    //if (_prevSegAmplitude + _prevSegAmplitude2 < beat_amplitude * 0.1)
    if (_prevSegAmplitude < beat_amplitude * 0.1)
    {
        // Don't fire a beat if the previous segments were too small, i.e. no acceleration
        min_amplitude = 1000;
    }
    
    bool beat_detected = (beat_amplitude >= min_amplitude &&
                          beat_avg_velocity >= min_avg_velocity &&
                          //(_prevSegIsAntiBeat || ) &&
                          seg_angle_to_z > 180 - AccSegmentIsABeatMaxAngleToZ &&
                          gyro_y_strength < 1.0);
    if (Config::DebugAccSegmentIsABeat && beat_amplitude >= MIN_BEAT_AMPLITUDE)
        Log::LogText("IsSegmentABeat(is=" + string(beat_detected ? "T" : "F") +
                     "): amp=" + Log::ToStr(beat_amplitude, 2) +
                     ", min_amp=" + Log::ToStr(min_amplitude, 2) +
                     (beat_amplitude >= min_amplitude ? "" : "(F)") +
                     ", avg_vel=" + Log::ToStr(beat_avg_velocity, 2) +
                     ", min_avg_vel=" + Log::ToStr(min_avg_velocity, 2) +
                     (beat_avg_velocity >= min_avg_velocity ? "" : "(F)") +
                     ", gyro_y=" + Log::ToStr(gyro_y_strength, 2) +
                     (gyro_y_strength < 1.0 ? "" : "(F)") +
                     ", a_to_z=" + Log::ToStr(seg_angle_to_z, 2) +
                     (seg_angle_to_z > 180 - AccSegmentIsABeatMaxAngleToZ ? "" : "(F)"));
    if (beat_detected && gyro_y_strength > 1.0)
        Log::LogText("!!!!!!!!!");
    
    return beat_detected;
}

//-----------------------------------------------------------------------
Coo AccEngine::GetAccBeatAmplitude()
{
    ASSERT(_prevBeatSegment);
    auto seg_vec = _prevBeatSegment->GetDPos();
    
    Coo beat_amplitude = seg_vec.Size();
    beat_amplitude = MIN(beat_amplitude, MAX_BEAT_AMPLITUDE);
    return beat_amplitude;
}

//-----------------------------------------------------------------------
Frequency AccEngine::GetAccBeatFrequency()
{
    ASSERT(_prevBeatSegment);
    auto center = _prevBeatSegment->middleOfStartEnd;
    
    Frequency freq = (PointToDeg(-center.z, -center.y) - BEATS_SCALE_START_ANGLE) / (BEATS_SCALE_END_ANGLE - BEATS_SCALE_START_ANGLE);
    freq = MIN(1, MAX(0, freq));
    return freq;
}

//-----------------------------------------------------------------------
Frequency AccEngine::GetAccNextBeatFrequency()
{
    auto acc_pos = _accTrajectory.GetPositions().GetAverage(0, 20);
    
    Frequency freq = (PointToDeg(-acc_pos.z, -acc_pos.y) - BEATS_SCALE_START_ANGLE) / (BEATS_SCALE_END_ANGLE - BEATS_SCALE_START_ANGLE);
    freq = MIN(1, MAX(0, freq));
    return freq;
}

//-----------------------------------------------------------------------
bool AccEngine::IsExpectingBeat()
{
    auto& seg = _accTrajectory.GetSegments().Get(0);
    bool seg_is_beat = IsSegmentABeat(seg);
    return seg_is_beat;
}

//-----------------------------------------------------------------------
void AccEngine::DrawTrajectory(graphics::Graphics* graphics)
{
    ASSERT(graphics);
    static const CooMultiplier DRAW_SCALE = 0.4;
    
    //graphics->StartFrame();
    graphics->SetLineWidth(3.0);
    graphics->PushCurrentTransforms(ZERO_VECTOR, ZERO_VECTOR, Vector3D(-DRAW_SCALE, DRAW_SCALE, 1));
    
    // Draw zero grid
    Vector3D v1(-2, 0, 0);
    Vector3D v2(2, 0, 0);
    Color color(0.3, 0.3, 0.3, 1);
    graphics->AddLine(v1, v2, color, color);
    
    v1 = Vector3D(0, -2, 0);
    v2 = Vector3D(0, 2, 0);
    graphics->AddLine(v1, v2, color, color);
    
    // Draw circle with radius 1
    for (Angle a = 0; a <= math::PI * 2; a += math::ONE_DEG * 5)
    {
        v2 = v1;
        v1 = Vector3D(sin(a), cos(a), 0);
        if (a > 0)
            graphics->AddLine(v1, v2, color, color);
    }
    
    // Draw trajectory
    auto& positions = _accTrajectory.GetPositions();
    auto& velocities = _accTrajectory.GetVelocities();
    auto& timestamps = _accTrajectory.GetTimestamps();
    auto& segments = _accTrajectory.GetSegments();
    
    auto& gyro_positions = _gyroTrajectory.GetPositions();
    auto& mag_positions = _magTrajectory.GetPositions();
    
    const int DRAW_LEN = 170;
    BufferBackPos from_backpos = (_freezeDrawingAt > 0 ? positions.GetBackPos(_freezeDrawingAt) : 0);
    BufferBackPos to_backpos = min(DRAW_LEN + from_backpos, timestamps.GetOccupiedSize() - 2);
    
    Segment* seg = nullptr;//_accTrajectory.GetSegmentAt(positions.GetTimestamp(to_backpos));

    for (BufferBackPos i = to_backpos; i > from_backpos; i--)
    {
        auto pos = positions.Get(i);
        auto next_pos = positions.Get(i - 1);
        auto vel = velocities.Get(i);
        auto timestamp = positions.GetTimestamp(i);
        auto colori = max(i - from_backpos, 0);
        
        ColorC red = 1;
        ColorC green = 1;//min(1.0, 7.0 / sqrt(colori));//min(1.0, 2.0 / sqrt(vel.Size()));
        ColorC blue = 0;
        ColorC alpha = min(1.0, (double)(DRAW_LEN - colori) / DRAW_LEN);
        
        if (!seg)
            seg = _accTrajectory.GetSegmentAt(positions.GetTimestamp(i));

        if (seg)
        {
            int segs_num = 1;
            if (seg->endPoint && timestamp == seg->endPoint->bufferTimestamp)
            {
                ASSERT(segments.GetNext(seg));
                ASSERT(segments.GetNext(seg) != seg);
                
                Segment* next_seg = seg;
                while (next_seg->endPoint && timestamp == next_seg->endPoint->bufferTimestamp)
                {
                    next_seg = segments.GetNext(next_seg);
                    segs_num++;
                }
                segs_num--;
                
                ASSERT(next_seg->startPoint->bufferTimestamp == seg->endPoint->bufferTimestamp);
                ASSERT(!next_seg->endPoint || next_seg->endPoint->bufferTimestamp != seg->endPoint->bufferTimestamp);
                
                seg = next_seg;
            }
            
            if (timestamp == seg->startPoint->bufferTimestamp)
            {
                // Draw marker
                Color color(0, 0, 1, 1);
                Coo mark_r = 0.03 * segs_num;
                Vector3D mark_pos = pos;
                Vector3D v1(mark_pos.y + mark_r, mark_pos.z, 0);
                Vector3D v2(mark_pos.y - mark_r, mark_pos.z, 0);
                graphics->AddLine(v1, v2, color, color);
                
                v1 = Vector3D(mark_pos.y, mark_pos.z - mark_r, 0);
                v2 = Vector3D(mark_pos.y, mark_pos.z + mark_r, 0);
                graphics->AddLine(v1, v2, color, color);
            }
            
            ASSERT(timestamp >= seg->startPoint->bufferTimestamp);
            ASSERT(!seg->endPoint || timestamp <= seg->endPoint->bufferTimestamp);
            
            Vector3D seg_vec = seg->endPoint ? seg->GetDPos() : -1.0 * Z_AXIS;
            if (seg_vec.AngleTo(Z_AXIS) < 90 * ONE_DEG)
            {
                red = 0.4;
            }
        }
        
        Color color(red, green, blue, alpha);
        Vector3D v1(pos.y, pos.z, 0);
        Vector3D v2(next_pos.y, next_pos.z, 0);
        graphics->AddLine(v1, v2, color, color);
        
        if (0)
        {
            Color color2(red, green, 1, alpha);
            v1.x = pos.y; v1.y = pos.x;
            v2.x = next_pos.y; v2.y = next_pos.x;
            graphics->AddLine(v1, v2, color2, color2);
        }
        
        if (1)
        {
            auto gyro_pos = gyro_positions.Get(i) * 0.2;
            auto gyro_next_pos = gyro_positions.Get(i - 1) * 0.2;
            
            Color color2 = COLOR_YELLOW.WithAlpha(alpha);
            v1.x = gyro_pos.y; v1.y = gyro_pos.x;
            v2.x = gyro_next_pos.y; v2.y = gyro_next_pos.x;
            graphics->AddLine(v1, v2, color2, color2);
        }

        if (1)
        {
            auto mag_pos = mag_positions.Get(i) * 0.005;
            auto mag_next_pos = mag_positions.Get(i - 1) * 0.005;

            Color color2 = COLOR_RED.WithAlpha(alpha);
            v1.x = mag_pos.y; v1.y = mag_pos.x;
            v2.x = mag_next_pos.y; v2.y = mag_next_pos.x;
            graphics->AddLine(v1, v2, color2, color2);
        }
    }
}

//-----------------------------------------------------------------------
void AccEngine::DrawSegmentInConsole(Segment segment)
{
    Log::ClearGr();
    
    static const char C_DEFAULT = '*';
    static const char C_SEG_START = '[';
    static const char C_SEG_END = ']';
    static const char C_BOTH_ENDS = 'S';
    static const char C_SEG_MIDDLE = '@';
    static const char C_PEAK = 'p';
    static const char C_WIDE_PEAK = 'w';
    static const char C_BOTH_PEAKS = 'W';
    
    auto& positions = _accTrajectory.GetPositions();
    
    const int show_prevs_num = 50;
    for (auto prev_t = max(segment.startPoint->bufferTimestamp - show_prevs_num, 0); prev_t < segment.startPoint->bufferTimestamp; prev_t++)
    {
        auto prev_back_t = positions.GetBackPos(prev_t);
        auto prev_acc = positions.Get(prev_back_t);
        char prev_pos_char = '1' + (char)(min(9, segment.startPoint->bufferTimestamp - prev_t) - 1);
        Log::AddGrPoint(prev_acc.z, -prev_acc.y, prev_pos_char);
    }
    
    const int show_nexts_num = 6;
    char next_pos_char = '1' + (char)(show_nexts_num - 1);
    for (auto next_t = min(segment.endPoint->bufferTimestamp + show_nexts_num, positions.GetTimestamp()); next_t > segment.endPoint->bufferTimestamp; next_t--)
    {
        auto next_back_t = positions.GetBackPos(next_t);
        auto next_acc = positions.Get(next_back_t);
        Log::AddGrPoint(next_acc.z, -next_acc.y, next_pos_char);
        
        next_pos_char--;
    }
    
    Vector3D seg_middle = segment.weightCenter;
    Vector3D seg_middle2 = segment.middleOfStartEnd;
    
    for (auto t = segment.startPoint->bufferTimestamp; t <= segment.endPoint->bufferTimestamp; t++)
    {
        auto back_t = positions.GetBackPos(t);
        auto acc = positions.Get(back_t);

        bool is_peak = (FOUND_IN_VECTOR(segment.debugPeaksTimestamps, t));
        bool is_wide_peak = (FOUND_IN_VECTOR(segment.debugWidePeaksTimestamps, t));
        char is_start = (t == segment.startPoint->bufferTimestamp);
        char is_end = (t == segment.endPoint->bufferTimestamp);
        char pos_char = C_DEFAULT; //(is_peak ? (is_wide_peak ? C_BOTH_PEAKS : C_PEAK) : (is_wide_peak ? C_WIDE_PEAK : C_DEFAULT));
        pos_char = (is_start ? (is_end ? C_BOTH_ENDS : C_SEG_START) : (is_end ? C_SEG_END : pos_char));

        Log::AddGrPoint(acc.z, -acc.y, pos_char);
    }
    
    //Log::AddGrPoint(seg_middle.z, -seg_middle.y, '1');
    Log::AddGrPoint(seg_middle2.z, -seg_middle2.y, C_SEG_MIDDLE);
    
    Log::DrawGr();
    
    if ((0)) Log::LogText("mid1 angle = " +
                 Log::ToStr(PointToDeg(-seg_middle.z, -seg_middle.y), 0) +
                 ", mid2 angle = " +
                 Log::ToStr(PointToDeg(-seg_middle2.z, -seg_middle2.y), 0));
    
    if ((0))
    {
        auto end_back_pos = positions.GetBackPos(segment.endPoint->bufferTimestamp);
        Vector3D dpos = positions.GetDiff(end_back_pos - 1);
        auto seg_vec = segment.GetDPos();
        if (dpos.Size() > ACC_NOISE_SIZE)
        {
            Log::LogText("seg_vec=(x:" + Log::ToStr(seg_vec.x) +
                         ", y:" + Log::ToStr(seg_vec.y) +
                         ", z:" + Log::ToStr(seg_vec.z) +
                         "), dpos=(x:" + Log::ToStr(dpos.x) +
                         ", y:" + Log::ToStr(dpos.y) +
                         ", z:" + Log::ToStr(dpos.z) +
                         "), overnoise" +
                         ", len:" + Log::ToStr(segment.traveledLength));
        }
        else
        {
            Vector3D prev_dpos = positions.GetDiff(end_back_pos);
            Log::LogText("seg_vec=(x:" + Log::ToStr(seg_vec.x) +
                         ", y:" + Log::ToStr(seg_vec.y) +
                         ", z:" + Log::ToStr(seg_vec.z) +
                         "), prev_dpos=(x:" + Log::ToStr(prev_dpos.x) +
                         ", y:" + Log::ToStr(prev_dpos.y) +
                         ", z:" + Log::ToStr(prev_dpos.z) +
                         "), len:" + Log::ToStr(segment.traveledLength));
        }
    }
}
