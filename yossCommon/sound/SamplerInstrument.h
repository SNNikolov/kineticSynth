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
        class SamplerInstrument;
        // sub-class struct SamplerInstrument::Beat;
        // sub-class struct SamplerInstrument::SamplerSample;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Types:
        
        //-----------------------------------------------------------------------
        
        
        
        //-----------------------------------------------------------------------
        class SamplerInstrument : virtual public Instrument
        {
        public:
            //-----------------------------------------------------------------------
            struct SamplerSample
            {
                std::vector<Sample> buffer;
                int offsetInBuffer;
                int lenInBuffer;
                Frequency nativeFrequency;
                Volume    nativeVolume;
            };
            
            //-----------------------------------------------------------------------
            struct Beat
            {
                Beat():
                    wave(WaveSource::WST_StereoSample)
                {}
                
                Frequency fundamentalFreq = 0;
                Frequency speedMultiplier = 1;
                bool      isFinished = false;
                
                Volume leftVolume = 1;
                Volume rightVolume = 1;
                WaveSource wave;
            };
            
            //-----------------------------------------------------------------------
            // Constants:
            static const int MAX_BEATS = 6; // Max num of simultaneously-played beats
            static constexpr Frequency DEFAULT_SAMPLE_NATIVE_FREQUENCY = 440;
            
            
            //-----------------------------------------------------------------------
            SamplerInstrument(Sample* sample_buffer = nullptr, int samples_num = 0, Frequency native_frequency = DEFAULT_SAMPLE_NATIVE_FREQUENCY);
            
            virtual void LoadSample(const std::string& file, bool is_stereo, Frequency native_freq, Volume native_vol, int start_offset = 0, int samples_num = -1);
            virtual SamplerSample& GetSample(int sample_index) { ASSERT(sample_index >= 0 && sample_index < _samples.size()); return _samples[sample_index]; }
            int GetSamplesNum() { return (int)_samples.size(); }
            std::vector<SamplerSample>& GetSamples() { return _samples; }
            
            virtual void SetCurrentSampleData(const Sample* sample_buffer, int samples_num = 0);
            virtual void SetCurrentSampleNativeFrequency(Frequency native_frequency);
            
            virtual void AddBeat(PartOfOne normalized_freq, Volume volume);
            virtual void AddBeat(PartOfOne normalized_freq, Volume volume, const SamplerSample& sample);
            CircularBuffer<Beat>& GetBeats() { return _beats; }
            
            virtual StereoSample GenerateSample();

        protected:
            StereoSample GenerateSample_Beat(Beat& beat);

            CircularBuffer<Beat> _beats;
            std::mutex _beatsMutex;
            
            std::vector<SamplerSample> _samples;
            
            const Sample* _sampleBuffer;
            int _sampleBufferSize;
            Frequency _nativeFreq;
        };
  
    }    
}

