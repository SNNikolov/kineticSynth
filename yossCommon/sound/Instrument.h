#pragma once

#include "Sound.h"
#include "SoundUnit.h"
#include "../structs/CircularSummedBuffer.h"

#include <vector>
#include <map>
#include <mutex>


namespace yoss
{
    namespace sound
    {
        
        //-----------------------------------------------------------------------
        // Structs and classes:
        class Instrument;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Types:
        using math::PartOfOne;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Constants:
        static const Frequency BEAT_MIN_SWING_FREQUENCY = 20.0; // Minimal freq allowed while the freq "swings" around to the minimal value due to inertia effects
        static const Frequency BEAT_FUNDAMENTAL_MIN_FREQUENCY = 200.0; //450.0;
        static const Frequency BEAT_FUNDAMENTAL_MAX_FREQUENCY = 600.0; //14000.0;
        //-----------------------------------------------------------------------
 
        
        
        //-----------------------------------------------------------------------
        // Base class of all instruments
        class Instrument
        {
        public:
            Instrument(): _isSustained(false) {}
            virtual ~Instrument() {}

            virtual void AddBeat(PartOfOne normalized_freq, Volume volume) {}
            virtual void SetSustain(bool do_sustain) { _isSustained = do_sustain; }
            virtual void SetPitch(PartOfOne normalized_freq) {}
            virtual void SetVolume(Volume volume) {}
            
            virtual Frequency UnnormalizeFrequency(PartOfOne normalized_freq);
            
            virtual StereoSample GenerateSample() = 0;
            
        protected:
            bool _isSustained;
        };
 
    }    
}

