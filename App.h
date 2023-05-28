#pragma once

#include "KineticInstrument.h"
#include "yossCommon/common/Math.h"
#include "yossCommon/common/Input.h"
#include "yossCommon/graphics/common/Graphics.h"
#include "yossCommon/sound/SoundEngine.h"
#include "yossCommon/acc/AccEngine.h"
#include "yossCommon/iap/IAP.h"

#include <math.h>


namespace yoss
{
    using yoss::math::Time;
    using yoss::math::Angle;
    using yoss::math::PartOfOne;
    using graphics::Coo;
    using graphics::CooMultiplier;
    using graphics::Rect2D;
    using graphics::Color;
    using yoss::sound::Instrument;
    using yoss::sound::KineticInstrument;
    using yoss::sound::Volume;
    typedef yoss::sound::KineticInstrument::Note Note;
    
    //-----------------------------------------------------------------------
    enum LoadingStep
    {
        LoadingStep_None,
        LoadingStep_Init,
        LoadingStep_Splash,
        LoadingStep_UI1,
        LoadingStep_UI2,
        LoadingStep_UI3,
        LoadingStep_Instruments1,
        LoadingStep_Instruments2,
        LoadingStep_Instruments3,
        LoadingStep_Instruments4,
        LoadingStep_Instruments5,
        LoadingStep_Instruments6,
        LoadingStep_Instruments7,
        LoadingStep_Instruments8,
        LoadingStep_Instruments9,
        LoadingStep_Instruments10,
        LoadingStep_Instruments11,
        LoadingStep_Finish
    };
    
    //-----------------------------------------------------------------------
    class App : public yoss::iap::IAPEngineDelegate
    {
    public:
        App(yoss::input::Input* input, yoss::sound::SoundEngine* sound, yoss::AccEngine* acc);
        ~App();
        
        void SetGraphics(yoss::graphics::Graphics* graphics);
        
        void UpdateFrame();
        void DrawFrame();
        
        void Suspend();
        void Resume();
        
        // IAPEngineDelegate callbacks
        virtual void OnProductsDownloaded();
        virtual void OnProductsDownloadFailed();
        virtual void OnProductPurchased(const std::string& product_id);
        virtual void OnProductRestored(const std::string& product_id);
        virtual void OnProductCancelled(const std::string& product_id);
        virtual void OnRestoringComplete(bool success);
        
    protected:
        void DebugPrint(const std::string& msg);
        void OnPointerEvent(input::Pointer* pointer);
        void OnAccFeed(bool beat_detected);
        
        void LoadOptions();
        void UpdateOptionsToAppVersion();
        void SaveOptions();
        void SetOption(const std::string& option, const std::string& value);
        std::string GetOption(const std::string& option);
        int         GetOptionInt(const std::string& option);
        
        void UpdateLoading();
        
        void CreateInstruments();
        void CreateInstrument(KineticInstrument* instrument, const std::string& name, const std::string& product_id, bool needs_reset_orient);
        void SetCurrentInstrument(int instrument_index);

        void InitIAP();
        void OpenBuyInstrumentDialog(int instrument_index);
        void UnlockInstrument(int instrument_index);
        void RestorePurchases();
        void StartIsWaiting(bool waiting_for_restore);
        
        void OrderHelpVideo();
        void PlayHelpVideo();
        
        void OrderResetOrientation();
        Angle GetGeoOrientationAngle();
        
        // Auto melodies functionality
        bool IsNotePlayingAllowed();
        
        // ----- Graphics -----
        void UpdateGraphicsSizes(Coo simulate_w = 0, Coo simulate_h = 0, CooMultiplier simulate_scale = 0);
        void SwitchScreenSimulation();
        
        // Drawing of background
        void DrawBackground();
        
        // Drawing of current instrument
        void DrawBuyButtons();
        void DrawIsWaiting();
        void DrawResetOrientButton();
        void DrawCrosshair();
        
