//
// Created by Rakesh on 17/05/2020.
//

#include "poller.h"
#include "queuemanager.h"
#include "contextholder.h"
#include "client/akumuli.h"
#include "client/mmdb.h"
#include "log/NanoLog.h"
#include "model/config.h"
#include "util/date.h"

#include <boost/algorithm/string/replace.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>

#include <chrono>
#include <sstream>

namespace spt::queue::tsdb
{
  struct MongoClient
  {
    MongoClient( const model::Configuration& configuration ) :
      database{ configuration.mongoDatabase }, collection{ configuration.mongoCollection },
      client{ std::make_unique<mongocxx::client>( mongocxx::uri{ configuration.mongoUri } ) }
    {
      LOG_INFO << "Connected to mongo";

      mongocxx::write_concern wc;
      wc.acknowledge_level( mongocxx::write_concern::level::k_unacknowledged );
      opts.write_concern( std::move( wc ) );
    }

    void save( const model::Metric& metric, client::MMDBConnection::Properties& fields )
    {
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using bsoncxx::builder::stream::finalize;

      try
      {
        auto us = std::chrono::duration_cast<std::chrono::milliseconds>( metric.timestamp.time_since_epoch() );
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>( metric.timestamp.time_since_epoch() ).count();

        auto doc = document{};
        doc << "method" << metric.method <<
          "resource" << metric.resource <<
          "mimeType" << metric.mimeType <<
          "ipaddress" << metric.ipaddress <<
          "size" << static_cast<int64_t>( metric.size ) <<
          "time" << metric.time <<
          "status" << metric.status <<
          "compressed" << metric.compressed <<
          "timestamp" << ns <<
          "created" << bsoncxx::types::b_date{ us };

        if ( !fields.empty() )
        {
          doc << "location" << open_document;
          for ( auto& [key, value] : fields ) doc << key << value;
          doc << close_document;
        }

        (*client)[database][collection].insert_one( doc << finalize, opts );
      }
      catch ( const std::exception& ex )
      {
        LOG_CRIT << "Error saving metric.\n" << metric.str() << '\n' << ex.what();
      }
    }

  private:
    mongocxx::options::insert opts;
    std::string database;
    std::string collection;
    std::unique_ptr<mongocxx::client> client = nullptr;
  };

  struct TSDBClient
  {
    TSDBClient( boost::asio::io_context& ioc, const model::Configuration& configuration ) :
        client{ ioc, configuration.akumuli, configuration.akumuliPort },
        metricPrefix{ configuration.metricPrefix }
    {
      client.connect();
    }

    ~TSDBClient()
    {
      client.stop();
    }

    void save( const model::Metric& metric, const client::MMDBConnection::Properties& fields )
    {
      ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch() );

      saveCount( metric );
      saveSize( metric );
      saveTime( metric );
      if ( !fields.empty() ) saveLocation( metric, fields );
    }

  private:
    void saveCount( const model::Metric& metric )
    {
      std::ostringstream ss;
      ss << metricPrefix;
      ss << ".counter";

      auto tags = client::Tags{};
      tags.reserve( 5 );
      tags.emplace_back( "method", metric.method );
      tags.emplace_back( "resource", resource( metric ) );
      tags.emplace_back( "ipaddress", metric.ipaddress );
      tags.emplace_back( "status", std::to_string( metric.status ) );
      if ( !metric.mimeType.empty() ) tags.emplace_back( "mimeType", metric.mimeType );
      client.addSeries( client::IntegerSeries{ ss.str(), std::move( tags ), ns, 1 } );
    }

    void saveSize( const model::Metric& metric )
    {
      std::ostringstream ss;
      ss << metricPrefix;
      ss << ".size";

      auto tags = client::Tags{};
      tags.reserve( 5 );
      tags.emplace_back( "method", metric.method );
      tags.emplace_back( "resource", resource( metric ) );
      tags.emplace_back( "ipaddress", metric.ipaddress );
      tags.emplace_back( "status", std::to_string( metric.status ) );
      if ( !metric.mimeType.empty() ) tags.emplace_back( "mimeType", metric.mimeType );
      client.addSeries( client::IntegerSeries{ ss.str(), std::move( tags ),
          ns, int64_t( metric.size ) } );
    }

