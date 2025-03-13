#include "pch.h"
#include "App.h"
#include "App.xaml.g.h"

//using namespace Microsoft::UI::Xaml;

using namespace winrt;
namespace winrt::moxaml::implementation
{
    void App::OnLaunched(winrt::Microsoft::UI::Xaml::LaunchActivatedEventArgs const&)
    {
    }
}


winrt::Microsoft::UI::Xaml::Application xamlapp{ nullptr };

extern "C" __declspec(dllexport) void by_the_power_of_grayskull_init_the_xaml()
{
    xamlapp = winrt::make<winrt::moxaml::implementation::App>();
}
