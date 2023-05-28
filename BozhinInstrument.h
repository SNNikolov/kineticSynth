#pragma once

#include "KineticInstrument.h"
#include "shaders/BozhinViz.h"
#include <mutex>

namespace yoss
{
    namespace sound
    {
        //-----------------------------------------------------------------------
        class BozhinInstrument : public KineticInstrument
        {
        public:
            typedef Envelope::EnvelopeStep EnvelopeStep;
            
            //-----------------------------------------------------------------------
            // Constants:
            static constexpr bool DebugSustain = !true;
            
            static constexpr int PartialsNum = 7;
            static constexpr int MasterEnvPartial = 0;
            static constexpr Volume NotePartialsVolume = 3.0 / PartialsNum;
            
            static constexpr Time FadeBeforeAttackDuration = 0.005;
            static constexpr Time AttackMinDuration = 0.05;
            static constexpr Time AttackMaxDuration = 0.055;//0.53;
            static constexpr Time DecayMinDuration = 0.1;//0.25;
            static constexpr Time DecayMaxDuration = 0.15;//0.35;
            static constexpr Volume SustainVolume = 0.8;
            static constexpr Time ReleaseFadeFactorStart = 0.99995; // 0.99994
            static constexpr Time ReleaseFadeFactorEnd   = 0.99995;

            static constexpr Angle BgGeoHemiRange = 90;
            static constexpr Angle AccYHemiRange = 50;
            static constexpr Angle SustainTriggerMinAcc = 10;
            static constexpr math::Velocity SustainTriggerMaxVAcc = 0.5;
            static constexpr math::Coo      SustainKeepMaxDAcc = 0.1;
            
            static constexpr math::Coo      KeyboardWidth = 1410;
            static constexpr math::Coo      KeyboardHeight = 500;
            static constexpr math::Coo      KeyboardKeysMarginL = 210;
            static constexpr math::Coo      KeyboardKeysMarginR = 208;
            static constexpr math::Coo      KeyboardKeysMarginT = 196;
            static constexpr math::Coo      KeyboardKeysMarginB = 104;
            static constexpr math::Coo      KeyboardKeysBlackHitHeight = 70;
            static constexpr math::Coo      KeyboardKeysBlackHeight = 122;
            static constexpr math::Coo      KeyboardKeysBlackWidth = 24;
            
            static constexpr Time BeatVizDuration = 0.6;
            
            static const std::vector<Frequency> Notes;
            
            //-----------------------------------------------------------------------
            struct Partial
            {
                Partial():
                    leftVolume(1), rightVolume(1), overtoneMultiplier(1.0),
                    wave(WaveSource::WST_Sine, 0),
                    tremoloVolume(0), tremoloLFO(WaveSource::WST_Sine, 0),
                    vibratoSize(0), vibratoLFO(WaveSource::WST_Sine, 0),
                    basePulseWidth(0.5),
                    pulseWidthLFO1(WaveSource::WST_Sine, 0),
                    pulseWidthLFO2(WaveSource::WST_Sine, 30),
                    pulseWidthLFO3(WaveSource::WST_Sine, 53) {}
                
                Volume leftVolume, rightVolume;
                Frequency overtoneMultiplier;
                WaveSource wave;
                WaveSource tremoloLFO, vibratoLFO;
                Volume tremoloVolume;
                Frequency vibratoSize;
                Envelope envelope;
                
                math::Stepper<Frequency> freqStepper;
                math::Stepper<Volume>    volStepper;
                math::Stepper<Volume>    lfoVolStepper;
                math::Stepper<Volume>    lfoFreqStepper;
                math::Stepper<Sample>    lowPassStepper;
                
                math::Stepper<Sample>    waveFilterStepper;
                Ratio basePulseWidth;
                WaveSource pulseWidthLFO1, pulseWidthLFO2, pulseWidthLFO3;
            };
            
            //-----------------------------------------------------------------------
            struct KeyboardKey
            {
                KeyboardKey(Frequency freq, const graphics::Rect2D& hit_rect, const graphics::Rect2D& visible_rect) :
                    freq(freq),
                    hitRect(hit_rect),
                    visibleRect(visible_rect) {}
                
                Frequency         freq;
                graphics::Rect2D  visibleRect;
                graphics::Rect2D  hitRect;
            };
            
            //-----------------------------------------------------------------------
            BozhinInstrument();
            ~BozhinInstrument();
            
            virtual void LoadSamples();
            virtual void LoadImages(graphics::Graphics* graphics);
            virtual void DrawInstrument();
            virtual void DrawEffectsAboveUI();
            virtual void UpdateGraphicsSizes(const graphics::Rect2D& container, const graphics::Rect2D& bg_container, math::CooMultiplier global_scale);
            virtual void OnGainFocus();
            virtual void OnLoseFocus();
            virtual void OnPointerEvent(input::Pointer* pointer);

            virtual void AddBeat(PartOfOne normalized_freq, Volume volume);
            
            virtual StereoSample GenerateSample();
            
        protected:
            virtual void OnUpdateInput();
            
            void InitPartials();
            void InitKeys();
            int  GetKeyAtPosInBGImage(const graphics::Point2D& point);
            bool IsSustainingWhiteKey(const graphics::Point2D& point);
            bool IsSustainingBlackKey(const graphics::Point2D& point);
            void UpdateEnvelope();
            void DrawFlyingNotesAndEffects();
            graphics::Rect2D GetGlowingKeyRect(int key_index);
            int GetGlowingKeyIndex(int key_index);
            
            bool IsKeyBlack(int key_index);

        protected:
            Frequency  _pitch;
            Frequency  _prevPitch;
            PartOfOne  _normalizedPitch;
            
            Volume     _envCurrentVolume;            
            Volume     _beatVolume;
            bool       _beatIsFinished;
            std::mutex _beatMutex;
            
            Partial _partials[PartialsNum];

            graphics::Image   _keyboardKeysImage;
            std::vector<graphics::Image> _glowingKeys;
            graphics::Rect2D  _keyboardRect;
            std::vector<KeyboardKey> _keys;
            
            graphics::Model      _keyboardModel;
            graphics::Model      _blackKeyModel;
            graphics::Model      _whiteKeyLModel;
            graphics::Model      _whiteKeyRModel;
            graphics::Model      _whiteKeyLRModel;
            graphics::GLProgram* _keyboardModelProgram;
            
            PartOfOne _sustainByYAxis;
            Angle _sustainGeoOrient;
            Angle _sustainGeoOrientBase;
            Angle _sustainAccAroundYBase;
            math::Stepper<Angle> _sustainGeoDiffStepper;
            math::Vector3D _sustainGPos;
            
            std::vector<BeatData> _beats;
            graphics::Point2D _lastHitPos;
            bool _lastHitKeyIsBlack;
            int _lastHitKey;
            int _hoveredKey;
            Time _lastHoverChangeTimestamp;
            
            Delays _leftDelay, _rightDelay;
        };
  
    }    
}

