#include "BozhinInstrument.h"
#include "yossCommon/common/System.h"
#include "yossCommon/acc/Trajectory.h"
#include "yossCommon/graphics/OpenGL/shaders/ReflectRefractGLProgram.h"

using namespace yoss;
using namespace yoss::math;
using namespace yoss::sound;
using namespace yoss::graphics;

//-----------------------------------------------------------------------
//const std::vector<Frequency> BozhinInstrument::Notes = { 110.0, 220.0, 330.0, 440.0, 550.0, 660.0, 770.0 };
const std::vector<Frequency> BozhinInstrument::Notes = {
    130.81, 138.59, 146.83, 155.56, 164.81, 174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 246.94,
    261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00, 466.16, 493.88,
    523.25, 554.37, 587.33, 622.25, 659.25, 698.46, 739.99, 783.99, 830.61, 880.00, 932.33, 987.77,
    1046.50
};


//-----------------------------------------------------------------------
BozhinInstrument::BozhinInstrument() :
    _pitch(0),
    _prevPitch(0),
    _beatVolume(0),
    _envCurrentVolume(0),
    _sustainByYAxis(0),
    _sustainGeoOrient(0),
    _sustainGeoOrientBase(0),
    _sustainAccAroundYBase(0),
    _sustainGPos(0),
    _leftDelay(1, 1.0),
    _rightDelay(1, 1.0),
    _beatIsFinished(true),
    _lastHitKey(-1),
    _hoveredKey(-1),
    _lastHoverChangeTimestamp(-1),
    _lastHitPos({ 0, 0 }),
    _keyboardModelProgram(nullptr)
{
    InitPartials();
    
    _leftDelay.AddDelay(0.434, 0.3, 0.0, 8);
    _rightDelay.AddDelay(0.29238, 0.3, 0.0, 4);
}

//-----------------------------------------------------------------------
BozhinInstrument::~BozhinInstrument()
{
    if (_keyboardModelProgram)
        delete _keyboardModelProgram;
}

//-----------------------------------------------------------------------
void BozhinInstrument::LoadSamples()
{
    
}

//-----------------------------------------------------------------------
void BozhinInstrument::LoadImages(graphics::Graphics* graphics)
{
    KineticInstrument::LoadImages(graphics, "bozhin");
    
    _keyboardKeysImage.LoadFromFile("37keys.png");
    _keyboardKeysImage.CreateTexture(_graphics);
    
    Image glowing_keys;
    glowing_keys.LoadFromFile("keys_glow.png");
    glowing_keys.CreateTexture(_graphics);
    _glowingKeys = glowing_keys.GetSubImages(5, 1);
    
    Model::LoadFromFile("keyboard.fbx",
        {"body", "key_black", "key_white", "key_white", "key_white"},
        {&_keyboardModel, &_blackKeyModel, &_whiteKeyLModel, &_whiteKeyRModel, &_whiteKeyLRModel});
    
    InitKeys();
    
    _viz = new BozhinViz((OpenGL*)_graphics);
}

