#include "App.h"
#include "DroneInstrument.h"
#include "BozhinInstrument.h"
#include "DrumKitInstrument.h"
#include "VoiceInstrument.h"
#include "yossCommon/common/Log.h"
#include "yossCommon/common/System.h"

using namespace std;
using namespace yoss;
using namespace yoss::math;
using namespace yoss::input;
using namespace yoss::graphics;
using namespace yoss::sound;
using namespace yoss::iap;


//-----------------------------------------------------------------------
const string App::SKU_DroneDrums = "...";

const Color App::BackgroundColor = Color::FromBytes(25, 12, 36, 255);
const Color App::InitialBackgroundColor = Color::FromBytes(25, 12, 36, 255);


//-----------------------------------------------------------------------
App::App(yoss::input::Input* input, yoss::sound::SoundEngine* sound, yoss::AccEngine* acc) :
    _input(input),
    _sound(sound),
    _acc(acc),
    _iap(nullptr),
    _buyingButtonsShown(false),
    _isWaitingForResponseToBuy(false),
    _isWaitingForResponseToRestore(false),
    _isWaitingAnim(0, EaseType_SlowStartAndEnd, 0, 1.0),
    _suppressInitialVideo(false),
    _optionsVersion(100),
    _currentInstrument(nullptr),
    _currentInstrumentIndex(-1),
    _prevInstrumentIndex(-1),
    _instrumentSwitchTimestamp(0),
    _instrumentsListShowAnim(0, EaseType_SlowStartAndEnd, 0, InstrumentsListShowHStep),
    _instrumentsListScrolledIndex(-1),
    _instrumentsListScrollAngle(0, EaseType_SlowStartAndEnd, 0, 0, InstrumentsListScrollSpringAcc, InstrumentsListScrollSpringDamp),
    _instrumentsListDAngle(10),
    _instrumentsListLastScrolledLeft(false),
    _touchesInsideInstrumentsList(0),
    _instrumentsListSwipeSpeed(0),
    _helpVideoRequested(false),
    _helpVideoPlaying(false),
    _helpButtonAnim(0, EaseType_SlowStart, 0, 0, HelpButtonAnimSpringAcc, HelpButtonAnimSpringDamp),
    _screenW(0),
    _screenH(0),
    _globalScale(1.0),
    _currentScreenSimulation(-1),
    _switchScreenSimulationRequested(false),
    _showAccTrajectory(false),
    _geoOrientationBaseAngle(0),
    _geoOrientationAngle(0),
    _framesCounter(0),
    _frameDt(1.0),
    _prevFrameTimestamp(0),
    _appStartTimestamp(system::GetCurrentTimestamp()),
    _loadingStep(LoadingStep_Init),
    _lastActivityTimestamp(system::GetCurrentTimestamp()),
    _lastInstrumentsListActivityTimestamp(system::GetCurrentTimestamp())
{
    ASSERT(_input && _sound && _acc);
    SeedRandomGenerator();
    LoadOptions();
    UpdateOptionsToAppVersion();
    CreateInstruments();
    InitIAP();
}

//-----------------------------------------------------------------------
App::~App()
{
    for (auto instrument : _instruments)
        delete instrument;
    
    _instruments.clear();
    
    if (_iap)
        delete _iap;
}

//-----------------------------------------------------------------------
void App::Suspend()
{
    
}

//-----------------------------------------------------------------------
void App::Resume()
{
    _lastActivityTimestamp = system::GetCurrentTimestamp();
    _lastInstrumentsListActivityTimestamp = system::GetCurrentTimestamp();
    _touchesInsideInstrumentsList = 0;
}

//-----------------------------------------------------------------------
void App::DebugPrint(const std::string& msg)
{
    if (App::DebugApp)
        Log::LogText(msg);
}

//-----------------------------------------------------------------------
void App::SetGraphics(yoss::graphics::Graphics* graphics)
{
    _graphics = graphics;
    ASSERT(_graphics);
}

//-----------------------------------------------------------------------
void App::UpdateOptionsToAppVersion()
{
    if (_optionsVersion == 100)
    {
        
    }
}

//-----------------------------------------------------------------------
void App::LoadOptions()
{
    auto filename = system::GetOutputPath("options.bin");
    if (system::FileExists(filename))
        _options = system::LoadMap(filename);
    
    if (_options.find("version") != _options.end())
        _optionsVersion = GetOptionInt("version");
    
    // Get default options from resources
    auto default_filename = system::GetResourcePath("options.bin");
    auto default_options = system::LoadMap(default_filename);
    
    // Set default values of missing options
    for (auto op_it : default_options)
        if (_options.find(op_it.first) == _options.end())
        {
            this->DebugPrint("Adding option '" + op_it.first + "' with default value");
            _options[op_it.first] = op_it.second;
        }
    
    
    ASSERT(_options.size() > 0);
}

//-----------------------------------------------------------------------
void App::SaveOptions()
{
    bool success = system::SaveMap(system::GetOutputPath("options.bin"), _options);
    ASSERT(success);
}

//-----------------------------------------------------------------------
std::string App::GetOption(const std::string& option)
{
    ASSERT(_options.find(option) != _options.end());
    
    return _options[option];
}

//-----------------------------------------------------------------------
int App::GetOptionInt(const std::string& option)
{
    return atoi(GetOption(option).c_str());
}

//-----------------------------------------------------------------------
void App::SetOption(const std::string& option, const std::string& value)
{
    ASSERT(_options.find(option) != _options.end());
    
    _options[option] = value;
    
    SaveOptions();
}

//-----------------------------------------------------------------------
void App::InitIAP()
{
    vector<string> all_skus;
    vector<bool> all_is_bought;
    for (auto instrument : _instruments)
        if (instrument->iapProductID.length() > 0)
        {
            if (std::find(all_skus.begin(), all_skus.end(), instrument->iapProductID) != all_skus.end())
                continue;
            
            all_skus.push_back(instrument->iapProductID);
            all_is_bought.push_back(!instrument->IsLocked());
        }
    
    _iap = new IAPEngine(this, all_skus, all_is_bought);
	if (all_skus.size() > 0)
    	_iap->RequestProductsDetails();
}

