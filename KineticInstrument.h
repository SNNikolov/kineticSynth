#pragma once

#include "shaders/KineticInstrumentViz.h"

#include "yossCommon/sound/SamplerInstrument.h"
#include "yossCommon/graphics/common/Graphics.h"
#include "yossCommon/common/Input.h"
#include "yossCommon/acc/AccEngine.h"

namespace yoss
{
    namespace sound
    {
        //-----------------------------------------------------------------------
        class KineticInstrument : virtual public Instrument
        {
        public:
            //-----------------------------------------------------------------------
            // Types:
            typedef int Note;
            //-----------------------------------------------------------------------
            
            //-----------------------------------------------------------------------
            struct BeatData
            {
                KineticInstrument* instrument;
                Note   note;
                Volume volume;
                Time   timestamp;
                bool   userGenerated;
                math::PartOfOne vizProgress;
            };

            //-----------------------------------------------------------------------
            // Constants:
            static const int NotesNum;
            static const Note PauseN;
            static const Note QuickN;
            static const std::vector<Frequency> NotesFrequencies;
            
            static constexpr bool                VizUseImageBuffer = true;
            static constexpr math::CooMultiplier VizRenderScale = 0.5;
            
            static constexpr Time FreeSessionDuration = 15;
            static constexpr Time FreeCooldownDuration = 15;
            static constexpr Time MaxFreeSessions      = 10;
            static constexpr Time InstrumentFadeOutDuration = 1.0;
            static constexpr Time InstrumentStopTimeout = 3.0;
            //-----------------------------------------------------------------------
            
            //-----------------------------------------------------------------------
            // Public members:
            Time             unlockTimestamp;
            Time             freeRemaining;
            Time             freeCooldownRemaining;
            Time             stopInstrumentTimeout;
            std::string      name;
            std::string      iapProductID;
            bool             isFocused;
            bool             needsResetOrient;
            
            graphics::Image  icon;
            graphics::Image  iconLocked;
            graphics::Rect2D iconRect;
            graphics::Coo    iconZ;
            math::CooMultiplier iconScale;
            math::Stepper<graphics::Coo> iconShowHideAnim;
            
            //-----------------------------------------------------------------------
            KineticInstrument();
            ~KineticInstrument();
            bool IsLocked() const { return unlockTimestamp == 0; }
            bool CanPlay() const { return !IsLocked() || (freeRemaining > 0); }

            static void SetEngines(graphics::Graphics* graphics, yoss::AccEngine* acc) { _graphics = graphics; _acc = acc; }
            
            // Virtual class Instrument methods
            virtual Frequency UnnormalizeFrequency(PartOfOne normalized_freq);

            void UpdateInput(Angle geo_orientation, Angle acc_angle_around_x, Angle acc_angle_around_y);
            
            virtual void LoadSamples() = 0;
            virtual void LoadImages(graphics::Graphics* graphics) = 0;
            virtual void UpdateGraphicsSizes(const graphics::Rect2D& container, const graphics::Rect2D& bg_container, math::CooMultiplier global_scale);
            virtual void DrawInstrument() = 0;
            virtual void DrawEffectsAboveUI() {}
            virtual void OnGainFocus();
            virtual void OnLoseFocus();
            virtual void OnPointerEvent(input::Pointer* pointer) = 0;
            
            virtual void PlayDemo(Volume volume);
            virtual BeatData CreateBeatData(Note note, Volume volume, bool user_generated);
            
            Note      GetNoteFromNormFreq(PartOfOne normalized_freq);
            Frequency GetFreqFromNormFreq(PartOfOne normalized_freq);
            Frequency GetFreqFromNote(Note note);
            PartOfOne GetNormFreqFromNote(Note note);
            PartOfOne GetNoteResponse(PartOfOne normalized_freq, Note note);
            PartOfOne GetNormFreqFromAxisXAngle(Angle a, Angle axis_x_start_angle = -1, Angle axis_x_end_angle = -1);
            
        protected:
            void LoadImages(graphics::Graphics* graphics, const std::string& prefix, math::CooMultiplier icon_scale = 0.7);
            void DrawViz();
            void DrawInput(Angle geo_max, Angle axis_y_max, Angle axis_x_start_angle = -1, Angle axis_x_end_angle = -1);
            void SetSamplesNativeFrequencyFromNotes(std::vector<SamplerInstrument::SamplerSample>& samples);
            
            virtual void OnUpdateInput() {}
            
            static yoss::AccEngine*    _acc;
            static graphics::Graphics* _graphics;
            graphics::Rect2D    _container;
            math::CooMultiplier _globalScale;

            graphics::Rect2D    _vizContainer;
            graphics::Image*    _vizImage;
            graphics::KineticInstrumentViz* _viz;
            
            Angle _accAngleAroundX;
            Angle _accAngleAroundY;
            Angle _geoOrientationAngle;
        };
    }
}

