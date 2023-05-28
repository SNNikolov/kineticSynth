#pragma once

#include <vector>
#include <map>

#include "Sound.h"
#include "../structs/CircularSummedBuffer.h"
#include "../common/Log.h"


namespace yoss
{
    namespace sound
    {
        
        //-----------------------------------------------------------------------
        // Structs and classes:
        class Unit;
        class HalfWayThere;
        class Inertia;
        class SmoothTransition;
        class WaveSource;
        class Envelope;
        class Compressor;
        class Delays;
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Types:
        //-----------------------------------------------------------------------
        
        //-----------------------------------------------------------------------
        // Constants:
        static const bool DEBUG_UNITS_INSTANCES = false; // Whether to print debug info on creating/deleting units
        static const int UNITS_LEAKAGE_WARNING_COUNT = 1000;
        
        //-----------------------------------------------------------------------

        
        //-----------------------------------------------------------------------
        // Base class of all sound Units
        class Unit
        {
        public:
            Unit();
            virtual ~Unit();
            inline void SetID(std::string new_id) { _id = new_id; }
            
            static void SetSamplesPerSec(Frequency samples_per_sec);
            inline static Frequency GetSamplesPerSec() { return _samplesPerSec; }
            inline static Time      GetSampleDuration() { return _sampleDuration; }
            inline static bool IsFrequencyPlayable(Frequency freq) { return freq >= 20 && freq <= _samplesPerSec / 2; }
        protected:
            static Frequency _samplesPerSec;
            static Time      _sampleDuration;
            
        private:
            static int _unitsNum; // Counter of currently existing units
            static std::vector<Unit*> _allUnits;
            
            std::string _id;
        };
        
        //-----------------------------------------------------------------------
        // Get inertial value of streaming input value
        class HalfWayThere : public Unit
        {
        public:
            HalfWayThere(Sample step_multiplier = 0.001, Sample initial_value = 0):
                _stepMultiplier(step_multiplier), _currentValue(initial_value) {}
            inline void SetStepMultiplier(Sample step_multiplier) { _stepMultiplier = step_multiplier; }
            inline void SetValue(Sample value) { _currentValue = value; }
            
            inline Sample Update(Sample new_input)
            {
                _currentValue += (new_input - _currentValue) * _stepMultiplier;
                return _currentValue;
            }
            
        protected:
            Sample _stepMultiplier;
            Sample _currentValue;
        };
        
        //-----------------------------------------------------------------------
        // Get inertial value of streaming input value
        class Inertia : public Unit
        {
        public:
            static constexpr Sample BASE_WEIGHT_FACTOR = 0.000001;
            static constexpr Sample BASE_VELOCITY_DAMPING_FACTOR = 0.999;
            
            Inertia(Sample weight = 1.0, Sample friction = 1.0, Sample initial_value = 0):
                _currentValue(initial_value), _velocity(0) { SetWeight(weight); SetFriction(friction); }
            inline void SetWeight(Sample weight) { _weightFactor = BASE_WEIGHT_FACTOR / weight; }
            inline void SetFriction(Sample friction) { _velocityDampingFactor = 1.0 - friction * (1.0 - BASE_VELOCITY_DAMPING_FACTOR); }
            inline void SetValue(Sample value) { _currentValue = value; }
            inline void Stop() { _velocity = 0; }
            
            inline Sample Update(Sample new_input)
            {
                //auto diff = (new_input - _currentValue) * (new_input - _currentValue);
                //if (new_input < _currentValue) diff = -diff;
                //_velocity += diff * 0.0000000000001;
                //_velocity *= 0.9998;
                //_currentValue += _velocity * (_currentValue - BEAT_FUNDAMENTAL_MIN_FREQUENCY);
                
                _velocity += (new_input - _currentValue) * _weightFactor;
                _velocity *= _velocityDampingFactor;
                
                _currentValue += _velocity;
                return _currentValue;
            }
            
        protected:
            Sample _weightFactor, _velocityDampingFactor;
            Sample _currentValue;
            Sample _velocity;
        };
        