//-----------------------------------------------------------------------
void App::OpenBuyInstrumentDialog(int instrument_index)
{
    ASSERT(instrument_index >= 0 && instrument_index < _instruments.size());
    
    auto& instrument = _instruments[instrument_index];
    ASSERT(instrument->IsLocked() && _currentInstrument->iapProductID.length() > 0);
    if (!instrument->IsLocked() || _currentInstrument->iapProductID.length() == 0) return;
    
    this->DebugPrint("OpenBuyInstrumentDialog() for instrument " + instrument->name);
    StartIsWaiting(false);
    if (ProductionMode)
        _iap->BuyProduct(_currentInstrument->iapProductID);
    else
        OnProductPurchased(_currentInstrument->iapProductID);
}

//-----------------------------------------------------------------------
void App::UnlockInstrument(int instrument_index)
{
    bool index_ok = (instrument_index >= 0 && instrument_index < _instruments.size());
    ASSERT(index_ok);
    if (!index_ok) return;
    
    auto& instrument = _instruments[instrument_index];

    if (!instrument->IsLocked())
    {
        this->DebugPrint("Unlocking instrument " + instrument->name + " skipped - instrument already unlocked");
        return;
    }
    
    this->DebugPrint("Unlocking instrument " + instrument->name);
    instrument->unlockTimestamp = system::GetCurrentTimestamp();
    SetOption(instrument->name, std::to_string((int)instrument->unlockTimestamp));
    
    if (_currentInstrument)
    {
        _buyingButtonsShown = _currentInstrument->IsLocked();
    }
}

//-----------------------------------------------------------------------
void App::RestorePurchases()
{
    this->DebugPrint("Restoring previous purchases...");
    
    // Cleaning current purchases records
    if (!true)
    for (auto instrument : _instruments)
        if (instrument->unlockTimestamp > 1)
        {
            this->DebugPrint("Cleaning purchase for " + instrument->name);
            instrument->unlockTimestamp = 0;
        }
    
    StartIsWaiting(true);
    _iap->RestoreBoughtProducts();
}

//-----------------------------------------------------------------------
void App::StartIsWaiting(bool waiting_for_restore)
{
    _isWaitingForResponseToBuy = !waiting_for_restore;
    _isWaitingForResponseToRestore = waiting_for_restore;
    _isWaitingAnim.SetPos(0);
    _isWaitingAnim.SetMovement(1.0, IsWaitingAnimDuration);
    
    _suppressInitialVideo = true;
}

//-----------------------------------------------------------------------
void App::CreateInstrument(KineticInstrument* instrument, const std::string& name, const std::string& product_id, bool needs_reset_orient)
{
    instrument->name = name;
    instrument->iapProductID = product_id;
    instrument->unlockTimestamp = GetOptionInt(instrument->name);
    instrument->needsResetOrient = needs_reset_orient;
    this->DebugPrint("Creating instrument #" + Log::ToStr((int)_instruments.size()) + " (" + name + ") with unlockTimestamp=" + Log::ToStr(instrument->unlockTimestamp, 1));
    
    _instruments.push_back(instrument);
}

//-----------------------------------------------------------------------
void App::CreateInstruments()
{
    KineticInstrument::SetEngines(_graphics, _acc);
    
    CreateInstrument(new BozhinInstrument(), "iSynth", "", true);
    CreateInstrument(new DroneInstrument(), "iDrone", "", true);
    CreateInstrument(new DrumKitInstrument(), "iDrumKit", "", true);
	CreateInstrument(new VoiceInstrument(), "iVoice", "", true);
	
	// Making all 3 instruments free as of v1.1.2
	for (int i = 0; i < _instruments.size(); i++)
		if (_instruments[i]->IsLocked())
			UnlockInstrument(i);
    
    _instrumentsListDAngle = 360.0 / _instruments.size();
}

//-----------------------------------------------------------------------
void App::SetCurrentInstrument(int instrument_index)
{
    ASSERT(instrument_index >= 0 && instrument_index < _instruments.size());
    if (instrument_index == _currentInstrumentIndex) return;
    
    this->DebugPrint("Setting current instrument " +
                 (instrument_index >= 0 ? _instruments[instrument_index]->name : "(none)") +
                 " from " + (_currentInstrumentIndex >= 0 ? _instruments[_currentInstrumentIndex]->name : "(none)"));
    
    if (_currentInstrument)
    {
        _instrumentSwitchTimestamp = system::GetCurrentTimestamp();
        _prevInstrumentIndex = _currentInstrumentIndex;
        _currentInstrument->OnLoseFocus();
    }
    
    _currentInstrumentIndex = _instrumentsListScrolledIndex = instrument_index;
    _currentInstrument = _instruments[_currentInstrumentIndex];
    _currentInstrument->OnGainFocus();
    
    if (_currentInstrument && !_sound->IsPlayingInstrument(_currentInstrument))
        _sound->AddInstrument((Instrument*)_currentInstrument);
    
    _buyingButtonsShown = _currentInstrument->IsLocked();

    if (_currentInstrumentIndex != GetOptionInt("current_i"))
        SetOption("current_i", to_string(_currentInstrumentIndex));
    
    if (_currentInstrument->IsLocked())
        _currentInstrument->PlayDemo(LockedInstrumentDemoBeatVolume);
}

//-----------------------------------------------------------------------
void App::OnProductsDownloaded()
{
}

//-----------------------------------------------------------------------
void App::OnProductsDownloadFailed()
{
}

//-----------------------------------------------------------------------
void App::OnProductPurchased(const std::string& product_id)
{
    _isWaitingForResponseToBuy = false;
    
    for (int i = 0; i < _instruments.size(); i++)
        if (_instruments[i]->iapProductID == product_id &&
            _instruments[i]->IsLocked())
            UnlockInstrument(i);
}

//-----------------------------------------------------------------------
void App::OnProductRestored(const std::string& product_id)
{
    OnProductPurchased(product_id);
}

//-----------------------------------------------------------------------
void App::OnProductCancelled(const std::string& product_id)
{
    _isWaitingForResponseToBuy = false;
}

//-----------------------------------------------------------------------
void App::OnRestoringComplete(bool success)
{
    ASSERT(_isWaitingForResponseToRestore);
    _isWaitingForResponseToRestore = false;
}

//-----------------------------------------------------------------------
void App::PlayHelpVideo()
{
    system::OrderVideoPlaying("video_" + _currentInstrument->name);
    _lastActivityTimestamp = system::GetCurrentTimestamp() + 10;

    int video_plays = GetOptionInt("video_plays");
    SetOption("video_plays", to_string(video_plays + 1));
    this->DebugPrint("Video is ordered");
}

