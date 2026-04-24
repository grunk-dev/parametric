<!--
SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>

SPDX-License-Identifier: Apache-2.0
-->

# v0.3.4

- Add a second defaulted template parameter to `parametric::param`, so that consumers of the library can provide their own serialization class (#20)

# v0.3.3

- Expose parents and children of DAGNode in the public interface (#19)

# v0.3.2

 - Support function returning references (#17)

# v0.3.1

 - Fix bug, where cloned nodes would have the same parent-child relations as the source, but not necessarily the same order of parents/children. Now
   `clone` preserves the ordering of parents and children (#15)
 - Fix regression from redesign introduced in v0.3.0, which made it tedious to write ComputeNodes for void functions (#16)
 - Add default template arguments to `ComputeNode`, as in some cases the result and argument types need not be specified (#16)

# v0.3.0

 - Redesign parametic::ComputeNode (#13) to eliminate redundant storange of parent-child relations. This strongly effects the way custom compute nodes are written. This redesign was necessary to enable the possibility of deep-copying a parametric tree
 - Enable deep-copying of a parametric tree (#12). nodes
   now have a clone method that clones the node and recursively all parent nodes, together with the parent-child relations.

# v0.2.3

 - Fix bug related to RAII in parametric::compute_node_ptr (#10)

# v0.2.2

 - Support Conan 2.0 (#7)

# v0.2.1

 - Minor fix for serialization
 