        //-----------------------------------------------------------------------
        // Get smooth transition between a starting an ending values
        class SmoothTransition : public Unit
        {
        public:
            enum TransitionType
            {
                Type_Linear,
                Type_Ease,
                Type_EaseIn,
                Type_EaseOut
            };
            
            SmoothTransition(TransitionType type, Time transition_duration = 1, Sample start_value = 0, Sample end_value = 0):
                _type(type) { SetDuration(transition_duration); StartTransition(start_value, end_value); }
            inline void SetDuration(Time transition_duration) { _duration = transition_duration; _progressStep = 1.0 / (_duration * _samplesPerSec); }
            inline void StartTransition(Sample start_value, Sample end_value) { _startValue = start_value; _endValue = end_value; _progress = 0; }
            
            inline Sample Update()
            {
                if (_progress >= 1) return _endValue;
                
                _progress += _progressStep;
                
                double eased_progress, eased_total;
                
                if (_type == Type_Linear)       { eased_total = 1; eased_progress = _progress; }
                else if (_type == Type_Ease)    { eased_total = 2; eased_progress = 1 - cos(yoss::math::PI * _progress); }
                else if (_type == Type_EaseIn)  { eased_total = 1; eased_progress = 1 - cos(yoss::math::PI * 0.5 * _progress); }
                else if (_type == Type_EaseOut) { eased_total = 1; eased_progress = - cos(yoss::math::PI * 0.5 * (1.0 + _progress)); }
                else ASSERT(false); // Unhandled type
                
                Sample interpolated = _startValue + (_endValue - _startValue) * (eased_progress / eased_total);
                return interpolated;
            }
            
        protected:
            TransitionType _type;
            Sample _startValue, _endValue;
            Time _duration;
            double _progress;
            double _progressStep;
        };
        
        //-----------------------------------------------------------------------
        class WaveSource : public Unit
        {
        public:
            enum WaveSourceType
            {
                WST_Sine,
                WST_Square,
                WST_Pulse,
                WST_Noise,
                WST_Triangular,
                WST_Saw,
                WST_ReverseSaw,
                WST_MultiSaw,
                WST_StereoSample
            };
            
            WaveSource(WaveSourceType type, AngularVelocity phase_speed = 0, Sample initial_phase = 0):
                _type(type), _phase(initial_phase), _phaseSpeed(phase_speed * _sampleDuration), _pulseWidth(0.5),
                _sampleBuffer(nullptr), _sampleBufferSize(0), _currentSampleIndex(0) {}
            inline void SetType(WaveSourceType type) { _type = type; }
            inline void SetPulseWidth(PartOfOne pulsew) { _pulseWidth = pulsew; }
            inline void SetPhaseSpeed(AngularVelocity phase_speed) { _phaseSpeed = phase_speed * _sampleDuration; }
            inline void SetFrequency(Frequency freq) { _phaseSpeed = math::FrequencyToPhaseSpeed(freq) * _sampleDuration; }
            inline void SetPhase(Angle phase) { _phase = phase; }
            inline void SetSample(const Sample* sample_buffer, int samples_num = 0) { _phase = 0; _sampleBuffer = sample_buffer; _sampleBufferSize = samples_num; _currentSampleIndex = 0; }
            inline void SetSamplePlaySpeed(AngularVelocity speed_multiplier) { _phaseSpeed = _sampleBufferSize > 0 ? (4.0 * math::PI * speed_multiplier) / _sampleBufferSize : 0; }
            inline bool SampleFinished() const { return (_phase >= 2 * math::PI || _currentSampleIndex >= _sampleBufferSize); }
            
            inline bool IsStartingNewCicle() const { return _phase - _phaseSpeed < 0; } // || (int)(_phase / 2 * M_PI) != (int)((_phase - _phaseSpeed) / 2 * M_PI); }
            inline Frequency GetCurrentPhase() const { return _phase; }
            inline WaveSourceType GetType() const { return _type; }
            