//-----------------------------------------------------------------------
void BozhinInstrument::InitPartials()
{
    //_sustainGeoDiffStepper.SetHalfStep(0.0001);
    _sustainGeoDiffStepper.SetSpring(0.0001, 0.99);
    
    #define SET_PARTIAL(type, multiplier, left_vol, right_vol, phase_deg) \
        partial.wave.SetType(type); \
        partial.overtoneMultiplier = multiplier; \
        partial.leftVolume = left_vol; partial.rightVolume = right_vol; \
        partial.wave.SetPhase(DegToRad(phase_deg));
    
    #define SET_P_TREMOLO(volume, phase_in_deg) \
        partial.tremoloLFO.SetPhase(DegToRad(phase_in_deg)); \
        partial.tremoloVolume = volume;
    
    #define SET_P_VIBRATO(size, phase_in_deg) \
        partial.vibratoLFO.SetPhase(DegToRad(phase_in_deg)); \
        partial.vibratoSize = size;
    
    #define SET_P_STEPPERS(freq_hstep, freq_acc, freq_damp, vol_hstep, vol_acc, vol_damp) \
        partial.freqStepper.SetSpring(freq_acc, freq_damp); \
        partial.freqStepper.SetHalfStep(freq_hstep); \
        partial.volStepper.SetSpring(vol_acc, vol_damp); \
        partial.volStepper.SetHalfStep(vol_hstep);
    
    #define SET_P_LFO_STEPPERS(freq_start, freq_hstep, freq_acc, freq_damp, vol_start, vol_hstep, vol_acc, vol_damp) \
        partial.lfoFreqStepper.SetTarget(freq_start, true); \
        partial.lfoFreqStepper.SetSpring(freq_acc, freq_damp); \
        partial.lfoFreqStepper.SetHalfStep(freq_hstep); \
        partial.lfoVolStepper.SetTarget(vol_start, true); \
        partial.lfoVolStepper.SetSpring(vol_acc, vol_damp); \
        partial.lfoVolStepper.SetHalfStep(vol_hstep);
    
    for (int pi = 0; pi < PartialsNum; pi++)
    {
        auto& partial = _partials[pi];
        
        switch (pi)
        {
            case 0:
                SET_PARTIAL(WaveSource::WST_Pulse, 1.00, 0.8, 0.08, 0);
                SET_P_TREMOLO(0.3, 25);
                SET_P_VIBRATO(4.0, 73);
                SET_P_STEPPERS(0.15, 0, 0,  0, 0.0001, 0.95);
                SET_P_LFO_STEPPERS(1.0, 0, 0.001, 0.4,   0.1, 0, 0.001, 0.4);
                partial.lowPassStepper.SetSpring(0.1, 0.97);
                //partial.lowPassStepper.SetHalfStep(1.0);
            
                partial.waveFilterStepper.SetHalfStep(0.9);
                partial.basePulseWidth = 0.24957657;
                partial.pulseWidthLFO1.SetFrequency(0.8543);
                partial.pulseWidthLFO2.SetFrequency(0.58730928);
                partial.pulseWidthLFO3.SetFrequency(13.452345);
                break;
            case 1:
                SET_PARTIAL(WaveSource::WST_Pulse, 0.99867575, 0.9, 0.6, 45);
                SET_P_TREMOLO(0.5, 98);
                SET_P_VIBRATO(7.0, 45);
                SET_P_STEPPERS(0.16, 0, 0,  0, 0.0001, 0.95);
                //SET_P_LFO_STEPPERS(1.0, 0, 0.001, 0.4,   0.1, 0, 0.001, 0.4);
                SET_P_LFO_STEPPERS(1.43967086, 0, 0.001, 0.4,   0.05, 0, 0.001, 0.4);
                partial.lowPassStepper.SetSpring(0.1, 0.97);
                //partial.lowPassStepper.SetHalfStep(1.0);
            
                partial.waveFilterStepper.SetHalfStep(0.9);
                partial.basePulseWidth = 0.15957657;
                partial.pulseWidthLFO1.SetFrequency(0.74299482);
                partial.pulseWidthLFO3.SetFrequency(0.674982095);
                partial.pulseWidthLFO2.SetFrequency(6.697837240);
                break;
            
            case 2:
                SET_PARTIAL(WaveSource::WST_Pulse, 1.0017567, 0.6, 0.9, 61);
                SET_P_TREMOLO(0.2, 112);
                SET_P_VIBRATO(7.0, 11);
                SET_P_STEPPERS(0.12, 0, 0,  0, 0.0001, 0.95);
                SET_P_LFO_STEPPERS(1.2, 0, 0.001, 0.4,   0.08, 0, 0.001, 0.4);
                //SET_P_LFO_STEPPERS(1.43967086, 0, 0.001, 0.4,   0.1, 0, 0.001, 0.4);
                partial.lowPassStepper.SetSpring(0.1, 0.97);
                //partial.lowPassStepper.SetHalfStep(1.0);
            
                partial.waveFilterStepper.SetHalfStep(0.9);
                partial.basePulseWidth = 0.8567;
                partial.pulseWidthLFO1.SetFrequency(0.56299482);
                partial.pulseWidthLFO2.SetFrequency(6.697837240);
                partial.pulseWidthLFO3.SetFrequency(7.7984593874597);
                break;
                
            case 3:
                SET_PARTIAL(WaveSource::WST_Sine, 0.5, 1.0, 1.0, 0);
                SET_P_TREMOLO(0.3, 25);
                SET_P_VIBRATO(2.0, 73);
                SET_P_STEPPERS(0.15, 0, 0,  0, 0.0001, 0.95);
                SET_P_LFO_STEPPERS(1.0, 0, 0.001, 0.4,   0.15, 0, 0.001, 0.4);
                partial.lowPassStepper.SetSpring(0.1, 0.97);
                partial.waveFilterStepper.SetHalfStep(0.9);
                break;
                
            case 4:
                SET_PARTIAL(WaveSource::WST_Sine, 1.00, 0.3, 0.3, 0);
                SET_P_TREMOLO(0.3, 25);
                SET_P_VIBRATO(3.0, 73);
                SET_P_STEPPERS(0.15, 0, 0,  0, 0.0001, 0.95);
                SET_P_LFO_STEPPERS(1.0, 0, 0.001, 0.4,   0.15, 0, 0.001, 0.4);
                partial.lowPassStepper.SetSpring(0.1, 0.97);
                partial.waveFilterStepper.SetHalfStep(0.9);
                break;
                
            case 5:
                SET_PARTIAL(WaveSource::WST_Saw, 0.998899273, 0.2, 0.1, 45);
                SET_P_TREMOLO(0.5, 98);
                SET_P_VIBRATO(3.0, 45);
                SET_P_STEPPERS(0.16, 0, 0,  0, 0.0001, 0.95);
                SET_P_LFO_STEPPERS(1.43967086, 0, 0.001, 0.4,   0.1, 0, 0.001, 0.4);
                partial.lowPassStepper.SetSpring(0.1, 0.97);
                break;
                
            case 6:
                SET_PARTIAL(WaveSource::WST_Saw, 1.001928374, 0.1, 0.2, 61);
                SET_P_TREMOLO(0.2, 112);
                SET_P_VIBRATO(3.0, 11);
                SET_P_STEPPERS(0.12, 0, 0,  0, 0.0001, 0.95);
                SET_P_LFO_STEPPERS(1.2, 0, 0.001, 0.4,   0.08, 0, 0.001, 0.4);
                partial.lowPassStepper.SetSpring(0.1, 0.97);
                break;

            default: ASSERT(false);
        }
        
        //partial.envelope.SetEasings(EaseType_Linear, EaseType_SlowEnd, EaseType_SlowStart);
    }
}