//-----------------------------------------------------------------------
void App::OrderHelpVideo()
{
    if (!_helpVideoPlaying)
        _helpVideoRequested = true;
}

//-----------------------------------------------------------------------
void App::OrderResetOrientation()
{
    //_geoOrientationBaseAngle = _acc->GetMagTrajectory().GetPositions().GetAverage(0, FPS).z;
    //_geoOrientationBaseAngle *= 180;
    _acc->ResetInput();
    // _geoOrientationBaseAngle = _geoOrientationAngle;
}

//-----------------------------------------------------------------------
Angle App::GetGeoOrientationAngle()
{
    Angle a = _geoOrientationAngle - _geoOrientationBaseAngle;
    while (a > 180) a -= 180;
    while (a < -180) a += 180;
    return a;
}

//-----------------------------------------------------------------------
void App::OnPointerEvent(input::Pointer* pointer)
{
    ASSERT(pointer && _input);
    Point2D pointer_pos = { pointer->x, pointer->y };
    bool inside_instruments_list = _instrumentsListInputRect.Contains(pointer_pos);
    bool was_inside_instruments_list = _instrumentsListInputRect.Contains({ pointer->x - pointer->dx, pointer->y - pointer->dy });
    
    //_graphics->DrawRect(true, pointer_pos, 5, COLOR_RED);
    
    if (inside_instruments_list)
    {
        _lastInstrumentsListActivityTimestamp = system::GetCurrentTimestamp();
    }
    
    if (pointer->state == PointerState_Begin)
    {
        if (inside_instruments_list)
            _touchesInsideInstrumentsList++;
    }
    else if (pointer->state == PointerState_Move)
    {
        if (inside_instruments_list && !was_inside_instruments_list)
            _touchesInsideInstrumentsList++;
        else if (!inside_instruments_list && was_inside_instruments_list)
            _touchesInsideInstrumentsList--;
        
        if (inside_instruments_list)
        {
            Coo dist_from_center = pointer->x - _instrumentsListInputRect.xCenter();
            ScrollInstrumentsList(pointer->dx, dist_from_center);
        }
    }
    else if (pointer->state == PointerState_End)
    {
        if (pointer->isClick && _helpButtonRect.Contains(pointer_pos))
        {
            OrderHelpVideo();
        }
        else if (pointer->isClick && _resetOrientButtonClickRect.Contains(pointer_pos) &&
            _currentInstrument && _currentInstrument->needsResetOrient)
        {
            _acc->ResetInput();
        }
        else if (inside_instruments_list)
        {
            _touchesInsideInstrumentsList--;
        
            if (pointer->isClick)
            {
                int clicked_index = -1;
                Coo clicked_z = 100;
                
                for (int i = 0; i < _instruments.size(); i++)
                {
                    auto& instrument = _instruments[i];
                    if (instrument->iconRect.Contains(pointer_pos) &&
                        instrument->iconZ < clicked_z)
                    {
                        clicked_index = i;
                        clicked_z = instrument->iconZ;
                    }
                }
                
                if (clicked_index >= 0 &&
                    clicked_index != _currentInstrumentIndex)
                {
                    ScrollToInstrumentInList(clicked_index);
                    SetCurrentInstrument(clicked_index);
                }
            }
        }
        else
        {
            if (_buyingButtonsShown && _currentInstrument &&
                _currentInstrument->freeCooldownRemaining == 0 &&
                _currentInstrument->freeRemaining == 0)
            {
                if (pointer->isClick && _buyInstrumentButtonRect.Contains(pointer_pos))
                    OpenBuyInstrumentDialog(_currentInstrumentIndex);
                
                if (pointer->isClick && _restorePurchasesButtonRect.Contains(pointer_pos))
                    RestorePurchases();
                
                if (pointer->isClick && _freeSessionButtonRect.Contains(pointer_pos))
                    _currentInstrument->freeRemaining = KineticInstrument::FreeSessionDuration;
            }
        }
        
        if (pointer->isClick && ShowSimulateScreensButton &&
            pointer->x > _screenW - 80 && pointer->y > _screenH - 80)
            _switchScreenSimulationRequested = true;
        
        if (pointer->isClick && ShowAccTrajectoryButton &&
            pointer->x > _screenW - 80 &&
            pointer->y > _screenH - 160 &&
            pointer->y < _screenH - 80)
            _showAccTrajectory = !_showAccTrajectory;
    }
    
    if (_currentInstrument && _currentInstrumentRect.Contains(pointer_pos))
        _currentInstrument->OnPointerEvent(pointer);
}

//-----------------------------------------------------------------------
void App::OnAccFeed(bool beat_detected)
{
    ASSERT(_acc && _sound);
    if (_helpVideoPlaying) return;
    
    auto& trajectory = _acc->GetAccTrajectory();
    auto& mag_trajectory = _acc->GetMagTrajectory();
    
    Vector3D mag_pos = mag_trajectory.GetPositions().Get();
    Vector3D pos = trajectory.GetPositions().GetAverage(0, 10);
    bool upper_hemisphere = (pos.AngleTo(Z_AXIS) < DegToRad(90));
    
    _geoOrientationAngle = mag_pos.z * 180;
    
    _accAngleAroundX = PointToDeg(-pos.z, -pos.y);
    
    //_accAngleAroundY = PointToDeg(-pos.z, pos.x) - (upper_hemisphere ? 180 : 0);
    _accAngleAroundY = mag_pos.y * (mag_pos.y >= 1 ? -180 : 180);
    _accAngleAroundY = _accAngleAroundY - 0.0093 * (_geoOrientationAngle * _accAngleAroundX);
    
    //static int every_nth_frame = 0;
    //if (every_nth_frame-- <= 0) { this->DebugPrint("geoA = " + Log::ToStr(GetGeoOrientationAngle()) + ", angleAroundY = " + Log::ToStr(_accAngleAroundY)); every_nth_frame = 50; }

    if (!_currentInstrument) return;
    
    _currentInstrument->UpdateInput(GetGeoOrientationAngle(), _accAngleAroundX, _accAngleAroundY);
    
    if (beat_detected)
    {
        auto beat_amplitude = _acc->GetAccBeatAmplitude();
        auto beat_freq = _acc->GetAccBeatFrequency();
        
        _currentInstrument->AddBeat(beat_freq, beat_amplitude);
    }
}

