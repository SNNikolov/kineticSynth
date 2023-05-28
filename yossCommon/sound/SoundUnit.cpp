#include "SoundUnit.h"

#include <iterator>
#include <algorithm>

using namespace yoss;
using namespace yoss::math;
using namespace yoss::sound;


//-----------------------------------------------------------------------
// Static defines, consts and vars

#define DEBUG_ZING(method_name, zing_condition) \
        Log::LogText("Ping " + std::string(method_name)); \
        if ((zing_condition)) Log::LogText("Zzing " + std::string(method_name));
#define DEBUG_ZING_ONCE(method_name, zing_condition) \
        static bool is_zinged = false; \
        if (!is_zinged && (zing_condition)) \
        { \
            DEBUG_ZING(method_name, zing_condition); \
            is_zinged = true; \
        }

//-----------------------------------------------------------------------
// Static members

int Unit::_unitsNum = 0;
std::vector<Unit*> Unit::_allUnits;

Frequency Unit::_samplesPerSec = 0;
Time      Unit::_sampleDuration = 0;



//-----------------------------------------------------------------------
Unit::Unit():
    _id()
{
    // Debugging
    _unitsNum++;
    if (DEBUG_UNITS_INSTANCES)
        _allUnits.push_back(this);
    if ((_unitsNum - 1) / UNITS_LEAKAGE_WARNING_COUNT != _unitsNum / UNITS_LEAKAGE_WARNING_COUNT)
    {
        Log::LogText("!!! Warning: sound::Units count = " + Log::ToStr(_unitsNum));
        
        if (DEBUG_UNITS_INSTANCES)
        {
            std::map<std::string, int> id_counters;
            for (auto it = _allUnits.begin(); it != _allUnits.end(); it++)
            {
                std::string id = (*it)->_id;
                if (id_counters.find(id) == id_counters.end())
                    id_counters[id] = 0;
                id_counters[id]++;
            }
            
            for (auto it = id_counters.begin(); it != id_counters.end(); it++)
            {
                Log::LogText("Unit #" + (*it).first + " count = " + Log::ToStr((*it).second));
            }
        }
    }
}

//-----------------------------------------------------------------------
Unit::~Unit()
{
    _unitsNum--;
    if (DEBUG_UNITS_INSTANCES)
    {
        Log::LogText("deleting unit #" + _id);
        
        auto pos_in_all = std::find(_allUnits.begin(), _allUnits.end(), this);
        ASSERT(pos_in_all != _allUnits.end());
        _allUnits.erase(pos_in_all);
    }
}

//-----------------------------------------------------------------------
void Unit::SetSamplesPerSec(Frequency samples_per_sec)
{
    _samplesPerSec = samples_per_sec;
    _sampleDuration = 1.0 / _samplesPerSec;
}

//-----------------------------------------------------------------------
void Envelope::SetStep(EnvelopeStep step)
{
    Time step_duration = 0;
    Volume volume_end = 0;
    EaseType step_ease = EaseType_Linear;
    
    switch (step)
    {
        case Step_FadeBeforeAttack:
            step_duration = _fadeBeforeStartDuration;
            volume_end = _fadeBeforeStartVolume;
            break;
        case Step_AttackJump:
            step_duration = (_attackJumpVolume > 0 ? _attackJumpDuration : 0);
            step_ease = _attackJumpEase;
            volume_end = _attackJumpVolume;
            break;
        case Step_Attack:
            step_duration = _attackDuration;
            step_ease = _attackEase;
            volume_end = _attackVolume;
            break;
        case Step_Decay:
            step_duration = _decayDuration;
            step_ease = _decayEase;
            volume_end = _sustainVolume;
            break;
        case Step_Sustain:
            step_duration = 0;
            volume_end = _sustainVolume;
            break;
        case Step_Release:
            step_duration = 0;
            volume_end = 0;
            break;
        default:
            ASSERT(false);
    }
    
    if (step < Step_Sustain && step_duration == 0)
        SetStep((EnvelopeStep)(step + 1));
    else
    {
        _step = step;
        _stepProgress = 0;
        _stepProgressStep = (step_duration == 0 ? 0 : 1.0 / (step_duration * _samplesPerSec));
        _stepStartVolume = _currentVolume;
        _stepEndVolume = volume_end;
        _stepEase = step_ease;
        ASSERT(!std::isnan(_stepStartVolume) && !std::isnan(_stepEndVolume));
    }
}

