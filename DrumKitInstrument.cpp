#include "DrumKitInstrument.h"
#include "shaders/DrumKitViz.h"
#include "yossCommon/common/System.h"
#include "yossCommon/graphics/OpenGL/shaders/ReflectRefractGLProgram.h"

using namespace yoss;
using namespace yoss::math;
using namespace yoss::sound;
using namespace yoss::graphics;


//-----------------------------------------------------------------------
DrumKitInstrument::DrumKitInstrument() :
    _lastHitTimestamp(-1),
    _lastHitDrum1(-1),
    _lastHitDrum2(-1),
    _hoveredDrum1(-1),
    _hoveredDrum2(-1),
    _lastHitPos({ 0, 0 }),
    _drumsProgram(nullptr)
{
}

//-----------------------------------------------------------------------
DrumKitInstrument::~DrumKitInstrument()
{
}

//-----------------------------------------------------------------------
void DrumKitInstrument::LoadDrum(const std::string& file, Volume native_vol, const Rect2D& rect, const Color& flash_col)
{
    LoadSample(file, true, UnnormalizeFrequency(), native_vol);
    _drums.push_back(Drum((int)_samples.size() - 1, rect, flash_col));
}

//-----------------------------------------------------------------------
void DrumKitInstrument::LoadSamples()
{
    //LoadDrum("china.pcm", 1, Rect2D::WithMaxPoint(52, 33, 174, 156));
    //LoadDrum("hihat_open.pcm", 1, Rect2D::WithMaxPoint(0, 154, 118, 220));
    //LoadDrum("tom2.pcm", 1, Rect2D::WithMaxPoint(165, 125, 250, 225));
    //LoadDrum("tom.pcm", 1, Rect2D::WithMaxPoint(250, 125, 347, 230));
    //LoadDrum("ride.pcm", 1, Rect2D::WithMaxPoint(347, 130, 498, 227));
    //LoadDrum("snare.pcm", 1, Rect2D::WithMaxPoint(94, 220, 210, 310));
    //LoadDrum("kick.pcm", 1, Rect2D::WithMaxPoint(172, 238, 293, 362));
    //LoadDrum("floor_tom.pcm", 1, Rect2D::WithMaxPoint(287, 226, 409, 385));
    
    LoadDrum("hihat_open.pcm", 1, Rect2D::WithMaxPoint(0, 0, 250, 300), Color::FromFloats(1.0, 1.0, 0.5));
    LoadDrum("ride.pcm", 1, Rect2D::WithMaxPoint(250, 0, 500, 300), COLOR_WHITE);
    LoadDrum("snare.pcm", 1, Rect2D::WithMaxPoint(0, 150, 150, 500), Color::FromFloats(1.0, 0.5, 0.5));
    LoadDrum("kick.pcm", 1, Rect2D::WithMaxPoint(150, 150, 330, 500), Color::FromFloats(0.5, 1.0, 0.5));
    LoadDrum("floor_tom.pcm", 1, Rect2D::WithMaxPoint(330, 150, 500, 500), Color::FromFloats(0.5, 0.5, 1.0));
}

//-----------------------------------------------------------------------
void DrumKitInstrument::LoadImages(graphics::Graphics* graphics)
{
    KineticInstrument::LoadImages(graphics, "drumkit");
    
    _viz = new DrumKitViz((OpenGL*)_graphics);
    Model::LoadFromFile("drumset.fbx",
                        {"scene_deco", "hihat", "ride", "snare", "kick", "floor_tom" },
                        { &_sceneDeco, &_drums[0].model, &_drums[1].model,
                            &_drums[2].model, &_drums[3].model, &_drums[4].model });
}

//-----------------------------------------------------------------------
void DrumKitInstrument::UpdateGraphicsSizes(const Rect2D &container, const Rect2D& bg_container, CooMultiplier global_scale)
{
    KineticInstrument::UpdateGraphicsSizes(container, bg_container, global_scale);
    
    CooMultiplier bg_scale = container.h() / BgHeight;
    bg_scale *= 1.6;
    _bgRect = container;
    _bgRect.max.x = _bgRect.min.x + BgWidth * bg_scale - 1;
    _bgRect.max.y = _bgRect.min.y + BgHeight * bg_scale - 1;
}

