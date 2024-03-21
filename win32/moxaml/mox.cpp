#include "pch.h"
#include "mox.h"
#include "mox.g.cpp"

namespace winrt::moxaml::implementation
{
    winrt::Windows::UI::Xaml::DependencyProperty mox::EventProperty()
    {
        return m_EventProperty;
    }

    hstring mox::GetEvent(winrt::Windows::UI::Xaml::DependencyObject const& target)
    {
        return winrt::unbox_value<hstring>(target.GetValue(m_EventProperty));
    }

    void mox::SetEvent(winrt::Windows::UI::Xaml::DependencyObject const& target, hstring const& value)
    {
        target.SetValue(m_EventProperty, winrt::box_value(value));
    }

    Windows::UI::Xaml::DependencyProperty mox::m_EventProperty =
        Windows::UI::Xaml::DependencyProperty::RegisterAttached(
            L"Event",
            winrt::xaml_typename<hstring>(),
            winrt::xaml_typename<moxaml::mox>(),
            Windows::UI::Xaml::PropertyMetadata{ winrt::box_value(L"") }
    );
}
