#include "pch.h"
#include "mox.h"
#include "mox.g.cpp"

#include <winrt/Windows.UI.Xaml.Interop.h>

namespace winrt::moxaml::implementation
{
    winrt::Microsoft::UI::Xaml::DependencyProperty mox::EventProperty()
    {
        return m_EventProperty;
    }

    hstring mox::GetEvent(winrt::Microsoft::UI::Xaml::DependencyObject const& target)
    {
        return winrt::unbox_value<hstring>(target.GetValue(m_EventProperty));
    }

    void mox::SetEvent(winrt::Microsoft::UI::Xaml::DependencyObject const& target, hstring const& value)
    {
        target.SetValue(m_EventProperty, winrt::box_value(value));
    }

    Microsoft::UI::Xaml::DependencyProperty mox::m_EventProperty =
        Microsoft::UI::Xaml::DependencyProperty::RegisterAttached(
            L"Event",
            winrt::xaml_typename<hstring>(),
            winrt::xaml_typename<moxaml::mox>(),
            Microsoft::UI::Xaml::PropertyMetadata{ winrt::box_value(L"") }
    );
}