//-----------------------------------------------------------------------
void App::UpdateFrame()
{
    ASSERT(_graphics && _input);
    
    if (_instrumentSwitchTimestamp == 0)
        _instrumentSwitchTimestamp = system::GetCurrentTimestamp();
    
    for (auto instrument : _instruments)
    {
        if (instrument->stopInstrumentTimeout > 0)
        {
            instrument->stopInstrumentTimeout -= _frameDt;
            if (instrument->stopInstrumentTimeout <= 0)
            {
                instrument->stopInstrumentTimeout = 0;
                this->DebugPrint("Removing instrument " + instrument->name);
                _sound->RemoveInstrument((Instrument*)instrument);
            }
        }
        else if (instrument->freeRemaining > 0)
        {
            instrument->freeRemaining -= _frameDt;
            if (instrument->freeRemaining <= 0)
            {
                instrument->freeRemaining = 0;
                instrument->freeCooldownRemaining = KineticInstrument::FreeCooldownDuration;
                this->DebugPrint("Ending free session for " + instrument->name);
            }
        }
        else if (instrument->freeCooldownRemaining > 0)
        {
            instrument->freeCooldownRemaining -= _frameDt;
            if (instrument->freeCooldownRemaining <= 0)
            {
                instrument->freeCooldownRemaining = 0;
                this->DebugPrint("Ending free cooldown for " + instrument->name);
            }
        }
    }
}

//-----------------------------------------------------------------------
void App::UpdateLoading()
{
    DebugPrint("Loading step " + Log::ToStr(_loadingStep));

    switch(_loadingStep)
    {
        case LoadingStep_Init:
            UpdateGraphicsSizes();
            break;
        case LoadingStep_Splash:
            _splashImage.LoadFromFile("splash.png");
            _splashImage.CreateTexture(_graphics);
            break;
        case LoadingStep_UI1:
            _instrumentsListBg.LoadFromFile("top_menu_bg.png");
            _instrumentsListBg.CreateTexture(_graphics);
            
            _instrumentsListFore.LoadFromFile("top_menu_fore.png");
            _instrumentsListFore.CreateTexture(_graphics);
            break;
            
        case LoadingStep_UI2:
        {
            
            break;
        }
        case LoadingStep_UI3:
            
            
            _helpButtonBg.LoadFromFile("help_button.png");
            _helpButtonBg.CreateTexture(_graphics);
            
            _buyingButtonsBg.LoadFromFile("buying_buttons.png");
            _buyingButtonsBg.CreateTexture(_graphics);
            
            _resetOrientButton.LoadFromFile("center_button.png");
            _resetOrientButton.CreateTexture(_graphics);
            
            _freeSessionButtonBg.LoadFromFile("free_button.png");
            _freeSessionButtonBg.CreateTexture(_graphics);
            
            _crosshair.LoadFromFile("crosshair.png");
            _crosshair.CreateTexture(_graphics);
            break;
            
        case LoadingStep_Instruments1:
            break;
            
        case LoadingStep_Instruments2:
            _instruments[0]->LoadSamples();
            break;
        case LoadingStep_Instruments3:
            _instruments[0]->LoadImages(_graphics);
            break;
        case LoadingStep_Instruments4:
            _instruments[1]->LoadSamples();
            break;
        case LoadingStep_Instruments5:
            _instruments[1]->LoadImages(_graphics);
            break;
        case LoadingStep_Instruments6:
            _instruments[2]->LoadSamples();
            break;
        case LoadingStep_Instruments7:
            _instruments[2]->LoadImages(_graphics);
            break;
        case LoadingStep_Instruments8:
            if (_instruments.size() > 3)
                _instruments[3]->LoadSamples();
            break;
        case LoadingStep_Instruments9:
            if (_instruments.size() > 3)
                _instruments[3]->LoadImages(_graphics);
            break;
        case LoadingStep_Instruments10:
            if (_instruments.size() > 4)
                _instruments[4]->LoadSamples();
            break;
        case LoadingStep_Instruments11:
            if (_instruments.size() > 4)
                _instruments[4]->LoadImages(_graphics);
            break;
            
        case LoadingStep_Finish:
        {
            _input->AddPointerListener([this] (Pointer* pointer) { OnPointerEvent(pointer); });
            _acc->AddAccFeedListener([this] (bool beat_detected) { OnAccFeed(beat_detected); });
            
            int current_i = GetOptionInt("current_i");
            if (_instruments[current_i]->IsLocked())
                current_i = 0;
            
            SetCurrentInstrument(current_i);
            ScrollToInstrumentInList(current_i);
            SetInstrumentsListVisibility(true);
            
            _screenW = 0; // Causes UpdateGraphicsSizes() to recalculate sizes
            _loadingStep = LoadingStep_None;
            break;
        }
        default:
            ASSERT(false);
    }
    
    _graphics->ClearScreen(BackgroundColor);
    
    if (_splashImage.GetWidth() > 0)
    {
        Rect2D rect = { { 0, 0 }, { (Coo)_splashImage.GetWidth() * _globalScale, (Coo)_splashImage.GetHeight() * _globalScale } };
        rect.Translate((_screenW - rect.w()) / 2, (_screenH - rect.h()) / 2);
        Color col = COLOR_WHITE.WithAlpha((float)_loadingStep / LoadingStep_Finish);
        _graphics->DrawImage(_splashImage, rect, col, col, col, col);
    }

    DebugPrint("End of loading step " + Log::ToStr(_loadingStep));
    
    if (_loadingStep != LoadingStep_None)
        _loadingStep = (LoadingStep)(_loadingStep + 1);
}

//-----------------------------------------------------------------------
bool App::IsNotePlayingAllowed()
{
    return !(_helpVideoPlaying || _helpVideoRequested || _currentInstrument->IsLocked());
}