//-----------------------------------------------------------------------
void BozhinInstrument::AddBeat(PartOfOne normalized_freq, Volume volume)
{
    if (!CanPlay()) return;
    
    volume *= 0.25;
    volume *= volume;// * volume;
    volume = (volume > 1.0 ? 1.0 : volume);
    
    PartOfOne x_axis_input = 1 - normalized_freq;
    PartOfOne geo_input = (_geoOrientationAngle + BgGeoHemiRange) / (2 * BgGeoHemiRange);
    geo_input = 1 - CLAMP(geo_input, 0, 1);
    
    _lastHitPos = {
        geo_input * KeyboardWidth,
        x_axis_input * KeyboardHeight
    };
    _lastHitKey = GetKeyAtPosInBGImage(_lastHitPos);
    _lastHitKeyIsBlack = IsKeyBlack(_lastHitKey);
    
    if (_lastHitKey < 0) return;
    
    //auto& master_envelope = _partials[MasterEnvPartial].envelope;
    PartOfOne clamped_volume = CLAMP(volume, 0, 1);
    auto min_pitch = Notes[0];
    auto max_pitch = Notes[Notes.size() - 1];
    auto new_pitch = _keys[_lastHitKey].freq;
    Ratio pitch_change = _prevPitch / new_pitch;
    pitch_change = MAX(pitch_change, 1.0 / pitch_change);
    
    _beats.push_back(CreateBeatData(_lastHitKey, volume, true));
    
    std::lock_guard<std::mutex> lock(_beatMutex);
    
    _prevPitch = _pitch;
    _pitch = new_pitch;
    _normalizedPitch = (_pitch - min_pitch) / (max_pitch - min_pitch);
    _beatVolume = volume;
    _beatIsFinished = false;
    _isSustained = false;
    _sustainGPos *= 0;
    _sustainGeoOrientBase = _sustainGeoOrient = _geoOrientationAngle;
    _sustainAccAroundYBase = _accAngleAroundY;
    _sustainGeoDiffStepper.SetTarget(0);

    Time attack_duration  = Interpolate(AttackMaxDuration, AttackMinDuration, clamped_volume);
    Time decay_duration   = Interpolate(DecayMinDuration, DecayMaxDuration, clamped_volume);
    
    for (int pi = 0; pi < PartialsNum; pi++)
    {
        auto& partial = _partials[pi];
        Frequency partial_freq = partial.overtoneMultiplier * _pitch;
        
        if (!Unit::IsFrequencyPlayable(partial_freq))
        {
            partial.leftVolume = partial.rightVolume = 0;
            continue;
        }
        
        partial.volStepper.SetTarget(_beatVolume * NotePartialsVolume);
        
        partial.envelope.SetIsSustained(_isSustained);
        partial.envelope.SetDurations(0, attack_duration, decay_duration, 0);
        partial.envelope.SetVolumes(0, 1.0, SustainVolume);
        partial.envelope.SetReleaseFadeFactor(ReleaseFadeFactorStart, ReleaseFadeFactorEnd);
        partial.envelope.StartFromCurrent();
        
        partial.freqStepper.SetTarget(partial_freq);
    }
}

