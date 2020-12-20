//
// Created by Rakesh on 11/08/2020.
//

#pragma once

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <thread>

namespace spt::client
{
  struct Configuration
  {
    Configuration() = default;

    Configuration(Configuration&&) = default;
    Configuration& operator=(Configuration&&) = default;

    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;

    uint32_t initialSize{ 1 };
    uint32_t maxPoolSize{ 25 };
    uint32_t maxConnections{ 100 };
    std::chrono::seconds maxIdleTime{ 300 };
  };

  template <typename Connection>
  struct Pool
  {
    using Ptr = std::unique_ptr<Connection>;
    using Factory = std::function<Ptr()>;

    struct ConnectionWrapper
    {
      explicit ConnectionWrapper( Ptr con ) : con{ std::move( con ) } {}

      ConnectionWrapper( ConnectionWrapper&& c ) = default;
      ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

      ConnectionWrapper(const ConnectionWrapper&) = delete;
      ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

      operator bool() const { return con.operator bool(); }

      Ptr con;
      std::chrono::time_point<std::chrono::system_clock> time = std::chrono::system_clock::now();
      uint32_t count;
    };

    struct Proxy
    {
      explicit Proxy( ConnectionWrapper c, Pool<Connection>* p ) :
        con{ std::move( c ) }, pool{ p } {}

      ~Proxy()
      {
        if ( con ) pool->release( std::move( con ) );
      }

      Proxy( Proxy&& p ) = default;
      Proxy& operator=( Proxy&& p ) = default;

      Proxy(const Proxy&) = delete;
      Proxy& operator=(const Proxy&) = delete;

      Connection& operator*() { return *con.con.get(); }
      Connection* operator->() { return con.con.get(); }

    private:
      ConnectionWrapper con;
      Pool<Connection>* pool;
    };

    explicit Pool( Factory c, Configuration conf = {} ) :
      creator{ c }, configuration{ std::move( conf ) }
    {
      for ( uint32_t i = 0; i < configuration.initialSize; ++i )
      {
        available.emplace_back( c() );
        ++created;
      }

      thread = std::thread{ &Pool<Connection>::ttlMonitor, this };
    }

    ~Pool()
    {
      stop.store( true );
      thread.join();
    }

    Pool( const Pool& ) = delete;
    Pool& operator=( const Pool& ) = delete;

    std::optional<Proxy> acquire()
    {
      if ( total >= configuration.maxConnections ) return std::nullopt;

      auto lock = std::unique_lock( mutex );
      ++total;

      if ( available.empty() )
      {
        ++created;
        return Proxy{ ConnectionWrapper{ creator() }, this };
      }

      auto con = std::move( available.front() );
      available.pop_front();
      ++con.count;
      return Proxy{ std::move( con ), this };
    }

    void release( ConnectionWrapper c )
    {
      auto lock = std::unique_lock( mutex );
      --total;
      if ( !c || !c.con->valid() ) return;
      c.time = std::chrono::system_clock::now();
      if ( available.size() < configuration.maxPoolSize ) available.emplace_back( std::move( c ) );
    }

    [[nodiscard]] uint32_t inactive() const
    {
      auto lock = std::shared_lock( mutex );
      return available.size();
    }

    [[nodiscard]] uint32_t active() const
    {
      auto lock = std::shared_lock( mutex );
      return total;
    }

    [[nodiscard]] uint32_t totalCreated() const
    {
      return created;
    }

  private:
    void removeExpired()
    {
      auto end = false;
      auto now = std::chrono::system_clock::now();

      while ( !end )
      {
        auto lock = std::unique_lock( mutex );
        auto iter = std::begin( available );
        if ( iter == std::end( available ) ) break;

        const auto diff = std::chrono::duration_cast<std::chrono::seconds>( now - iter->time );
        if ( diff < configuration.maxIdleTime ) end = true;
        else
        {
          available.erase( iter );
        }
      }
    }

    void ttlMonitor()
    {
      while ( !stop.load() )
      {
        removeExpired();
        std::this_thread::sleep_for( std::chrono::seconds{ 1 } );
      }
    }

    Factory creator;
    Configuration configuration;
    std::deque<ConnectionWrapper> available;
    std::thread thread;
    mutable std::shared_mutex mutex;
    std::atomic_bool stop{ false };
    uint32_t total = 0;
    uint32_t created = 0;
  };
}
