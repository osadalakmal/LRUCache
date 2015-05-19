#ifndef INCLUDED_LRU_H
#define INCLUDED_LRU_H

#include <boost/lockfree/queue.hpp>
#include <map>
#include <list>
#include <memory>
#include <cstddef>
#include <mutex>
#include <thread>
#include <atomic>

template<typename KEY, typename VALUE>
class LruCache {
  //Deleted functions
  LruCache(const LruCache&);
  LruCache(const LruCache&&);
  LruCache& operator=(const LruCache&);
  LruCache&& operator=(const LruCache&&);

  public:
  // Typedefs Publicly Available
  typedef std::list<KEY> KEY_LIST_TYPE;
  typedef typename KEY_LIST_TYPE::iterator KEY_LIST_ITERATOR;
  struct DataMapElement {
    VALUE d_value;
    KEY_LIST_ITERATOR d_placeInList;
    std::chrono::system_clock::time_point d_lastAccessTime;
  };
  typedef typename LruCache<KEY,VALUE>::DataMapElement DATA_MAP_ELEMENT;
  typedef typename std::map<KEY, DATA_MAP_ELEMENT> DATA_MAP_TYPE;
  typedef typename DATA_MAP_TYPE::iterator DATA_MAP_ITERATOR;
  typedef typename std::shared_ptr<boost::lockfree::queue<KEY> > DELETE_QUEUE_SHARED_PTR;
  typedef typename std::weak_ptr<boost::lockfree::queue<KEY> > DELETE_QUEUE_WEAK_PTR;

    // Public Interface
    LruCache(const size_t cacheSize, 
        const std::chrono::milliseconds quietPeriod = 0);
    ~LruCache();
    void add(const KEY& key, const VALUE& value);
    std::unique_ptr<VALUE> get(const KEY& key);
  private:
    void evict(const size_t numOfElements);
    void deleterThread();

    const size_t                        d_cacheSize;
    DATA_MAP_TYPE                       d_dataMap;
    KEY_LIST_TYPE                       d_timeOrderedList;
    std::mutex                          d_accessMutex;
    std::chrono::milliseconds           d_quietPeriod;
    DELETE_QUEUE_SHARED_PTR             d_deleterQueue;
    std::atomic_flag                    d_deleterExitFlag;
    std::thread                         d_deleterThread;
};

#endif

template <typename KEY, typename VALUE>
void LruCache<KEY,VALUE>::deleterThread() {
  while(!d_deleterExitFlag.test_and_set()) {
    d_deleterQueue->consume_all([this] (KEY key) { d_dataMap.erase(key); });
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

template <typename KEY, typename VALUE>
LruCache<KEY,VALUE>::LruCache(const size_t cacheSize,
        const std::chrono::milliseconds quietPeriod) :
  d_cacheSize(cacheSize),
  d_accessMutex(),
  d_quietPeriod(quietPeriod),
  d_deleterQueue(std::make_shared<boost::lockfree::queue<KEY> >(500)),
  d_deleterExitFlag()
{
  d_deleterThread = std::thread(std::bind(&LruCache<KEY,VALUE>::deleterThread,this));
}

template <typename KEY, typename VALUE>
LruCache<KEY,VALUE>::~LruCache() {
  d_deleterExitFlag.clear();
  d_deleterThread.join();
  d_timeOrderedList.clear();
  d_dataMap.clear();
}

template <typename KEY, typename VALUE>
void LruCache<KEY,VALUE>::add(const KEY& key, const VALUE& value) {
  std::lock_guard<std::mutex> accessLockGuard(d_accessMutex);
  if (d_dataMap.size() == d_cacheSize) {
    evict(1);
  }
  d_timeOrderedList.emplace_front(key);
  DataMapElement dataElement = {value,d_timeOrderedList.begin(),std::chrono::system_clock::now()};
  auto dataMapItem = std::make_pair(key, dataElement);
  d_dataMap.insert(dataMapItem);
}

template <typename KEY, typename VALUE>
std::unique_ptr<VALUE> LruCache<KEY,VALUE>::get(const KEY& key) {
  DATA_MAP_ITERATOR iter = d_dataMap.find(key);
  if ( iter != d_dataMap.end() ) {
    if (std::chrono::milliseconds(d_quietPeriod) <
        std::chrono::duration_cast<std::chrono::milliseconds>(
          (std::chrono::system_clock::now() - iter->second.d_lastAccessTime))) {
      std::lock_guard<std::mutex> accessLockGuard(d_accessMutex);
      d_timeOrderedList.splice(d_timeOrderedList.begin(), d_timeOrderedList, iter->second.d_placeInList);
    }
    return std::unique_ptr<VALUE>( new VALUE(iter->second.d_value) );
  }
  return std::unique_ptr<VALUE>();
}


template <typename KEY, typename VALUE>
void LruCache<KEY,VALUE>::evict(const size_t numOfElements) {
  for(size_t i=0; i<numOfElements; i++) {
    d_deleterQueue->push(d_timeOrderedList.back());
    d_timeOrderedList.pop_back();
  }
}
