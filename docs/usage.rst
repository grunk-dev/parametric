.. `toctree`

Usage
=====

Creating parameters
-------------------

In parametric, parameters are values that might change later. Parameters are
implemented with the template class ``parametric::param<T>``. To create a *valid* parameter, use


.. code-block:: cpp

    parametric::param<float> my_param(10.0, "myparam");

A valid parameter is a parameter with a value - in this case 10.0.
Here, the first argument is the current value of the parameter, the second is the identifier, which is optional.

To create an invalid parameter, just omit the value argument

.. code-block:: cpp

    parametric::param<float> my_invalid_param("myparam");

An invalid parameter is a parameter that does not yet have a value, but it will have one in future,
similar to a placeholder.

Similarly to ``std::make_unique``, parametric also offers the convenience factory function

.. code-block:: cpp

    auto my_param = parametric::new_param(10.0);

which deduces the type of the parameter from the first argument.


Wrapping free functions
-----------------------

If a free function should be converted into a compute node,
use the function ``parametric::eval``.
This function converts each input argument of the function into
a parameter of the same type. It returns an output parameter
of type ``parametric::param<RTYPE>``, where ``RTYPE`` is the type
of the function's return value.

Here is an example:

.. code-block:: cpp
   :emphasize-lines: 7

    double divide(double v1, double v2) {
      return v1 / v2;
    }

    auto v1 = parametric::new_param(10.0, "v1");
    auto v2 = parametric::new_param(2.0, "v2");
    auto result = parametric::eval(divide, v1, v2);

Similarly to free functions, ``parametric::eval`` can also be used on lambdas:

.. code-block:: cpp

    auto sin_v1 = parametric::eval([](double v) {
        return sin(v);
    }, v1);

.. note::  Limitations of ``parametric::eval``

   - All input values must be converted into parameters
   - It does not work in objects or object methods

Of course, the limitations can be overcome by creating convenience functions that encapsulate
the call to ``parametric::eval``:

.. code-block:: cpp

    parametric::param<double> p_pow(const parametric::param<double>& base, double exponent) {
        return parametric::eval([exponent](double b) {
            return pow(b, exponent);
        }, base);
    }

In this example, only ``base`` goes in as a parameter whereas the exponent is constant
and captured by the lambda.


The object oriented approach
----------------------------

If wrapping free functions does not provide enough flexibility, also compute objects
can be defined. A compute object is a node in the compute graph
that is derived from the ``parametric::ComputeNode`` base class.

Here's an example to define a custom compute node:

.. code-block:: cpp

    class DivComputer : public parametric::ComputeNode
    {
    public:
        DivComputer(const parametric::param<double>& op1, const parametric::param<double>& op2)
        : v1(op1), v2(op2)
        {
            depends_on(v1);
            depends_on(v2);
            computes(theresult, parametric::param<double>("result"));
        }

        parametric::param<double> result() const {return theresult;}

        void eval() const override
        {
            if (!theresult.expired())
                theresult.set_value(v1.value() / v2.value());
        }

    private:
        const parametric::InterfaceParam<double> v1, v2;
        mutable parametric::InterfaceParam<double> theresult;
    };

The important ingredients are:

 1. The class must be derived from ``parametric::ComputeNode``
 2. The class must have  a (private) parametric::InterfaceParam object for each input and output parameter
 3. Input and output parameters must be registered using ``ComputeNode::depends_on`` and ``ComputeNode::computes``
 4. The class must override the ``ComputeNode::eval`` method to perform the actual computation

To use this compute node,  is has to be constructed with ``parametric::new_node<ClassName> (arg1, arg2, ...)`` like in the following example:

.. code-block:: cpp
   :emphasize-lines: 3

    auto v1 = parametric::new_param(10.0, "v1");
    auto v2 = parametric::new_param(2.0, "v2");
    auto divNode = parametric::new_node<DivComputer>(v1, v2);
    auto result = divNode->result();
