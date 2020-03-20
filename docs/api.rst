.. `toctree`

.. _parametric-api:

*************
API Reference
*************

The parametric library API consists of the following parts:

* :ref:`parametric/core.hpp <core-api>`: the core API providing argument handling
  facilities and a lightweight subset of formatting functions
* :ref:`parametric/dag.hpp <dag-api>`: the full format API providing compile-time
  format string checks, output iterator and user-defined type support
* :ref:`parametric/operators.hpp <operators-api>`: additional formatting support for ranges
  and tuples

All functions and types provided by the library reside in namespace ``fmt`` and
macros have prefix ``FMT_``.

.. _core-api:

Core
====

``parametric/core.hpp`` defines everything required to build up compute graphs.
The main building blocks are parmetric values (``parametric::param``), compute nodes (``parametric::ComputeNode``)
and interface parameters(``parametric::InterfaceParam``).
The role of each object is described at the :ref:`design concept <concepts>` section.

.. doxygenfile:: core.hpp


.. _dag-api:

DAG
===

.. doxygentypedef:: parametric::NodeRef

.. doxygenclass:: parametric::DAGNode
   :members:

.. _operators-api:



Operators
=========