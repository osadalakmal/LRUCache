#include <gtest/gtest.h>
#include <string>
#include "lru.h"
#include <chrono>
#include <future>
#include <random>

using namespace std;
using namespace ::testing;

typedef LruCache<int,std::string> LruTestCache;
typedef std::vector<std::chrono::microseconds> TimingVector;

TEST(LruTest,AddTest) {
  LruTestCache cache(50, std::chrono::milliseconds(0));
  cache.add(5,"some");
  auto a = cache.get(5);
  ASSERT_TRUE((bool)a);
  ASSERT_STREQ(a->c_str(), "some");
}

TEST(LruTest,WithoutAdditionTest) {
  LruTestCache cache(50, std::chrono::milliseconds(0));
  ASSERT_FALSE(cache.get(5));
}

TEST(LruTest,EvictTest) {
  LruTestCache cache(2, std::chrono::milliseconds(0));
  cache.add(1,"apple");
  cache.add(2,"bee");
  cache.add(3,"cat");
  sleep(2);
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
typedef std::vector<DATA_VECTOR> DATA_VECTOR_LIST;
typedef std::unique_ptr<DATA_VECTOR> DATA_VECTOR_UNIQUE_PTR;

void insertToCache(LruTestCache& cache, DATA_VECTOR& dataVector) {
  for(auto& dataItem : dataVector) {
    cache.add(dataItem.first,dataItem.second);
  }
}

void setupDataVectors(DATA_VECTOR_LIST& dataVectors, int numDataPoints, 
    int numberOfThreads) {
  std::uniform_int_distribution<int> d(0, numDataPoints * 10);
  std::random_device rd1; // uses RDRND or /dev/urandom

  for (int i = 0; i < numberOfThreads; i++) {
    for (int j = 0; j < numDataPoints; j++) {
      dataVectors[i].push_back(std::make_pair(d(rd1), getRandomString(15)));
    }
  }
}

void waitForInsertions(LruTestCache& cache, const DATA_VECTOR_LIST& dataVectors, 
    int numberOfThreads) {
  std::vector<std::future<void> > futures;
  for (int i = 0; i < numberOfThreads; i++) {
    futures.push_back(std::async(std::bind(insertToCache, std::ref(cache), dataVectors[i])));
  }
  for (auto& future : futures) {
    future.wait();
  }
}

void readPercentageOfData(LruCache<int, std::string>& cache, const DATA_VECTOR& dataVector,
    int numDataPoints, double percentageToRead) {
  std::uniform_int_distribution<int> indexDist(0, dataVector.size());
  std::random_device rd1;
  for (int i = static_cast<int>(dataVector.size() * percentageToRead);
      i > 0; i-- ) {
      cache.get(dataVector[indexDist(rd1)].first);
  }
  std::uniform_int_distribution<int> dataDist(numDataPoints * 10, numDataPoints * 12);
  for (int i = static_cast<int>(dataVector.size() * percentageToRead);
      i > 0; i-- ) {
      cache.get(dataVector[dataDist(rd1)].first);
  }
}

const TimingVector getTimingForInsertTest(const int cacheSize, 
    const int numDataPoints, const int numberOfThreads = 5) {
  LruTestCache cache(cacheSize, std::chrono::milliseconds(0));
  //auto numOfReads = 500;
  auto fractionInCache = 0.5;
  DATA_VECTOR_LIST dataVectors(numberOfThreads);
  setupDataVectors(dataVectors, numDataPoints, numberOfThreads);

  auto t1 = std::chrono::high_resolution_clock::now();
  waitForInsertions(cache, dataVectors, numberOfThreads);
  auto t2 = std::chrono::high_resolution_clock::now();
  readPercentageOfData(cache,dataVectors[0],numDataPoints,fractionInCache);
  auto t3 = std::chrono::high_resolution_clock::now();

  return { std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1),
           std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2),};
}

TEST(LruTest,AddTimingTest) {
  TimingVector durationsForTest = getTimingForInsertTest(500,500,5);
  std::cout << "Random Add Test took "
    << durationsForTest[0].count()
    << " microseconds\n";
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
