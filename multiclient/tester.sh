nc -w 1 localhost 5555 < 0.client | diff - 0.client.out
nc -w 1 localhost 5555 < 1.client | diff - 1.client.out
nc -w 1 localhost 5555 < 2.client | diff - 2.client.out
nc -w 1 localhost 5555 < 3.client | diff - 3.client.out
nc -w 1 localhost 5555 < 4.client | diff - 4.client.out