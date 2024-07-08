## jthread

`std::jthread` 会尝试两种调用

```cpp
std::invoke(decay-copy(std::forward<F>(f)), get_stop_token(), decay-copy(std::forward<Args>(args))...)
// 如果以上不能调用，则执行
std::invoke(decay-copy(std::forward<F>(f)), decay-copy(std::forward<Args>(args))...)
```

所以对于成员函数想要使用 `std::stop_token` 不能借助默认的传递方式

## tcpdump

实时抓包工具

## fd 阻塞方式

读写阻塞是 fd 的属性，而不是单次读写设置

普通文件都是非阻塞方式打开，设备文件/套接字都是阻塞，设置非阻塞需要

```c
open(..., O_NONBLOCK);

int old_option = fcntl(fd, F_GETFL);
int new_option = old_option | O_NONBLOCK;
fcntl(fd, F_SETFL, new_option);
```

并且只能设置非阻塞