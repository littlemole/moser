#include "pch.h"
#include "ObservableMap.h"
#include "Map.g.cpp"
#include "IntMap.g.cpp"
#include "DoubleMap.g.cpp"
#include "StringMap.g.cpp"

namespace winrt::moxaml::implementation
{
    /*
    ObservableMap::ObservableMap() : impl_(winrt::single_threaded_observable_map<hstring, winrt::Windows::Foundation::IInspectable>())
    {}

    winrt::Windows::Foundation::Collections::IIterator<winrt::Windows::Foundation::Collections::IKeyValuePair<hstring, winrt::Windows::Foundation::IInspectable>> ObservableMap::First()
    {
        return impl_.First();
    }
    winrt::Windows::Foundation::IInspectable ObservableMap::Lookup(hstring const& key)
    {
        return impl_.Lookup(key);
    }
    uint32_t ObservableMap::Size()
    {
        return impl_.Size();
    }
    bool ObservableMap::HasKey(hstring const& key)
    {
        return impl_.HasKey(key);
    }
    winrt::Windows::Foundation::Collections::IMapView<hstring, winrt::Windows::Foundation::IInspectable> ObservableMap::GetView()
    {
        return impl_.GetView();
    }
    bool ObservableMap::Insert(hstring const& key, winrt::Windows::Foundation::IInspectable const& value)
    {
        return impl_.Insert(key, value);
    }
    void ObservableMap::Remove(hstring const& key)
    {
        impl_.Remove(key);
    }
    void ObservableMap::Clear()
    {
        impl_.Clear();
    }
    winrt::event_token ObservableMap::MapChanged(winrt::Windows::Foundation::Collections::MapChangedEventHandler<hstring, winrt::Windows::Foundation::IInspectable> const& vhnd)
    {
        return impl_.MapChanged(vhnd);
    }
    void ObservableMap::MapChanged(winrt::event_token const& token) noexcept
    {
        impl_.MapChanged(token);
    }
    */
}
