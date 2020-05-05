#pragma once

#include <algorithm>
#include <condition_variable>
#include <chrono>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

namespace spt::util
{
  class Synchrophasotron
  {
    template<typename KeyType, typename ValueType, int TTL/*seconds*/ >
    friend
    class ExpirationCache;

  private:
    Synchrophasotron() {}

    virtual int getTTL() const = 0; // get current object ttl , overridden in template
    virtual void clearAll() = 0; // remove all elements in current object
    virtual int count() const = 0; // count of element in current instantiated object

    static void RemoveExpiredElements() // clear all objects from caches() for which the TTL was expired
    {
      int count = 1;
      while ( !endExecution())
      {
        count++;
        for ( auto& i:caches())
        {
          if ( count % i->getTTL() == 0 )
          {
            i->clearAll();
          }
        }
        std::this_thread::sleep_for( std::chrono::seconds( 1 ));
      }
    }

    virtual ~Synchrophasotron()
    {
      if ( caches().size() == 0 )
      {
        endExecution() = true;
        removeingThread().join();
      }
    }

  protected:
    static std::thread& removeingThread()
    {
      static std::thread t;
      return t;
    }

    static std::list<Synchrophasotron*>& caches()
    {
      static std::list<Synchrophasotron*> t;
      return t;
    }

    static bool& endExecution()
    {
      static bool t = false;
      return t;
    }
  };

  template<typename _KeyType, typename _ValueType, int TTL = 20 /*seconds*/ >
  class ExpirationCache : public Synchrophasotron
  {
  public:
    typedef _KeyType KeyType;
    typedef _ValueType ValueType;

  public:
    ExpirationCache()
    {
      // add the new created object to objects list
      caches().push_back( this );
      // if previously the list was empty , start monitoring thread
      if ( caches().size() == 1 )
      {
        endExecution() = false;// loop monitoring until there is a element in caches()
        removeingThread() = std::thread( &Synchrophasotron::RemoveExpiredElements );
      }

    };

    ExpirationCache(const ExpirationCache&) = delete;
    ExpirationCache& operator=(const ExpirationCache&) = delete;

    auto end() const
    {
      std::shared_lock lock( mt_ );
      return map_.end();
    }

    auto find( const KeyType& ky ) const
    {
      std::shared_lock lock( mt_ );
      return map_.find( ky );
    }

    /// @brief get the value by @param key, if value does not exists throws an out_of_range exception
    ValueType get( const KeyType& ky ) const
    {
      std::shared_lock lock( mt_ );
      return map_.at( ky );
    }

    void put( const KeyType& ky, const ValueType& val )
    {
      std::unique_lock lock( mt_ );
      map_[ky] = val;
    }

    void put( const KeyType& ky, ValueType&& val )
    {
      std::unique_lock lock( mt_ );
      map_[ky] = std::move( val );
    }

    ~ExpirationCache()
    {
      auto ret = std::find( caches().begin(), caches().end(), this );
      if ( ret != caches().end())
      {
        ( *ret )->clearAll();
        caches().erase( ret );
      }

    }

    virtual int count() const override
    {
      std::shared_lock lock( mt_ );
      return map_.size();
    }

  private:
    virtual int getTTL() const override { return TTL; }

    virtual void clearAll() override
    {
      std::unique_lock lock( mt_ );
      map_.clear();
    }

  private:
    std::unordered_map<KeyType, ValueType> map_;
    mutable std::shared_mutex mt_;
  };
}
