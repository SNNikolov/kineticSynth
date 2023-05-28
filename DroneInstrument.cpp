#include "DroneInstrument.h"
#include "shaders/DroneViz.h"
#include "yossCommon/common/System.h"

using namespace yoss;
using namespace yoss::math;
using namespace yoss::sound;
using namespace yoss::graphics;


//-----------------------------------------------------------------------
DroneInstrument::DroneInstrument() :
    _fundamentalFreq(DroneFundamental),
    _leftDelay(1, 1.0),
    _rightDelay(1, 1.0),
    _sustainGeoOrientBase(0),
    _sustainByYAxis(0),
    _lfoPowerByXAxis(0),
    _powerClipByXAxis(0)
{
    InitPartials();
    
    _leftDelay.AddDelay(0.63487, 0.23, 0.0, 8);
    _rightDelay.AddDelay(0.41238, 0.28, 0.0, 4);
}

//-----------------------------------------------------------------------
DroneInstrument::~DroneInstrument()
{
}

//-----------------------------------------------------------------------
void DroneInstrument::LoadSamples()
{
}

//-----------------------------------------------------------------------
void DroneInstrument::LoadImages(graphics::Graphics* graphics)
{
    KineticInstrument::LoadImages(graphics, "drone");
    
    _dronePlate.LoadFromFile("drone_plate.png");
    _dronePlate.CreateTexture(_graphics);
    
    _dronePlateDistort.LoadFromFile("drone_plate_distort.png");
    _dronePlateDistort.CreateTexture(_graphics);
    
    _dronePlateLFO.LoadFromFile("drone_plate_lfo.png");
    _dronePlateLFO.CreateTexture(_graphics);
    
    _viz = new DroneViz((OpenGL*)_graphics);
}