//-----------------------------------------------------------------------
void DrumKitInstrument::OnPointerEvent(input::Pointer* pointer)
{
    ASSERT(pointer);
    Point2D pointer_pos = { pointer->x, pointer->y };

}

//-----------------------------------------------------------------------
void DrumKitInstrument::OnGainFocus()
{
    KineticInstrument::OnGainFocus();
}

//-----------------------------------------------------------------------
void DrumKitInstrument::OnLoseFocus()
{
    KineticInstrument::OnLoseFocus();
}

//-----------------------------------------------------------------------
void DrumKitInstrument::AddBeat(PartOfOne normalized_freq, Volume volume)
{
    if (!CanPlay()) return;
    
    PartOfOne geo_input = (_geoOrientationAngle + BgGeoHemiRange) / (2 * BgGeoHemiRange);
    geo_input = 1 - CLAMP(geo_input, 0, 1);
    
    PartOfOne x_axis_input = 1 - normalized_freq;
    
    _lastHitTimestamp = system::GetCurrentTimestamp();
    _lastHitPos = { geo_input * BgWidth, x_axis_input * BgHeight };
    GetDrumsAtPosInBGImage(_lastHitPos, _lastHitDrum1, _lastHitDrum2);
    if (_lastHitDrum1 >= 0)
        SamplerInstrument::AddBeat(0, volume, GetSample(_lastHitDrum1));
    if (_lastHitDrum2 >= 0)
        SamplerInstrument::AddBeat(0, volume, GetSample(_lastHitDrum2));
}

