#include "Drone.h"

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
Drone::Drone() :
    _pitch(0),
    _targetPitch(0),
    _pitchVelocity(0)
{
    Volume volume = 0.5;
    Frequency freq = 50;
    
    _fundamentalFreq = freq;
    
    for (int hi = 0; hi < HARMONICS_PER_DRONE; hi++)
    {
        auto& harmonic = _harmonics[hi];
        
        if (hi == 0)
        {
            // Fundamental frequency
            harmonic.leftVolume = harmonic.rightVolume = volume;
            harmonic.overtoneMultiplier = 1;
        }
        else
        {
            // Overtone
            harmonic.leftVolume = harmonic.rightVolume = volume * 0.3;//volume * RandomCoo(0.02, 0.05);
            harmonic.overtoneMultiplier = 1;
            
            static const double harm_multiplier[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
            if (Unit::IsFrequencyPlayable(harm_multiplier[hi] * freq))
                harmonic.overtoneMultiplier = harm_multiplier[hi];
            else
                harmonic.leftVolume = harmonic.rightVolume = 0;
            
            harmonic.leftVolume *= RandomCoo(0.3, 0.9);
            harmonic.rightVolume *= RandomCoo(0.3, 0.9);
        }
        
        if ((false))
            Log::LogText("Spawning partial " + Log::ToStr(hi) +
                         ": fundamental=" + Log::ToStr(freq) +
                         ", volL=" + Log::ToStr(harmonic.leftVolume) +
                         ", volR=" + Log::ToStr(harmonic.rightVolume) +
                         ", overtone=" + Log::ToStr(harmonic.overtoneMultiplier));
        
        // Initialize partial's units
        harmonic.wave.SetPhase(0);
        
        harmonic.lfo.SetFrequency(50); //RandomCoo(4, 8));
        harmonic.lfo.SetPhase(0);
        harmonic.lfoVolume = 1;
        
        harmonic.freqInertia.SetStepMultiplier(0.001);
        harmonic.freqInertia.SetValue(freq * harmonic.overtoneMultiplier);
    }
}

//-----------------------------------------------------------------------
StereoSample Drone::GenerateSample()
{
    StereoSample output_sample;
    
    // Update drone pitch
    if (_targetPitch > 0 || _pitch > 0)
    {
        //auto diff = (_targetPitch - _pitch) * (_targetPitch - _pitch);
        //if (_targetPitch < _pitch) diff = -diff;
        //_pitchVelocity += diff * 0.0000000000001;
        //_pitchVelocity *= 0.9998;
        //_pitch += _pitchVelocity * (_pitch - MIN_DRONE_PITCH);
        
        //_pitchVelocity += diff * 0.000000001;
        //_pitchVelocity *= 0.99999;
        //_pitchVelocity = diff / 1000000;
        //_pitch = _pitch * (1 + _pitchVelocity);
        //ASSERT(_pitch >= MIN_DRONE_PITCH && _targetPitch >= MIN_DRONE_PITCH);
        
        //_pitch = _targetPitch;
        _pitch += (_targetPitch - _pitch) * 0.00004;
    }
    
    //Beat* last_beat = (_beats.GetOccupiedSize() > 0 ? &_beats.Get() : nullptr);
    
    if (_pitch > 0)
        _fundamentalFreq = _pitch;
    
    for (int hi = 0; hi < HARMONICS_PER_DRONE; hi++)
    {
        auto& harmonic = _harmonics[hi];
        if (harmonic.leftVolume == 0 &&
            harmonic.rightVolume == 0) continue;
        
        if (_pitch > 0)
            harmonic.lfo.SetFrequency((hi + 1) * _pitch / 1000.0);
        
        auto harmonic_freq = _fundamentalFreq * harmonic.overtoneMultiplier;
        harmonic_freq = harmonic.freqInertia.Update(harmonic_freq);
        
        auto lfo_output = harmonic.lfo.Update() * harmonic.lfoVolume;
        
        harmonic.wave.SetFrequency(harmonic_freq);
        Sample wave_sample = harmonic.wave.Update();
        output_sample.left += wave_sample * harmonic.leftVolume * (1 + lfo_output);
        output_sample.right += wave_sample * harmonic.rightVolume * (1 + lfo_output);
    }
    
    return output_sample;
}

//-----------------------------------------------------------------------
void Drone::SetPitch(PartOfOne normalized_freq)
{
    ASSERT(normalized_freq >= 0 && normalized_freq <= 1);
    Frequency freq = UnnormalizeFrequency(normalized_freq);
    
    freq = freq / 4;
    freq = 50 + sqrt(freq - 50);
    
    ASSERT(freq >= MIN_DRONE_PITCH);
    _targetPitch = freq;
    if (_pitch == 0)
        _pitch = freq;
}

