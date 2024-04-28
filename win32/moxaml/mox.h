#pragma once
#include "mox.g.h"

namespace winrt::moxaml::implementation
{
    struct mox : moxT<mox>
    {
        mox() = default;

        static winrt::Microsoft::UI::Xaml::DependencyProperty EventProperty();
        static hstring GetEvent(winrt::Microsoft::UI::Xaml::DependencyObject const& target);
        static void SetEvent(winrt::Microsoft::UI::Xaml::DependencyObject const& target, hstring const& value);
    
    private:
        static Microsoft::UI::Xaml::DependencyProperty m_EventProperty;
    };
}
namespace winrt::moxaml::factory_implementation
{
    struct mox : moxT<mox, implementation::mox>
    {
    };
}
