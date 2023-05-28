#include "SingleBeatInstrument.h"

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
SingleBeatInstrument::SingleBeatInstrument() :
    _pitch(0),
    _targetPitch(0),
    _pitchVelocity(0),
    _pitchTransition(SmoothTransition::Type_Ease),
    _volume(0),
    _beatIsFinished(true)
{
    _pitchInertia.SetWeight(0.01);
    _pitchInertia.SetFriction(50.0);
    _pitchTransition.SetDuration(0.2);
    
    _volumeInertia.SetWeight(0.01);
    _volumeInertia.SetFriction(50.0);
    
    // Initialize partials units
    for (int hi = 0; hi < MAX_HARMONICS; hi++)
    {
        auto& harmonic = _harmonics[hi];
        
        harmonic.envelope.SetDurations(ATTACK_DELAY + hi * 0.0005, ATTACK_LEN, DECAY_LEN + hi * 0.05);
        harmonic.envelope.SetVolumes(0, 1, 0.4);
        harmonic.envelope.SetReleaseFadeFactor(RELEASE_FADE_FACTOR, RELEASE_FADE_FACTOR);
    
        harmonic.lfo.SetFrequency(hi == 0 ? 50 : 50);
        harmonic.lfo.SetPhase(hi == 0 ? 0 : DegToRad(30));
        harmonic.lfoVolume = (hi == 0 ? 0 : 0);
    }
}

//-----------------------------------------------------------------------
void SingleBeatInstrument::SetVolume(Volume volume)
{
    if (_harmonics[0].envelope.GetStep() >= Envelope::Step_Sustain)
        _volume = volume;
}

//-----------------------------------------------------------------------
void SingleBeatInstrument::AddBeat(PartOfOne normalized_freq, Volume volume)
{
    volume *= 0.3;
    volume *= volume * volume;
    volume = (volume > 1.0 ? 1.0 : volume);
    
    Frequency fundamental_freq = UnnormalizeFrequency(normalized_freq);
    
    std::lock_guard<std::mutex> lock(_beatMutex);

    _targetPitch = fundamental_freq;
    if (_pitch == 0)
    {
        _pitch = fundamental_freq;
        _pitchInertia.SetValue(fundamental_freq);
    }
    _pitchTransition.StartTransition(_pitch, _targetPitch);
    
    _volume = volume;
    _beatIsFinished = false;
    
    //_volumeInertia.SetValue(volume);
    
    if ((!false))
        Log::LogText("Spawning beat: fundamental=" + Log::ToStr(fundamental_freq) +
                     ", vol=" + Log::ToStr(volume) +
                     ", envReleaseFactor=" + Log::ToStr(RELEASE_FADE_FACTOR, 6));
    
    for (int hi = 0; hi < MAX_HARMONICS; hi++)
    {
        auto& harmonic = _harmonics[hi];
        
#define SET_HARMONIC(multiplier, left_vol, right_vol, phase_deg) \
        harmonic.overtoneMultiplier = multiplier; harmonic.leftVolume = left_vol; harmonic.rightVolume = right_vol; \
        harmonic.lfo.SetFrequency(fundamental_freq / 10); \
        harmonic.lfo.SetPhase(DegToRad(phase_deg));
        
        switch (hi)
        {
            case 0: SET_HARMONIC(1.00, 1.00, 1.00, 0); break;
            case 1: SET_HARMONIC(2.00, 0.20, 0.15, 0); break;
            case 2: SET_HARMONIC(3.00, 0.00, 0.00, 0); break;
            case 3: SET_HARMONIC(4.00, 0.15, 0.20, 0); break;
            case 4: SET_HARMONIC(5.00, 0.10, 0.07, 0); break;
            case 5: SET_HARMONIC(6.00, 0.00, 0.00, 0); break;
            case 6: SET_HARMONIC(7.00, 0.40, 0.30, 0); break;
            case 7: SET_HARMONIC(8.00, 0.07, 0.10, 0); break;
            case 8: SET_HARMONIC(9.00, 0.00, 0.00, 0); break;
            case 9: SET_HARMONIC(10.0, 0.35, 0.20, 0); break;
                
            //case 0: SET_HARMONIC(1.00, 1.00, 1.00, 0); break;
            //case 1: SET_HARMONIC(0.50, 0.05, 0.03, 90); break;
            //case 2: SET_HARMONIC(0.25, 0.01, 0.02, 180); break;
            //case 3: SET_HARMONIC(0.12, 0.01, 0.01, 270); break;
            //case 4: SET_HARMONIC(60.0 / fundamental_freq, 1, 1, 180); break;
            default: ASSERT(false);
        }
 
        if (!Unit::IsFrequencyPlayable(harmonic.overtoneMultiplier * fundamental_freq))
        {
            harmonic.leftVolume = harmonic.rightVolume = 0;
            continue;
        }
        
        if ((false))
            Log::LogText("Spawning partial " + Log::ToStr(hi) +
                         ": fundamental=" + Log::ToStr(fundamental_freq) +
                         ", volL=" + Log::ToStr(harmonic.leftVolume) +
                         ", volR=" + Log::ToStr(harmonic.rightVolume) +
                         ", overtone=" + Log::ToStr(harmonic.overtoneMultiplier));
        
        harmonic.envelope.SetIsSustained(false);
        //harmonic.envelope.FadeCurrentAndStart(FADE_OUT_BEFORE_ATTACK);
        harmonic.envelope.StartFromCurrent();
    }
}