//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// GRAPHICS
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
void App::DrawFrame()
{
    ASSERT(_graphics);
    _graphics->StartFrame();

    if (_showAccTrajectory)
    {
        _graphics->ClearScreen(COLOR_BLACK);
        _acc->DrawTrajectory(_graphics);
        _graphics->PopCurrentTransforms();
        _graphics->PushCurrentTransforms_UseScreenPixels();
        _graphics->DrawRect(true, { _graphics->GetWidth() - 40, _graphics->GetHeight() - 120 } , 35, Color::FromFloats(1, 0, 0, 0.7));
        return;
    }
    
    _graphics->PushCurrentTransforms_UseScreenPixels();
    
    Time current_time = system::GetCurrentTimestamp();
    _frameDt = current_time - _prevFrameTimestamp;
    _prevFrameTimestamp = current_time;
    _framesCounter++;
    
    if (_loadingStep != LoadingStep_None)
    {
        UpdateLoading();
        return;
    }

    UpdateFrame();
    UpdateGraphicsSizes();

    if (_switchScreenSimulationRequested)
    {
        _switchScreenSimulationRequested = false;
        SwitchScreenSimulation();
    }

    DrawBackground();

    //if (_prevInstrumentIndex >= 0 && current_time - _instrumentSwitchTimestamp < InstrumentsSwitchTransitionDuration)
    //    DrawInstrument(_prevInstrumentIndex);

    if (_currentInstrument)
        _currentInstrument->DrawInstrument();
    
    if (_isWaitingForResponseToBuy || _isWaitingForResponseToRestore)
        DrawIsWaiting();
    else if (_buyingButtonsShown)
        DrawBuyButtons();
    
    DrawInstrumentsList();
    DrawHelpButton();
    DrawResetOrientButton();
    DrawCrosshair();
    
    if (_currentInstrument)
        _currentInstrument->DrawEffectsAboveUI();

    if (_currentScreenSimulation >= 0)
    {
        _graphics->DrawRect(true, _screenW, 0, _screenW + 5000, _screenH, Color::FromFloats(0.5, 0.5, 0.5, 1));
        _graphics->DrawRect(true, 0, _screenH, _screenW + 5000 , _screenH + 5000, Color::FromFloats(0.5, 0.5, 0.5, 1));
    }
    
    if (ShowSimulateScreensButton)
        _graphics->DrawRect(true, { _graphics->GetWidth() - 40, _graphics->GetHeight() - 40 } , 35, Color::FromFloats(0, 1, 0, 0.7));
    
    if (ShowAccTrajectoryButton)
        _graphics->DrawRect(true, { _graphics->GetWidth() - 40, _graphics->GetHeight() - 120 } , 35, Color::FromFloats(1, 0, 0, 0.5));
}

//-----------------------------------------------------------------------
void App::SwitchScreenSimulation()
{
    _currentScreenSimulation++;
    switch (_currentScreenSimulation)
    {
        case 0: UpdateGraphicsSizes(1242, 2688, 0); break;
        //case 1: UpdateGraphicsSizes(1242, 2688, 0.3); break;
        //case 2: UpdateGraphicsSizes(1242, 2688, 1); break;
        //case 3: UpdateGraphicsSizes(600, 800, 0); break;
        //case 4: UpdateGraphicsSizes(600, 800, 0.5); break;
        case 1: UpdateGraphicsSizes(2048, 2732, 0); break;
        default:
            _currentScreenSimulation = -1;
            UpdateGraphicsSizes(_graphics->GetWidth(), _graphics->GetHeight(), 0);
    }
}

//-----------------------------------------------------------------------
void App::UpdateGraphicsSizes(Coo simulate_w, Coo simulate_h, CooMultiplier simulate_scale)
{
    Coo orig_screen_w = _graphics->GetWidth();
    Coo orig_screen_h = _graphics->GetHeight();
    Coo screen_w = _currentScreenSimulation >= 0 ? _screenW : orig_screen_w;
    Coo screen_h = _currentScreenSimulation >= 0 ? _screenH : orig_screen_h;
    
    if (simulate_w > 0 && simulate_h > 0)
    {
        Ratio orig_screen_ratio = orig_screen_w / orig_screen_h;
        Ratio sim_screen_ratio = simulate_w / simulate_h;
        
        if (simulate_scale != 0)
        {
            screen_w = simulate_w * simulate_scale;
            screen_h = simulate_h * simulate_scale;
        }
        else
        {
            if (sim_screen_ratio > orig_screen_ratio)
            {
                screen_w = orig_screen_w;
                screen_h = screen_w / sim_screen_ratio;
            }
            else
            {
                screen_w = orig_screen_h * sim_screen_ratio;
                screen_h = orig_screen_h;
            }
        }
    }
    
    if (screen_w == _screenW && screen_h == _screenH) return;
    _screenW = screen_w;
    _screenH = screen_h;
    _globalScale = _screenW / MasterScreenW;
    
    Ratio screen_ratio = _screenW / _screenH;
    Ratio master_screen_ratio = MasterScreenW / MasterScreenH;
    
    this->DebugPrint("App::UpdateGraphicsSizes(): screen(" + Log::ToStr(screen_w) + ", " + Log::ToStr(screen_h) + ")  scale = " + Log::ToStr(_globalScale, 2));
    
    _graphics->SetCameraFromScreen(20, 0.1, 100);
    
    if (_loadingStep != LoadingStep_None) return;
    
    Ratio bg_aspect_ratio = _instrumentsListBg.GetWidth() * 1.0 / _instrumentsListBg.GetHeight();
    _instrumentsListStaticYPos = _instrumentsListBg.GetHeight() * (VisiblePartOfInstrumentsListBg - 1) * _globalScale;
    
    Coo orig_list_ypos = _instrumentsListStaticYPos;
    if (screen_ratio < master_screen_ratio)
    {
        Ratio clamped_scr_ratio = MAX(screen_ratio, ScreenAspectRatioForMaxInstrumentsListBg);
        Ratio ratio = 1 - (clamped_scr_ratio - ScreenAspectRatioForMaxInstrumentsListBg) / (master_screen_ratio - ScreenAspectRatioForMaxInstrumentsListBg);
        _instrumentsListStaticYPos += _instrumentsListBg.GetHeight() * (VisiblePartOfMaxInstrumentsListBg - VisiblePartOfInstrumentsListBg) * ratio * _globalScale;
    }
    _instrumentsListStaticYPos = MIN(1, _instrumentsListStaticYPos);
    _instrumentsListRect = { { 0, _instrumentsListStaticYPos }, { screen_w, _instrumentsListStaticYPos - 1 + screen_w / bg_aspect_ratio } };
    _instrumentsListInputRect = _instrumentsListRect;
    _instrumentsListInputRect.max.y = _instrumentsListInputRect.min.y + InstrumentsListInteractionHeight * _globalScale;
    
    _currentInstrumentRect = { { 0, _instrumentsListRect.max.y }, { screen_w, screen_h } };
    Rect2D instrument_bg_rect = { { -1, -1 }, { screen_w + 1, screen_h + 1 } };
    for (auto instrument : _instruments)
        instrument->UpdateGraphicsSizes(_currentInstrumentRect, instrument_bg_rect, _globalScale);
    
    Coo instruments_list_bottom = _instrumentsListRect.max.y;
    
    Coo help_b_w = HelpButtonSize * _globalScale;
    Coo help_b_h = HelpButtonSize * _globalScale;
    Coo help_b_y = _instrumentsListRect.min.y + HelpButtonYInListBG * _globalScale;
    Coo help_b_x = _instrumentsListRect.min.x + HelpButtonXInListBG * _globalScale;
    _helpButtonRect = Rect2D::WithSizes(help_b_x, help_b_y, help_b_w, help_b_h);
    
    Coo reset_b_w = _resetOrientButton.GetWidth() * ResetOrientButtonScale * _globalScale;
    Coo reset_b_h = _resetOrientButton.GetHeight() * ResetOrientButtonScale * _globalScale;
    Coo reset_b_y = instruments_list_bottom + ResetOrientButtonDY * _globalScale;
    Coo reset_b_x = (_screenW - reset_b_w) * 0.5;
    _resetOrientButtonRect = Rect2D::WithSizes(reset_b_x, reset_b_y, reset_b_w, reset_b_h);
    _resetOrientButtonClickRect = Rect2D::WithSizes(reset_b_x + ResetOrientButtonPaddingX, reset_b_y + ResetOrientButtonPaddingY, reset_b_w - 2 * ResetOrientButtonPaddingX, reset_b_h - 2 * ResetOrientButtonPaddingX);
    
    Coo buttons_y = _screenH - _buyingButtonsBg.GetHeight() * _globalScale;
    Coo button_w = _freeSessionButtonBg.GetWidth() * _globalScale;
    Coo button_h = _freeSessionButtonBg.GetHeight() * _globalScale;
    Coo button_x = (_screenW - button_w) * 0.5;
    
    _buyingButtonsRect = Rect2D::WithMaxPoint(0, buttons_y, _screenW - 1, _screenH - 1);
    
    Coo free_b_y = buttons_y + FreeButtonDYInBuyButtonsBG * _globalScale;
    _freeSessionButtonRect = Rect2D::WithSizes(button_x, free_b_y, button_w, button_h);
    
    Coo buy_b_y = buttons_y + BuyButtonDYInBuyButtonsBG * _globalScale;
    _buyInstrumentButtonRect = Rect2D::WithSizes(button_x, buy_b_y, button_w, button_h);
    
    Coo restore_b_y = buttons_y + RestoreButtonDYInBuyButtonsBG * _globalScale;
    _restorePurchasesButtonRect = Rect2D::WithSizes(button_x, restore_b_y, button_w, button_h);
}

