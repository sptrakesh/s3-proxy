//
// Created by Rakesh on 23/10/2019.
//

#include "akumuli.h"

using spt::client::Akumuli;

Akumuli::Akumuli( boost::asio::io_context& context, const std::string& host, int port ) :
  socket{ context }, reconnectTimer{ context }, deadline{ context }
{
  boost::asio::ip::tcp::resolver resolver{ context };
  endpoints = resolver.resolve( host, std::to_string( port ));
}

void Akumuli::connect()
{
  startConnection( endpoints.begin());
  deadline.async_wait( std::bind( &Akumuli::checkDeadline, this ));
}

void Akumuli::stop()
{
  stopped.store( true );
  boost::system::error_code ignored;
  socket.close( ignored );
  reconnectTimer.cancel();
}

void Akumuli::doWrite( const std::string& data )
{
  if ( !connected )
  {
    connect();

    // TODO: use synchronous connection to avoid this nonsense.
    while ( !stopped.load() && !connected )
    {
      std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }
  }

  boost::system::error_code error;
  boost::asio::write( socket, boost::asio::buffer( data ), error );
  if ( error )
  {
    LOG_WARN << "Error sending data to akumuli " << error.message();
    doStop();
  }
}

void Akumuli::startConnection( boost::asio::ip::tcp::resolver::results_type::iterator iter )
{
  if ( iter == endpoints.end()) return stop();

  reconnectTimer.expires_after( std::chrono::seconds( 60 ));
  socket.async_connect( iter->endpoint(),
      std::bind( &Akumuli::handleConnect, this, std::placeholders::_1,
          iter ));
}

void Akumuli::restartConnection( const boost::system::error_code& error )
{
  if ( stopped.load()) return;

  if ( error )
  {
    LOG_WARN << error.message();
    return;
  }

  startConnection( endpoints.begin());
}

void Akumuli::handleConnect( const boost::system::error_code& error,
    boost::asio::ip::tcp::resolver::results_type::iterator iter )
{
  if ( stopped.load()) return;

  if ( !socket.is_open())
  {
    LOG_WARN << "Connect timed out";

    // Try the next available endpoint.
    startConnection( ++iter );
  }
  else if ( error )
  {
    LOG_WARN << "Connect error: " << error.message();

    try
    {
      socket.close();
    }
    catch ( const std::exception& ex )
    {
      LOG_WARN << ex.what();
    }

    startConnection( ++iter );
  }
  else
  {
    std::ostringstream oss;
    oss << iter->endpoint();
    if ( initial ) LOG_INFO << "Connected to " << oss.str();
    initial = false;
    startRead();
    connected = true;
  }
}

void Akumuli::doStop()
{
  boost::system::error_code ignored;
  if ( socket.is_open() )
  {
    socket.shutdown( boost::asio::socket_base::shutdown_both, ignored );
    if ( !ignored ) socket.close( ignored );
    if ( ignored ) LOG_WARN << ignored.message();
  }
  connected = false;
  reconnectTimer.expires_from_now( std::chrono::seconds{ 2 } );
  reconnectTimer.async_wait(
      std::bind( &Akumuli::restartConnection, this,
          std::placeholders::_1 ));
}

void Akumuli::startRead()
{
  deadline.expires_after( std::chrono::seconds( 30 ));

  boost::asio::async_read_until( socket,
      boost::asio::dynamic_buffer( inputBuffer ), '\n',
      std::bind( &Akumuli::handleRead, this, std::placeholders::_1,
          std::placeholders::_2 ));
}

void Akumuli::handleRead( const boost::system::error_code& error, std::size_t n )
{
  if ( stopped.load()) return;
  if ( !error )
  {
    std::string line( inputBuffer.substr( 0, n - 1 ));
    inputBuffer.erase( 0, n );
    if ( !line.empty())
    {
      LOG_WARN << "Akumuli write error\n" << line;
      doStop();
    }
  }
}

void Akumuli::checkDeadline()
{
  if ( stopped.load()) return;

  if ( deadline.expiry() <= boost::asio::steady_timer::clock_type::now() )
  {
    deadline.expires_after( std::chrono::seconds( 30 ));
  }

  deadline.async_wait( std::bind( &Akumuli::checkDeadline, this ) );
}
