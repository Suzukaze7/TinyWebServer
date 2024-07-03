`std::jthread` 会尝试两种调用

```cpp
std::invoke(decay-copy(std::forward<F>(f)), get_stop_token(), decay-copy(std::forward<Args>(args))...)
// 如果以上不能调用，则执行
std::invoke(decay-copy(std::forward<F>(f)), decay-copy(std::forward<Args>(args))...)
```

所以对于成员函数想要使用 `std::stop_token` 不能借助默认的传递方式