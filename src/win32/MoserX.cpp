#include "pch.h"
#include "MoserX.h"

#include <App.xaml.h>
//#include <MainPage.h>
//#include <winrt/Microsoft.UI.Windowing.h>
//#include <Microsoft.UI.Dispatching.Interop.h> // For ContentPreTranslateMessage
#include <winrt/moxaml.h>
#include <roapi.h>

#include <win32/comobj.h>

namespace winrt
{
    using namespace winrt::Microsoft::UI;
    using namespace winrt::Microsoft::UI::Dispatching;
    using namespace winrt::Microsoft::UI::Xaml;
    using namespace winrt::Microsoft::UI::Xaml::Hosting;
    using namespace winrt::Microsoft::UI::Xaml::Markup;
}


winrt::Microsoft::UI::Xaml::Application xamlapp{nullptr};


void MoserX::init()
{
    xamlapp = winrt::make<winrt::xmoser::implementation::App>();
}
