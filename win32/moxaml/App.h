#pragma once
#include "App.xaml.g.h"
//#include "App.base.h"
#include "mox.h"
#include <winrt/Microsoft.UI.Xaml.Hosting.h>

namespace winrt::moxaml::implementation
{
    struct App : AppT<App>
    {
        App()
        {
            m_windowsXamlManager = winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread();
            InitializeComponent();
        }

        void OnLaunched(winrt::Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Microsoft::UI::Xaml::Hosting::WindowsXamlManager m_windowsXamlManager{ nullptr };
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