//-----------------------------------------------------------------------
void App::DrawBackground()
{
    _graphics->ClearScreen(BackgroundColor);
}

//-----------------------------------------------------------------------
void App::DrawResetOrientButton()
{
    if (!_currentInstrument || !_currentInstrument->needsResetOrient) return;
    
    Angle geo = abs(GetGeoOrientationAngle());
    ColorC alpha = (geo < 90 ? 0.15 + geo * 0.005 : 0.5 + abs(sin(system::GetCurrentTimestamp() * 5.0)) * 0.5);
    Color col = COLOR_WHITE.WithAlpha(alpha);
    _graphics->DrawImage(_resetOrientButton, _resetOrientButtonRect, col, col, col, col);
}

//-----------------------------------------------------------------------
void App::DrawHelpButton()
{
    if (_frameDt < MaxFrameDTForSimulations)
        _helpButtonAnim.UpdateMovement(_frameDt);
    Coo anim_pos = _helpButtonAnim.UpdateLagged();
    
    if (_helpVideoRequested)
    {
        bool is_video_closed = (_helpButtonAnim.GetPos() == 0);
        bool is_video_opened = (_helpButtonAnim.GetPos() == 1 && ABS(_helpButtonAnim.GetSpringSpeed()) < 0.01 && ABS(1 - anim_pos) < 0.02);
        
        if (is_video_closed)
        {
            _helpButtonAnim.SetMovement(1, ShowHideVideoDuration);
        }
        else if (is_video_opened)
        {
            PlayHelpVideo();
            _helpVideoRequested = false;
            _helpVideoPlaying = true;
        }
    }
    else if (_helpVideoPlaying && system::IsVideoPlayingFinished())
    {
        _helpVideoPlaying = false;
        _helpButtonAnim.SetMovement(0, ShowHideVideoDuration);
    }
    
    if (!IsInstrumentsListShown()) return;
    
    CooMultiplier max_scale_x = _screenW / _helpButtonRect.w();
    CooMultiplier max_scale_y = _screenH / _helpButtonRect.h();
    CooMultiplier max_scale = MAX(max_scale_x, max_scale_y);
    CooMultiplier rect_scale = Interpolate(1, 1 + 2 * max_scale, anim_pos);
    Coo rect_hw = _helpButtonRect.w() * rect_scale / 2;
    Coo rect_hh = _helpButtonRect.h() * rect_scale / 2;
    Coo rect_cx = Interpolate(_helpButtonRect.xCenter(), _screenW / 2, anim_pos);
    Coo rect_cy = Interpolate(_helpButtonRect.yCenter(), _screenH / 2, anim_pos);
    Rect2D anim_rect = { { rect_cx - rect_hw, rect_cy - rect_hh }, { rect_cx + rect_hw, rect_cy + rect_hh} };
    float cc = CLAMP(1 - anim_pos * 0.5, 0, 1);
    Color col = Color::FromFloats(cc, cc, cc, 1);
    
    _graphics->DrawImage(_helpButtonBg, anim_rect, col, col, col, col);
}

