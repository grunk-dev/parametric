.. `toctree`

.. _parametric-api:

*************
API Reference
*************

The parametric library API consists of the following parts:

* :ref:`parametric/core.hpp <core-api>`: the core API everything required tp build up compute graphs.
* :ref:`parametric/dag.hpp <dag-api>`: the DAG API provides functions and classes to build up directed acyclic graphs.
* :ref:`parametric/operators.hpp <operators-api>`: the header file adds provides parametric operator overloads

All functions and types provided by the library reside in namespace ``parametric``.

.. _core-api:

Core
====

``parametric/core.hpp`` defines everything required to build up compute graphs.
The main building blocks are parmetric values (``parametric::param``), compute nodes (``parametric::ComputeNode``)
and interface parameters(``parametric::InterfaceParam``).
The role of each object is described at the :ref:`design concept <concepts>` section.


.. doxygenclass:: parametric::param
   :members:

.. doxygenfunction:: parametric::new_param(const T&, const std::string&)
.. doxygenfunction:: parametric::new_param(const T&)
.. doxygenfunction:: parametric::new_param()

.. doxygenfunction:: parametric::eval

.. doxygenclass:: parametric::InterfaceParam
   :members:   

.. doxygenclass:: parametric::ComputeNode
   :members:


.. doxygenfunction:: parametric::new_node

.. _dag-api:

DAG
===

.. doxygentypedef:: parametric::NodeRef

.. doxygenclass:: parametric::DAGNode
   :members:

.. _operators-api:



Operators
=========

.. doxygenfile:: operators.hpp