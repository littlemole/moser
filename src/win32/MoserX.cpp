#include "pch.h"
#include "MoserX.h"

#include <App.xaml.h>
//#include <MainPage.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <Microsoft.UI.Dispatching.Interop.h> // For ContentPreTranslateMessage
#include <winrt/moxaml.h>

namespace winrt
{
    using namespace winrt::Microsoft::UI;
    using namespace winrt::Microsoft::UI::Dispatching;
    using namespace winrt::Microsoft::UI::Xaml;
    using namespace winrt::Microsoft::UI::Xaml::Hosting;
    using namespace winrt::Microsoft::UI::Xaml::Markup;
}


winrt::DispatcherQueueController dispatcherQueueController{nullptr};
winrt::Microsoft::UI::Xaml::Application xamlapp{nullptr};

//{ winrt::DispatcherQueueController::CreateOnCurrentThread() };

struct WindowInfo
{
    winrt::DesktopWindowXamlSource DesktopWindowXamlSource{ nullptr };
    winrt::event_token TakeFocusRequestedToken{};
    HWND LastFocusedWindow{ NULL };
};


void MoserX::init()
{
    //winrt::init_apartment(winrt::apartment_type::single_threaded);

    dispatcherQueueController = winrt::DispatcherQueueController::CreateOnCurrentThread();
    xamlapp = winrt::make<winrt::xmoser::implementation::App>();
}

void MoserX::create(HWND hwnd)
{
    WindowInfo* windowInfo = new WindowInfo();
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(windowInfo));

    windowInfo->DesktopWindowXamlSource = winrt::DesktopWindowXamlSource{};
    windowInfo->DesktopWindowXamlSource.Initialize(winrt::GetWindowIdFromWindow(hwnd));

}

void MoserX::load(HWND hwnd, const std::wstring& xaml)
{
    WindowInfo* windowInfo = reinterpret_cast<WindowInfo*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));

    windowInfo->TakeFocusRequestedToken = windowInfo->DesktopWindowXamlSource.TakeFocusRequested(
        [hwnd](winrt::DesktopWindowXamlSource const& /*sender*/, winrt::DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args) {
            if (args.Request().Reason() == winrt::XamlSourceFocusNavigationReason::First)
            {
                // The reason "First" means the user is tabbing forward, so put the focus on the button in the tab order
                // after the DesktopWindowXamlSource.
                //::GetNext
                //::SetFocus(::GetDlgItem(hWnd, 502));
            }
            else if (args.Request().Reason() == winrt::XamlSourceFocusNavigationReason::Last)
            {
                // The reason "Last" means the user is tabbing backward (shift-tab, so put the focus on button prior to
                // the DesktopWindowXamlSource.
                //::SetFocus(::GetDlgItem(hWnd, 501));
            }
        });

    winrt::hstring xaml_hstr(xaml);
    winrt::Windows::Foundation::IInspectable insp = winrt::Microsoft::UI::Xaml::Markup::XamlReader::Load(xaml_hstr);
    winrt::Microsoft::UI::Xaml::UIElement uiel{ nullptr };
    insp.as(uiel);

    windowInfo->DesktopWindowXamlSource.Content(uiel);

//    auto id = windowInfo->DesktopWindowXamlSource.SiteBridge().WindowId();

    auto id = winrt::GetWindowIdFromWindow(hwnd);

    auto appWnd = winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(id);

    auto titleBar = appWnd.TitleBar();
    titleBar.ExtendsContentIntoTitleBar(true);

    RECT r;
    ::GetClientRect(hwnd, &r);
    winrt::Windows::Graphics::RectInt32 rect{r.left,r.top,r.right -80,64};
    winrt::array_view<winrt::Windows::Graphics::RectInt32> v(&rect,1);
    titleBar.SetDragRectangles(v);
}

void MoserX::activate(HWND hwnd, WPARAM wParam)
{
    WindowInfo* windowInfo = reinterpret_cast<WindowInfo*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));

    const bool isGettingDeactivated = (LOWORD(wParam) == WA_INACTIVE);
    if (isGettingDeactivated)
    {
        // Remember the HWND that had focus.
        windowInfo->LastFocusedWindow = ::GetFocus();
    }
    else if (windowInfo->LastFocusedWindow != NULL)
    {
        ::SetFocus(windowInfo->LastFocusedWindow);
    }
}

void MoserX::size(HWND hwnd, const RECT& r)
{
    WindowInfo* windowInfo = reinterpret_cast<WindowInfo*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (windowInfo->DesktopWindowXamlSource)
    {
        winrt::Windows::Graphics::RectInt32 rect{ r.left,r.top,r.right,r.bottom };
        windowInfo->DesktopWindowXamlSource.SiteBridge().MoveAndResize(rect);
    }
}

bool MoserX::translate(MSG& msg)
{
    return ::ContentPreTranslateMessage(&msg);
}

void MoserX::destroy(HWND hwnd)
{
    WindowInfo* windowInfo = reinterpret_cast<WindowInfo*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (windowInfo->DesktopWindowXamlSource && windowInfo->TakeFocusRequestedToken.value != 0)
    {
        windowInfo->DesktopWindowXamlSource.TakeFocusRequested(windowInfo->TakeFocusRequestedToken);
        windowInfo->TakeFocusRequestedToken = {};
    }
    /*
    if (windowInfo->DesktopWindowXamlSource)
    {
        windowInfo->DesktopWindowXamlSource.SiteBridge().Close();
    }
    */
    delete windowInfo;
    ::SetWindowLong(hwnd, GWLP_USERDATA, NULL);
}

void MoserX::shutdown()
{
    dispatcherQueueController.ShutdownQueue();
}