//-----------------------------------------------------------------------
StereoSample SingleBeatInstrument::GenerateSample()
{
    StereoSample output_sample;
    
    std::lock_guard<std::mutex> lock(_beatMutex);
    
    // Update single-beat pitch
    if (_targetPitch > 0 || _pitch > 0)
    {
        //_pitch = _targetPitch;
        
        // Pitch -> Target Pitch via simple half step
        //_pitch += (_targetPitch - _pitch) * 0.004;
        
        // Pitch -> Target Pitch via Transition unit
        //_pitch = _pitchTransition.Update();
        
        // Pitch -> Target Pitch via Inertia unit
        _pitch = _pitchInertia.Update(_targetPitch);
        
        if (_pitch < BEAT_MIN_SWING_FREQUENCY)
            _pitch = BEAT_MIN_SWING_FREQUENCY;
        else if (_pitch > BEAT_FUNDAMENTAL_MAX_FREQUENCY)
            _pitch = BEAT_FUNDAMENTAL_MAX_FREQUENCY;
    }
    
    bool beat_is_finished = true;
    Volume beat_volume = _volumeInertia.Update(_volume);
    
    if (beat_volume != 0)
        for (int hi = 0; hi < MAX_HARMONICS; hi++)
        {
            auto& harmonic = _harmonics[hi];
            if (harmonic.leftVolume == 0 &&
                harmonic.rightVolume == 0) continue;
            
            harmonic.wave.SetFrequency(_pitch * harmonic.overtoneMultiplier);
            harmonic.envelope.SetIsSustained(_isSustained);
            
            auto lfo_output = harmonic.lfo.Update();
            auto wave_output = harmonic.wave.Update();
            
            StereoSample harmonic_sample;
            harmonic_sample.left = wave_output * harmonic.leftVolume * (1 + lfo_output * harmonic.lfoVolume);
            harmonic_sample.right = wave_output * harmonic.rightVolume * (1 + lfo_output * harmonic.lfoVolume);
            
            harmonic_sample *= beat_volume * harmonic.envelope.Update();
            
            if (!harmonic.envelope.IsFinished())
                beat_is_finished = false;
            
            output_sample += harmonic_sample;
        }
    
    _beatIsFinished = beat_is_finished;
    
    return output_sample;
}

