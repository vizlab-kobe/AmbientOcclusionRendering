#pragma once
#include <kvs/WidgetBase>
#include <kvs/OrientationAxis>
#include <kvs/ColorMapBar>
#include <kvs/glut/TransferFunctionEditor>
#include <kvs/glut/Screen>
#include "Input.h"
#include "Vis.h"


namespace local
{

class Widget
{
private:
    std::vector<kvs::WidgetBase*> m_ui_widgets;
    kvs::OrientationAxis* m_axis;
    kvs::ColorMapBar* m_bar;
    kvs::glut::TransferFunctionEditor* m_editor;

public:
    Widget( kvs::glut::Screen& screen, local::Input& input, local::Vis& vis );
    ~Widget();

    bool isShown() const;
    void show();
    void hide();
};

} // end of namespace local
