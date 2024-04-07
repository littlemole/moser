#pragma once
#include "Map.g.h"
#include "IntMap.g.h"
#include "DoubleMap.g.h"
#include "StringMap.g.h"

namespace winrt::moxaml::implementation
{
    template<class C, class T>
    struct ObservableMapImpl : public C
    {
        ObservableMapImpl() : impl_(winrt::single_threaded_observable_map<hstring, T>())
        {}

        winrt::Windows::Foundation::Collections::IIterator<winrt::Windows::Foundation::Collections::IKeyValuePair<hstring, T>> First()
        {
            return impl_.First();
        }
        T Lookup(hstring const& key)
        {
            return impl_.Lookup(key);
        }
        uint32_t Size()
        {
            return impl_.Size();
        }
        bool HasKey(hstring const& key)
        {
            return impl_.HasKey(key);
        }
        winrt::Windows::Foundation::Collections::IMapView<hstring, T> GetView()
        {
            return impl_.GetView();
        }
        bool Insert(hstring const& key, T const& value)
        {
            return impl_.Insert(key, value);
        }
        void Remove(hstring const& key)
        {
            impl_.Remove(key);
        }
        void Clear()
        {
            impl_.Clear();
        }
        winrt::event_token MapChanged(winrt::Windows::Foundation::Collections::MapChangedEventHandler<hstring, T> const& vhnd)
        {
            return impl_.MapChanged(vhnd);
        }
        void MapChanged(winrt::event_token const& token) noexcept
        {
            impl_.MapChanged(token);
        }

    protected:
        winrt::Windows::Foundation::Collections::IObservableMap<hstring, T> impl_;
    };

    struct Map : public ObservableMapImpl<MapT<Map>, winrt::Windows::Foundation::IInspectable>
    {};

    struct IntMap : public ObservableMapImpl<IntMapT<IntMap>, int32_t>
    {};

    struct DoubleMap : public ObservableMapImpl<DoubleMapT<DoubleMap>, double>
    {};

    struct StringMap : public ObservableMapImpl<StringMapT<StringMap>, hstring>
    {};

    /*
    struct ObservableMap : ObservableMapT<ObservableMap>
    {
        ObservableMap();

        winrt::Windows::Foundation::Collections::IIterator<winrt::Windows::Foundation::Collections::IKeyValuePair<hstring, winrt::Windows::Foundation::IInspectable>> First();
        winrt::Windows::Foundation::IInspectable Lookup(hstring const& key);
        uint32_t Size();
        bool HasKey(hstring const& key);
        winrt::Windows::Foundation::Collections::IMapView<hstring, winrt::Windows::Foundation::IInspectable> GetView();
        bool Insert(hstring const& key, winrt::Windows::Foundation::IInspectable const& value);
        void Remove(hstring const& key);
        void Clear();
        winrt::event_token MapChanged(winrt::Windows::Foundation::Collections::MapChangedEventHandler<hstring, winrt::Windows::Foundation::IInspectable> const& vhnd);
        void MapChanged(winrt::event_token const& token) noexcept;

    protected:
        winrt::Windows::Foundation::Collections::IObservableMap<hstring, winrt::Windows::Foundation::IInspectable> impl_;
    };
    */
}


namespace winrt::moxaml::factory_implementation
{
    class Map : public MapT<Map, implementation::Map>
    {
    };

    class IntMap : public IntMapT<IntMap, implementation::IntMap>
    {
    };

    class DoubleMap : public DoubleMapT<DoubleMap, implementation::DoubleMap>
    {
    };

    class StringMap : public StringMapT<StringMap, implementation::StringMap>
    {
    };
}