//-----------------------------------------------------------------------
void App::DrawBuyButtons()
{
    if (!_currentInstrument) return;
    
    PartOfOne free_progress = (KineticInstrument::FreeSessionDuration - _currentInstrument->freeRemaining) / KineticInstrument::FreeSessionDuration;
    PartOfOne cooldown_progress = (KineticInstrument::FreeCooldownDuration - _currentInstrument->freeCooldownRemaining) / KineticInstrument::FreeCooldownDuration;
    
    if (_currentInstrument->freeRemaining == 0)
    {
        PartOfOne free_alpha = (cooldown_progress < 1 ? 0.5 * cooldown_progress : 1.0);
        Color free_col = COLOR_WHITE.WithAlpha(free_alpha);
        
        _graphics->DrawImage(_buyingButtonsBg, _buyingButtonsRect);
        _graphics->DrawImage(_freeSessionButtonBg, _freeSessionButtonRect, free_col, free_col, free_col, free_col);
    }
    else
    {
        PartOfOne free_alpha = 0.5 + 0.5 * sin(system::GetCurrentTimestamp() * 4.0);
        Rect2D free_rect = _freeSessionButtonRect;
        free_rect.Translate(0, free_progress * (_screenH - free_rect.min.y));
        Color free_col = COLOR_WHITE.WithAlpha(free_alpha);
        _graphics->DrawImage(_freeSessionButtonBg, free_rect, free_col, free_col, free_col, free_col);
    }
}

//-----------------------------------------------------------------------
void App::DrawIsWaiting()
{
    Image& anim_image = _instruments[0]->icon;
    Coo anim_r = IsWaitingAnimRadius * _globalScale;
    auto anim_progress = _isWaitingAnim.UpdateMovement(_frameDt);
    if (anim_progress >= 1.0)
    {
        _isWaitingAnim.SetPos(0);
        _isWaitingAnim.SetMovement(1.0, IsWaitingAnimDuration);
    }
    Rect2D anim_rect = Rect2D::WithSizes(0, 0, anim_image.GetWidth(), anim_image.GetHeight());
    Angle anim_a = (anim_progress + 0.25) * DOUBLE_PI;
    anim_rect.Translate(anim_r * cos(anim_a), anim_r * sin(anim_a));
    anim_rect.Translate(anim_rect.w() * -0.5, anim_rect.h() * -0.5);
    anim_rect.Scale(IsWaitingAnimScale * _globalScale, IsWaitingAnimScale * _globalScale, true);
    anim_rect.Translate(_screenW * 0.5, _screenH * 0.5);
    
    Rect2D anim_shadow_rect = anim_rect;
    anim_shadow_rect.Translate(IsWaitingAnimShadowDX, IsWaitingAnimShadowDX);
    Color shadow_col = COLOR_BLACK.WithAlpha(0.3);
    _graphics->DrawImage(anim_image, anim_shadow_rect, shadow_col, shadow_col, shadow_col, shadow_col);
    
    _graphics->DrawImage(anim_image, anim_rect);
}

//-----------------------------------------------------------------------
void App::DrawCrosshair()
{
    if (_buyingButtonsShown && _currentInstrument->freeRemaining == 0) return;
    
    const Point2D center = { _currentInstrumentRect.xCenter(), _currentInstrumentRect.yCenter() };
    Rect2D center_rect = { center, center };
    center_rect.Expand(_crosshair.GetWidth() * 0.5);
    _graphics->DrawImage(_crosshair, center_rect);
}

//-----------------------------------------------------------------------
void App::ScrollToInstrumentInList(int instrument_index)
{
    Angle current_angle = _instrumentsListScrollAngle.GetLaggedPos();
    Angle target_angle = instrument_index * -_instrumentsListDAngle;
    while (target_angle - current_angle > 180)
        target_angle -= 360;
    while (target_angle - current_angle < -180)
        target_angle += 360;

    _instrumentsListScrolledIndex = instrument_index;
    _instrumentsListScrollAngle.SetTarget(target_angle);
    _instrumentsListLastScrolledLeft = (current_angle > target_angle);
}

//-----------------------------------------------------------------------
void App::ScrollInstrumentsList(Coo dragged_width, Coo distance_from_center)
{
    Angle current_angle = _instrumentsListScrollAngle.GetPos();
    Coo drag_speed = dragged_width;
    
    CooMultiplier norm_dist_from_center = distance_from_center / GetInstrumentsListCircleRadius();
    norm_dist_from_center = CLAMP(norm_dist_from_center, -1, 1);
    Angle target_angle = RadToDeg(asin(norm_dist_from_center));
    
    CooMultiplier norm_dist_from_center_prev = (distance_from_center - dragged_width) / GetInstrumentsListCircleRadius();
    norm_dist_from_center_prev = CLAMP(norm_dist_from_center_prev, -1, 1);
    Angle target_angle_prev = RadToDeg(asin(norm_dist_from_center_prev));
    
    Angle dangle = target_angle - target_angle_prev;
    while (dangle > 90) dangle -= 90;
    while (dangle < -90) dangle += 90;
    
    target_angle = current_angle + dangle;
    
    int target_index = GetInstrumentIndexAtFrontFromAngle(target_angle);
    
    _instrumentsListScrolledIndex = target_index;
    _instrumentsListScrollAngle.SetTarget(target_angle);
    _instrumentsListSwipeSpeed += drag_speed;
    _instrumentsListSwipeSpeed *= 0.5;
}

//-----------------------------------------------------------------------
int App::GetInstrumentIndexAtFrontFromAngle(Angle angle)
{
    int index = (int)round(-angle / _instrumentsListDAngle);

    index = (index >= 0 ? index % _instruments.size() : -(-index % _instruments.size()));
    if (index < 0)
        index += _instruments.size();
    
    return index;
}

//-----------------------------------------------------------------------
Angle App::GetClosestInstrumentAngleFromAngle(Angle angle)
{
    auto index = round(angle / _instrumentsListDAngle);
    return index * _instrumentsListDAngle;
}

//-----------------------------------------------------------------------
Coo App::GetInstrumentsListCircleRadius()
{
    return _instrumentsListRect.w() * InstrumentsListCircleRadius;
}

//-----------------------------------------------------------------------
PartOfOne App::GetInstrumentsListBgVisiblePart()
{
    if (_instrumentsListRect.min.y >= 0) return 1;
    else if (_instrumentsListRect.min.y <= -_instrumentsListRect.h()) return 0;
    return 1 + _instrumentsListRect.min.y / _instrumentsListRect.h();
}

