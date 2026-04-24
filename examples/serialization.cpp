// SPDX-FileCopyrightText: 2026 Jan Kleinert <jan.kleinert@dlr.de>
//
// SPDX-License-Identifier: Apache-2.0

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

namespace parametric {
    template  <>
    std::string serialize(MyDouble const& md){
        return std::to_string(md.value);
    }
}

std::string serialize(parametric::param<MyDouble> const& p){

    parametric::RecursiveSerializer serializer(*(p.node_pointer()));

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
    auto a = parametric::new_param("a", MyDouble{0.5});
    auto b = parametric::new_param("b", MyDouble{0.1});
    auto c = my_eval("c", BinaryOp::plus, a, b);
    auto d = my_eval("d", BinaryOp::div, c, b);

    std::cout<<serialize(d);

    return 0;
}
