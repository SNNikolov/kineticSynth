#include "SoundEngine.h"
#include "../common/System.h"

using namespace yoss;
using namespace yoss::sound;
using namespace yoss::math;


//-----------------------------------------------------------------------
SoundEngine::SoundEngine(int samples_per_sec):
    _samplesPerSec((Frequency)samples_per_sec),
    _samplesCounter(0),
    _finalCompressor(nullptr),
    _delays(nullptr),
    _isFunctional(false)
{
    Unit::SetSamplesPerSec(_samplesPerSec);
    
    if (USE_COMPRESSOR)
        _finalCompressor = new Compressor(OUTPUT_CHANELS);

    if (USE_DELAYS)
    {
        _delays = new Delays(OUTPUT_CHANELS, 3.0);
        
        //AddEcho(0.03, 0.3, 1);
        //AddEcho(0.30, 0.2, 30);
        ////AddEcho(0.35, 0.16, 10);
        ////AddEcho(0.9, 0.16, 10);
        //AddEcho(0.82, 0.5, 50);
        
        Volume echo_vol = 1.5 / 8.0;
        Volume feedback_vol = 1.0 / 8.0;
        Time echo_start = 0.10;
        Time delay_scale = 1.0;
        
        AddEcho(echo_start + delay_scale * 0.00, echo_vol * 1.0, feedback_vol * 0.90, 170);
        AddEcho(echo_start + delay_scale * 0.12, echo_vol * 1.0, feedback_vol * 1.03, 800);
        AddEcho(echo_start + delay_scale * 0.24, echo_vol * 1.0, feedback_vol * 1.07, 1200);
        AddEcho(echo_start + delay_scale * 0.26, echo_vol * 0.5, feedback_vol * 0.90, 50);
        AddEcho(echo_start + delay_scale * 0.36, echo_vol * 0.9, feedback_vol * 1.10, 1500);
        AddEcho(echo_start + delay_scale * 0.48, echo_vol * 0.8, feedback_vol * 1.13, 3500);
        AddEcho(echo_start + delay_scale * 0.50, echo_vol * 0.7, feedback_vol * 0.90, 190);
        AddEcho(echo_start + delay_scale * 0.60, echo_vol * 0.6, feedback_vol * 1.19, 7000);
    }
    
    _isFunctional = true;
    Log::LogText("SoundEngine constructed");
}

//-----------------------------------------------------------------------
SoundEngine::~SoundEngine()
{
    _isFunctional = false;
    
    if (_finalCompressor) delete _finalCompressor;
    if (_delays) delete _delays;
}

//-----------------------------------------------------------------------
void SoundEngine::GenerateSlice(OutputSampleType* output_left, OutputSampleType* output_right, int num_samples)
{
    if (!_isFunctional) return;
    
    const int step = (output_left == output_right ? 2 : 1);
    output_right = (output_left == output_right ? output_right + 1 : output_right);
    
    std::lock_guard<std::mutex> lock(_instrumentsListMutex);
    
    for(int sample_i = 0; sample_i < num_samples; sample_i++)
    {
        StereoSample output_sample;

        // Calculate beat instrument
        StereoSample beats_sample;
        for (auto instrument : _instruments)
            output_sample += instrument->GenerateSample();
        
        // Add delay effect
        if (USE_DELAYS)
            output_sample += _delays->Update(beats_sample);
        
        ASSERT(output_sample.left >= -100 && output_sample.left <= 100 &&
               output_sample.right >= -100 && output_sample.right <= 100);
        
        if (USE_COMPRESSOR)
            output_sample = _finalCompressor->Update(output_sample);
        
        *output_left = (OutputSampleType)output_sample.left;
        *output_right = (OutputSampleType)output_sample.right;
        
        output_left += step;
        output_right += step;
        
        _samplesCounter++;
    }
}

//-----------------------------------------------------------------------
void SoundEngine::AddEcho(Time normalized_delay, Volume volume, Volume feedback_volume, BufferBackPos take_average)
{
    if (normalized_delay < 0) normalized_delay = 0;
    if (volume < 0) volume = 0;
    
    Delays::Delay delay;
    
    delay.volume[0] = delay.volume[1] = volume;
    delay.feedbackVolume = feedback_volume;
    delay.delay = MIN_ECHO_DELAY + (MAX_ECHO_DELAY - MIN_ECHO_DELAY) * normalized_delay;
    delay.delayBackPos = (BufferBackPos)(delay.delay * _samplesPerSec);
    delay.takeAverage = take_average;
    
    _delays->List().push_back(delay);
}

//-----------------------------------------------------------------------
void SoundEngine::AddInstrument(Instrument* instrument, Instrument* add_before)
{
    std::lock_guard<std::mutex> lock(_instrumentsListMutex);
    
    if (add_before)
    {
        auto it = std::find(_instruments.begin(), _instruments.end(), instrument);
        ASSERT(it != _instruments.end());
        _instruments.insert(it, instrument);
    }

    _instruments.push_back(instrument);
}

//-----------------------------------------------------------------------
void SoundEngine::RemoveInstrument(Instrument* instrument)
{
    std::lock_guard<std::mutex> lock(_instrumentsListMutex);
    
    auto it = std::find(_instruments.begin(), _instruments.end(), instrument);
    if (it != _instruments.end())
        _instruments.erase(it);
}

//-----------------------------------------------------------------------
bool SoundEngine::IsPlayingInstrument(Instrument *instrument)
{
    std::lock_guard<std::mutex> lock(_instrumentsListMutex);
    
    return (std::find(_instruments.begin(), _instruments.end(), instrument) != _instruments.end());
}