//-----------------------------------------------------------------------
void App::SetInstrumentsListVisibility(bool state_shown)
{
    _instrumentsListShowAnim.SetMovement(state_shown ? 1 : 0, ShowHideInstrumentsListDuration);
    for (auto instrument : _instruments)
    {
        instrument->iconShowHideAnim.SetSpring(InstrumentsListIconShowSpringAcc, InstrumentsListIconShowSpringDamp);
        instrument->iconShowHideAnim.SetMovement(state_shown ? 1 : 0, ShowHideInstrumentsListDuration * RandomCoo(0.9, 1.2));
    }
}

//-----------------------------------------------------------------------
bool App::IsInstrumentsListShown()
{
    return (_instrumentsListShowAnim.GetTargetPos() == 1);
}

//-----------------------------------------------------------------------
void App::DrawInstrumentsList()
{
    if (_frameDt < MaxFrameDTForSimulations)
        _instrumentsListShowAnim.UpdateMovement(_frameDt);
    Coo list_pos = _instrumentsListStaticYPos + (_instrumentsListShowAnim.UpdateLagged() - 1) * _instrumentsListRect.h();
    
    _instrumentsListRect.Translate(0, list_pos - _instrumentsListRect.min.y);
    
    _graphics->DrawImage(_instrumentsListBg, _instrumentsListRect);
    
    Coo center_x = _instrumentsListRect.xCenter();
    Coo center_y = _instrumentsListRect.max.y - GetInstrumentsListBgVisiblePart() * _instrumentsListRect.h() / 2 + InstrumentsListIconsDY * _globalScale;
    Coo radius = GetInstrumentsListCircleRadius();
    Time current_time = system::GetCurrentTimestamp();
    
    bool after_swipe = (_instrumentsListSwipeSpeed != 0 && _touchesInsideInstrumentsList == 0);
    if (after_swipe)
    {
        _instrumentsListScrollAngle.SetTarget(_instrumentsListScrollAngle.GetLaggedPos());
        _instrumentsListScrollAngle.SetSpringSpeed(_instrumentsListSwipeSpeed);
    }
    
    if (_touchesInsideInstrumentsList == 0)
        _instrumentsListScrollAngle.SetSpring(InstrumentsListScrollSpringAcc, InstrumentsListScrollSpringDamp);
    else
        _instrumentsListScrollAngle.SetSpring(InstrumentsListScrollPressedSpringAcc, InstrumentsListScrollPressedSpringDamp);
    Angle current_angle = _instrumentsListScrollAngle.UpdateLagged();
    
    if (after_swipe)
    {
        _instrumentsListSwipeSpeed = _instrumentsListScrollAngle.GetSpringSpeed();
        if (ABS(_instrumentsListSwipeSpeed) < 20)
        {
            Angle target_angle = current_angle + _instrumentsListSwipeSpeed * 8;
            target_angle = GetClosestInstrumentAngleFromAngle(target_angle);
            _instrumentsListScrollAngle.SetTarget(target_angle);
            
            int target_index = GetInstrumentIndexAtFrontFromAngle(target_angle);
            //SetCurrentInstrument(target_index);
            ScrollToInstrumentInList(target_index);
            
            _instrumentsListSwipeSpeed = 0;
        }
    }
    else if (current_time - _lastInstrumentsListActivityTimestamp > AutoFocusCurrentInstrumentAfterInactivityTimeout &&
             _instrumentsListScrolledIndex != _currentInstrumentIndex)
    {
        ScrollToInstrumentInList(_currentInstrumentIndex);
    }

    int front_index = GetInstrumentIndexAtFrontFromAngle(current_angle);
    int back_index = GetInstrumentIndexAtFrontFromAngle(current_angle + 180);
    int calc_front_index = back_index - 1 - (_instruments.size() - 1) / 2;
    calc_front_index = (calc_front_index + _instruments.size()) % _instruments.size();
    
    if (_instruments.size() % 2 == 1 && front_index != calc_front_index)
        back_index = (back_index + 1 + _instruments.size()) % _instruments.size();

    for (int i = 0; i < _instruments.size(); i++)
    {
        int ii = back_index + (i % 2 == 0 ? i / 2 : -1 - i / 2);
        ii = (ii + _instruments.size()) % _instruments.size();
        auto instrument = _instruments[ii];
        
        Angle angle = current_angle + ii * _instrumentsListDAngle;
        Coo& depth = instrument->iconZ;
        depth = (1 - cos(DegToRad(angle))) * 0.5;
        CooMultiplier icon_scale = InstrumentsListIconScaleFront - depth * (InstrumentsListIconScaleFront - InstrumentsListIconScaleBack);
        icon_scale *= _globalScale;// * instrument->iconScale;
        if (ii == _currentInstrumentIndex)
            icon_scale *= 1.1 + 0.1 * sin(current_time * PI * 1.2) * _globalScale;
        
        Coo x = center_x + radius * sin(DegToRad(angle));
        Coo y = center_y - depth * InstrumentsListIconYDepth * _globalScale;
        Coo w = instrument->icon.GetWidth() * icon_scale;
        Coo h = instrument->icon.GetHeight() * icon_scale;
        
        if (_frameDt <= MaxFrameDTForSimulations)
            instrument->iconShowHideAnim.UpdateMovement(_frameDt);
        y -= (1 - instrument->iconShowHideAnim.UpdateLagged()) * _instrumentsListRect.h();
        
        auto& rect = instrument->iconRect;
        rect.min = { x - w / 2, y - h / 2 };
        rect.max = { x + w / 2, y + h / 2 };
        
        auto icon = &instrument->icon;
        
        if (ii == _currentInstrumentIndex)
        {
            Rect2D highlight_rect = rect;
            highlight_rect.Expand(10 * icon_scale);
            //_graphics->DrawRect(true, highlight_rect, COLOR_YELLOW);
        }
        
        if (instrument->unlockTimestamp == 0)
        {
            icon = &instrument->iconLocked;
        }
        else if (current_time - instrument->unlockTimestamp < InstrumentIsNewDuration)
        {
            //icon = &instrument->iconNew;
        }
        
        ColorC c = 1 - depth * 0.3;
        Color icon_color = Color::FromFloats(1, 1, c, 1 - depth * 0.2);
        if (ii == _currentInstrumentIndex)
            icon_color = icon_color.MultipliedBy(1.2);
        _graphics->DrawImage(*icon, rect, icon_color, icon_color, icon_color, icon_color);
    }
    
    _graphics->DrawImage(_instrumentsListFore, _instrumentsListRect);
}
