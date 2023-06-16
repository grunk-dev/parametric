.. `toctree`

Examples
========

Tutorial
--------

.. literalinclude :: ../examples/tutorial.cpp
   :language: cpp

Custom compute nodes
--------------------

.. literalinclude :: ../examples/custom_computenode.cpp
   :language: cpp

Parametric structures / compositions
------------------------------------

.. literalinclude :: ../examples/parametric_struct.cpp
   :language: cpp

A parametric CAD example
------------------------

This example requires OCE (OpenCASCADE Community Edition) to build.

.. literalinclude :: ../examples/parametric_cad.cpp
   :language: cpp

Advanced: Adding serialization
------------------------------

With parametric, you can build a feature tree that tracks the dependencies
through any calculation. parametric allows you to serialize the dependency 
tree to a string. Using this feature, you could come up with a file format for
your application and serialize the dependency tree to a file.

Consider the following conceptualized code, that defines a wrapper type 
``MyDouble`` around a ``double`` and defines four binary operations ``+,-,*,/``
for it. We want to be able to serialize the dependency tree resulting from
any chain of calculations that uses this custom type and the four binary
operations. 

.. code-block:: cpp

   struct MyDouble {
    double value;
   };

   MyDouble operator+(MyDouble const& l, MyDouble const& r)
   {
      return {l.value + r.value};
   }

   MyDouble operator-(MyDouble const& l, MyDouble const& r)
   {
      return {l.value - r.value};
   }

   MyDouble operator*(MyDouble const& l, MyDouble const& r)
   {
      return {l.value * r.value};
   }

   MyDouble operator/(MyDouble const& l, MyDouble const& r)
   {
      return {l.value / r.value};
   }

We start out by defining a new custom compute node so that we can use 
parametric withour struct and our operations.

.. code-block:: cpp

   #include <parametric/core.hpp>

   enum struct BinaryOp {
      plus,
      minus,
      mult,
      div
   };

   class MyBinaryOperation : public parametric::ComputeNode<MyBinaryOperation, 
                                                            parametric::Results<MyDouble>,
                                                            parametric::Arguments<MyDouble, MyDouble>>
   {
   public:
      MyBinaryOperation(std::string const& id,
                        BinaryOp op
      )
      : operation(op)
      {
         this->set_id(id);
      }

      void eval() const override 
      {
         switch (operation) {
               case BinaryOp::plus:
                  if (auto r = res<0>(); r)
                     r->set_value(arg<0>().value() + arg<1>().value());
                  return;
               case BinaryOp::minus:
                  if (auto r = res<0>(); r)
                     r->set_value(arg<0>().value() - arg<1>().value());
                  return;
               case BinaryOp::mult:
                  if (auto r = res<0>(); r)
                     r->set_value(arg<0>().value() * arg<1>().value());
                  return;
               case BinaryOp::div:
                  if (auto r = res<0>(); r)
                     r->set_value(arg<0>().value() / arg<1>().value());
                  return;
               default:
                  throw std::logic_error("Not implemented\n");
         }
      }

      void post_connect() const override 
      {
         if (auto r = res<0>(); r)
            r->set_id(id());
      }

   private:
      BinaryOp operation;
   };

   parametric::param<MyDouble> my_eval(const char* id, 
                                       BinaryOp op, 
                                       parametric::param<MyDouble> const& left, 
                                       parametric::param<MyDouble> const& right
   )
   {
      auto ptr = std::make_shared<MyBinaryOperation>(id, op);
      return parametric::compute(ptr, left, right);
   }

A dependecy tree in parametric consists of alternating ``parametric::ComputeNode``\s 
and ``parametric::impl::param_holder<T>``\s, which both are derived 
from ``parametric::DAGNode``. 

``parametric::DAGNode`` has a virtual 
``serialize`` method returning a string. The overridden 
``parametric::impl::param_holder<T>::serialize`` will return an empty string 
for dependent parameters and will delegate to the templated function

.. code-block:: cpp

   template <typename T>
   std::string parametric::serialize(T const&);

for root parameters. So to be able to serialize any kind of node in our
tree, we must

 * override the virtual ``serialize`` method of our custom compute node.
 * specialize ``std::string parametric::serialize<T>`` for the types we
   intend to use as root parameters. 

Lets begin by overriding the virtual ``serialize`` method of our custom 
compute node.

.. code-block:: cpp

   class MyBinaryOperation : public parametric::ComputeNode
   {
   public:

      // ...

      std::string serialize() const override 
      {
         std::string out = arg<0>().id();
         switch (operation) {
               default:
               case BinaryOp::plus:
                     out += " + ";
                     break;
                  case BinaryOp::minus:
                     out += " - ";
                     break;
                  case BinaryOp::mult:
                     out += " * ";
                     break;
                  case BinaryOp::div:
                     out += " / ";
                     break;
                  throw std::logic_error("Not implemented\n");
         }
         out += arg<1>().id();
         return out;
      }

      // ...

   }

Next, let us specialize ``parametric::serialize`` for ``MyDouble``:

.. code-block:: cpp

   namespace parametric {
      template  <>
      std::string serialize(MyDouble const& md){
         return std::to_string(md.value);
      }
   }

Now we can serialize individual nodes of our dependency tree, but not the 
tree itself. parametric provides a convenience class to help us with this task, 
the ``parametric::Serializer``. Given a ``parametric::impl::param_holder<T>``,
it will navigate the dependency tree and store the serialized strings of all 
nodes in two stacks, one for the root parameters and one for the compute nodes.
We will add a function to unwind these two stacks to get the serialized strings
in topological order:

.. code-block:: cpp

   std::string serialize(parametric::param<MyDouble> const& p){

      parametric::Serializer serializer(*(p.node_pointer()));

      std::string out;

      auto& params = serializer.parameter_stack();

      while (!params.empty()) {
         auto& e = params.top();
         out += e.id + " = " + e.serialized + "\n";
         params.pop();
      }

      auto& compute_nodes = serializer.compute_node_stack();
      while (!compute_nodes.empty()) {
         auto& e = compute_nodes.top();
         out += e.id + " = " + e.serialized + "\n";
         compute_nodes.pop();
      }

      return out;
   }

Finally, we can test our serialization framework:

.. code-block:: cpp

   int main()
   {
      auto a = parametric::new_param(MyDouble{0.5}, "a");
      auto b = parametric::new_param(MyDouble{0.1}, "b");
      auto c = my_eval("c", BinaryOp::plus, a, b);
      auto d = my_eval("d", BinaryOp::div, c, b);

      std::cout<<serialize(d);

      return 0;
   }

The output will look like this:

.. code-block:: text

   b = 0.100000
   a = 0.500000
   c = a + b
   d = c / b

This example can be extended by using a thirdparty serialization framework 
like Boost::serialize, jsoncpp etc or by writing the functionality to deserialize 
a string to a dependency tree by hand.