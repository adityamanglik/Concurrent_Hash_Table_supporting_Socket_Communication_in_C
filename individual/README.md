
# Individual tests

To use the individual tests, run your server and run `runtest.sh [TEST]` where `[TEST]` refers to the test that should be run.
Note that you have to restart your server between tests to clear the KV store.

There are four individual tests, i.e., `[TEST]` can be `basic`, `longkey`, `longvalue` and `crazystring`.

Example
```
./runtest.sh basic
```

You may have to adapt the way netcat (`nc`) is invoked to adapt to your netcat version.