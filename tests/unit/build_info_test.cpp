#include "chronos/core/build_info.hpp"
#include <gtest/gtest.h>

TEST(BuildInfo, IdentifiesScaffoldHonestly) {
    EXPECT_EQ(chronos::version(), "0.2.0-unverified");
    EXPECT_NE(chronos::description().find("not a production kernel"), std::string_view::npos);
}
