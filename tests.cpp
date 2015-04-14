#include <gtest/gtest.h>
#include <string>
#include "lru.h"
#include <chrono>
#include <future>
#include <random>

using namespace std;
using namespace ::testing;

TEST(LruTest,AddTest) {
  LruCache<int,string> cache(50, std::chrono::milliseconds(0));
  cache.add(5,"some");
  auto a = cache.get(5);
  ASSERT_TRUE((bool)a);
  ASSERT_STREQ(a->c_str(), "some");
}

TEST(LruTest,WithoutAdditionTest) {
  LruCache<int,string> cache(50, std::chrono::milliseconds(0));
  ASSERT_FALSE(cache.get(5));
}

TEST(LruTest,EvictTest) {
  LruCache<int,string> cache(2, std::chrono::milliseconds(0));
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

typedef std::vector<std::pair<int,std::string> > DATA_VECTOR;
typedef std::vector<std::pair<int,std::string> >* DATA_VECTOR_PTR;
typedef std::unique_ptr<DATA_VECTOR> DATA_VECTOR_UNIQUE_PTR;

void insertToCache(LruCache<int,std::string>* cache, DATA_VECTOR* dataVector) {
  DATA_VECTOR_UNIQUE_PTR autoReleaser(dataVector); //take ownership
  for(auto& dataItem : *dataVector) {
    cache->add(dataItem.first,dataItem.second);
  }
}

const std::chrono::microseconds getTimingForInsertTest(const int cacheSize, 
    const int numDataPoints, const int numberOfThreads = 5) {
  LruCache<int,std::string> cache(cacheSize, std::chrono::milliseconds(0));
  std::uniform_int_distribution<int> d(0, numDataPoints * 10);
  std::random_device rd1; // uses RDRND or /dev/urandom
  auto dataVector = new DATA_VECTOR_PTR[numberOfThreads];
  for (int i = 0; i < numberOfThreads; i++) {
    dataVector[i] = new DATA_VECTOR();
    for (int j = 0; j < numDataPoints; j++) {
      dataVector[i]->push_back(std::make_pair(d(rd1), getRandomString(15)));
    }
  }
  std::vector<std::future<void> > futures;
  auto t1 = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < numberOfThreads; i++) {
    futures.push_back(std::async( std::bind(insertToCache, &cache, dataVector[i])));
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
