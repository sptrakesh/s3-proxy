# Integration Tests
Integration tests written as Lua scripts using [busted](http://olivinelabs.com/busted/)

## Pre-requisites
Install necessary Lua modules via `luarocks`
* `luarocks install busted --local`
* `luarocks install requests --local`

## Running tests
Start local **S3 Proxy** instance before running the tests.

```shell script
<path to>/s3proxy -c true -o /tmp/ -a abc123 \
  -r <AWS region> -b <bucket> \
  -k <AWS Key> -s <AWS Secret>
```

Run tests by pointing `busted` at the test directory root.

```shell script
~/.luarocks/bin/busted -v test/lua
```

## References
* [busted](http://olivinelabs.com/busted/)
* [requests](https://github.com/JakobGreen/lua-requests)