            inline Sample Update()
            {
                _phase = fmod(_phase + _phaseSpeed, math::DOUBLE_PI);
                
                switch (_type)
                {
                    case WST_Sine:
                        return (Sample)sin((double)_phase);
                    case WST_Noise:
                        if (IsStartingNewCicle())
                            _currentSample = math::RandomCoo(-1.0, 1.0);
                        return _currentSample;
                    case WST_Pulse:
                        return (Sample)(_phase * math::DIV_DOUBLE_PI < _pulseWidth ? 1.0 : 0.0);
                    case WST_Triangular:
                        return (Sample)(1.0 - ABS(fmod(_phase, math::DOUBLE_PI) * math::DIV_PI - 1.0));
                    case WST_Saw:
                        return (Sample)(-1.0 + fmod(_phase + math::PI, math::DOUBLE_PI) * math::DIV_PI);
                    case WST_ReverseSaw:
                        return (Sample)(-1.0 + fmod(math::DOUBLE_PI - _phase + math::PI, math::DOUBLE_PI) * math::DIV_PI);
                    case WST_MultiSaw:
                    {
                        static constexpr int    small_saws_num      = 4;
                        static constexpr Sample small_saw_amplitude = 0.2;
                        static constexpr double div_modulo = math::DIV_DOUBLE_PI * (2.0 + small_saws_num * small_saw_amplitude);
                        auto passed_small_saws = floor(_phase * math::DIV_DOUBLE_PI * (small_saws_num + 1));
                        return (Sample)(-1.0 + fmod(_phase + math::PI, math::DOUBLE_PI) * div_modulo
                                         - passed_small_saws * small_saw_amplitude);
                    }
                    case WST_Square:
                        return (Sample)(1.0 - 2.0 * round(_phase * math::DIV_DOUBLE_PI));

                    default:
                        ASSERT(false);
                        return 0;
                }
            }
            
            inline StereoSample UpdateStereo()
            {
                ASSERT(_type == WST_StereoSample);
                ASSERT(_sampleBuffer);
                
                _phase += _phaseSpeed;
#define INTERPOLATE_SAMPLE 1
#if INTERPOLATE_SAMPLE
                if (_phase > 2 * math::PI)
                    return StereoSample(0);
                
                math::PartOfOne progress = _phase / (2.0 * math::PI);
                math::Coo pos_in_buffer = progress * (_sampleBufferSize >> 1);
                math::PartOfOne interpolate_progress = pos_in_buffer - floor(pos_in_buffer);
                int pos_in_buffer1 = 2 * (int)floor(pos_in_buffer);
                int pos_in_buffer2 = 2 * (int)ceil(pos_in_buffer);
                auto left_sample  = math::Interpolate(_sampleBuffer[pos_in_buffer1], _sampleBuffer[pos_in_buffer2], interpolate_progress);
                auto right_sample = math::Interpolate(_sampleBuffer[pos_in_buffer1 + 1], _sampleBuffer[pos_in_buffer2 + 1], interpolate_progress);
#else
                int pos_in_buffer = 2 * (int)round((_phase * _sampleBufferSize) / (4.0 * math::PI));
                if (pos_in_buffer >= _sampleBufferSize - 1)
                    return StereoSample(0);
                auto left_sample  = _sampleBuffer[pos_in_buffer];
                auto right_sample = _sampleBuffer[pos_in_buffer + 1];
#endif
                return StereoSample(left_sample, right_sample);
            }
            
            inline StereoSample UpdateStereoFixedSpeed()
            {
                ASSERT(_type == WST_StereoSample);
                ASSERT(_sampleBuffer);
                
                if (_currentSampleIndex >= _sampleBufferSize)
                    return StereoSample(0);
                auto left_sample  = _sampleBuffer[_currentSampleIndex];
                auto right_sample = _sampleBuffer[_currentSampleIndex + 1];
                
                _currentSampleIndex += 2;

                return StereoSample(left_sample, right_sample);
            }
            
        protected:
            WaveSourceType _type;
            Angle _phase;
            AngularVelocity _phaseSpeed;
            PartOfOne _pulseWidth;
            Sample _currentSample;
            const Sample* _sampleBuffer;
            int _sampleBufferSize;
            int _currentSampleIndex;
        };
        
