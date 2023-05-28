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
        // sub-class struct SingleBeatInstrument::BeatPartial;
        class SingleBeatInstrument;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Types:
        using math::PartOfOne;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Constants:
        
        //-----------------------------------------------------------------------

        
        //-----------------------------------------------------------------------
        class SingleBeatInstrument : virtual public Instrument
        {
        public:
            //-----------------------------------------------------------------------
            struct BeatPartial
            {
                BeatPartial():
                leftVolume(1.0), rightVolume(1.0), lfoVolume(1.0), overtoneMultiplier(1.0),
                wave(WaveSource::WST_Sine, 0),
                lfo(WaveSource::WST_Sine, 0),
                envelope() {}
                
                Volume leftVolume, rightVolume, lfoVolume;
                Frequency overtoneMultiplier;
                WaveSource wave, lfo;
                Envelope envelope;
            };
            
            static constexpr int MAX_HARMONICS = 10; // Max num of harmonics per instrument
            static constexpr Time RELEASE_FADE_FACTOR = 0.9999;
            static constexpr Time FADE_OUT_BEFORE_ATTACK = 0.004;
            static constexpr Time ATTACK_DELAY = 0.02;
            static constexpr Time ATTACK_LEN = 0.03;
            static constexpr Time DECAY_LEN = 0.05;
            
            SingleBeatInstrument();

            virtual void AddBeat(PartOfOne normalized_freq, Volume volume);
            virtual void SetVolume(Volume volume);
            //virtual void SetPitch(PartOfOne normalized_freq);
            
            virtual StereoSample GenerateSample();
            
        protected:
            Frequency _pitch;
            Frequency _targetPitch;
            Frequency _pitchVelocity;
            Inertia   _pitchInertia;
            SmoothTransition _pitchTransition;
            Inertia   _volumeInertia;
            Volume    _volume;
            bool      _beatIsFinished;
            
            BeatPartial _harmonics[MAX_HARMONICS];
            
            std::mutex _beatMutex;
        };
        
    }    
}

