#include <gtest/gtest.h>

#include <parametric/core.hpp>

namespace {
    struct Foo {
        std::string name;
        double age;
    };
}

namespace parametric {
    template <> //why is template specialization needed? why does this not work with an overload?
    std::string serialize(Foo const& f)
    {
        using namespace std::string_literals;
        return "{\n"s +
            "    \"name\": \"" + f.name + "\",\n" + 
            "    \"age\": " + std::to_string(f.age) + "\n" +
            "}\n";
    }
}

TEST(Serialize, basic)
{
    auto x = parametric::new_param(Foo{"XYZ", 44.3});
    std::string expected = "{\n    \"name\": \"XYZ\",\n    \"age\": 44.300000\n}\n";
    EXPECT_EQ(x.node_pointer()->serialize(), expected);
}