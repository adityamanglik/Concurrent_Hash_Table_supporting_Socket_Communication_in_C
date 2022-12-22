
# Multiclient test

To use the multi-client test, run your server and run `runtest.sh`.
The test spawns 5 clients that access different keys in parallel.
The keys between the clients are unique, so that no race conditions can occur resulting in different outputs.
Note that you have to restart your server between tests to clear the KV store.

Example
```
./runtest.sh
```

You may have to adapt the way netcat (`nc`) is invoked to adapt to your netcat version.