        //-----------------------------------------------------------------------
        class Envelope : public Unit
        {
        public:
            enum EnvelopeStep
            {
                Step_FadeBeforeAttack,
                Step_AttackJump,
                Step_Attack,
                Step_Decay,
                Step_Sustain,
                Step_Release,
                Steps_Num
            };
            typedef math::EaseType EaseType;

            Envelope():
                _fadeBeforeStartDuration(0), _attackJumpDuration(0), _attackDuration(0.1), _decayDuration(0.3),
                _attackJumpVolume(0), _attackVolume(1), _sustainVolume(0.8), _isSustained(false),
                _attackJumpEase(math::EaseType_Linear), _attackEase(math::EaseType_Linear), _decayEase(math::EaseType_Linear),
                _releaseFadeFactorStart(0.9997), _releaseFadeFactorEnd(0.9997) { Finish(); }
            Envelope(Time attack_delay, Time attack_duaration, Time decay_duration,
                     Volume start_vol, Volume attack_vol, Volume sustain_vol, Time release_fade_factor):
                _fadeBeforeStartDuration(0), _attackJumpDuration(attack_delay), _attackDuration(attack_duaration), _decayDuration(decay_duration),
                _attackJumpVolume(start_vol), _attackVolume(attack_vol), _sustainVolume(sustain_vol), _isSustained(false),
                _attackJumpEase(math::EaseType_Linear), _attackEase(math::EaseType_Linear), _decayEase(math::EaseType_Linear),
                _releaseFadeFactorStart(release_fade_factor), _releaseFadeFactorEnd(release_fade_factor) { Finish(); }
            
            inline void SetDurations(Time attack_jump, Time attack, Time decay, Time fade_before_start = 0) { _attackJumpDuration = attack_jump; _attackDuration = attack; _decayDuration = decay; _fadeBeforeStartDuration = fade_before_start; }
            inline void SetVolumes(Volume attack_jump_vol, Volume attack_vol, Volume sustain_vol) { _attackJumpVolume = attack_jump_vol; _attackVolume = attack_vol; _sustainVolume = sustain_vol; }
            inline void SetSustainVolume(Volume sustain_vol) { _sustainVolume = sustain_vol; }
            inline void SetReleaseFadeFactor(Volume release_fade_factor_start, Volume release_fade_factor_end) { _releaseFadeFactorStart = release_fade_factor_start; _releaseFadeFactorEnd = release_fade_factor_end; }
            inline void SetAttackJump(Volume volume, Time duration = 0.005) { _attackJumpDuration = duration; _attackJumpVolume = volume; }
            inline void SetEasings(EaseType attack_jump, EaseType attack, EaseType decay) { _attackJumpEase = attack_jump; _attackEase = attack; _decayEase = decay; }
            inline void SetIsSustained(bool is_sustained) { _isSustained = is_sustained; }
            inline void StopSustainIf(bool stop_condition) { _isSustained = _isSustained && !stop_condition; }
            inline void Start() { _currentVolume = 0; SetStep(Step_AttackJump); }
            inline void StartFromCurrent() { SetStep(Step_AttackJump); }
            inline void FadeCurrentAndStart(Volume fade_to_volume = 0) { _fadeBeforeStartVolume = fade_to_volume; SetStep(Step_FadeBeforeAttack); }
            inline void Release() { SetStep(Step_Release); }
            inline void Finish() { _currentVolume = 0; SetStep(Step_Release); _stepProgress = 1; }

            inline EnvelopeStep GetStep() const { return _step; }
            inline double       GetStepProgress() const { return _stepProgress; }
            Volume              GetVolume() const { return _currentVolume; }
            bool                GetIsSustained() const { return _isSustained; }
            bool                IsFinished() const { return _step == Step_Release && _stepProgress >= 1; }
            bool                IsPlaying() const { return !IsFinished(); }
            
