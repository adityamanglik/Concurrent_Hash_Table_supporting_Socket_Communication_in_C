nc -w 1 localhost 5555 < 0.client | tee 0.outcheck &
nc -w 1 localhost 5555 < 1.client | tee 1.outcheck &
nc -w 1 localhost 5555 < 2.client | tee 2.outcheck &
nc -w 1 localhost 5555 < 3.client | tee 3.outcheck &
nc -w 1 localhost 5555 < 4.client | tee 4.outcheck &