//-----------------------------------------------------------------------
void BozhinInstrument::OnGainFocus()
{
    KineticInstrument::OnGainFocus();
    
    _pitch = _prevPitch = 0;
    _sustainGeoDiffStepper.SetTarget(0, true);
    _lastHitKey = _hoveredKey = -1;
}

//-----------------------------------------------------------------------
void BozhinInstrument::OnLoseFocus()
{
    KineticInstrument::OnLoseFocus();
    
    _sustainGeoDiffStepper.SetMovement(0, InstrumentFadeOutDuration);
    
    for (int pi = 0; pi < PartialsNum; pi++)
        _partials[pi].volStepper.SetMovement(0, InstrumentFadeOutDuration);
}

//-----------------------------------------------------------------------
void BozhinInstrument::OnUpdateInput()
{
    bool can_play = CanPlay();
    auto& master_envelope = _partials[MasterEnvPartial].envelope;
    auto master_step = master_envelope.GetStep();
    
    Angle acc_around_y = _accAngleAroundY; //- _sustainAccAroundYBase;
    
    PartOfOne geo_input = (_geoOrientationAngle + BgGeoHemiRange) / (2 * BgGeoHemiRange);
    PartOfOne acc_x_input = GetNormFreqFromAxisXAngle(_accAngleAroundX);
    PartOfOne acc_y_input = (acc_around_y + AccYHemiRange) / (2 * AccYHemiRange);
    
    geo_input = 1 - CLAMP(geo_input, 0, 1);
    acc_y_input = CLAMP(acc_y_input, 0, 1);
    
    bool is_sustained = _isSustained;
    bool acc_y_dont_sustain = (acc_around_y >= SustainTriggerMinAcc);
    bool acc_y_do_sustain   = (acc_around_y <= -SustainTriggerMinAcc);
    
    _sustainByYAxis = (acc_y_do_sustain ? (-SustainTriggerMinAcc - acc_around_y) / (AccYHemiRange - SustainTriggerMinAcc) : 0);
    _sustainByYAxis = _sustainByYAxis * _sustainByYAxis;
    _sustainByYAxis = MIN(_sustainByYAxis, 1.0);
    //Log::LogText("sustainByYAxis = " + Log::ToStr(_sustainByYAxis, 2));
    
    if (!_isSustained && !acc_y_dont_sustain &&
        master_envelope.GetStep() == EnvelopeStep::Step_Decay)
    {
        auto acc_dt = _acc->GetAccTrajectory().GetVelocities().GetAverage(0, 5);
        //Log::LogText("sustain ON check " + Log::ToStr(acc_dt.Size(), 2));
        if (acc_y_do_sustain || acc_dt.Size() <= SustainTriggerMaxVAcc)
        {
            _sustainGPos = _acc->GetAccTrajectory().GetPositions().GetAverage(0, 3);
            _isSustained = true;
            
            if constexpr (DebugSustain)
                Log::LogText("sustain ON");
        }
    }
    else if (_isSustained)
    {
        //Point2D current_pos = { geo_input * KeyboardWidth, (1 - acc_x_input) * KeyboardHeight };
        //_isSustained = _lastHitKeyIsBlack ?
        //    IsSustainingBlackKey(current_pos) :
        //    IsSustainingWhiteKey(current_pos);
        
        auto dpos = _sustainGPos - _acc->GetAccTrajectory().GetPositions().GetAverage(0, 3);
        //Log::LogText("sustain OFF check " + Log::ToStr(dpos.Size(), 2));
        _isSustained = acc_y_do_sustain || (dpos.Size() <= SustainKeepMaxDAcc);
        if (!_isSustained && DebugSustain)
            Log::LogText("sustain OFF");
    }
    
    if (_isSustained != is_sustained)
        for (int pi = 0; pi < PartialsNum; pi++)
            _partials[pi].envelope.SetIsSustained(_isSustained && can_play);
    
    for (int pi = 0; pi < PartialsNum; pi++)
        _partials[pi].volStepper.SetTarget(can_play ? (_beatVolume + _sustainByYAxis) * NotePartialsVolume : 0);
    
    if ((1 || _isSustained || _sustainByYAxis > 0) &&
        master_step >= EnvelopeStep::Step_Sustain)
    {
        _sustainGeoOrient = _geoOrientationAngle;
        _sustainGeoDiffStepper.SetTarget(can_play ? _sustainGeoOrient - _sustainGeoOrientBase : 0);
    }
}

