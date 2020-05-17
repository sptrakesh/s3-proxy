//
// Created by Rakesh on 2019-05-24.
//
// https://github.com/bjlaub/simpletsdb-client

#pragma once

#include <boost/asio.hpp>

#include "timeseries.h"
#include "log/NanoLog.h"

namespace wirepulse::akumuli
{
  struct Akumuli
  {
    Akumuli( boost::asio::io_context& context, const std::string& host, int port );

    void connect();

    template<typename Value, char ValueType>
    void addSeries( const TimeSeries<Value, ValueType>& ts )
    {
      doWrite( ts.resp());
    }

    void stop();

    Akumuli( const Akumuli& ) = delete;
    Akumuli& operator=( const Akumuli& ) = delete;
    ~Akumuli() = default;

  private:
    void doWrite( const std::string& data );
    void startConnection( boost::asio::ip::tcp::resolver::results_type::iterator iter );
    void restartConnection( const boost::system::error_code& error );
    void handleConnect( const boost::system::error_code& error,
        boost::asio::ip::tcp::resolver::results_type::iterator iter );
    void doStop();
    void startRead();
    void handleRead( const boost::system::error_code& error, std::size_t n );
    void checkDeadline();

  private:
    boost::asio::ip::tcp::resolver::results_type endpoints;
    boost::asio::ip::tcp::socket socket;
    boost::asio::steady_timer reconnectTimer;
    boost::asio::steady_timer deadline;
    std::string inputBuffer;;
    std::atomic_bool stopped{ false };
    bool connected = false;
    bool initial = true;
  };
}
