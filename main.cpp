#include <iostream>
#include "parametric_core.hpp"

template <class T> parametric::param_ptr<T> add(const parametric::param_ptr<T>& a, const parametric::param_ptr<T>& b)
{

    class Adder : public parametric::ComputeNode
    {
    public:
        static std::shared_ptr<Adder> New(const parametric::param_ptr<T>& a, const parametric::param_ptr<T>& b)
        {
            std::shared_ptr<Adder> self(new Adder(a, b));
            ComputeNode::connect_ins_outs(self);
            return self;
        }

        void compute()
        {
            std::cout << "Comput add: " << _a->Value() << "+" << _b->Value() << std::endl;
            _sum->SetValue(_a->Value() + _b->Value());
        }

        parametric::param_ptr<T> sum() const { return _sum;}

    private:
        Adder(const parametric::param_ptr<int>& a, const parametric::param_ptr<int>& b)
            : _a(a)
            , _b(b)
            , _sum(parametric::new_param<int>())
        {
            define_input(_a);
            define_input(_b);
            define_output(_sum);
        }

        parametric::param_ptr<T> _a, _b;
        parametric::param_ptr<T> _sum;
    };

    return Adder::New(a, b)->sum();
}

int main()
{
    parametric::param_ptr<int> k = parametric::new_param(1);
    parametric::param_ptr<int> j = parametric::new_param(2);

    parametric::param_ptr<int> sum = add(k, j);
    sum = add(sum, sum);

    std::cout << "Until here, nothing has been computed!" << std::endl;
    std::cout << "Sum: " << sum->Value() << std::endl;

    j->SetValue(10);
    std::cout << "Sum: " << sum->Value() << std::endl;
    std::cout << "Sum: " << sum->Value() << std::endl;
    
    j->SetValue(11);
    std::cout << "Sum: " << sum->Value() << std::endl;

    return 0;
}