//-----------------------------------------------------------------------
void BozhinInstrument::UpdateEnvelope()
{
    auto& master_partial = _partials[MasterEnvPartial];
    auto& master_envelope = master_partial.envelope;
    auto master_step = master_envelope.GetStep();
    
    // On start of envelope step
    if (master_envelope.GetStepProgress() == 0)
    {
        //Log::LogText("step=" + Log::ToStr((int)master_envelope.GetStep()));
        
        bool is_start_of_attack = (master_step == EnvelopeStep::Step_Attack);
        bool is_start_of_sustain = (master_step == EnvelopeStep::Step_Sustain);
        
        for (int pi = 0; pi < PartialsNum; pi++)
        {
            auto& partial = _partials[pi];
            Frequency partial_freq = partial.overtoneMultiplier * _pitch;
            
            if (Unit::IsFrequencyPlayable(partial_freq))
            {
                if (is_start_of_sustain)
                {
                    _sustainGeoOrient = _sustainGeoOrientBase = _geoOrientationAngle;
                    _sustainAccAroundYBase = _accAngleAroundY;
                }
            }
        }
    }
    
    _envCurrentVolume = master_envelope.GetVolume();
}

//-----------------------------------------------------------------------
StereoSample BozhinInstrument::GenerateSample()
{
    StereoSample output_sample;
    Time dt = Unit::GetSampleDuration();
    
    if (_pitch == 0)
        return output_sample;
    
    std::lock_guard<std::mutex> lock(_beatMutex);
    UpdateEnvelope();
    
    auto& master_partial = _partials[MasterEnvPartial];
    auto& master_envelope = master_partial.envelope;
    auto master_step = master_envelope.GetStep();
    
    _sustainGeoDiffStepper.UpdateMovement(dt);
    Angle sustain_geo_diff = _sustainGeoDiffStepper.UpdateLagged();
   
    for (int pi = 0; pi < PartialsNum; pi++)
    {
        auto& partial = _partials[pi];
        if (partial.leftVolume == 0 && partial.rightVolume == 0) continue;
        
        partial.volStepper.UpdateMovement(dt);
        auto partial_vol = partial.volStepper.UpdateLagged();
        auto lfo_volume = partial.lfoVolStepper.UpdateLagged();
        auto lfo_freq = partial.lfoFreqStepper.UpdateLagged();
        
        partial.tremoloLFO.SetFrequency(lfo_freq);
        partial.vibratoLFO.SetFrequency(lfo_freq);
        auto tremolo_output = partial.tremoloLFO.Update() * partial.tremoloVolume * lfo_volume;
        auto vibrato_output = partial.vibratoLFO.Update() * partial.vibratoSize * lfo_volume;
        
        if (partial.overtoneMultiplier != 0)
        {
            partial.freqStepper.UpdateMovement(dt);
            auto partial_freq = partial.freqStepper.UpdateLagged();
            partial_freq += vibrato_output;
            
            CLAMP(partial_freq, BEAT_MIN_SWING_FREQUENCY, 20000);
            partial.wave.SetFrequency(partial_freq);
        }
    
        auto wave_output = partial.wave.Update();
        
        partial.waveFilterStepper.SetTarget(wave_output);
        wave_output = partial.waveFilterStepper.UpdateLagged();
        
        auto pulse_lfo_output = 0.8 * partial.pulseWidthLFO1.Update() + 0.16 * partial.pulseWidthLFO2.Update() + 0.04 * partial.pulseWidthLFO3.Update();
        partial.wave.SetPulseWidth(partial.basePulseWidth + 0.10 * pulse_lfo_output);
        
        wave_output *= partial_vol * (1 + tremolo_output);

        Volume env_vol = partial.envelope.Update();
        env_vol = MAX(env_vol, _sustainByYAxis);
        wave_output *= env_vol;
        ASSERT(ABS(wave_output) < 10.0);
        
        CooMultiplier spring_acc = partial.freqStepper.GetLaggedPos() * 0.0009;// * (_envCurrentVolume * 0.2 + 0.8);
        spring_acc = spring_acc * std::powf(0.9, sustain_geo_diff * 2.0);
        spring_acc = CLAMP(spring_acc, 0.001, 0.95);
        
        partial.lowPassStepper.SetSpringAcc(spring_acc);
        partial.lowPassStepper.SetTarget(wave_output);
        wave_output = partial.lowPassStepper.UpdateLagged();
        //ASSERT(ABS(wave_output) < 10.0);
        
        StereoSample partial_sample;
        partial_sample.left = wave_output * partial.leftVolume;
        partial_sample.right = wave_output * partial.rightVolume;
        
        output_sample += partial_sample;
    }

    StereoSample delay_sample;
    delay_sample.left = _leftDelay.Update(output_sample.right + _rightDelay.GetCurrentOutput());
    delay_sample.right = _rightDelay.Update(output_sample.left + _leftDelay.GetCurrentOutput());
    output_sample += delay_sample;
    
    return output_sample;
}



