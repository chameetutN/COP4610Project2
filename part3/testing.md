# How to Test Elevator

1. Build the module

```sh
cd part3
make
sudo insmod elevator.ko
```

2. Build and start tests

```sh
cd tests/elevator-test
make
./consumer --start
./producer 20
```

3. Watch /proc/elevator


```sh
watch -n1 cat /proc/elevator
```

4. Cleanup

```sh
./consumer --stop
sudo rmmod elevator
```
