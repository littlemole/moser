#pragma once
#include "App.g.h"
#include "App.base.h"
#include "mox.h"

namespace winrt::moxaml::implementation
{
    class App : public AppT2<App>
    {
    public:
        App();
        ~App();
    };

}

namespace winrt::moxaml::factory_implementation
{
    class App : public AppT<App, implementation::App>
    {
    };
}