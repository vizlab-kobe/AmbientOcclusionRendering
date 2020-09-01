#pragma once
#include "Model.h"
#include <kvs/Application>
#include <kvs/Screen>


namespace local
{

class View
{
private:
    local::Model* m_model;
    kvs::Screen m_screen;

public:
    View( kvs::Application* app, local::Model* model );

    kvs::Screen& screen() { return m_screen; }
    void show();
    void redraw();
    void setup();
};

} // end of namespace local
