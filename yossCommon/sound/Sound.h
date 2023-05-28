#pragma once

#include "../common/Config.h"

#ifdef YOSS_SYSTEM_IOS
    #include <AudioToolbox/AudioToolbox.h>
#endif

#include "../common/Math.h"
#include "../structs/CircularBuffer.h"

namespace yoss
{
    namespace sound
    {
        using yoss::math::Time;
        using yoss::math::Angle;
        using yoss::math::AngularVelocity;
        using yoss::math::Frequency;
        using yoss::math::Ratio;
        using yoss::math::PartOfOne;
        
        //-----------------------------------------------------------------------
        // Structs and classes:
        struct StereoSample;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Types:
        typedef double Volume; // [1.0 = max volume]
        typedef double Sample;
#ifdef YOSS_SYSTEM_IOS
        typedef Float32 OutputSampleType;
#else
        typedef float OutputSampleType;
#endif // YOSS_SYSTEM_IOS
        
        typedef CircularBuffer<Sample>::BackPos BufferBackPos;
        typedef CircularBuffer<Sample>::Timestamp BufferTimestamp;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Constants:
        static constexpr Sample SEMITONE_RATIO   = 1.059463094359295;
        static constexpr Sample SEMITONE_RATIO2  = SEMITONE_RATIO * SEMITONE_RATIO;
        static constexpr Sample SEMITONE_RATIO3  = SEMITONE_RATIO2 * SEMITONE_RATIO;
        static constexpr Sample SEMITONE_RATIO4  = SEMITONE_RATIO3 * SEMITONE_RATIO;
        static constexpr Sample SEMITONE_RATIO5  = SEMITONE_RATIO4 * SEMITONE_RATIO;
        static constexpr Sample SEMITONE_RATIO6  = SEMITONE_RATIO5 * SEMITONE_RATIO;
        static constexpr Sample SEMITONE_RATIO7  = SEMITONE_RATIO6 * SEMITONE_RATIO;
        static constexpr Sample SEMITONE_RATIO8  = SEMITONE_RATIO7 * SEMITONE_RATIO;
        static constexpr Sample SEMITONE_RATIO9  = SEMITONE_RATIO8 * SEMITONE_RATIO;
        static constexpr Sample SEMITONE_RATIO10 = SEMITONE_RATIO9 * SEMITONE_RATIO;
        static constexpr Sample SEMITONE_RATIO11 = SEMITONE_RATIO10 * SEMITONE_RATIO;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Helper methods:
        std::vector<Sample> ConvertBuffer_Int16ToSample(void* int16_buffer, int buffer_byte_size);
        //-----------------------------------------------------------------------
        
        
        //-----------------------------------------------------------------------
        struct StereoSample
        {
            Sample left;
            Sample right;
            
            StereoSample(): left(0), right(0) {};
            StereoSample(const Sample& left_and_right): left(left_and_right), right(left_and_right) {};
            StereoSample(const Sample& left, const Sample& right): left(left), right(right) {};
            StereoSample& operator += (const StereoSample& another) { left += another.left; right += another.right; return *this; }
            StereoSample& operator *= (const StereoSample& another) { left *= another.left; right *= another.right; return *this; }
            StereoSample& operator *= (const Sample& scalar) { left *= scalar; right *= scalar; return *this; }
        };
        
    }
}