//-----------------------------------------------------------------------
void DroneInstrument::InitPartials()
{
    _sustainGeoDiffStepper.SetSpring(0.0001, 0.99);
    _powerClipVolStepper.SetSpring(0.0001, 0.99);
    
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
    
#define SET_P_POWER_LFO(type, freq, phase_in_deg, vol, vol_hstep, vol_acc, vol_damp) \
        partial.powerLFO.SetType(type); \
        partial.powerLFO.SetFrequency(freq); \
        partial.powerLFO.SetPhase(DegToRad(phase_in_deg)); \
        partial.powerLFOVolume = vol; \
        partial.powerLFOVolStepper.SetTarget(0, true); \
        partial.powerLFOVolStepper.SetSpring(vol_acc, vol_damp); \
        partial.powerLFOVolStepper.SetHalfStep(vol_hstep);
#define DETUNED(master_freq, detuned_freq) (master_freq + (detuned_freq - master_freq) * DetuneSize)
    
    
    for (int pi = 0; pi < PartialsNum; pi++)
    {
        auto& partial = _partials[pi];
        
        switch (pi)
        {
            case 0:
                SET_PARTIAL(WaveSource::WST_Pulse, DETUNED(1.00, 1.00), 0.8, 0.8, 0);
                SET_P_TREMOLO(0.3 * TremoloVol, 25);
                SET_P_VIBRATO(1.0 * VibratoSize, 73);
                SET_P_STEPPERS(0.15, 0, 0,  0, 0.0001, 0.95);
                SET_P_LFO_STEPPERS(1.0, 0, 0.001, 0.4,   0.15, 0, 0.001, 0.4);
                SET_P_POWER_LFO(WaveSource::WST_Sine, PowerFreq, 0, 0.95 * PowerVol, 0, 0.001, 0.4);
                partial.lowPassStepper.SetSpring(0.1, 0.97);
                //partial.lowPassStepper.SetHalfStep(1.0);
                
                partial.waveFilterStepper.SetHalfStep(WaveFilterHalfStep);
                partial.basePulseWidth = 0.24957657;
                partial.pulseWidthLFO1.SetFrequency(0.8543);
                partial.pulseWidthLFO2.SetFrequency(0.58730928);
                partial.pulseWidthLFO3.SetFrequency(13.452345);
                break;
            case 1:
                SET_PARTIAL(WaveSource::WST_Pulse, DETUNED(1.0, 0.99927575), 0.9, 0.6, 45);
                SET_P_TREMOLO(0.5 * TremoloVol, 98);
                SET_P_VIBRATO(1.0 * VibratoSize, 45);
                SET_P_STEPPERS(0.16, 0, 0,  0, 0.0001, 0.95);
                //SET_P_LFO_STEPPERS(1.0, 0, 0.001, 0.4,   0.1, 0, 0.001, 0.4);
                SET_P_LFO_STEPPERS(1.43967086, 0, 0.001, 0.4,   0.1, 0, 0.001, 0.4);
                SET_P_POWER_LFO(WaveSource::WST_Sine, PowerFreq, 90, 1.0 * PowerVol, 0, 0.001, 0.4);
                partial.lowPassStepper.SetSpring(0.1, 0.97);
                //partial.lowPassStepper.SetHalfStep(1.0);
                
                partial.waveFilterStepper.SetHalfStep(WaveFilterHalfStep);
                partial.basePulseWidth = 0.15957657;
                partial.pulseWidthLFO1.SetFrequency(0.74299482);
                partial.pulseWidthLFO3.SetFrequency(0.674982095);
                partial.pulseWidthLFO2.SetFrequency(6.697837240);
                break;
                /*
            case 2:
                SET_PARTIAL(WaveSource::WST_Pulse, DETUNED(1.0, 1.0017567), 0.1, 0.2, 61);
                SET_P_TREMOLO(0.2 * TremoloVol, 112);
                SET_P_VIBRATO(1.0 * VibratoSize, 11);
                SET_P_STEPPERS(0.12, 0, 0,  0, 0.0001, 0.95);
                SET_P_LFO_STEPPERS(1.2, 0, 0.001, 0.4,   0.1, 0, 0.001, 0.4);
                SET_P_POWER_LFO(WaveSource::WST_Sine, PowerFreq * 0.5, 0, 1.0 * PowerVol, 0, 0.001, 0.4);
                partial.lowPassStepper.SetSpring(0.1, 0.97);
                //partial.lowPassStepper.SetHalfStep(1.0);
                
                partial.waveFilterStepper.SetHalfStep(WaveFilterHalfStep);
                partial.basePulseWidth = 0.9267;
                partial.pulseWidthLFO1.SetFrequency(0.56299482);
                partial.pulseWidthLFO2.SetFrequency(6.697837240);
                partial.pulseWidthLFO3.SetFrequency(7.7984593874597);
                break;
                */
            case 2:
                SET_PARTIAL(WaveSource::WST_Sine, DETUNED(2.0, 2.001458), 0.8, 0.8, 0);
                SET_P_TREMOLO(0.2 * TremoloVol, 25);
                SET_P_VIBRATO(0.75 * VibratoSize, 73);
                SET_P_STEPPERS(0.15, 0, 0,  0, 0.0001, 0.95);
                SET_P_LFO_STEPPERS(1.073, 0, 0.001, 0.4,   0.1, 0, 0.001, 0.4);
                SET_P_POWER_LFO(WaveSource::WST_Sine, PowerFreq * 0.5, 90, 1.0 * PowerVol, 0, 0.001, 0.4);
                partial.lowPassStepper.SetSpring(0.1, 0.97);
                partial.waveFilterStepper.SetHalfStep(0.1);
                partial.basePulseWidth = 0.9334;
                break;
                
            case 3:
                SET_PARTIAL(WaveSource::WST_Sine, DETUNED(1.0, 0.99948), 1.0, 1.0, 0);
                SET_P_TREMOLO(0.3 * TremoloVol, 25);
                SET_P_VIBRATO(1.0 * VibratoSize, 73);
                SET_P_STEPPERS(0.15, 0, 0,  0, 0.0001, 0.95);
                SET_P_LFO_STEPPERS(1.3928347, 0, 0.001, 0.4,   0.1, 0, 0.001, 0.4);
                SET_P_POWER_LFO(WaveSource::WST_Sine, PowerFreq / 3.0, 0, 1.0 * PowerVol, 0, 0.001, 0.4);
                partial.lowPassStepper.SetSpring(0.1, 0.97);
                partial.waveFilterStepper.SetHalfStep(0.2);
                partial.basePulseWidth = 0.05;
                break;
           
            default: ASSERT(false);
        }
    }
}

//-----------------------------------------------------------------------
void DroneInstrument::OnGainFocus()
{
    KineticInstrument::OnGainFocus();
    
    _sustainGeoDiffStepper.SetTarget(0, true);
}

//-----------------------------------------------------------------------
void DroneInstrument::OnLoseFocus()
{
    KineticInstrument::OnLoseFocus();
    
    _sustainGeoDiffStepper.SetMovement(0, InstrumentFadeOutDuration);
    
    for (int pi = 0; pi < PartialsNum; pi++)
        _partials[pi].volStepper.SetMovement(0, InstrumentFadeOutDuration);
}

//-----------------------------------------------------------------------
void DroneInstrument::AddBeat(PartOfOne normalized_freq, Volume volume)
{
    
}

