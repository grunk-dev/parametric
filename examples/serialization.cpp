#include <iostream>
#include <stack>
#include <unordered_map>
#include <parametric/core.hpp>
#include <parametric/serialization.hpp>

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

enum struct BinaryOp {
    plus,
    minus,
    mult,
    div
};

class MyBinaryOperation : public parametric::ComputeNode<MyBinaryOperation>
{
public:
    MyBinaryOperation(const char* id,
                      BinaryOp op,
                      parametric::param<MyDouble> const& l,
                      parametric::param<MyDouble> const& r
    )
     : operation(op)
     , left(l)
     , right(r)
    {
        set_id(id);
        depends_on(left);
        depends_on(right);
        computes(output, parametric::param<MyDouble>(id));
    }

    void eval() const override 
    {
        if (!output.expired()) {
            switch (operation) {
                case BinaryOp::plus:
                    output.set_value(left.value() + right.value());
                    return;
                case BinaryOp::minus:
                    output.set_value(left.value() - right.value());
                    return;
                case BinaryOp::mult:
                    output.set_value(left.value() * right.value());
                    return;
                case BinaryOp::div:
                    output.set_value(left.value() / right.value());
                    return;
                default:
                    throw std::logic_error("Not implemented\n");
            }
        }
    }

    std::string serialize() const override 
    {
        std::string out = left.id();
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
        out += right.id();
        return out;
    }

    parametric::param<MyDouble> result() const {
        return output;
    }

private:
    BinaryOp operation;
    parametric::param<MyDouble> const left;
    parametric::param<MyDouble> const right;
    parametric::OutputParam<MyDouble> mutable output;
};

parametric::param<MyDouble> my_eval(const char* id, 
                                    BinaryOp op, 
                                    parametric::param<MyDouble> const& left, 
                                    parametric::param<MyDouble> const& right
)
{
    return parametric::compute_node_ptr<MyBinaryOperation>(new MyBinaryOperation(id, op, left, right))->result();
}

namespace parametric {
    template  <>
    std::string serialize(MyDouble const& md){
        return std::to_string(md.value);
    }
}

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

int main()
{
    auto a = parametric::new_param(MyDouble{0.5}, "a");
    auto b = parametric::new_param(MyDouble{0.1}, "b");
    auto c = my_eval("c", BinaryOp::plus, a, b);
    auto d = my_eval("d", BinaryOp::div, c, b);

    std::cout<<serialize(d);

    return 0;
}
