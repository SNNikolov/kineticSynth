#pragma once

#include "KineticInstrument.h"

namespace yoss
{
    namespace sound
    {
        //-----------------------------------------------------------------------
        class DrumKitInstrument : public KineticInstrument, public SamplerInstrument
        {
        public:
            //-----------------------------------------------------------------------
            struct Drum
            {
                Drum(int sample_index, const graphics::Rect2D& rect, const graphics::Color& flash_col) :
                    sampleIndex(sample_index),
                    rect(rect),
                    model(),
                    flashColor (flash_col) {}

                int               sampleIndex;
                graphics::Rect2D  rect;
                graphics::Model   model;
                graphics::Color   flashColor;
            };
            
            //-----------------------------------------------------------------------
            DrumKitInstrument();
            ~DrumKitInstrument();
            
            void LoadDrum(const std::string& file, Volume native_vol, const graphics::Rect2D& rect, const graphics::Color& flash_col);
            void  GetDrumsAtPosInBGImage(const graphics::Point2D& point, int& returned_drum1, int& returned_drum2);
            
            virtual Frequency UnnormalizeFrequency(PartOfOne normalized_freq = 0) { return 200; }
            
            virtual void LoadSamples();
            virtual void LoadImages(graphics::Graphics* graphics);
            virtual void UpdateGraphicsSizes(const graphics::Rect2D& container, const graphics::Rect2D& bg_container, math::CooMultiplier global_scale);
            virtual void OnGainFocus();
            virtual void OnLoseFocus();
            virtual void DrawInstrument();
            virtual void OnPointerEvent(input::Pointer* pointer);
            
            virtual void AddBeat(PartOfOne normalized_freq, Volume volume);
            
            //virtual StereoSample GenerateSample();

        protected:
            static constexpr bool CanHitTwoDrums = false;
            static constexpr Angle BgGeoHemiRange = 90;
            static constexpr Angle ModelsViewHemiRange = 45;
            
            static constexpr math::Coo BgWidth  = 500;
            static constexpr math::Coo BgHeight = 500;
            
            graphics::Rect2D  _bgRect;
            std::vector<Drum> _drums;
            graphics::Model   _sceneDeco;
            graphics::GLProgram* _drumsProgram;
            
            graphics::Point2D _lastHitPos;
            Time _lastHitTimestamp;
            int _lastHitDrum1;
            int _lastHitDrum2;
            int _hoveredDrum1;
            int _hoveredDrum2;
        };
  
    }    
}