//-----------------------------------------------------------------------
Volume Envelope::Update()
{
    // Update state progress
    switch (_step)
    {
        case Step_FadeBeforeAttack:
        case Step_AttackJump:
        case Step_Attack:
        case Step_Decay:
            _stepProgress += _stepProgressStep;
            if (_stepProgress >= 1)
                SetStep((EnvelopeStep)(_step + 1));
            break;
        case Step_Sustain:
            _stepProgress = 0.5;
            if (!_isSustained)
                SetStep(Step_Release);
            break;
        case Step_Release:
            if (_stepProgress < 1.0)
            {
                Volume fade_factor = Interpolate(_releaseFadeFactorStart, _releaseFadeFactorEnd, _stepProgress);
                _stepProgress = 1.0 - (1.0 - _stepProgress) * fade_factor;
                if (_stepProgress >= 0.999)
                    _stepProgress = 1.0;
            }
            break;
            
        default:
            ASSERT(false);
    }
    
    ASSERT(_stepProgress >= 0 && _stepProgress <= 1);
    auto eased_progress = GetEasedProgress(_stepProgress, _stepEase);
    
    _currentVolume = Interpolate(_stepStartVolume, _stepEndVolume, eased_progress);
    ASSERT(!std::isnan(_currentVolume));
    
    return _currentVolume;
}

//-----------------------------------------------------------------------
Compressor::Compressor(int chanels_num):
    _chanelsNum(chanels_num),
    _level(MAX_COMPRESSOR_OUTPUT_LEVEL),
    _minLevel(MAX_COMPRESSOR_OUTPUT_LEVEL),
    _prevMinLevel(MAX_COMPRESSOR_OUTPUT_LEVEL),
    _levelStep(0),
    _delay((BufferBackPos)(COMPRESSOR_DELAY * _samplesPerSec) + ((BufferBackPos)(COMPRESSOR_DELAY * _samplesPerSec) % 2 == 1 ? 1 : 0)),
    _buffers(new CircularBuffer<Sample>*[chanels_num]),
    _currentOutput(new Sample[chanels_num])
{
    ASSERT(_delay != 0);
    
    for (int i = 0; i < _chanelsNum; i++)
    {
        _buffers[i] = new CircularBuffer<Sample>(_delay + 1, true);
        _buffers[i]->FillWith(0);
        
        _currentOutput[i] = 0;
    }
}

//-----------------------------------------------------------------------
Compressor::~Compressor()
{
    for (int i = 0; i < _chanelsNum; i++)
        delete _buffers[i];

    delete[] _buffers;
    delete[] _currentOutput;
}

//-----------------------------------------------------------------------
void Compressor::UpdateInternal(Sample* input)
{
    auto pos_in_delay = _buffers[0]->GetTimestamp() % (_delay / 2);
    if (pos_in_delay == 0)
    {
        auto next_min_level = MIN(_minLevel, _prevMinLevel);
        _levelStep = (next_min_level - _level) / (Sample)(_delay / 2);
        _prevMinLevel = _minLevel;
        _minLevel = MAX_COMPRESSOR_OUTPUT_LEVEL;
    }
    
    for (int i = 0; i < _chanelsNum; i++)
    {
        Sample input_value = input[i];
        Sample abs_input_value = abs(input_value);
        _buffers[i]->Push(input_value);
        
        if (abs_input_value > MAX_COMPRESSOR_OUTPUT_LEVEL)
        {
            Volume level_to_fit_sample = MAX_COMPRESSOR_OUTPUT_LEVEL / abs_input_value;
            if (level_to_fit_sample < _minLevel)
                _minLevel = level_to_fit_sample;
        }
    }
    
    _level += _levelStep;
    
    for (int i = 0; i < _chanelsNum; i++)
    {
        _currentOutput[i] = _buffers[i]->Get(_delay) * _level;

        //if (i == 0) {DEBUG_ZING_ONCE("Compressor", _currentOutput[i] > 0.1 || _currentOutput[i] < -0.1);}
        ASSERT(abs(_currentOutput[i]) <= MAX_COMPRESSOR_OUTPUT_LEVEL + 0.0001);
    }
}

