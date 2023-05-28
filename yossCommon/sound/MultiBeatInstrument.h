#pragma once

#include "Sound.h"
#include "SoundUnit.h"
#include "Instrument.h"
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
        class MultiBeatInstrument;
        // sub-class struct MultiBeatInstrument::BeatPartial;
        // sub-class struct MultiBeatInstrument::Beat;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Types:
        
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Constants:
        static const int HARMONICS_PER_BEAT = 10; // Max num of harmonics of a played beat
        //-----------------------------------------------------------------------
  
        
        //-----------------------------------------------------------------------
        class MultiBeatInstrument : virtual public Instrument
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
            
            //-----------------------------------------------------------------------
            struct Beat
            {
                Frequency fundamentalFreq = 0;
                Volume    volume = 1;
                bool      isFinished = false;
                
                BeatPartial harmonics[HARMONICS_PER_BEAT];
            };
            
            static const int MAX_BEATS = 6; // Max num of simultaneously-played beats
            
            MultiBeatInstrument();
            
            virtual void AddBeat(PartOfOne normalized_freq, Volume volume);
            //virtual void SetPitch(PartOfOne normalized_freq);
            
            virtual StereoSample GenerateSample();

        protected:
            StereoSample GenerateSample_Beat(Beat& beat);

            CircularBuffer<Beat> _beats;
            std::mutex _beatsMutex;
        };
  
    }    
}

