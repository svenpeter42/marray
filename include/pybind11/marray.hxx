#include <andres/marray.hxx>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/cast.h>
#include <type_traits>

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
    template <class ShapeIterator>
    PyView(pybind11::array_t<Type> array, Type *data, ShapeIterator begin, ShapeIterator end)
        : View<Type, false>(begin, end, data, FirstMajorOrder, FirstMajorOrder), py_array(array)
    {
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
            nullptr, sizeof(Type), pybind11::format_descriptor<Type>::value(), shape.size(), shape, strides));
        pybind11::buffer_info info = py_array.request();
        Type *ptr = (Type *)info.ptr;

        this->assign(begin, end, ptr, FirstMajorOrder, FirstMajorOrder);
    }

    PyView(std::initializer_list<std::size_t> shape) : PyView(shape.begin(), shape.end())
    {
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

    typedef type_caster<pybind11::array_t<Type>> pyarray_caster;

    bool load(handle src, bool convert)
    {
        // convert numpy array to py::array_t
        pyarray_caster conv;
        if (!conv.load(src, convert))
            return false;

        auto pyarray = (pybind11::array_t<Type>)conv;

        // convert py::array_t to andres::PyView
        auto info = pyarray.request();
        Type *ptr = (Type *)info.ptr;

        andres::PyView<Type> result(pyarray, ptr, info.shape.begin(), info.shape.end());
        value = result;
        return true;
    }

    static handle cast(ViewType src, return_value_policy policy, handle parent)
    {
        pyarray_caster conv;
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