//-----------------------------------------------------------------------
void DroneInstrument::OnUpdateInput()
{
    bool can_play = CanPlay();
    
    //std::lock_guard<std::mutex> lock(_paramsChangeMutex);
    
    _lfoPowerByXAxis = (_accAngleAroundX - PowerLFOMinRotAroundX) / (PowerLFOMaxRotAroundX - PowerLFOMinRotAroundX);
    _lfoPowerByXAxis = CLAMP(_lfoPowerByXAxis, 0, 1);
    _lfoPowerByXAxis = _lfoPowerByXAxis * _lfoPowerByXAxis;
    //_lfoPowerByXAxis = 0;

    _powerClipByXAxis = (_accAngleAroundX - PowerClipMinRotAroundX) / (PowerClipMaxRotAroundX - PowerClipMinRotAroundX);
    _powerClipByXAxis = CLAMP(_powerClipByXAxis, 0, 1);
    _powerClipByXAxis = _powerClipByXAxis * _powerClipByXAxis * _powerClipByXAxis;
    //_powerClipByXAxis = 0;
    
    _sustainByYAxis = (_accAngleAroundY - SustainMinRotAroundY) / (SustainMaxRotAroundY - SustainMinRotAroundY);
    _sustainByYAxis = CLAMP(_sustainByYAxis, 0, 1);
    _sustainByYAxis = _sustainByYAxis * _sustainByYAxis;
    //Log::LogText("sustainByYAxis = " + Log::ToStr(_sustainByYAxis, 2));
    
    for (int pi = 0; pi < PartialsNum; pi++)
    {
        auto& partial = _partials[pi];
        Frequency partial_freq = partial.overtoneMultiplier * _fundamentalFreq;
        
        if (!Unit::IsFrequencyPlayable(partial_freq))
        {
            partial.leftVolume = partial.rightVolume = 0;
            continue;
        }
        
        partial.volStepper.SetTarget(can_play ? _sustainByYAxis * PartialsVolume : 0);
        partial.freqStepper.SetTarget(partial_freq);
        
        partial.powerLFOVolStepper.SetTarget(_lfoPowerByXAxis);
    }
    
    _powerClipVolStepper.SetTarget(_powerClipByXAxis);
    
    //if (_sustainByYAxis > 0)
    {
        _sustainGeoOrient = _geoOrientationAngle;
        _sustainGeoDiffStepper.SetTarget(can_play ? _sustainGeoOrient - _sustainGeoOrientBase : 0);
    }
}

