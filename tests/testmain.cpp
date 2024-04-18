#include <gtest/gtest.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    
    // https://github.com/google/googletest/blob/main/docs/advanced.md#death-tests-and-threads
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    
    return RUN_ALL_TESTS();
}
