#pragma once
#include "App.xaml.g.h"
//#include "App.base.h"
#include "mox.h"

namespace winrt::moxaml::implementation
{
    class App : public AppT<App>
    {
    public:
        App();
        ~App();
    };

}
/*
namespace winrt::moxaml::factory_implementation
{
    class App : public AppT<App, implementation::App>
    {
    };
}
*/