//-----------------------------------------------------------------------
StereoSample DroneInstrument::GenerateSample()
{
    StereoSample output_sample;
    Time dt = Unit::GetSampleDuration();
    
    //if (_pitch == 0)
    //    return output_sample;
    
    //std::lock_guard<std::mutex> lock(_paramsChangeMutex);
    
    _sustainGeoDiffStepper.UpdateMovement(dt);
    
    Angle sustain_geo_diff = _sustainGeoDiffStepper.UpdateLagged();
    Volume power_clip_volume = _powerClipVolStepper.UpdateLagged();
    
    for (int pi = 0; pi < PartialsNum; pi++)
    {
        auto& partial = _partials[pi];
        if (partial.leftVolume == 0 && partial.rightVolume == 0) continue;
        
        partial.volStepper.UpdateMovement(dt);
        auto partial_vol = partial.volStepper.UpdateLagged();
        auto lfo_volume = partial.lfoVolStepper.UpdateLagged();
        auto lfo_freq = partial.lfoFreqStepper.UpdateLagged();
        
        auto power_lfo_volume = partial.powerLFOVolStepper.UpdateLagged();
        auto power_lfo_output = partial.powerLFO.Update() * power_lfo_volume * partial.powerLFOVolume;
        
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
        
        auto pulse_lfo_output = PulseWidthLFO1Vol * partial.pulseWidthLFO1.Update() + PulseWidthLFO2Vol * partial.pulseWidthLFO2.Update() + PulseWidthLFO3Vol * partial.pulseWidthLFO3.Update();
        auto pulse_w = partial.basePulseWidth + pulse_lfo_output;
        pulse_w = CLAMP(pulse_w, 0.01, 0.99);
        partial.wave.SetPulseWidth(pulse_w);
        
        auto wave_output = partial.wave.Update();
        //if (partial.wave.GetType() == WaveSource::WST_Sine)
        
        partial.waveFilterStepper.SetTarget(wave_output);
        wave_output = partial.waveFilterStepper.UpdateLagged();
        
        CooMultiplier spring_acc = partial.freqStepper.GetLaggedPos() * 0.001;// * (_envCurrentVolume * 0.2 + 0.8);
        spring_acc = spring_acc * std::powf(0.9, sustain_geo_diff * 1.3);
        spring_acc = CLAMP(spring_acc, 0.0003, 0.95);
        
        partial.lowPassStepper.SetSpringAcc(spring_acc);
        partial.lowPassStepper.SetTarget(wave_output);
        wave_output = partial.lowPassStepper.UpdateLagged();
        ASSERT(ABS(wave_output) < 5.0);
        
        if (power_clip_volume > 0)
        {
            Volume clip_vol_step = 0.5;
            int stepped = (int)(abs(wave_output) / clip_vol_step);
            auto step_progress = fmod(wave_output, clip_vol_step);
            bool in_clip_step = (stepped % 2 == 1);
            if (in_clip_step)
            {
                auto disp = (wave_output > 0 ? stepped + 1 : -stepped - 1) * clip_vol_step - wave_output;
                wave_output += disp * power_clip_volume;
            }
            else
                wave_output += step_progress * 2.5 * power_clip_volume;
        }
        
        wave_output *= partial_vol * (1 + tremolo_output) * (1 + power_lfo_output);
        ASSERT(ABS(wave_output) < 10.0);
        
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

    //partial.lfo.SetFrequency((pi + 1) * partial_freq / (1000.0 * partial.overtoneMultiplier));
}



//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// GRAPHICS
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
void DroneInstrument::UpdateGraphicsSizes(const Rect2D &container, const Rect2D& bg_container, CooMultiplier global_scale)
{
    KineticInstrument::UpdateGraphicsSizes(container, bg_container, global_scale);
    
    CooMultiplier bg_scale = container.h() / DronePlateHeight;
    bg_scale *= .8;
    _dronePlateRect = container;
    _dronePlateRect.max.x = _dronePlateRect.min.x + DronePlateWidth * bg_scale - 1;
    _dronePlateRect.max.y = _dronePlateRect.min.y + DronePlateHeight * bg_scale - 1;
}

//-----------------------------------------------------------------------
void DroneInstrument::DrawInstrument()
{
    PartOfOne geo_input = (_geoOrientationAngle + BgGeoHemiRange) / (2 * BgGeoHemiRange);
    geo_input = CLAMP(geo_input, 0, 1);
    
    PartOfOne lfo_power = (_accAngleAroundX - PowerLFOMinRotAroundX) / (PowerLFOMaxRotAroundX - PowerLFOMinRotAroundX);
    lfo_power = CLAMP(lfo_power, 0, 1);
    //_lfoPowerByXAxis = _lfoPowerByXAxis * _lfoPowerByXAxis;
    
    PartOfOne clip_power = (_accAngleAroundX - PowerClipMinRotAroundX) / (PowerClipMaxRotAroundX - PowerClipMinRotAroundX);
    clip_power = CLAMP(clip_power, 0, 1);
    
    PartOfOne x_axis_input = 0.5 + 0.5 * lfo_power - 0.5 * clip_power;
    
    const Time current_time = system::GetCurrentTimestamp();
    const CooMultiplier bg_scale = _dronePlateRect.w() / DronePlateWidth;
    const Point2D center = { _container.xCenter(), _container.yCenter() + DronePlateDY };
    Coo dx = (geo_input - 0.5) * _dronePlateRect.w();
    Coo dy = (x_axis_input - 0.5) * _dronePlateRect.h();
    Coo current_x = _dronePlateRect.xCenter() - center.x;
    Coo current_y = _dronePlateRect.yCenter() - center.y;
    
    auto viz = (DroneViz*)_viz;
    viz->Feed(_sustainByYAxis, _lfoPowerByXAxis, _sustainGeoDiffStepper.GetLaggedPos());

    KineticInstrument::DrawViz();
    
    _dronePlateRect.Translate(dx - current_x, dy - current_y);
    _graphics->DrawImage(_dronePlate, _dronePlateRect);
    
    if (_lfoPowerByXAxis > 0)
    {
        auto& partial = _partials[0];
        Color col = COLOR_WHITE.WithAlpha(_lfoPowerByXAxis * (0.5 + 0.5 * (1 - _sustainByYAxis) + 0.5 * _sustainByYAxis * sin(partial.powerLFO.GetCurrentPhase())));
        _graphics->DrawImage(_dronePlateLFO, _dronePlateRect, col, col, col, col);
    }
    else if (_powerClipByXAxis > 0)
    {
        Color col = COLOR_WHITE.WithAlpha(_powerClipByXAxis * (fmod(current_time * 15, 1) < 0.3 ? 1 - _sustainByYAxis : 1));
        _graphics->DrawImage(_dronePlateDistort, _dronePlateRect, col, col, col, col);
    }
    
    //DrawInput(30, 15, -90, 270);
}
