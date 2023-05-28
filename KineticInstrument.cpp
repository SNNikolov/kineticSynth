#include "KineticInstrument.h"
#include "yossCommon/common/System.h"
#include "yossCommon/acc/AccEngine.h"

using namespace yoss;
using namespace yoss::math;
using namespace yoss::sound;
using namespace yoss::graphics;

const int KineticInstrument::NotesNum = 7;
const KineticInstrument::Note KineticInstrument::PauseN = -1;
const KineticInstrument::Note KineticInstrument::QuickN = -2;

//const std::vector<Frequency> KineticInstrument::NotesFrequencies = { 261.626, 293.665, 329.628, 349.228, 391.995, 440.0, 493.883}; //, 523.251 };
//const std::vector<Frequency> KineticInstrument::NotesFrequencies = { 110.0, 220.0, 440.0, 880.0, 1760.0, 3520.0, 7100.0 };
const std::vector<Frequency> KineticInstrument::NotesFrequencies = { 110.0, 220.0, 330.0, 440.0, 550.0, 660.0, 770.0 };
const KineticInstrument::Note Bb = 6;

yoss::AccEngine*    KineticInstrument::_acc      = nullptr;
graphics::Graphics* KineticInstrument::_graphics = nullptr;


//-----------------------------------------------------------------------
KineticInstrument::KineticInstrument() :
    unlockTimestamp(0),
    freeRemaining(0),
    freeCooldownRemaining(0),
    stopInstrumentTimeout(0),
    iconZ(0),
    isFocused(false),
    needsResetOrient(false),
    _viz(nullptr),
    _vizImage(nullptr)
{
    ASSERT(KineticInstrument::NotesFrequencies.size() == KineticInstrument::NotesNum);
}

//-----------------------------------------------------------------------
KineticInstrument::~KineticInstrument()
{
    if (_viz)
        delete _viz;
    if (_vizImage)
        delete _vizImage;
}

//-----------------------------------------------------------------------
void KineticInstrument::LoadImages(graphics::Graphics* graphics, const std::string& prefix, math::CooMultiplier icon_scale)
{
    ASSERT(graphics);
    _graphics = graphics;
    iconScale = icon_scale;
    
    icon.LoadFromFile(prefix + "_icon.png");
    icon.CreateTexture(_graphics);
    
    iconLocked.LoadFromFile(prefix + "_icon.png");//_locked.png");
    iconLocked.CreateTexture(_graphics);
}

//-----------------------------------------------------------------------
void KineticInstrument::UpdateGraphicsSizes(const Rect2D &container, const Rect2D &bg_container, CooMultiplier global_scale)
{
    _container = container;
    _vizContainer = bg_container;
    _globalScale = global_scale;
    
    if (_viz)
    {
        int viz_render_w = (int)(_vizContainer.w() * VizRenderScale);
        int viz_render_h = (int)(_vizContainer.h() * VizRenderScale);
        
        if (!_vizImage ||
            _vizImage->GetWidth() != viz_render_w ||
            _vizImage->GetHeight() != viz_render_h)
        {
            if (_vizImage)
                delete _vizImage;
            _vizImage = new Image(viz_render_w, viz_render_h);
            _vizImage->CreateTexture(_graphics, true);
        }
        
        _viz->UpdateGraphicsSizes(_vizContainer);
    }
}

//-----------------------------------------------------------------------
Frequency KineticInstrument::UnnormalizeFrequency(PartOfOne normalized_freq)
{
    ASSERT(normalized_freq >= 0 && normalized_freq <= 1);
    
    int note = GetNoteFromNormFreq(normalized_freq);
    Frequency note_freq = GetFreqFromNote(note);
    //Log::LogText("normalized_freq=" + Log::ToStr(normalized_freq) + " -> note=" + Log::ToStr(note) + ", freq=" + Log::ToStr(note_freq));
    return note_freq;
}