//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// GRAPHICS
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
bool BozhinInstrument::IsKeyBlack(int key_index)
{
    int ii = key_index % 12;
    return (ii == 1 || ii == 3 || ii == 6 || ii == 8 || ii == 10);
}

//-----------------------------------------------------------------------
void BozhinInstrument::InitKeys()
{
    Coo key_w = (KeyboardWidth - KeyboardKeysMarginL - KeyboardKeysMarginR) / 22;
    auto key_vis_rect = Rect2D::WithSizes(KeyboardKeysMarginL, KeyboardKeysMarginT, key_w, KeyboardHeight - KeyboardKeysMarginT - KeyboardKeysMarginB);
    auto key_hit_rect = key_vis_rect;
    key_hit_rect.min.y += KeyboardKeysBlackHitHeight;
    
    Coo black_hit_top = key_hit_rect.min.y - key_hit_rect.h() / 2;
    Coo black_hit_bottom = key_hit_rect.min.y;
    
    _keys.clear();
    
    for (int i = 0; i < Notes.size(); i++)
    {
        if (IsKeyBlack(i))
        {
            Coo dx;
            switch (i % 12)
            {
                case 1: dx = -5; break;
                case 3: dx = 5; break;
                case 6: dx = -7; break;
                case 10: dx = 7; break;
                default: dx = 0;
            }
            
            Rect2D black_key_hit_rect = Rect2D::WithMaxPoint(key_hit_rect.min.x + dx - key_w / 2, black_hit_top, key_hit_rect.min.x + dx + key_w / 2, black_hit_bottom);
            Rect2D black_key_vis_rect = Rect2D::WithMaxPoint(key_vis_rect.min.x + dx - KeyboardKeysBlackWidth / 2, key_vis_rect.min.y, key_vis_rect.min.x + dx + KeyboardKeysBlackWidth / 2, key_vis_rect.min.y + KeyboardKeysBlackHeight);
            _keys.push_back(KeyboardKey(Notes[i], black_key_hit_rect, black_key_vis_rect));
        }
        else
        {
            _keys.push_back(KeyboardKey(Notes[i], key_hit_rect, key_vis_rect));
            key_hit_rect.Translate(key_w, 0);
            key_vis_rect.Translate(key_w, 0);
        }
    }
}

//-----------------------------------------------------------------------
bool BozhinInstrument::IsSustainingWhiteKey(const graphics::Point2D& point)
{
    return true;//(point.y >= _whiteKeysTopInBgImage);
}

//-----------------------------------------------------------------------
bool BozhinInstrument::IsSustainingBlackKey(const graphics::Point2D& point)
{
    return false;//(point.y >= _blackKeysTopInBgImage);
}

//-----------------------------------------------------------------------
void BozhinInstrument::UpdateGraphicsSizes(const Rect2D &container, const Rect2D& bg_container, CooMultiplier global_scale)
{
    KineticInstrument::UpdateGraphicsSizes(container, bg_container, global_scale);
    
    CooMultiplier bg_scale = container.h() / KeyboardHeight;
    bg_scale *= 1.0;
    _keyboardRect = container;
    _keyboardRect.max.x = _keyboardRect.min.x + KeyboardWidth * bg_scale - 1;
    _keyboardRect.max.y = _keyboardRect.min.y + KeyboardHeight * bg_scale - 1;
}

//-----------------------------------------------------------------------
void BozhinInstrument::OnPointerEvent(input::Pointer* pointer)
{
    ASSERT(pointer);
    Point2D pointer_pos = { pointer->x, pointer->y };

}

