#pragma once 

#include <parametric/dag.hpp>

#include <stack>
#include <unordered_map>

namespace parametric {

template <typename T>
class param;

template <typename T>
std::string serialize(T const&){
    throw std::logic_error("Not implemented");
}

namespace impl {
    template <typename T>
    class param_holder;
}

class Serializer
{
public:

    struct Entry {
        std::string id;
        std::string serialized;
    };

    template <typename T>
    Serializer(impl::param_holder<T> const& p)
     : start(p)
    {
        parse_tree();
    }

    std::stack<Entry>& parameter_stack()
    {
        dirty = true;
        return parameters;
    }

    std::stack<Entry>& compute_node_stack()
    {
        dirty = true;
        return compute_nodes;
    }

private:

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

    class SerializationVisitor
    {
    public:

        SerializationVisitor(
            std::stack<Entry>& p,
            std::stack<Entry>& n
        )
         : parameters(p)
         , compute_nodes(n)
        {}

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