//-----------------------------------------------------------------------
Delays::Delays(int chanels_num, Time buffer_len):
    _chanelsNum(chanels_num),
    _delays(),
    _buffers(new CircularSummedBuffer<Sample>*[chanels_num]),
    _currentOutput(new Sample[chanels_num]),
    _smoothedFeedback(new Sample[chanels_num])
{
    ASSERT(_chanelsNum <= MAX_DELAYS_CHANELS);
    
    _buffersSize = buffer_len * _samplesPerSec + 1;
    
    for (int i = 0; i < _chanelsNum; i++)
    {
        _buffers[i] = new CircularSummedBuffer<Sample>(_buffersSize, true);
        _buffers[i]->FillWith(0);
        
        _currentOutput[i] = 0;
        _smoothedFeedback[i] = 0;
    }
}

//-----------------------------------------------------------------------
Delays::~Delays()
{
    for (int i = 0; i < _chanelsNum; i++)
        delete _buffers[i];
    
    delete[] _buffers;
    delete[] _currentOutput;
    delete[] _smoothedFeedback;
}

//-----------------------------------------------------------------------
void Delays::UpdateInternal(Sample* input)
{
    for (int chanel_i = 0; chanel_i < _chanelsNum; chanel_i++)
        _buffers[chanel_i]->Push(input[chanel_i]);

    for (int chanel_i = 0; chanel_i < _chanelsNum; chanel_i++)
    {
        Sample chanel_output = 0;
        Sample chanel_feedback = 0;
        
        for (std::vector<Delay*>::size_type delay_i = 0; delay_i < _delays.size(); delay_i++)
        {
            auto& delay = _delays[delay_i];
            auto& delay_chanel_volume = delay.volume[chanel_i];
            
            if (delay_chanel_volume > 0)
            {
                auto delay_last_back_pos = delay.delayBackPos + delay.takeAverage - 1;
                ASSERT(_buffersSize > delay_last_back_pos);
                auto delayed_value = delay.takeAverage > 1 ?
                    _buffers[chanel_i]->GetAverage(delay.delayBackPos, delay_last_back_pos) :
                    _buffers[chanel_i]->Get(delay.delayBackPos);
                
                chanel_output += delayed_value * delay_chanel_volume;
                chanel_feedback += delayed_value * delay.feedbackVolume;
                
                //{DEBUG_ZING_ONCE("Delays feedback > 1", chanel_feedback > 1 || chanel_feedback < -1);}
                //{DEBUG_ZING_ONCE("Delays feedback > 2", chanel_feedback > 2 || chanel_feedback < -2);}
                //{DEBUG_ZING_ONCE("Delays feedback > 3", chanel_feedback > 3 || chanel_feedback < -3);}
                //{DEBUG_ZING_ONCE("Delays feedback > 4", chanel_feedback > 4 || chanel_feedback < -4);}
                //{DEBUG_ZING_ONCE("Delays feedback > 5", chanel_feedback > 5 || chanel_feedback < -5);}
                if (chanel_feedback > 3 || chanel_feedback < -3) ASSERT(false);
            }
        }
        
        _smoothedFeedback[chanel_i] = chanel_feedback * FEEDBACK_SMOOTH_FACTOR + _smoothedFeedback[chanel_i] * (1 - FEEDBACK_SMOOTH_FACTOR);
        _buffers[chanel_i]->AddToLast(chanel_feedback - _smoothedFeedback[chanel_i]);
        
        _currentOutput[chanel_i] = chanel_output;

        //if (chanel_i == 0) {DEBUG_ZING_ONCE("Delays", _currentOutput[chanel_i] > 0.1 || _currentOutput[chanel_i] < -0.1);}
    }
}
