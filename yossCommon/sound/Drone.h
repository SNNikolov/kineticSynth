#pragma once

#include <vector>
#include <map>
#include <mutex>

#include "Sound.h"
#include "SoundUnit.h"
#include "Instrument.h"


namespace yoss
{
    namespace sound
    {
        
        //-----------------------------------------------------------------------
        // Structs and classes:
        struct DronePartial;
        class Drone;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Types:
        
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Constants:

        //-----------------------------------------------------------------------
        

        //-----------------------------------------------------------------------
        struct DronePartial
        {
            DronePartial():
            leftVolume(1.0), rightVolume(1.0), lfoVolume(1.0), overtoneMultiplier(1.0),
            wave(WaveSource::WST_Sine, 0),
            lfo(WaveSource::WST_Sine, 0),
            freqInertia(0.001) {}
            
            Volume leftVolume, rightVolume, lfoVolume;
            Frequency overtoneMultiplier;
            WaveSource wave, lfo;
            HalfWayThere freqInertia;
        };


        //-----------------------------------------------------------------------
        class Drone : public Instrument
        {
        public:
            static const int HARMONICS_PER_DRONE = 3; // Max num of harmonics of a played drone
            static constexpr Frequency MIN_DRONE_PITCH = 10;
            
            Drone();
            
            virtual void SetPitch(PartOfOne normalized_freq);
            
            virtual StereoSample GenerateSample();
            
            DronePartial* GetPartials() { return _harmonics; }
            
        protected:
            Frequency _fundamentalFreq;
            Frequency _pitch;
            Frequency _targetPitch;
            Frequency _pitchVelocity;
            
            DronePartial _harmonics[HARMONICS_PER_DRONE];
        };
    }    
}

