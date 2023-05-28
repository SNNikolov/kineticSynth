#include "SamplerInstrument.h"
#include "../common/System.h"

#include <iterator>
#include <algorithm>

using namespace yoss;
using namespace yoss::math;
using namespace yoss::sound;


//-----------------------------------------------------------------------
// Static defines, consts and vars

//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// Static members

//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
SamplerInstrument::SamplerInstrument(Sample* sample_buffer, int samples_num, Frequency native_frequency) :
    _beats(MAX_BEATS, false),
    _sampleBuffer(sample_buffer),
    _sampleBufferSize(samples_num),
    _nativeFreq(native_frequency)
{
}

//-----------------------------------------------------------------------
void SamplerInstrument::LoadSample(const std::string& file, bool is_stereo, Frequency native_freq, Volume native_vol, int start_offset, int samples_num)
{
    ASSERT(is_stereo); // ToDo: make WaveSource handle mono samples
    
    auto sample_contents = system::LoadFile(system::GetResourcePath(file));
    
    _samples.push_back(SamplerSample());
    auto& new_sample = _samples.back();
    new_sample.buffer = ConvertBuffer_Int16ToSample(sample_contents.data(), sample_contents.size());
    new_sample.offsetInBuffer = start_offset;
    new_sample.lenInBuffer = (samples_num >= 0 ? samples_num : new_sample.buffer.size() - start_offset);
    new_sample.nativeFrequency = native_freq;
    new_sample.nativeVolume = native_vol;
}

//-----------------------------------------------------------------------
void SamplerInstrument::SetCurrentSampleData(const Sample* sample_buffer, int samples_num)
{
    _sampleBuffer = sample_buffer;
    _sampleBufferSize = samples_num;
}

//-----------------------------------------------------------------------
void SamplerInstrument::SetCurrentSampleNativeFrequency(Frequency native_frequency)
{
    _nativeFreq = native_frequency;
}

//-----------------------------------------------------------------------
void SamplerInstrument::AddBeat(PartOfOne normalized_freq, Volume volume, const SamplerSample& sample)
{
    SetCurrentSampleData(sample.buffer.data() + sample.offsetInBuffer, sample.lenInBuffer);
    SetCurrentSampleNativeFrequency(sample.nativeFrequency);
    
    SamplerInstrument::AddBeat(normalized_freq, volume / sample.nativeVolume);
}

//-----------------------------------------------------------------------
void SamplerInstrument::AddBeat(PartOfOne normalized_freq, Volume volume)
{
    volume *= 0.3;
    volume *= volume;
    volume = (volume > 1.0 ? 1.0 : volume);
    
    Beat beat;
    beat.fundamentalFreq = UnnormalizeFrequency(normalized_freq);
    beat.leftVolume = beat.rightVolume = volume;
    beat.speedMultiplier = beat.fundamentalFreq / _nativeFreq;
    
    // Initialize partial's units
    beat.wave.SetSample(_sampleBuffer, _sampleBufferSize);
    beat.wave.SetSamplePlaySpeed(beat.speedMultiplier);
    
    if ((false))
        Log::LogText("Spawning sampler beat: speedMultiplier=" + Log::ToStr(beat.speedMultiplier, 2) +
                     ", vol=" + Log::ToStr(volume, 2));
    
    std::lock_guard<std::mutex> lock(_beatsMutex);
    
    // Free data of beat that will drop out of buffer
    if (false && _beats.GetOccupiedSize() == MAX_BEATS)
    {
        //auto& beat_to_drop_out_of_buffer = _beats.Get(_beats.GetOccupiedSize() - 1);
    }
    
    _beats.Push(beat);
}

//-----------------------------------------------------------------------
StereoSample SamplerInstrument::GenerateSample_Beat(Beat& beat)
{
    auto wave_output = (beat.speedMultiplier == 1) ?
        beat.wave.UpdateStereoFixedSpeed() : beat.wave.UpdateStereo();
    beat.isFinished = beat.wave.SampleFinished();
    
    return StereoSample(
        wave_output.left * beat.leftVolume,
        wave_output.right * beat.rightVolume);
}

//-----------------------------------------------------------------------
StereoSample SamplerInstrument::GenerateSample()
{
    const int beats_num = _beats.GetOccupiedSize();
    StereoSample output_sample;
    
    std::lock_guard<std::mutex> lock(_beatsMutex);
    
    for (int beat_i = 0; beat_i < beats_num; beat_i++)
    {
        auto& beat = _beats.Get(beat_i); //beats_num - beat_i - 1);
        if (!beat.isFinished && (beat.leftVolume != 0 || beat.rightVolume != 0))
            output_sample += GenerateSample_Beat(beat);
    }
    
    return output_sample;
}