//-----------------------------------------------------------------------
void DrumKitInstrument::DrawInstrument()
{
    PartOfOne geo_input = (_geoOrientationAngle + BgGeoHemiRange) / (2 * BgGeoHemiRange);
    geo_input = CLAMP(geo_input, 0, 1);
    
    PartOfOne x_axis_input = GetNormFreqFromAxisXAngle(_accAngleAroundX);
    
    const Color& last_hit_color = (_lastHitDrum1 >= 0 ? _drums[_lastHitDrum1].flashColor : COLOR_WHITE);
    Time since_last_beat = (_lastHitTimestamp >= 0 ? system::GetCurrentTimestamp() - _lastHitTimestamp : 1000);
    PartOfOne hit_strength = (since_last_beat > 0 ? 0.01 / (since_last_beat * since_last_beat * since_last_beat) : 10);
    hit_strength = CLAMP(hit_strength, 0, 1);
    auto viz = (DrumKitViz*)_viz;
    viz->Feed(hit_strength * 7.0, last_hit_color);
    KineticInstrument::DrawViz();
    
    const CooMultiplier bg_scale = _bgRect.w() / BgWidth;
    const Point2D center = _container.center();
    Point2D bg_center = _bgRect.center();
    Coo current_x = bg_center.x - center.x;
    Coo current_y = bg_center.y - center.y;
    Coo dx = (geo_input - 0.5) * _bgRect.w() - current_x;
    Coo dy = (x_axis_input - 0.5) * _bgRect.h() - current_y;
    _bgRect.Translate(dx, dy);
    bg_center.Translate(dx, dy);
    //_graphics->DrawImage(_bg, _bgRect);
    
    GetDrumsAtPosInBGImage({
        (center.x - _bgRect.min.x) / bg_scale,
        (center.y - _bgRect.min.y) / bg_scale
    }, _hoveredDrum1, _hoveredDrum2);
    
    if (!_drumsProgram)
    {
        ASSERT(_vizImage->GetTexture());
        auto reflect_refrag_prog = new ReflectRefractGLProgram((OpenGL*)_graphics);
        reflect_refrag_prog->SetRefraction(0.9, 0.7, _vizImage->GetTexture());
        reflect_refrag_prog->SetReflection(0.8, COLOR_BLACK, nullptr);
        reflect_refrag_prog->SetSpecularLight(Vector3D(-1, 1.5, 1), COLOR_WHITE, 3, -20);
        reflect_refrag_prog->SetCullFaces(true);
        _drumsProgram = reflect_refrag_prog;
    }
    
    CooMultiplier mscale = 2.5; //_keyboardRect.h() / _keyboardModel.GetBoundingBoxSize().y;
    Angle rot_angle_axis_x = DegToRad(-20 + x_axis_input * 80);
    Angle rot_angle_axis_y = DegToRad(-ModelsViewHemiRange + geo_input * (2 * ModelsViewHemiRange));
    Vector3D rot_axis = Vector3D(rot_angle_axis_x, rot_angle_axis_y, 0);
    Angle rot_angle = sqrt(rot_angle_axis_x * rot_angle_axis_x + rot_angle_axis_y * rot_angle_axis_y);
    ((OpenGL*)_graphics)->SetCurrentProgram(_drumsProgram);
    ((ReflectRefractGLProgram*)_drumsProgram)->SetBody(COLOR_WHITE, nullptr);
    _graphics->SetCameraFromScreen(60, 0.1, 5000);
    _graphics->PushCurrentTransforms(Vector3D(0, 0, -800), Vector3D(0, 0, 0), Vector3D(-1, 1, 1), rot_axis, rot_angle, false);
    _graphics->PushCurrentTransforms(ZERO_VECTOR, ZERO_VECTOR, ONE_VECTOR * mscale, X_AXIS, DegToRad(-45), !false);
    //_graphics->PushCurrentTransforms(Vector3D(0, 0, -1000), ZERO_VECTOR, ONE_VECTOR, X_AXIS, 0, !false);
    
    _graphics->DrawModel(_sceneDeco);
    
    for (int di = 0; di < _drums.size(); di++)
    {
        if (di == _hoveredDrum1 || di == _hoveredDrum2) continue;
        auto& drum = _drums[di];
        _graphics->DrawModel(drum.model);
    }
    
    if (_hoveredDrum1 >= 0)
    {
        _graphics->Flush();
        
        ((ReflectRefractGLProgram*)_drumsProgram)->SetBody(Color::FromFloats(0.5, 1.0, 0.5, 1.0), nullptr);
        
        _graphics->DrawModel(_drums[_hoveredDrum1].model);
        if (_hoveredDrum2 >= 0)
            _graphics->DrawModel(_drums[_hoveredDrum2].model);
    }
    
    _graphics->PopCurrentTransforms();
    _graphics->PopCurrentTransforms();
    ((OpenGL*)_graphics)->SetCurrentProgram(nullptr);
    _graphics->SetCameraFromScreen(20, 0.1, 100);
    
    if (0)
    for (int di = 0; di < _drums.size(); di++)
    {
        auto& drum = _drums[di];
        Rect2D drum_rect = drum.rect;
        drum_rect.Multiply(bg_scale, bg_scale);
        drum_rect.Translate(_bgRect.min.x, _bgRect.min.y);
        
        _graphics->DrawRect(false, drum_rect, COLOR_BLUE.WithAlpha(0.5));
    }
    
    //DrawInput(30, 15);
}

//-----------------------------------------------------------------------
void DrumKitInstrument::GetDrumsAtPosInBGImage(const graphics::Point2D &point, int& returned_drum1, int& returned_drum2)
{
    returned_drum2 = -1;
    
    std::vector<int> inside_drums;
    for (int di = 0; di < _drums.size(); di++)
    {
        auto& drum = _drums[di];
        if (drum.rect.Contains(point))
            inside_drums.push_back(di);
    }
    
    if (inside_drums.size() == 1)
    {
        returned_drum1 = inside_drums.back();
    }
    else if (inside_drums.size() == 0 || (!CanHitTwoDrums && inside_drums.size() >= 2))
    {
        int inside_drums_i = 0;
        Coo closest_dist = 1000000;
        for (int di = 0; di < _drums.size(); di++)
        {
            if (inside_drums.size() > 0 &&
                inside_drums[inside_drums_i] != di)
                continue;
            
            auto& drum = _drums[di];
            Coo dx = point.x - drum.rect.xCenter();
            Coo dy = point.y - drum.rect.yCenter();
            Coo dist = sqrt(dx * dx + dy * dy);
            if (dist < closest_dist)
            {
                returned_drum1 = di;
                closest_dist = dist;
            }
            
            inside_drums_i++;
        }
    }
    else if (inside_drums.size() >= 2)
    {
        returned_drum1 = inside_drums[0];
        returned_drum2 = inside_drums[1];
    }
}
