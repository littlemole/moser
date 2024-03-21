#pragma once
#include "Vector.g.h"
#include "IntVector.g.h"
#include "DoubleVector.g.h"
#include "StringVector.g.h"
namespace winrt::moxaml::implementation
{
    template<class C, class T>
    struct ObservableVectorImpl : C
    {
        ObservableVectorImpl() : impl_(winrt::single_threaded_observable_vector<T>())
        {}

        winrt::Windows::Foundation::Collections::IIterator<T> First()
        {
            return impl_.First();
        }
        T GetAt(uint32_t index)
        {
            return impl_.GetAt(index);
        }
        uint32_t Size()
        {
            return impl_.Size();
        }
        winrt::Windows::Foundation::Collections::IVectorView<T> GetView()
        {
            return impl_.GetView();
        }
        bool IndexOf(T const& value, uint32_t& index)
        {
            return impl_.IndexOf(value, index);
        }
        void SetAt(uint32_t index, T const& value)
        {
            impl_.SetAt(index, value);
        }
        void InsertAt(uint32_t index, T const& value)
        {
            impl_.InsertAt(index, value);
        }
        void RemoveAt(uint32_t index)
        {
            impl_.RemoveAt(index);
        }
        void Append(T const& value)
        {
            impl_.Append(value);
        }
        void RemoveAtEnd()
        {
            impl_.RemoveAtEnd();
        }
        void Clear()
        {
            impl_.Clear();
        }
        uint32_t GetMany(uint32_t startIndex, array_view<T> items)
        {
            return impl_.GetMany(startIndex, items);
        }
        void ReplaceAll(array_view<T const> items)
        {
            impl_.ReplaceAll(items);
        }
        winrt::event_token VectorChanged(winrt::Windows::Foundation::Collections::VectorChangedEventHandler<T> const& vhnd)
        {
            return impl_.VectorChanged(vhnd);
        }
        void VectorChanged(winrt::event_token const& token) noexcept
        {
            impl_.VectorChanged(token);
        }

    protected:
        winrt::Windows::Foundation::Collections::IObservableVector<T> impl_;
    };

    struct IntVector : public ObservableVectorImpl<IntVectorT<IntVector>, int32_t>
    {};

    struct DoubleVector : public ObservableVectorImpl<DoubleVectorT<DoubleVector>, double>
    {};

    struct StringVector : public ObservableVectorImpl<StringVectorT<StringVector>, hstring>
    {};

    struct Vector : public ObservableVectorImpl<VectorT<Vector>, winrt::Windows::Foundation::IInspectable>
    {};
}

namespace winrt::moxaml::factory_implementation
{
    class Vector : public VectorT<Vector, implementation::Vector>
    {
    };

    class IntVector : public IntVectorT<IntVector, implementation::IntVector>
    {
    };

    class DoubleVector : public DoubleVectorT<DoubleVector, implementation::DoubleVector>
    {
    };

    class StringVector : public StringVectorT<StringVector, implementation::StringVector>
    {
    };
}