        // Drawing of instruments list
        void DrawInstrumentsList();
        void SetInstrumentsListVisibility(bool state_shown);
        bool IsInstrumentsListShown();
        void ScrollToInstrumentInList(int instrument_index);
        void ScrollInstrumentsList(Coo dragged_width, Coo distance_from_center);
        Coo  GetInstrumentsListCircleRadius();
        PartOfOne GetInstrumentsListBgVisiblePart();
        int   GetInstrumentIndexAtFrontFromAngle(Angle angle);
        Angle GetClosestInstrumentAngleFromAngle(Angle angle);
        
        // Drawing of help button
        void DrawHelpButton();
        
    protected:
        static constexpr bool ProductionMode = !true;
        static constexpr bool DebugApp = !ProductionMode;
        static const std::string SKU_DroneDrums;
        
        static constexpr Coo MasterScreenW = 750;
        static constexpr Coo MasterScreenH = 1334;
        static constexpr math::Ratio SimulateScreenRatio_Tallest = (1242.0 / 2688.0); // = 0.5625
        static constexpr math::Ratio SimulateScreenRatio_Widest = (3.0 / 4.0);
        static constexpr bool ShowSimulateScreensButton = true && !ProductionMode;
        static constexpr bool ShowAccTrajectoryButton = true && !ProductionMode;
        static constexpr Time MaxFrameDTForSimulations = 0.1;
        static constexpr int  FPS = 60;
#define SPRING_ACC(acc) (FPS == 30 ? acc : FPS == 60 ? acc / 2 : 1/0)
#define SPRING_DAMP(damp) (FPS == 30 ? damp : FPS == 60 ? math::constexprSqrt(damp) : 1/0)
    
        static constexpr Volume    LockedInstrumentDemoBeatVolume = 0.8;
        
        static constexpr Time ShowHideInstrumentsListDuration = 0.3;
        static constexpr Time ShowHideVideoDuration = 0.2;
        static constexpr Time ShowHideNotesPaneDuration = 0.6;
        static constexpr Time      InstrumentsSwitchTransitionDuration = 0.6;
        static constexpr PartOfOne InstrumentsSwitchTransitionDX       = 0.2;
        
        static constexpr Time          IsWaitingAnimDuration = 1.0;
        static constexpr Coo           IsWaitingAnimRadius   = 100;
        static constexpr CooMultiplier IsWaitingAnimScale    = 1.0;
        static constexpr Coo           IsWaitingAnimShadowDX = 10;
        
        static constexpr CooMultiplier SpritesParalaxDepthStrength = 0.4;	
        static constexpr CooMultiplier SpritesSpringDepthStrength = 50;
        static const     Color         BackgroundColor;
        static const     Color         InitialBackgroundColor;
        
        static constexpr PartOfOne     VisiblePartOfInstrumentsListBg = 1.0;
        static constexpr PartOfOne     VisiblePartOfMaxInstrumentsListBg = 1.0;
        static constexpr math::Ratio   ScreenAspectRatioForMaxInstrumentsListBg = 0.5;
        static constexpr CooMultiplier InstrumentsListShowHStep = SPRING_DAMP(0.5);
        static constexpr CooMultiplier InstrumentsListShowSpringDamp = SPRING_DAMP(0.75);
        static constexpr CooMultiplier InstrumentsListIconShowSpringAcc = SPRING_ACC(0.08);
        static constexpr CooMultiplier InstrumentsListIconShowSpringDamp = SPRING_DAMP(0.8);
        static constexpr PartOfOne     InstrumentsListCircleRadius = 0.38;
        static constexpr CooMultiplier InstrumentsListScrollSpringAcc = SPRING_ACC(0.05);
        static constexpr CooMultiplier InstrumentsListScrollSpringDamp = SPRING_DAMP(0.85);
        static constexpr CooMultiplier InstrumentsListScrollPressedSpringAcc = SPRING_ACC(0.7);
        static constexpr CooMultiplier InstrumentsListScrollPressedSpringDamp = SPRING_DAMP(0.6);
        static constexpr CooMultiplier InstrumentsListIconScaleFront = 1.3;
        static constexpr CooMultiplier InstrumentsListIconScaleBack = 0.75;
        static constexpr Coo           InstrumentsListIconYDepth = 40;
        static constexpr Coo           InstrumentsListIconsDY = -90;
        static constexpr Coo           InstrumentsListInteractionHeight = 205;
        static constexpr Time          AutoFocusCurrentInstrumentAfterInactivityTimeout = 5;
        static constexpr Time          InstrumentIsNewDuration = 5 * 24 * 3600;
        
