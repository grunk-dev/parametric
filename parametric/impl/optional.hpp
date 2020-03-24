#ifndef OPTIONAL_HPP
#define OPTIONAL_HPP

#include <stdexcept>
#include <memory>

namespace parametric
{

    // a optional class similar to std::optional
    template <typename T> class optional
    {
    public:
        optional()
            : data(nullptr)
        {
        }

        optional(const T& t)
            : data(new T(t))
        {
        }
        const T& value() const
        {
            if (!data) {
                throw std::runtime_error("Value not initialized");
            }
            return *data;
        }

        T& value()
        {
            if (!data) {
                throw std::runtime_error("Value not initialized");
            }
            return *data;
        }

        operator T const& () const
        {
            return value();
        }

        bool is_initialized() const
        {
            return data != nullptr;
        }

        void reset()
        {
            data.reset();
        }

    private:
        std::unique_ptr<T> data;
    };

} // namespace parametric


#endif // OPTIONAL_HPP
