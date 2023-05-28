#include "MultiBeatInstrument.h"

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
MultiBeatInstrument::MultiBeatInstrument() :
    _beats(MAX_BEATS, false)
{
}

//-----------------------------------------------------------------------
void MultiBeatInstrument::AddBeat(PartOfOne normalized_freq, Volume volume)
{
    volume *= 0.3;
    volume *= volume;
    volume = (volume > 1.0 ? 1.0 : volume);
    
    Frequency fundamental_freq = UnnormalizeFrequency(normalized_freq);
    
    Beat beat;
    beat.fundamentalFreq = fundamental_freq;
    beat.volume = volume;
    
    Time release_fade_factor = 0.9999;//0.9998;//Interpolate(0.9995, 0.9998, volume);
    Time attack_delay = 0.04;
    Time attack_len = 0.05;//Interpolate(0.1, 0.05, volume);
    Time decay_len = 0.05;//Interpolate(0.1, 0.1, volume);
    static const double HARMONIC_MULTIPLIER[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    
    if ((!false))
        Log::LogText("Spawning beat: fundamental=" + Log::ToStr(fundamental_freq) +
                     ", vol=" + Log::ToStr(volume) +
                     ", envReleaseFactor=" + Log::ToStr(release_fade_factor, 6));
    
    for (int i = 0; i < HARMONICS_PER_BEAT; i++)
    {
        auto& harmonic = beat.harmonics[i];
        
        if (i == 0)
        {
            // Fundamental frequency
            harmonic.leftVolume = harmonic.rightVolume = 1;
            harmonic.overtoneMultiplier = 1;
        }
        else
        {
            // Overtone
            harmonic.leftVolume = harmonic.rightVolume = 0.9;
            
            if (i == HARMONICS_PER_BEAT - 1)
            {
                harmonic.overtoneMultiplier = 60.0 / fundamental_freq;
            }
            else if (Unit::IsFrequencyPlayable(HARMONIC_MULTIPLIER[i] * fundamental_freq))
            {
                harmonic.overtoneMultiplier = HARMONIC_MULTIPLIER[i];
                //harmonic.leftVolume *= RandomCoo(0.3, 0.9);
                //harmonic.rightVolume *= RandomCoo(0.3, 0.9);
            }
            else
                harmonic.leftVolume = harmonic.rightVolume = 0;
        }
        
        if ((false))
            Log::LogText("Spawning partial " + Log::ToStr(i) +
                         ": fundamental=" + Log::ToStr(fundamental_freq) +
                         ", volL=" + Log::ToStr(harmonic.leftVolume) +
                         ", volR=" + Log::ToStr(harmonic.rightVolume) +
                         ", overtone=" + Log::ToStr(harmonic.overtoneMultiplier));
        
        // Initialize partial's units
        harmonic.wave.SetPhase(0);
        harmonic.wave.SetFrequency(beat.fundamentalFreq * harmonic.overtoneMultiplier);
        
        harmonic.lfo.SetFrequency(i == 0 ? 0 : 40);
        harmonic.lfo.SetPhase(i == 0 ? 0 : 0);
        harmonic.lfoVolume = (i == 0 ? 0 : 0.7);
        
        if (i == 0)
        {
            harmonic.envelope.SetDurations(attack_delay, attack_len, decay_len);
            harmonic.envelope.SetVolumes(0, beat.volume * 1, beat.volume * 1);
        }
        else
        {
            harmonic.envelope.SetDurations(attack_delay + i * 0.05, attack_len, decay_len + i * 0.005);
            harmonic.envelope.SetVolumes(0, beat.volume * 1, beat.volume * 1);
        }
        
        harmonic.envelope.SetReleaseFadeFactor(release_fade_factor, release_fade_factor);
    }
    
    std::lock_guard<std::mutex> lock(_beatsMutex);
    
    // Free data of beat that will drop out of buffer
    if (false && _beats.GetOccupiedSize() == MAX_BEATS)
    {
        //auto& beat_to_drop_out_of_buffer = _beats.Get(_beats.GetOccupiedSize() - 1);
    }
    
    _beats.Push(beat);
}

//-----------------------------------------------------------------------
StereoSample MultiBeatInstrument::GenerateSample_Beat(Beat& beat)
{
    StereoSample output_sample;
    bool beat_is_finished = true;
    
    if (beat.volume != 0)
        for (int hi = 0; hi < HARMONICS_PER_BEAT; hi++)
        {
            auto& harmonic = beat.harmonics[hi];
            if (harmonic.leftVolume == 0 &&
                harmonic.rightVolume == 0) continue;
            
            auto lfo_output = harmonic.lfo.Update();
            auto wave_output = harmonic.wave.Update();
            
            StereoSample harmonic_sample;
            harmonic_sample.left = wave_output * harmonic.leftVolume * (1 + lfo_output * harmonic.lfoVolume);
            harmonic_sample.right = wave_output * harmonic.rightVolume * (1 + lfo_output * harmonic.lfoVolume);
            
            harmonic_sample *= harmonic.envelope.Update();
            
            if (!harmonic.envelope.IsFinished())
                beat_is_finished = false;
            
            output_sample += harmonic_sample;
        }
    
    beat.isFinished = beat_is_finished;
    
    return output_sample;
}

//-----------------------------------------------------------------------
StereoSample MultiBeatInstrument::GenerateSample()
{
    const int beats_num = _beats.GetOccupiedSize();
    StereoSample output_sample;
    
    std::lock_guard<std::mutex> lock(_beatsMutex);
    
    for (int beat_i = 0; beat_i < beats_num; beat_i++)
    {
        auto& beat = _beats.Get(beat_i); //beats_num - beat_i - 1);
        if (!beat.isFinished)
            output_sample += GenerateSample_Beat(beat);
    }
    
    return output_sample;
}