        static constexpr Coo           HelpButtonSize = 120;
        static constexpr Coo           HelpButtonXInListBG = 594;
        static constexpr Coo           HelpButtonYInListBG = 215;
        static constexpr CooMultiplier HelpButtonAnimSpringAcc = SPRING_ACC(0.85);
        static constexpr CooMultiplier HelpButtonAnimSpringDamp = SPRING_DAMP(0.6);
        
        static constexpr Coo           FreeButtonDYInBuyButtonsBG = 34;
        static constexpr Coo           BuyButtonDYInBuyButtonsBG = 146;
        static constexpr Coo           RestoreButtonDYInBuyButtonsBG = 258;
        
        static constexpr CooMultiplier ResetOrientButtonScale = 1.0;
        static constexpr Coo           ResetOrientButtonPaddingX = 50;
        static constexpr Coo           ResetOrientButtonPaddingY = 0;
        static constexpr Coo           ResetOrientButtonDY = -80;
        
        yoss::input::Input* _input;
        yoss::graphics::Graphics* _graphics;
        yoss::sound::SoundEngine* _sound;
        yoss::AccEngine* _acc;
        yoss::iap::IAPEngine* _iap;
        
        std::map<std::string, std::string> _options;
        int _optionsVersion;
        
        LoadingStep _loadingStep;
        int  _framesCounter;
        Time _frameDt;
        Time _prevFrameTimestamp;
        Time _appStartTimestamp;
        bool _isWaitingForResponseToRestore;
        bool _isWaitingForResponseToBuy;
        bool _suppressInitialVideo;
        
        std::vector<KineticInstrument*> _instruments;
        KineticInstrument* _currentInstrument;
        int _currentInstrumentIndex;
        int _prevInstrumentIndex;
        Time _instrumentSwitchTimestamp;
        
        Time _lastActivityTimestamp;
        PartOfOne _pitch;
        Angle     _accAngleAroundX;
        Angle     _accAngleAroundY;
        Angle     _geoOrientationAngle;
        Angle     _geoOrientationBaseAngle;
        
        // ----- Graphics -----
        Coo _screenW, _screenH;
        CooMultiplier _globalScale;
        int _currentScreenSimulation;
        bool _switchScreenSimulationRequested;
        bool _showAccTrajectory;
        
        graphics::Image _splashImage;
        
        // Current instrument scene
        Rect2D _currentInstrumentRect;
        bool            _buyingButtonsShown;
        Rect2D          _buyingButtonsRect;
        graphics::Image _buyingButtonsBg;
        Rect2D          _buyInstrumentButtonRect;
        Rect2D          _restorePurchasesButtonRect;
        Rect2D          _freeSessionButtonRect;
        graphics::Image _freeSessionButtonBg;
        graphics::Image _crosshair;
        math::Stepper<PartOfOne> _isWaitingAnim;
        
        // Instruments list
        graphics::Image _instrumentsListBg;
        graphics::Image _instrumentsListFore;
        Rect2D _instrumentsListRect;
        Rect2D _instrumentsListInputRect;
        math::Stepper<PartOfOne> _instrumentsListShowAnim;
        Coo  _instrumentsListStaticYPos;
        int  _instrumentsListScrolledIndex;
        bool _instrumentsListLastScrolledLeft;
        math::Stepper<Angle> _instrumentsListScrollAngle;
        Angle _instrumentsListDAngle;
        int  _touchesInsideInstrumentsList;
        math::Velocity _instrumentsListSwipeSpeed;
        Time _lastInstrumentsListActivityTimestamp;
        
        // Help button
        graphics::Image _helpButtonBg;
        Rect2D _helpButtonRect;
        math::Stepper<PartOfOne> _helpButtonAnim;
        bool _helpVideoRequested;
        bool _helpVideoPlaying;
        
        // Reset orientation button
        graphics::Image _resetOrientButton;
        Rect2D _resetOrientButtonRect;
        Rect2D _resetOrientButtonClickRect;
    };
}

