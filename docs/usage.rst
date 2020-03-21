.. `toctree`

Usage
=====

Creating parameters
-------------------

Wrapping free functions
-----------------------

If a free function should be converted into a compute node,
use the function ``parametric::eval``.
This function converts each input argument of the function into
a parameter of the same type. It returns an output parameter
of type ``parametric::param<RTYPE>``, where RTYPE is the type
of the functions return value.

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

The object oriented approach
----------------------------

If free function do not provide enough flexibility, compute objects
can be defined. A compute object is a node in the compute graph
that is derived from the ``parametric::ComputeNode`` base class.

Here's an example to define a custom compute node:

.. code-block:: cpp

    // define computing node
    class DivComputer : public parametric::ComputeNode
    {
    public:
        DivComputer(const parametric::param<double>& op1, const parametric::param<double>& op2)
        : v1(op1), v2(op2)
        {
            DefineInput(v1);
            DefineInput(v2);
            DefineOutput(theresult, parametric::param<double>("result"));
        }

        parametric::param<double> result() const {return theresult;}

        void eval() const override
        {
            if (!theresult.Expired())
                theresult.SetValue(v1.Value() / v2.Value());
        }

    private:
        const parametric::InterfaceParam<double> v1, v2;
        mutable parametric::InterfaceParam<double> theresult;
    };

The important ingredients are:

 1. The class must be derived from ``parametric::ComputeNode``
 2. The class must have  a (private) parametric::InterfaceParam object for each input and output parameter
 3. Input and output parameters must be registered using ``ComputeNode::DefineInput`` and ``ComputeNode::DefineOutput``
 4. The class must override the ``ComputeNode::eval`` method to perform the actual computation

To use this compute node,  is has to be constructed with ``parametric::new_node<ClassName> (arg1, arg2, ...)`` like in the following example:

.. code-block:: cpp
   :emphasize-lines: 3

    auto v1 = parametric::new_param(10.0, "v1");
    auto v2 = parametric::new_param(2.0, "v2");
    auto divNode = parametric::new_node<DivComputer>(v1, v2);
    auto result = divNode->result();
