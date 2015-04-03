#include <gtest/gtest.h>
#include <string>
#include "lru.h"

using namespace std;
using namespace ::testing;

TEST(LruTest,AddTest) {
  LruCache<int,string> cache(50);
  cache.add(5,"some");
  ASSERT_STREQ(cache.get(5)->c_str(), "some");
}

TEST(LruTest,WithoutAdditionTest) {
  LruCache<int,string> cache(50);
  ASSERT_FALSE(cache.get(5));
}

TEST(LruTest,EvictTest) {
  LruCache<int,string> cache(2);
  cache.add(1,"apple");
  cache.add(2,"bee");
  cache.add(3,"cat");
  ASSERT_FALSE(cache.get(1));
  ASSERT_STREQ(cache.get(2)->c_str(), "bee");
  ASSERT_STREQ(cache.get(3)->c_str(), "cat");
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