//-----------------------------------------------------------------------
void BozhinInstrument::DrawInstrument()
{
    PartOfOne geo_input = (_geoOrientationAngle + BgGeoHemiRange) / (2 * BgGeoHemiRange);
    geo_input = CLAMP(geo_input, 0, 1);
    
    PartOfOne x_axis_input = GetNormFreqFromAxisXAngle(_accAngleAroundX);
    
    const CooMultiplier bg_scale = _keyboardRect.w() / KeyboardWidth;
    const Point2D center = { _container.xCenter(), _container.yCenter() };
    Coo dx = (geo_input - 0.5) * _keyboardRect.w();
    Coo dy = (x_axis_input - 0.5) * _keyboardRect.h();
    Coo current_x = _keyboardRect.xCenter() - center.x;
    Coo current_y = _keyboardRect.yCenter() - center.y;
    
    static Time app_start_time = system::GetCurrentTimestamp();
    static Time prev_frame_time = app_start_time;
    
    Time current_time = system::GetCurrentTimestamp();
    Time frame_dt = current_time - prev_frame_time;
    prev_frame_time += frame_dt;
    
    auto viz = (BozhinViz*)_viz;
    viz->Feed((_pitch > 0 ? _sustainByYAxis : 0), _envCurrentVolume, _sustainGeoDiffStepper.GetLaggedPos());
    
    KineticInstrument::DrawViz();
    
    _keyboardRect.Translate(dx - current_x, dy - current_y);
    
    if (!_keyboardModelProgram)
    {
        ASSERT(_vizImage->GetTexture());
        auto reflect_refrag_prog = new ReflectRefractGLProgram((OpenGL*)_graphics);
        reflect_refrag_prog->SetBody(Color::FromFloats(0.2, 0.4, 1.0), nullptr);
        reflect_refrag_prog->SetRefraction(0.7, 1.6, _vizImage->GetTexture());
        reflect_refrag_prog->SetReflection(2.0, COLOR_BLACK, nullptr);
        reflect_refrag_prog->SetSpecularLight(Vector3D(-1, 1.5, 1), COLOR_WHITE, 7.5, -27);
        reflect_refrag_prog->SetCullFaces(true);
        _keyboardModelProgram = reflect_refrag_prog;
    }
    
    CooMultiplier mscale = _keyboardRect.h() * 0.01054; //_keyboardRect.h() / _keyboardModel.GetBoundingBoxSize().y;
    ((OpenGL*)_graphics)->SetCurrentProgram(_keyboardModelProgram);
    _graphics->PushCurrentTransforms(Vector3D(_keyboardRect.xCenter() / mscale, _keyboardRect.yCenter() / mscale), ZERO_VECTOR, ONE_VECTOR * mscale, X_AXIS, 0);
    _graphics->DrawModel(_keyboardModel);
    _graphics->PopCurrentTransforms();
    ((OpenGL*)_graphics)->SetCurrentProgram(nullptr);
    
    Rect2D keys_rect = { _keys[0].visibleRect.min, _keys[_keys.size() - 1].visibleRect.max };
    keys_rect.Multiply(bg_scale, bg_scale);
    keys_rect.Translate(_keyboardRect.min);
    _graphics->DrawImage(_keyboardKeysImage, keys_rect);
    
    //_graphics->DrawRect(false, _keyboardRect, COLOR_RED);

    int new_hovered_key = GetKeyAtPosInBGImage({
        (center.x - _keyboardRect.min.x) / bg_scale,
        (center.y - _keyboardRect.min.y) / bg_scale
    });
    if (_hoveredKey != new_hovered_key)
        _lastHoverChangeTimestamp = current_time;
    _hoveredKey = new_hovered_key;
    
    for (int ki = 0; ki < _keys.size(); ki++)
    {
        auto& key = _keys[ki];
        
        int glow_index = GetGlowingKeyIndex(ki);
        auto glow_rect = GetGlowingKeyRect(ki);
        
        if (ki == _hoveredKey)
        {
            PartOfOne hover_alpha = 8 * (current_time - _lastHoverChangeTimestamp);
            hover_alpha = hover_alpha * hover_alpha;
            Color hover_color = COLOR_GREEN.WithAlpha(MIN(hover_alpha, 1.0) * (1 - _sustainByYAxis));
            _graphics->DrawImage(_glowingKeys[glow_index], glow_rect, hover_color, hover_color, hover_color, hover_color);
        }
        
        if (ki == _lastHitKey)
        {
            PartOfOne hit_alpha = 1.0 * MAX(_envCurrentVolume, _sustainByYAxis) * (0.5 + 0.5 * sin(90 * (current_time - app_start_time)));
            Color hit_color = COLOR_YELLOW.WithAlpha(MIN(hit_alpha, 1.0));
            _graphics->DrawImage(_glowingKeys[glow_index], glow_rect, hit_color, hit_color, hit_color, hit_color);
        }
        
        //Rect2D key_rect = key.visibleRect;
        //key_rect.Multiply(bg_scale, bg_scale);
        //key_rect.Translate(_keyboardRect.min.x, _keyboardRect.min.y);
        //_graphics->DrawRect(false, key_rect, COLOR_BLUE);
        
        //Rect2D key_hit_rect = key.hitRect;
        //key_hit_rect.Multiply(bg_scale, bg_scale);
        //key_hit_rect.Translate(_keyboardRect.min.x, _keyboardRect.min.y);
        //_graphics->DrawRect(false, key_hit_rect, COLOR_BLUE);
    }
    
    //DrawInput(30, 15);
    
    if (0 && _lastHitKey >= 0)
    {
        Rect2D pos_rect = { _lastHitPos, _lastHitPos };
        pos_rect.Multiply(bg_scale, bg_scale);
        pos_rect.Expand(20);
        pos_rect.Translate(_keyboardRect.min.x, _keyboardRect.min.y);
        _graphics->DrawRect(true, pos_rect, COLOR_RED.WithAlpha(0.5));
    }
}

