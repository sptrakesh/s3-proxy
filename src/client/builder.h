//
// Created by Rakesh on 2019-05-26.
//

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace spt::client
{
  struct Builder
  {
    Builder() = default;
    ~Builder() = default;

    Builder(Builder&&) = delete;
    Builder& operator=(Builder&&) = delete;
    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    Builder& startRecord( std::string_view name );
    Builder& addTag( std::string_view key, std::string_view value );

    Builder& addValue( std::string_view key, bool value );
    Builder& addValue( std::string_view key, int32_t value );
    Builder& addValue( std::string_view key, uint32_t value );
    Builder& addValue( std::string_view key, int64_t value );
    Builder& addValue( std::string_view key, uint64_t value );
    Builder& addValue( std::string_view key, float value );
    Builder& addValue( std::string_view key, double value );
    Builder& addValue( std::string_view key, std::string_view value );
    Builder& timestamp( std::chrono::nanoseconds value );
    Builder& endRecord();

    [[nodiscard]] std::string finish();

  private:
    struct Record
    {
      explicit Record( std::string_view name ) : name{ name } {}

      ~Record() = default;
      Record(Record&&) = default;
      Record& operator=(Record&&) = default;

      Record(const Record&) = delete;
      Record& operator=(const Record&) = delete;

      std::string name;
      std::string tags;
      std::string value;
      std::chrono::nanoseconds timestamp{ std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::system_clock::now().time_since_epoch() ) };
    };

    std::string value;
    std::optional<Record> record{ std::nullopt };
  };
}