//-----------------------------------------------------------------------
void KineticInstrument::SetSamplesNativeFrequencyFromNotes(std::vector<SamplerInstrument::SamplerSample>& samples)
{
    ASSERT(samples.size() >= NotesNum);
    for (Note note = 0; note < NotesNum; note++)
        samples[note].nativeFrequency = GetFreqFromNote(note);
}

//-----------------------------------------------------------------------
Frequency KineticInstrument::GetFreqFromNormFreq(PartOfOne normalized_freq)
{
    ASSERT(normalized_freq >= 0 && normalized_freq <= 1);
    
    int note = GetNoteFromNormFreq(normalized_freq);
    Frequency note_freq = GetFreqFromNote(note);
    //Log::LogText("normalized_freq=" + Log::ToStr(normalized_freq) + " -> note=" + Log::ToStr(note) + ", freq=" + Log::ToStr(note_freq));
    return note_freq;
}

//-----------------------------------------------------------------------
int KineticInstrument::GetNoteFromNormFreq(PartOfOne normalized_freq)
{
    int note = MIN(NotesNum - 1, (int)(normalized_freq * NotesNum));
    return note;
}

//-----------------------------------------------------------------------
Frequency KineticInstrument::GetNormFreqFromNote(int note)
{
    ASSERT(note >= 0 && note < NotesNum);
    
    PartOfOne note_size = 1.0 / NotesNum;
    Frequency freq = note_size * (note + 0.5);
    return freq;
}

//-----------------------------------------------------------------------
Frequency KineticInstrument::GetFreqFromNote(int note)
{
    ASSERT(note >= 0 && note < NotesNum);
    
    Frequency freq = NotesFrequencies[note];
    return freq;
}

//-----------------------------------------------------------------------
PartOfOne KineticInstrument::GetNoteResponse(PartOfOne normalized_freq, int note)
{
    PartOfOne note_size = 1.0 / NotesNum;
    PartOfOne note_pos = (note + 0.5) * note_size;
    PartOfOne diff = ABS(normalized_freq - note_pos) / (note_size * 1.5);
    diff = MIN(diff, 1.0);
    return 1.0 - diff;
}

//-----------------------------------------------------------------------
PartOfOne KineticInstrument::GetNormFreqFromAxisXAngle(Angle a, Angle axis_x_start_angle, Angle axis_x_end_angle)
{
    if (axis_x_start_angle == -1)
        axis_x_start_angle = AccEngine::BEATS_SCALE_START_ANGLE;
    
    if (axis_x_end_angle == -1)
        axis_x_end_angle = AccEngine::BEATS_SCALE_END_ANGLE;
    
    PartOfOne norm = (a - axis_x_start_angle) / (axis_x_end_angle - axis_x_start_angle);
    norm = CLAMP(norm, 0, 1);
    return norm;
}

//-----------------------------------------------------------------------
void KineticInstrument::PlayDemo(Volume volume)
{
    AddBeat(0.5, volume);
}

//-----------------------------------------------------------------------
KineticInstrument::BeatData KineticInstrument::CreateBeatData(Note note, Volume volume, bool user_generated)
{
    BeatData beat;
    beat.instrument = this;
    beat.note = note;
    beat.volume = volume;
    beat.timestamp = system::GetCurrentTimestamp();
    beat.userGenerated = user_generated;
    beat.vizProgress = 0;
    
    return beat;
}

//-----------------------------------------------------------------------
void KineticInstrument::UpdateInput(Angle geo_orientation, Angle acc_angle_around_x, Angle acc_angle_around_y)
{
    _geoOrientationAngle = geo_orientation;
    _accAngleAroundX     = acc_angle_around_x;
    _accAngleAroundY     = acc_angle_around_y;
    
    OnUpdateInput();
}

//-----------------------------------------------------------------------
void KineticInstrument::OnGainFocus()
{
    isFocused = true;
    stopInstrumentTimeout = 0;
    
    if (needsResetOrient)
        _acc->ResetInput();
}

//-----------------------------------------------------------------------
void KineticInstrument::OnLoseFocus()
{
    isFocused = false;
    stopInstrumentTimeout = InstrumentStopTimeout;
}