//-----------------------------------------------------------------------
void BozhinInstrument::DrawEffectsAboveUI()
{
    DrawFlyingNotesAndEffects();
}

//-----------------------------------------------------------------------
void BozhinInstrument::DrawFlyingNotesAndEffects()
{
    auto current_time = system::GetCurrentTimestamp();
    
    // Draw beats vizualizations
    for (auto beat_it = _beats.rbegin(); beat_it != _beats.rend(); beat_it++)
    {
        auto& beat = *beat_it;
        if (beat.vizProgress > 1) break;
        
        int glow_index = GetGlowingKeyIndex(beat.note);
        auto note_rect = GetGlowingKeyRect(beat.note);
        Coo dy = _keyboardRect.yCenter() * /*beat.volume */ beat.vizProgress * beat.vizProgress;
        note_rect.Translate(0, -dy);
        note_rect.Expand((beat.userGenerated ? -20 + 30 * (1 - beat.vizProgress * beat.vizProgress) : 5) * _globalScale);
        
        PartOfOne norm_freq = (float)beat.note / (_keys.size() - 1);
        Color note_color = Color::FromFloats(1.0 - norm_freq, norm_freq, 1.0 - 1.5 * abs(0.5 - norm_freq), 1);
        if (!beat.userGenerated)
            note_color = note_color.MultipliedBy(0.5);
        note_color.a = (beat.userGenerated ? 1.0 : 0.7) - 0.5 * beat.vizProgress * beat.vizProgress;
        
        _graphics->DrawImage(_glowingKeys[glow_index], note_rect, note_color, note_color, note_color, note_color);
        
        beat.vizProgress = (current_time - beat.timestamp) / BeatVizDuration;
    }
}

//-----------------------------------------------------------------------
Rect2D BozhinInstrument::GetGlowingKeyRect(int key_index)
{
    const CooMultiplier bg_scale = _keyboardRect.w() / KeyboardWidth;
    
    auto& key = _keys[key_index];
    Rect2D key_rect = key.visibleRect;
    key_rect.Multiply(bg_scale, bg_scale);
    key_rect.Translate(_keyboardRect.min.x, _keyboardRect.min.y);
    
    Rect2D glow_rect = key_rect;
    glow_rect.Expand(20 * bg_scale);
    
    return glow_rect;
}

//-----------------------------------------------------------------------
int BozhinInstrument::GetGlowingKeyIndex(int key_index)
{
    if (key_index == _keys.size() - 1)
        return 3;
    else if (IsKeyBlack(key_index))
        return 4;
    else
        switch (key_index % 12)
    {
        case 0:
        case 5: return 0;
        case 4:
        case 11: return 2;
        default: return 1;
    }
}

//-----------------------------------------------------------------------
int BozhinInstrument::GetKeyAtPosInBGImage(const graphics::Point2D &point)
{
    std::vector<int> inside_keys;
    for (int ki = 0; ki < _keys.size(); ki++)
    {
        auto& key = _keys[ki];
        if (key.hitRect.Contains(point))
            inside_keys.push_back(ki);
    }
    
    //if (inside_keys.size() == 0) return -1;
    if (inside_keys.size() == 1) return inside_keys.back();
    
    int inside_keys_i = 0;
    int closest_key = -1;
    Coo closest_dist = 1000000;
    for (int ki = 0; ki < _keys.size(); ki++)
    {
        if (inside_keys.size() > 0 &&
            inside_keys[inside_keys_i] != ki)
            continue;
        
        auto& key = _keys[ki];
        Coo dx = point.x - key.hitRect.xCenter();
        Coo dy = point.y - key.hitRect.yCenter();
        Coo dist = sqrt(dx * dx + dy * dy);
        if (dist < closest_dist)
        {
            closest_key  = ki;
            closest_dist = dist;
        }
        
        inside_keys_i++;
    }
    
    return closest_key;
}
