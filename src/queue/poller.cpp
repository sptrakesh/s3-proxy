//
// Created by Rakesh on 17/05/2020.
//

#include "poller.hpp"
#include "queuemanager.hpp"
#include "contextholder.hpp"
#include "client/builder.hpp"
#include "client/ilp.hpp"
#include "client/mmdb.hpp"
#include "log/NanoLog.hpp"
#include "model/config.hpp"
#include "util/date.hpp"

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
        client{ ioc, configuration.ilpHost, std::to_string( configuration.ilpPort ) },
        seriesName{ configuration.ilpSeries } {}

    void save( const model::Metric& metric, const client::MMDBConnection::Properties& fields )
    {
      ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch() );

      auto builder = client::Builder{};
      builder.addTag( "method", metric.method ).
        addTag( "resource", resource( metric ) ).
        addTag( "ipaddress", metric.ipaddress );

      if ( !metric.mimeType.empty() ) builder.addTag( "mimeType", metric.mimeType );

      auto nanos = std::chrono::nanoseconds{ metric.time };
      auto millis = std::chrono::duration_cast<std::chrono::milliseconds>( nanos );
      builder.addValue( "size", static_cast<uint64_t>( metric.size ) ).
        addValue( "time", millis.count() ).
        addValue( "status", metric.status );

      if ( !fields.empty() ) saveLocation( fields, builder );
    }

  private:
    void saveLocation( const client::MMDBConnection::Properties& fields, client::Builder& builder )
    {
      if ( fields.empty() ) return;

      auto it = fields.find( "latitude" );
      if ( it == fields.end() ) return;
      const auto lat = it->second;

      it = fields.find( "longitude" );
      if ( it == fields.end() ) return;
      const auto lng = it->second;

      builder.addValue( "latitude", lat );
      builder.addValue( "longitude", lng );

      addTag( builder, fields, "city" );
      addTag( builder, fields, "continent" );
      addTag( builder, fields, "country" );
      addTag( builder, fields, "country_iso_code" );
      addTag( builder, fields, "subdivision" );
    }

    std::string resource( const model::Metric& metric )
    {
      const auto idx = metric.resource.find_first_of( '?' );
      return idx == std::string::npos ? metric.resource : metric.resource.substr( 0, idx );
    }

    void addTag( client::Builder& builder,
        const client::MMDBConnection::Properties& fields, const std::string& key )
    {
      const auto it = fields.find( key );
      if ( it == fields.end() ) return;
      auto v = boost::algorithm::replace_all_copy( it->second, " ", "_" );
      LOG_DEBUG << "Adding tag key: " << key << ", value: " << v;
      builder.addTag( key, std::move( v ) );
    };

    client::ILPClient client;
    std::string seriesName;
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
  if ( !configuration.ilpHost.empty() )
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

void Poller::stop()
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

    if ( auto proxy = client::MMDBConnectionPool::instance().acquire(); proxy )
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

