#include <gtest/gtest.h>
#include <string>
#include "lru.h"
#include <chrono>
#include <future>
#include <random>

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

const std::string getRandomString(const int stringLength) {
  std::uniform_int_distribution<int> d(30, 126);
  std::random_device rd1; // uses RDRND or /dev/urandom
  std::string retString(' ',stringLength);
  for (int i = 0; i < stringLength; i++) {
     *(const_cast<char*>(retString.data())) = static_cast<char>(d(rd1));
  }
  return retString;
}

void insertToCache(LruCache<int,std::string>* cache, const std::vector<std::pair<int,std::string> >& dataVector) {
  for(auto& dataItem : dataVector) {
    cache->add(dataItem.first,dataItem.second);
  }
}

const std::chrono::microseconds getTimingForInsertTest(const int cacheSize, 
    const int numDataPoints, const int numberOfThreads = 5) {
  LruCache<int,std::string> cache(cacheSize);
  std::uniform_int_distribution<int> d(0, numDataPoints * 10);
  std::random_device rd1; // uses RDRND or /dev/urandom
  std::vector<std::pair<int,std::string> > dataVector[numberOfThreads];
  for (int i = 0; i < numberOfThreads; i++) {
    for (int j = 0; j < numDataPoints; j++) {
      dataVector[i].push_back(std::make_pair(d(rd1), getRandomString(15)));
    }
  }
  std::vector<std::future<void> > futures;
  auto t1 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < numberOfThreads; i++) {
    futures.push_back(std::async( std::bind(insertToCache, &cache, std::cref(dataVector[i]))));
  }
  for (auto& future : futures) {
    future.wait();
  }
  auto t2 = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1);
}

TEST(LruTest,AddTimingTest) {
  std::chrono::microseconds durationForTest = getTimingForInsertTest(500,500,5);
  std::cout << "Random Add Test took "
    << durationForTest.count()
    << " microseconds\n";
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
