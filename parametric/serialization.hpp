#pragma once 

#include <parametric/dag.hpp>

#include <stack>
#include <unordered_map>

namespace parametric {

/**
 * @brief serialize will serialize an instance of type T to a std::string. 
 *
 * If a user of parametric wants to serialize a parametric tree to an
 * std::string or a file, all types that could be root parameters of the 
 * tree must provide a specialization of this function template.
 * 
 * @tparam T the type to be serialized
 * @return std::string A serialized string
 */
template <typename T>
inline std::string serialize(T const&){
    throw std::logic_error("Not implemented");
}

// forward declaration
namespace impl {
    template <typename T>
    class param_holder;
}

/**
 * @brief The Serializer class is a convenience class to recursively serialize
 * a dependency tree starting at one node. The class will traverse all ancestor
 * nodes, serialize them to a string and store the strings in two stacks. One
 * for the root parameters, and one for the compute nodes. 
 *
 * The compute node stack will be in reverse topological order, so that 
 * popping from it will provide the compute nodes in topological order.
 *
 * The Serializer constructor requires that the types used in root nodes have
 * specialized versions of ``template <typename T> parametric::serialize(T const&)``
 * and that all ``ComputeNode``\s along the way 
 * implement ``virtual std::string DAGNode::serialize() const``.
 * 
 */
class Serializer
{
public:

    /**
     * @brief The serializer creates two stacks holding Entry instances. 
     *
     * An Entry consists of the id of a node, as well as its serialized 
     * string.
     * 
     */
    struct Entry {
        std::string id;
        std::string serialized;
    };

    /**
     * @brief Construct a new Serializer object given a parametrc::impl::param_holder<T>.
     *
     * Given a parametric::param<T> p, instantiate the Serializer with 
     *
     * .. code-block:: cpp
     *    Serializer s(*(p.node_pointer()));
     * 
     * @tparam T The type held by the param_holder
     * @param p The param_holder, that is a node in the feature tree holding a parameter
     */
    template <typename T>
    Serializer(impl::param_holder<T> const& p)
     : start(p)
    {
        parse_tree();
    }

    /**
     * @brief returns a reference to the stack holding all string representations of 
     * independent root nodes of the tree. Since this is a non-const reference, the 
     * Serializer is left in "dirty" state after a call to this function. 
     * 
     * @return std::stack<Entry>& 
     */
    std::stack<Entry>& parameter_stack()
    {
        dirty = true;
        return parameters;
    }

    /**
     * @brief returns a reference to the stack holding all string representations of 
     * compute nodes of the tree. Since this is a non-const reference, the 
     * Serializer is left in "dirty" state after a call to this function. 
     * 
     * @return std::stack<Entry>& 
     */
    std::stack<Entry>& compute_node_stack()
    {
        dirty = true;
        return compute_nodes;
    }

    /**
     * @brief getter for the dirty flag
     * 
     * @return true if the stacks are dirty, because non-const 
               references to the stacks have been returned previously
     * @return false true if the stacks not dirty, and the stacks are
               up to date.
     */
    bool is_dirty(){
        return dirty;
    }

    /**
     * @brief recursively parses the dependency tree upwards starting from the 
     * start node. Does nothing, if the state of the Serializer is not dirty. 
     *
     * The dirty flag is true, if either Serializer::parameter_stack() or 
     * Serializer::compute_node_stack() have been called.
     * 
     */
    void parse_tree() {
        if (dirty) {

            parameters = std::stack<Entry>();
            compute_nodes = std::stack<Entry>();

            SerializationVisitor visitor(parameters, compute_nodes);
            start.accept(
                visitor, 
                0, 
                parametric::DAGNode::Direction::up
            );

            dirty = false;
        }
    }

private:

    /**
     * @brief This class is a vistor, that visits the dependency tree and 
     * collects the serialized strings of all nodes in the two stacks parameters
     * and compute_nodes
     * 
     */
    class SerializationVisitor
    {
    public:

        /**
         * @brief Construct a new SerializationVisitor object from 
         * refrences to the parameter stack and compute node stack of the 
         * parent Serializer.
         * 
         * @param p Reference to the parameter stack
         * @param n Reference to the node stack
         */
        SerializationVisitor(
            std::stack<Entry>& p,
            std::stack<Entry>& n
        )
         : parameters(p)
         , compute_nodes(n)
        {}

        /**
         * @brief visit a node and store its serialized string in one
         * of the two stacks, depending of wether it is a parameter node
         * or a compute node. This is distinguished based on the depth 
         * modulo 2, because compute nodes and parameters alternate in 
         * the tree.
         * 
         * @param n The visited node in the dependency tree.
         * @param depth The current depth measured from the starting node.
         */
        void visit(parametric::DAGNode const& n, size_t depth)
        {
            if (visited[&n]) {
                return;
            }

            std::string str = n.serialize();
            if (!str.empty()) {

                bool is_parameter = ((depth % 2) == 0);

                if (is_parameter) {
                    parameters.push({n.id(), str});
                } else {
                    compute_nodes.push({n.id(), str});
                }
            }

            visited[&n] = true;
        }
    private:
        using Visited = std::unordered_map<DAGNode const*, bool>;
        Visited visited;

        std::stack<Entry>& parameters;
        std::stack<Entry>& compute_nodes;
    };

    bool dirty {true };
    DAGNode const& start;
    std::stack<Entry> parameters;
    std::stack<Entry> compute_nodes;

};

} // namespace parametric