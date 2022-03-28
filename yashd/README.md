# How to use Yashd

## Make the yashd daemon

```sh
make
``` 

## Execute the yashd daemon

```sh
bash start_yashd.sh
```

## Make the yash client

```sh
make yash
```

## Execute the yash client

```sh
bash start_yash.sh
# send
# some
# commands
# ^d (to exit yash)
```


## View resulting logs from daemon

```sh
cat /tmp/yashd.log
```

## Kill yashd daemon

```sh
kill -9 `cat /tmp/yashd.pid`
```

## Clean up

```sh
make clean
```