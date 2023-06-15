#include <gtest/gtest.h>

#include <parametric/core.hpp>
#include <sstream>

using namespace std::string_literals;

namespace {

    struct Foo {
        std::string name;
        double age;
    };

    class Bar : public parametric::ComputeNode<Bar, parametric::Results<Foo>, parametric::Arguments<Foo, Foo>>
    {
    public:
        Bar(std::string const& id)
        {
            set_id(id);
        }

        std::string serialize() const override
        {
            return "{\n    \"name\": \"Bar\",\n    \"inputs\": \n    [\n        \"" +
                   arg<0>().id() + "\"\n        \"" + arg<1>().id() + "\n    ]\n}\n";
        }
    };
}

namespace parametric {
    template <> //why is template specialization needed? why does this not work with an overload?
    std::string serialize(Foo const& f)
    {
        std::ostringstream oss;
        oss << std::setprecision(4) << f.age;
        std::string age_str = oss.str();

        return "{\n"s +
            "    \"name\": \"" + f.name + "\",\n" + 
            "    \"age\": " + age_str + "\n" +
            "}\n";
    }
}

TEST(Serialize, CustomType)
{
    auto x = parametric::new_param(Foo{"XYZ", 44.3});
    std::string expected = "{\n    \"name\": \"XYZ\",\n    \"age\": 44.3\n}\n";
    EXPECT_EQ(x.node_pointer()->serialize(), expected);
}

TEST(Serialize, CustomComputeNode)
{
    auto x = parametric::new_param(Foo{"XYZ", 44.3}, "x");
    auto y = parametric::new_param(Foo{"ABC", 1.23}, "y");
    auto z = parametric::compute(std::make_shared<Bar>("z"), x, y);

    std::string expected = 
        "{\n    \"name\": \"Bar\",\n    \"inputs\": \n    [\n        \"x\"\n        \"y\n    ]\n}\n";
    EXPECT_EQ(z.compute_node()->serialize(), expected);
}

TEST(Serialize, Serializer)
{
    // create a dependency tree
    auto a= parametric::new_param(Foo{"XYZ", 44.3}, "a");
    auto b = parametric::new_param(Foo{"ABC", 1.23}, "b");
    auto c = parametric::compute(std::make_shared<Bar>("z"), a, b);
    auto d = parametric::compute(std::make_shared<Bar>("d"), b, c);

    // create a serializer for d
    parametric::Serializer s(*(d.node_pointer()));
    EXPECT_FALSE(s.is_dirty());

    // extract the stacks of serialized nodes
    auto& pstack = s.parameter_stack();
    auto& cstack = s.compute_node_stack();
    EXPECT_TRUE(s.is_dirty());

    // there should be two root parameters. Although b gets
    // visited twice, it should be on the stack only once.
    std::string expected;

    expected = 
        "{\n    \"name\": \"XYZ\",\n    \"age\": 44.3\n}\n";
    EXPECT_EQ(pstack.top().id, "a");
    EXPECT_EQ(pstack.top().serialized, expected);
    pstack.pop();

    expected = 
        "{\n    \"name\": \"ABC\",\n    \"age\": 1.23\n}\n";
    EXPECT_EQ(pstack.top().id, "b");
    EXPECT_EQ(pstack.top().serialized, expected);
    pstack.pop();

    EXPECT_TRUE(pstack.empty());

    // there should be two compute nodes on the stack, the
    // one for c on top, then the one for d.

    expected = 
        "{\n    \"name\": \"Bar\",\n    \"inputs\": \n    [\n        \"a\"\n        \"b\n    ]\n}\n";
    EXPECT_EQ(cstack.top().id, "c");
    EXPECT_EQ(cstack.top().serialized, expected);
    cstack.pop();

    expected = 
        "{\n    \"name\": \"Bar\",\n    \"inputs\": \n    [\n        \"b\"\n        \"c\n    ]\n}\n";
    EXPECT_EQ(cstack.top().id, "d");
    EXPECT_EQ(cstack.top().serialized, expected);
    cstack.pop();

    EXPECT_TRUE(cstack.empty());

    // reparsing the tree should replenish the two stacks.
    s.parse_tree();
    EXPECT_FALSE(s.is_dirty());
    EXPECT_EQ(s.parameter_stack().size(), 2);
    EXPECT_EQ(s.compute_node_stack().size(), 2);
}
