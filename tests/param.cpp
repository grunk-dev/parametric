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