            Volume Update();
            void SetStep(EnvelopeStep step);
            //void SetProgress(EnvelopeStep step, double step_progress = 0) { ASSERT(step_progress >= 0 && step_progress <= 1); SetStep(step); _stepProgress = step_progress; }
            
        protected:
            Time _fadeBeforeStartDuration, _attackJumpDuration, _attackDuration, _decayDuration;
            Volume _attackJumpVolume, _attackVolume, _sustainVolume, _fadeBeforeStartVolume;
            Volume _releaseFadeFactorStart, _releaseFadeFactorEnd;
            EaseType _attackJumpEase, _attackEase, _decayEase;
            
            Volume _currentVolume;
            Volume _stepStartVolume, _stepEndVolume;
            EaseType _stepEase;
            EnvelopeStep _step;
            double _stepProgress;
            double _stepProgressStep;
            bool _isSustained;
        };
        
        //-----------------------------------------------------------------------
        class Compressor : public Unit
        {
        public:
            static constexpr Time COMPRESSOR_DELAY = 0.02; // [seconds] Compressor buffer size
            static constexpr Volume MAX_COMPRESSOR_OUTPUT_LEVEL = 0.9999;
            
            Compressor(int chanels_num);
            ~Compressor();
            
            inline Sample Update(Sample new_input)
            {
                ASSERT(_chanelsNum == 1);
                UpdateInternal(&new_input);
                return _currentOutput[0];
            }
            
            inline StereoSample Update(StereoSample new_input)
            {
                ASSERT(_chanelsNum == 2);
                UpdateInternal((Sample*)&new_input);
                return *((StereoSample*)&_currentOutput[0]);
            }

        protected:
            int       _chanelsNum;
            Volume    _level;
            Volume    _minLevel;
            Volume    _prevMinLevel;
            Volume    _levelStep;
            BufferBackPos _delay;
            CircularBuffer<Sample>** _buffers;
            Sample*   _currentOutput;
            
            void UpdateInternal(Sample* input);
        };

        //-----------------------------------------------------------------------
        class Delays : public Unit
        {
        public:
            static constexpr int MAX_DELAYS_CHANELS = 2;
            static constexpr Sample FEEDBACK_SMOOTH_FACTOR = 0.0001;
            
            struct Delay
            {
                Volume volume[MAX_DELAYS_CHANELS];
                Volume feedbackVolume = 0;
                Time delay = 0;
                BufferBackPos delayBackPos = 0;
                BufferBackPos takeAverage = 1;
            };
            
            Delays(int chanels_num, Time buffer_len);
            ~Delays();
            
            inline std::vector<Delay>& List() { return _delays; }
            
            inline Sample       GetCurrentOutput()       { return _currentOutput[0]; }
            inline StereoSample GetCurrentStereoOutput() { return *((StereoSample*)&_currentOutput[0]); }
            
            inline Sample Update(Sample new_input)
            {
                ASSERT(_chanelsNum == 1);
                UpdateInternal(&new_input);
                return _currentOutput[0];
            }
            
            inline StereoSample Update(StereoSample new_input)
            {
                ASSERT(_chanelsNum == 2);
                UpdateInternal((Sample*)&new_input);
                return *((StereoSample*)&_currentOutput[0]);
            }
            
            void AddDelay(Time delay, Volume volume, Volume feedback_volume, BufferBackPos take_average)
            {
                delay = MAX(delay, 0);
                volume = MAX(volume, 0);
                
                Delay d;
                d.volume[0] = d.volume[1] = volume;
                d.feedbackVolume = feedback_volume;
                d.delay = delay;
                d.delayBackPos = (BufferBackPos)(delay * _samplesPerSec);
                d.takeAverage = take_average;
                _delays.push_back(d);
            }
            
        protected:
            int _chanelsNum;
            int _buffersSize;
            std::vector<Delay>       _delays;
            CircularSummedBuffer<Sample>** _buffers;
            Sample*                  _currentOutput;
            Sample*                  _smoothedFeedback;
            
            void UpdateInternal(Sample* input);
        };
        
    }    
}

