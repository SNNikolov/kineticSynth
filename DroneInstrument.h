#pragma once

#include "KineticInstrument.h"

namespace yoss
{
    namespace sound
    {
        //-----------------------------------------------------------------------
        class DroneInstrument : public KineticInstrument
        {
        public:
            
            //-----------------------------------------------------------------------
            // Constants:
            static constexpr int PartialsNum = 4;
            static constexpr Volume PartialsVolume = 3.0 / PartialsNum;
            static constexpr Frequency DroneFundamental = 50.0 / 1.0;
            static constexpr Frequency MinDronePitch = 10;
            static constexpr Frequency DetuneSize = 1.9;
            static constexpr Volume    TremoloVol = 3.0;
            static constexpr Frequency VibratoSize = 1.5;
            static constexpr Volume    PowerVol = 1.0; // PowerLFO
            static constexpr Frequency PowerFreq = 0.2;
            static constexpr Volume    PulseWidthLFOVol = 0.04;
            static constexpr Volume    PulseWidthLFO1Vol = 0.70 * PulseWidthLFOVol;
            static constexpr Volume    PulseWidthLFO2Vol = 0.25 * PulseWidthLFOVol;
            static constexpr Volume    PulseWidthLFO3Vol = 0.05 * PulseWidthLFOVol;
            static constexpr Sample    WaveFilterHalfStep = 1.0;
            
            static constexpr Angle BgGeoHemiRange = 45;
            static constexpr Angle ZeroRotAroundX = 10;
            static constexpr Angle PowerLFOMinRotAroundX = ZeroRotAroundX;
            static constexpr Angle PowerLFOMaxRotAroundX = ZeroRotAroundX + 35;
            static constexpr Angle PowerClipMinRotAroundX = ZeroRotAroundX;
            static constexpr Angle PowerClipMaxRotAroundX = ZeroRotAroundX - 35;
            static constexpr Angle SustainMinRotAroundY = -10;
            static constexpr Angle SustainMaxRotAroundY = -50;
            
            static constexpr math::Coo DronePlateWidth = 1200;
            static constexpr math::Coo DronePlateHeight = 500;
            static constexpr math::Coo DronePlateDY     = 0;
            
            
            //-----------------------------------------------------------------------
            struct Partial
            {
                Partial():
                    leftVolume(1), rightVolume(1), overtoneMultiplier(1.0),
                    wave(WaveSource::WST_Sine, 0),
                    powerLFO(WaveSource::WST_Sine, 0),
                    tremoloVolume(0), tremoloLFO(WaveSource::WST_Sine, 0),
                    vibratoSize(0), vibratoLFO(WaveSource::WST_Sine, 0),
                    basePulseWidth(0.5),
                    pulseWidthLFO1(WaveSource::WST_Sine, 0),
                    pulseWidthLFO2(WaveSource::WST_Sine, 30),
                    pulseWidthLFO3(WaveSource::WST_Sine, 53) {}
                
                Volume leftVolume, rightVolume;
                Frequency overtoneMultiplier;
                WaveSource wave;
                WaveSource tremoloLFO, vibratoLFO, powerLFO;
                Volume tremoloVolume, powerLFOVolume;
                Frequency vibratoSize;
                Envelope envelope;
                
                math::Stepper<Frequency> freqStepper;
                math::Stepper<Volume>    volStepper;
                math::Stepper<Volume>    powerLFOVolStepper;
                math::Stepper<Volume>    lfoVolStepper;
                math::Stepper<Volume>    lfoFreqStepper;
                math::Stepper<Sample>    lowPassStepper;
                
                math::Stepper<Sample>    waveFilterStepper;
                Ratio basePulseWidth;
                WaveSource pulseWidthLFO1, pulseWidthLFO2, pulseWidthLFO3;
            };
            
            //-----------------------------------------------------------------------
            DroneInstrument();
            ~DroneInstrument();
            
            virtual void LoadSamples();
            virtual void LoadImages(graphics::Graphics* graphics);
            virtual void OnGainFocus();
            virtual void OnLoseFocus();
            virtual void UpdateGraphicsSizes(const graphics::Rect2D& container, const graphics::Rect2D& bg_container, math::CooMultiplier global_scale);
            virtual void DrawInstrument();
            virtual void OnPointerEvent(input::Pointer* pointer) {}

            virtual void AddBeat(PartOfOne normalized_freq, Volume volume);
            
            virtual StereoSample GenerateSample();
            
        protected:
            virtual void OnUpdateInput();
            
            void InitPartials();
            
        protected:
            Frequency _fundamentalFreq;

            Partial _partials[PartialsNum];
            Delays _leftDelay, _rightDelay;
            
            std::mutex _paramsChangeMutex;
            
            PartOfOne _lfoPowerByXAxis;
            PartOfOne _powerClipByXAxis;
            PartOfOne _sustainByYAxis;
            Angle     _sustainGeoOrient;
            Angle     _sustainGeoOrientBase;
            Angle     _sustainAccAroundYBase;
            math::Stepper<Angle> _sustainGeoDiffStepper;
            math::Stepper<Volume> _powerClipVolStepper;
            
            graphics::Image  _dronePlate;
            graphics::Image  _dronePlateDistort;
            graphics::Image  _dronePlateLFO;
            graphics::Rect2D _dronePlateRect;
        };
  
    }    
}

