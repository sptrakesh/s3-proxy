//
// Created by Rakesh on 17/05/2020.
//

#include "poller.h"
#include "queuemanager.h"
#include "client/akumuli.h"
#include "client/mmdb.h"
#include "log/NanoLog.h"
#include "util/date.h"

#include <boost/algorithm/string/replace.hpp>
#include <sstream>

namespace spt::queue::tsdb
{
  struct TSDBClient
  {
    TSDBClient( boost::asio::io_context& ioc, model::Configuration* configuration ) :
        client{ ioc, configuration->akumuli, configuration->akumuliPort },
        configuration{ configuration }
    {
      client.connect();
    }

    ~TSDBClient()
    {
      client.stop();
    }

    void save( const model::Metric& metric )
    {
      //if ( ns > metric.time ) ns = std::chrono::nanoseconds{ ns.count() + 1 };
      //else ns = metric.time;
      ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch() );

      saveCount( metric );
      saveSize( metric );

      auto fields = client::MMDBClient::instance().query( metric.ipaddress );
      if ( fields.empty() )
      {
        LOG_WARN << "No properties for ip address " << metric.ipaddress;
        return;
      }

      saveLocation( metric, fields );
    }

  private:
    void saveCount( const model::Metric& metric )
    {
      std::ostringstream ss;
      ss << configuration->metricPrefix;
      ss << ".counter";

      auto tags = client::Tags{};
      tags.reserve( 5 );
      tags.emplace_back( "method", metric.method );
      tags.emplace_back( "resource", metric.resource );
      tags.emplace_back( "ipaddress", metric.ipaddress );
      tags.emplace_back( "status", std::to_string( metric.status ) );
      if ( !metric.mimeType.empty() ) tags.emplace_back( "mimeType", metric.mimeType );
      client.addSeries( client::IntegerSeries{ ss.str(), std::move( tags ), ns, 1 } );
    }

    void saveSize( const model::Metric& metric )
    {
      std::ostringstream ss;
      ss << configuration->metricPrefix;
      ss << ".size";

      auto tags = client::Tags{};
      tags.reserve( 5 );
      tags.emplace_back( "method", metric.method );
      tags.emplace_back( "resource", metric.resource );
      tags.emplace_back( "ipaddress", metric.ipaddress );
      tags.emplace_back( "status", std::to_string( metric.status ) );
      if ( !metric.mimeType.empty() ) tags.emplace_back( "mimeType", metric.mimeType );
      client.addSeries( client::IntegerSeries{ ss.str(), std::move( tags ),
          ns, int64_t( metric.size ) } );
    }

    void saveLocation( const model::Metric& metric, client::MMDBClient::Properties & fields )
    {
      const auto geojson = [this, &fields]()
      {
        const auto& lat = fields["latitude"];
        const auto& lng = fields["longitude"];
        auto mms = std::chrono::duration_cast<std::chrono::microseconds>( ns );

        std::ostringstream oss;
        oss << R"({"type": "geo:json", "value": {"type": "Point", "coordinates": [)" <<
            lat << ',' << lng <<
            R"(]}, "metadata": {"timestamp": {"type": "DateTime", "value": ")" <<
            util::isoDate( mms.count() ) <<
            R"("}}})";
        return oss.str();
      };

      const auto data = geojson();
      std::ostringstream ss;
      ss << configuration->metricPrefix;
      ss << ".location";

      auto tags = client::Tags{};
      tags.reserve( 10 );
      tags.emplace_back( "method", metric.method );
      tags.emplace_back( "resource", metric.resource );
      tags.emplace_back( "ipaddress", metric.ipaddress );
      tags.emplace_back( "status", std::to_string( metric.status ) );
      if ( !metric.mimeType.empty() ) tags.emplace_back( "mimeType", metric.mimeType );

      addTag( tags, fields, "city" );
      addTag( tags, fields, "continent" );
      addTag( tags, fields, "country" );
      addTag( tags, fields, "country_iso_code" );
      addTag( tags, fields, "subdivision" );

      client.addSeries( client::StringEvent{ ss.str(), std::move( tags ), ns, data } );
    }

    void addTag( client::Tags& tags,
        const client::MMDBClient::Properties& fields, std::string key )
    {
      const auto it = fields.find( key );
      if ( it == fields.end() ) return;
      tags.emplace_back( std::move( key ), boost::algorithm::replace_all_copy( it->second, " ", "\\ " ) );
    };

    client::Akumuli client;
    model::Configuration* configuration;
    std::chrono::nanoseconds ns;
  };
}

using spt::queue::Poller;

Poller::Poller( boost::asio::io_context& ioc, spt::model::Configuration::Ptr configuration ) :
    ioc{ ioc }, configuration{ std::move( configuration ) } {}

Poller::~Poller() = default;

void Poller::run()
{
  client = std::make_unique<tsdb::TSDBClient>( ioc, configuration.get() );
  running.store( true );
  LOG_INFO << "Metrics queue monitor starting";

  while ( running.load() )
  {
    try
    {
      loop();
    }
    catch ( const std::exception& ex )
    {
      LOG_WARN << "Error monitoring metrics queue\n" << ex.what();
    }
  }

  LOG_INFO << "Processed " << count << " total metrics from queue";
}

void spt::queue::Poller::stop()
{
  running.store( false );
  LOG_INFO << "Stop requested";
}

void Poller::loop()
{
  auto& queue = QueueManager::instance();
  while ( ! queue.empty() )
  {
    client->save( queue.front() );
    queue.pop();
    if ( ( ++count % 100 ) == 0 ) LOG_INFO << "Published " << count << " metrics to TSDB";
  }
  if ( running.load() ) std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
}

