#include <parametric/core.hpp>
#include <gtest/gtest.h>


class Param : public ::testing::Test 
{
public:

    class Counter
    {
    public:
        Counter() {
            m_ctor_counter++;
        }

        Counter(Counter const&){
            m_copy_counter++;
        }
        
        Counter& operator=(Counter const&){
            m_copy_counter++;
            return *this;
        }

        Counter(Counter&&){
            m_move_counter++;
        }
        
        Counter& operator=(Counter&&){
            m_move_counter++;
            return *this;
        }

        ~Counter() {
            m_dtor_counter++;
        }
    };

    void SetUp() 
    {
        m_ctor_counter = 0;
        m_copy_counter = 0;
        m_move_counter = 0;
        m_dtor_counter = 0;
    }

    static int m_ctor_counter;
    static int m_copy_counter;
    static int m_move_counter;
    static int m_dtor_counter;
};
int Param::m_ctor_counter;
int Param::m_copy_counter;
int Param::m_move_counter;
int Param::m_dtor_counter;

TEST_F(Param, make_param){

    {
        auto t = parametric::make_param<Counter>("");
    }

    EXPECT_EQ(m_ctor_counter, 1);
    EXPECT_EQ(m_copy_counter, 0);
    EXPECT_EQ(m_move_counter, 0);
    EXPECT_EQ(m_dtor_counter, 1);
}

TEST_F(Param, new_param){

    {
        auto t = parametric::new_param(Counter(), "");
    }

    EXPECT_EQ(m_ctor_counter, 1);
    EXPECT_EQ(m_copy_counter, 1);
    EXPECT_EQ(m_move_counter, 0);
    EXPECT_EQ(m_dtor_counter, 2);
}

TEST_F(Param, param_copy){

    {
        auto t = parametric::make_param<Counter>("");

        // this should copy the shared_ptr
        auto s = t;
    }

    EXPECT_EQ(m_ctor_counter, 1);
    EXPECT_EQ(m_copy_counter, 0);
    EXPECT_EQ(m_move_counter, 0);
    EXPECT_EQ(m_dtor_counter, 1);
}

namespace {

    class Add : public parametric::ComputeNode<Add>
    {
    public:
        Add(const parametric::param<double>& a, const parametric::param<double> b)
            : _a(a), _b(b)
            , _resultNode(parametric::new_param<double>())
        {
            depends_on(_a);
            depends_on(_b);
            computes(_resultNode, parametric::param<double>("result"));
        }

        void eval() const
        {
            if(!_resultNode.expired()) {
                _resultNode.set_value(
                    _a + _b
                 );
            }
        }

        parametric::param<double> result() const
        {
            return _resultNode;
        }

    private:
        parametric::param<double> _a;
        parametric::param<double> _b;
        mutable parametric::OutputParam<double> _resultNode;
    };
}

TEST_F(Param, compute_node_ptr_move)
{
    auto a = parametric::new_param(1.1);
    auto b = parametric::new_param(2.2);

    auto c = [&]{
        auto tmp = parametric::new_node<Add>(a, b);
        return std::move(tmp);
        // tmp destructor is called here. This should not 
        // release the nodes of the moved-to compute node ptr.
    }();
    EXPECT_NEAR(c->result().value(), 3.3, 1e-14);
}