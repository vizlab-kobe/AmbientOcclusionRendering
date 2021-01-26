#pragma once
#include <kvs/CheckBox>
#include <kvs/Slider>
#include <kvs/OrientationAxis>
#include <kvs/ColorMapBar>
#include <kvs/TransferFunctionEditor>
#include <kvs/KeyPressEventListener>
#include <kvs/ScreenCaptureEvent>
#include <kvs/TargetChangeEvent>
#include <kvs/PaintEventListener>
#include "Model.h"
#include "View.h"


namespace local
{

  class PaintEvent : public kvs::PaintEventListener
  {
    void update()
    {
      static size_t counter = 1;
      static float time = 0.0f;

      time += scene()->renderer("Renderer")->timer().msec();
      if( counter++ == 50 )
	{
	  std::cout << "Rendering time " << time / counter << " [msec]" << std::endl;
	  counter = 1;
	  time = 0.0f;
	}
    }
  };
  
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
    kvs::Slider m_edge_slider;
    kvs::OrientationAxis m_axis;
    kvs::ColorMapBar m_bar;
    kvs::TransferFunctionEditor m_editor;
    kvs::KeyPressEventListener m_key_press_event;
    kvs::ScreenCaptureEvent m_capture_event;
    kvs::TargetChangeEvent m_target_change_event;
    PaintEvent m_paint_event;

public:
    Controller( local::Model& model, local::View& view );

protected:
    bool isVisible() const;
    void show();
    void hide();
};

} // end of namespace local
