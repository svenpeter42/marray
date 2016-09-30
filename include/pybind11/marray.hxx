#include <andres/marray.hxx>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/cast.h>
#include <type_traits>
#include <vector>

using std::vector;

namespace pybind11
{
namespace detail
{
template <typename Type> struct pymarray_caster;
}
}

namespace andres
{

template <typename Type> class PyView : public View<Type, false>
{
    friend struct pybind11::detail::pymarray_caster<Type>;

  private:
    pybind11::array_t<Type> py_array;

  public:
    template <class ShapeIterator, class StrideIterator>
    PyView(pybind11::array_t<Type> array, Type *data, ShapeIterator begin, ShapeIterator end, StrideIterator strides)
        : View<Type, false>(begin, end, data, FirstMajorOrder, FirstMajorOrder), py_array(array)
    {
        this->assign(begin, end, strides, data, FirstMajorOrder);
    }

    PyView()
    {
    }

    template <class ShapeIterator> PyView(ShapeIterator begin, ShapeIterator end)
    {
        std::vector<size_t> shape, strides;

        for (auto i = begin; i != end; ++i)
            shape.push_back(*i);

        for (size_t i = 0; i < shape.size(); ++i) {
            size_t stride = sizeof(Type);
            for (size_t j = i + 1; j < shape.size(); ++j)
                stride *= shape[j];
            strides.push_back(stride);
        }

        py_array = pybind11::array(pybind11::buffer_info(
            nullptr, sizeof(Type), pybind11::format_descriptor<Type>::value, shape.size(), shape, strides));
        pybind11::buffer_info info = py_array.request();
        Type *ptr = (Type *)info.ptr;

        this->assign(begin, end, ptr, FirstMajorOrder, FirstMajorOrder);
    }

#ifdef HAVE_CPP11_INITIALIZER_LISTS
    PyView(std::initializer_list<std::size_t> shape) : PyView(shape.begin(), shape.end())
    {
    }

    PyView(std::vector<Type> v) : PyView({v.size()})
    {
        for (size_t i = 0; i < v.size(); ++i)
            this->operator[](i) = v[i];
    }
#endif

    const Type & operator[](const uint64_t idx) const {
        return this->operator()(idx);
    }

    Type & operator[](const uint64_t idx) {
        return this->operator()(idx);
    }


};
}

namespace pybind11
{

namespace detail
{

template <typename Type> struct pymarray_caster {
    typedef typename andres::PyView<Type> ViewType;
    typedef type_caster<typename intrinsic_type<Type>::type> value_conv;

    typedef typename pybind11::array_t<Type> pyarray_type;
    typedef type_caster<pyarray_type> pyarray_conv;

    typedef vector<Type> vector_type;
    typedef type_caster<vector_type> vector_conv;

#if 0
    bool load_vector(handle src, bool convert)
    {
        vector_conv conv;

        if (!conv.load(src, convert))
            return false;

        vector_type v = (vector_type)conv;
        ViewType result({v.size()});

        for (size_t i = 0; i < v.size(); ++i) {
            std::cout << "convert " << i << " " << v[i] << "\n";
            result(i) = v[i];
        }

        value = result;

        return true;
    }
#endif

    bool load(handle src, bool convert)
    {
        // convert numpy array to py::array_t
        pyarray_conv conv;
        if (!conv.load(src, convert))
        //    return load_vector(src, convert);
        return false;

        auto pyarray = (pyarray_type)conv;

        // convert py::array_t to andres::PyView
        auto info = pyarray.request();
        Type *ptr = (Type *)info.ptr;

        std::vector<size_t> strides(info.strides.begin(),info.strides.end());
        for (size_t i = 0; i < strides.size(); ++i)
            strides[i] /= sizeof(Type);

        ViewType result(pyarray, ptr, info.shape.begin(), info.shape.end(), strides.begin());
        value = result;
        return true;
    }

    static handle cast(ViewType src, return_value_policy policy, handle parent)
    {
        pyarray_conv conv;
        return conv.cast(src.py_array, policy, parent);
    }

    PYBIND11_TYPE_CASTER(ViewType, _("array<") + value_conv::name() + _(">"));
};

template <typename Type> struct type_caster<andres::PyView<Type>> : pymarray_caster<Type> {
};

template <typename Type> struct marray_caster {
    static_assert(std::is_same<Type, void>::value,
                  "Please use andres::PyView instead of andres::View for arguments and return values.");
};

template <typename Type> struct type_caster<andres::View<Type>> : marray_caster<Type> {
};
}
}
