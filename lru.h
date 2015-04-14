#ifndef INCLUDED_LRU_H
#define INCLUDED_LRU_H

#include <map>
#include <list>
#include <memory>
#include <cstddef>
#include <mutex>

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
    typedef typename std::pair<VALUE, KEY_LIST_ITERATOR> DATA_MAP_ELEMENT;
    typedef typename std::map<KEY, DATA_MAP_ELEMENT> DATA_MAP_TYPE;
    typedef typename DATA_MAP_TYPE::iterator DATA_MAP_ITERATOR;
   
    // Public Interface
    LruCache(const size_t cacheSize, 
        const std::chrono::milliseconds quietPeriod = 0);
    ~LruCache();
    void add(const KEY& key, const VALUE& value);
    std::unique_ptr<VALUE> get(const KEY& key);
  private:
    void evict(const size_t numOfElements);

    const size_t                        d_cacheSize;
    DATA_MAP_TYPE                       d_dataMap;
    KEY_LIST_TYPE                       d_timeOrderedList;
    std::mutex                          d_accessMutex;
    std::chrono::milliseconds                d_quietPeriod;

};

#endif

template <typename KEY, typename VALUE>
LruCache<KEY,VALUE>::LruCache(const size_t cacheSize,
        const std::chrono::milliseconds quietPeriod) :
  d_cacheSize(cacheSize),
  d_accessMutex(),
  d_quietPeriod(quietPeriod)
{
}

template <typename KEY, typename VALUE>
LruCache<KEY,VALUE>::~LruCache() {
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
  auto dataElement = std::make_pair(value,d_timeOrderedList.begin());
  auto dataMapItem = std::make_pair(key, dataElement);
  d_dataMap.insert(dataMapItem);
}

template <typename KEY, typename VALUE>
std::unique_ptr<VALUE> LruCache<KEY,VALUE>::get(const KEY& key) {
  DATA_MAP_ITERATOR iter = d_dataMap.find(key);
  if ( iter != d_dataMap.end() ) {
    std::lock_guard<std::mutex> accessLockGuard(d_accessMutex);
    d_timeOrderedList.splice(d_timeOrderedList.begin(), d_timeOrderedList, iter->second.second);
    return std::unique_ptr<VALUE>( new VALUE(iter->second.first) );;
  } else {
    return std::unique_ptr<VALUE>();
  }
}


template <typename KEY, typename VALUE>
void LruCache<KEY,VALUE>::evict(const size_t numOfElements) {
  for(size_t i=0; i<numOfElements; i++) {
    d_dataMap.erase(d_timeOrderedList.back());
    d_timeOrderedList.pop_back();
  }
}
