#pragma once
#include <kvs/CheckBox>
#include <kvs/Slider>
#include <kvs/OrientationAxis>
#include <kvs/ColorMapBar>
#include <kvs/TransferFunctionEditor>
#include "Model.h"
#include "View.h"


namespace local
{

class Controller
{
private:
    local::Model& m_model;
    local::View& m_view;
    kvs::CheckBox m_ssao_check_box;
    kvs::CheckBox m_lod_check_box;
    kvs::Slider m_repeat_slider;
    kvs::Slider m_radius_slider;
    kvs::Slider m_points_slider;
    kvs::OrientationAxis m_axis;
    kvs::ColorMapBar m_bar;
    kvs::TransferFunctionEditor m_editor;

public:
    Controller( local::Model& model, local::View& view );

    bool isVisible() const;
    void show();
    void hide();
};

} // end of namespace local