//-----------------------------------------------------------------------
void KineticInstrument::DrawViz()
{
    static const Ratio ROT_STRENGTH = 1.2;
    PartOfOne geo_input = ROT_STRENGTH * _geoOrientationAngle / -60.;
    PartOfOne x_axis_input = ROT_STRENGTH * (GetNormFreqFromAxisXAngle(_accAngleAroundX) * -2.0 + 1.0);
    //geo_input = CLAMP(geo_input, -1.0, 1.0);
    //x_axis_input = CLAMP(x_axis_input, -1.0, 1.0);
    _viz->SetViewPoint(geo_input, x_axis_input);
    
    //static int frame_counter = 0;
    //if (frame_counter++ % 2 > 0)
    //{
    //    _graphics->DrawImage(*_vizImage, _vizContainer);
    //    return;
    //}
    
    ((OpenGL*)_graphics)->SetCurrentProgram(_viz);
    if (VizUseImageBuffer)
    {
        ((OpenGL*)_graphics)->SetCurrentOutputImage(_vizImage, true);
        _graphics->ClearScreen(COLOR_WHITE);
        
        Rect2D viz_render_rect = Rect2D::WithSizes(0, 0, _vizImage->GetWidth(), _vizImage->GetHeight());
        _graphics->DrawRect(true, viz_render_rect, COLOR_WHITE);
        
        ((OpenGL*)_graphics)->SetCurrentOutputImage(nullptr, true);
        ((OpenGL*)_graphics)->SetCurrentProgram(nullptr);
        _graphics->DrawImage(*_vizImage, _vizContainer);
    }
    else
    {
        _graphics->DrawRect(true, _vizContainer, COLOR_WHITE);
        ((OpenGL*)_graphics)->SetCurrentProgram(nullptr);
    }
}

//-----------------------------------------------------------------------
void KineticInstrument::DrawInput(Angle geo_max, Angle axis_y_max, Angle axis_x_start_angle, Angle axis_x_end_angle)
{
    Coo rect_hw = 200;
    Coo rect_h  = 80;
    Coo x_center = _container.min.x + _container.w() / 2;
    Rect2D rect = { { -rect_hw, 0 }, { rect_hw, rect_h } };
    rect.Translate(x_center, _container.min.y);
    
    if (axis_y_max > 0)
    {
        _graphics->DrawRect(true, rect, COLOR_CIAN.WithAlpha(0.5));
        CooMultiplier scale = rect_hw / axis_y_max;
        Coo x_pos = x_center + _accAngleAroundY * scale;
        
        Rect2D irect = rect;
        irect.min.x = irect.max.x = x_pos;
        irect.Expand(5);
        _graphics->DrawRect(true, irect, COLOR_BLUE);
    }
    
    rect.Translate(0, rect_h + 10);
    if (geo_max > 0)
    {
        _graphics->DrawRect(true, rect, COLOR_CIAN.WithAlpha(0.5));
        CooMultiplier geo_angle_scale = rect_hw / geo_max;
        Coo x_pos = x_center - _geoOrientationAngle * geo_angle_scale;
        
        Rect2D irect = rect;
        irect.min.x = irect.max.x = x_pos;
        irect.Expand(5);
        _graphics->DrawRect(true, irect, COLOR_BLUE);
    }
    
    if (axis_x_start_angle != axis_x_end_angle ||
        axis_x_start_angle == -1)
    {
        rect = { { 40, 40 }, { 100, _container.h() - 40 } };
        rect.Translate(_container.min.x, _container.min.y);
        _graphics->DrawRect(true, rect, COLOR_CIAN.WithAlpha(0.5));
        
        Coo indicator_ypos = GetNormFreqFromAxisXAngle(_accAngleAroundX, axis_x_start_angle, axis_x_end_angle);
        indicator_ypos = rect.max.y - indicator_ypos * rect.h();
        Rect2D irect = rect;
        irect.min.y = irect.max.y = indicator_ypos;
        irect.Expand(5);
        _graphics->DrawRect(true, irect, COLOR_BLUE);
    }
}
