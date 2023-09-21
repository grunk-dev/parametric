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
 