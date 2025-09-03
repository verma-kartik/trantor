#include <gtest/gtest.h>

// Basic test to verify setup works
TEST(BasicTest, AlwaysPass) {
    EXPECT_TRUE(true);
}

TEST(BasicTest, SimpleAssertions) {
    EXPECT_EQ(2 + 2, 4);
    EXPECT_NE(2 + 2, 5);
    EXPECT_LT(2, 3);
    EXPECT_GT(3, 2);
}
