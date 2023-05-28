#pragma once

#ifdef YOSS_SYSTEM_IOS
    #include <AudioToolbox/AudioToolbox.h>
#endif

#include "../common/Math.h"
#include "../structs/CircularBuffer.h"
#include "../structs/CircularSummedBuffer.h"

#include "Sound.h"
#include "Instrument.h"
#include "SingleBeatInstrument.h"
#include "MultiBeatInstrument.h"
#include "SamplerInstrument.h"
#include "Drone.h"

namespace yoss
{
    namespace sound
    {

        //-----------------------------------------------------------------------
        // Structs and classes:
        class SoundEngine;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Types:
        
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Constants:
        static const int OUTPUT_CHANELS = 2; // Number of output chanels
        
        static const bool USE_COMPRESSOR = true; // Set this flag to enable compressing of output value between -1.0 and 1.0
        static const bool USE_DELAYS     = !true;
        
        static const int MAX_ECHOES = 10; // Max num of simultaneously-played echoes        
        static const Time MIN_ECHO_DELAY = 0.0; // [seconds] Minimal delay of normalized echo
        static const Time MAX_ECHO_DELAY = 1.0; // [seconds] Maximal delay of normalized echo
        static const Time ECHO_DELAY_CHANGE_FACTOR = 1;//0.00001;
        //-----------------------------------------------------------------------
        
        
        
        //-----------------------------------------------------------------------
        class SoundEngine
        {
        public:
            SoundEngine(int samples_per_sec);
            ~SoundEngine();
            
            void AddInstrument(Instrument* instrument, Instrument* add_before = nullptr);
            void RemoveInstrument(Instrument* instrument);
            bool IsPlayingInstrument(Instrument* instrument);
            
            void AddEcho(Time normalized_delay, Volume volume, Volume feedback_volume, BufferBackPos take_average); // normalized_delay: [0.0 - 1.0], volume: [0.0 - 1.0]
            
            // Called regularly by native sound functionality
            // Single buffer is also supported - then output_left == output_right
            void GenerateSlice(OutputSampleType* output_left, OutputSampleType* output_right, int num_samples);
            
        private:
            Frequency _samplesPerSec;
            int _samplesCounter;

            std::vector<Instrument*> _instruments;
            std::mutex _instrumentsListMutex;
            Delays* _delays;
            Compressor* _finalCompressor;
            bool _isFunctional;
        };
    }    
}