    void saveTime( const model::Metric& metric )
    {
      std::ostringstream ss;
      ss << metricPrefix;
      ss << ".time";

      auto nanos = std::chrono::nanoseconds{ metric.time };
      auto millis = std::chrono::duration_cast<std::chrono::milliseconds>( nanos );

      auto tags = client::Tags{};
      tags.reserve( 5 );
      tags.emplace_back( "method", metric.method );
      tags.emplace_back( "resource", resource( metric ) );
      tags.emplace_back( "ipaddress", metric.ipaddress );
      tags.emplace_back( "status", std::to_string( metric.status ) );
      if ( !metric.mimeType.empty() ) tags.emplace_back( "mimeType", metric.mimeType );
      client.addSeries( client::IntegerSeries{ ss.str(), std::move( tags ),
          ns, millis.count() } );
    }

    void saveLocation( const model::Metric& metric, const client::MMDBConnection::Properties& fields )
    {
      if ( fields.empty() ) return;

      const auto geojson = [this, &fields]() -> std::string
      {
        auto it = fields.find( "latitude" );
        if ( it == fields.end() ) return {};
        const auto lat = it->second;

        it = fields.find( "longitude" );
        if ( it == fields.end() ) return {};
        const auto lng = it->second;

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
      if ( data.empty() )
      {
        LOG_WARN << "No location data in metric.\n" << metric.str();
        return;
      }
      std::ostringstream ss;
      ss << metricPrefix;
      ss << ".location";

      auto tags = client::Tags{};
      tags.reserve( 10 );
      tags.emplace_back( "method", metric.method );
      tags.emplace_back( "resource", resource( metric ) );
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

    std::string resource( const model::Metric& metric )
    {
      const auto idx = metric.resource.find_first_of( '?' );
      return idx == std::string::npos ? metric.resource : metric.resource.substr( 0, idx );
    }

    void addTag( client::Tags& tags,
        const client::MMDBConnection::Properties& fields, std::string key )
    {
      const auto it = fields.find( key );
      if ( it == fields.end() ) return;
      auto v = boost::algorithm::replace_all_copy( it->second, " ", "__#SPACE#__" );
      LOG_DEBUG << "Adding tag key: " << key << ", value: " << v;
      tags.emplace_back( std::move( key ), std::move( v ) );
    };

    client::Akumuli client;
    std::string metricPrefix;
    std::chrono::nanoseconds ns;
  };
}

using spt::queue::Poller;

Poller::Poller() = default;
Poller::~Poller() = default;

void Poller::run()
{
  const auto& configuration = model::Configuration::instance();
  if ( !configuration.mongoUri.empty() )
  {
    mongo = std::make_unique<tsdb::MongoClient>( configuration );
  }
  if ( !configuration.akumuli.empty() )
  {
    tsdb = std::make_unique<tsdb::TSDBClient>( ContextHolder::instance().ioc, configuration );
  }

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
  auto metric = model::Metric{};
  while ( queue.consume( metric ) )
  {
    client::MMDBConnection::Properties fields;
    auto proxy = client::MMDBConnectionPool::instance().acquire();
    if ( proxy )
    {
      fields = (*proxy)->fields( metric.ipaddress );
    }
    else LOG_WARN << "Unable to connect to mmdb-ws service";

    if ( fields.empty() )
    {
      LOG_WARN << "No properties for ip address " << metric.ipaddress;
    }
    if ( mongo ) mongo->save( metric, fields );
    if ( tsdb ) tsdb->save( metric, fields );
    if ( ( ++count % 100 ) == 0 ) LOG_INFO << "Published " << count << " metrics to database(s)";
  }
  if ( running.load() ) std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
}

