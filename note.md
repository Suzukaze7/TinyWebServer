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

## wsl 配置git代理

开启[镜像模式](https://learn.microsoft.com/zh-cn/windows/wsl/networking#mirrored-mode-networking)

## 手动编译 gcc 源码

1. 源码下载[地址](https://ftp.gnu.org/gnu/gcc/)
2. 解压缩

    ```sh
    tar -zxf xxx
    ```

3. 以下称源文件目录为 `src`，build 目录为 `build`，安装目录为 `bin`，官方推荐三个文件都没有嵌套
4. 按步骤编译

    ```sh
    # 下载需要的库
    cd src
    ./contrib/download_prerequisites
    
    # 配置编译选项
    cd dst
    src/configure --prefix=bin --enable-threads=posix --disable-checking --enable--long-long --with-system-zlib --enable-languages=c,c++ --disable-multilib

    # 编译
    make -j$nproc

    # 安装
    sudo make install
    ```

    tips:
    - 一定要先下载需要的库再配置，否则必须删除 `build` 里的文件再重新操作
    - 编译时出现 `g++: fatal error: Killed signal terminated program cc1plus`，是因为内存和 swap 分区空间不够

        wsl 设置内存和 swap 分区大小，在 `.wslconfig` 中

        ```
        [wsl2]
        memory=xxx
        swap=xxx
        ```

5. Linux 使用手动编译的高版本 `gcc` 编译程序运行后报错 

    ```
    /lib/x86_64-linux-gnu/libstdc++.so.6: version `GLIBCXX_3.4.32' not found
    ```

    是因为 `gcc` 默认会去 `/lib, /usr/lib` 下找动态链接库，而系统又缺少高版本 `gcc` 所需的库

    解决办法，`gcc` 会优先去 `LD_LIBRARY_PATH` 环境变量存的路径去找库，所以在 `.bashrc` 中加上 `export LD_LIBRARY_PATH=xxx` 即可

## HTTP 请求报文

#### 请求头

- `Content-Length`： 指明请求体大小
- `Transfer-Encoding`： 指明将请求体传递的编码形式

#### 判断报文结束：

- 服务器端：`Content-Length` 指明，或是 `Transfer-Encoding: chunked` 指明按块传输
- 浏览器端：除了服务器端的方式，还有直接断开 `tcp` 连接

## tips

- `-g` 在编译时使用，链接时不需要
- `write` 不会在缓冲区末尾补 `\0`

## 记录一次因为 ub 引起的 bug

在加上 keep alive 功能后，浏览器端关闭连接后，服务器会无限触发读事件，中间考虑过可能会是 `EPOLLRDHUP` 的事件没考虑到。实际上是浏览器端会采用***优雅关闭***方式关闭连接，服务器端体现就是一直会引起 `EPOLLIN` 事件，但 `read(fd)` 会返回 `0`。于是在读的时候加上了判断。

但随后运行就出现了各种段错误，例如 `free(): double free detected in tcache 2`。各种调试过后，发现了如下的情况：

```cpp
struct Test {
    std::string s;
};

int main() {
    Test t = {"123456"};
    std::cout << t.s << " " << t.s.size() << "\n";
    t.~Test();
    std::cout << t.s << " " << t.s.size() << "\n";

    std::vector<int> v;
    v.pop_back();
}

/*
stdout:
123456 6
123456 6
*/
```

经过请教之后，发现自己对**显式调用析构**存在误解。之前以为的是，析构函数单纯执行对象所获取资源的释放，但并不释放对象本身的空间，类似于容器的 `clear()`。而实际上析构对应着一个变量生命周期的结束，所以对于一些无所谓的操作不会处理(例如指针置空)，并且为了防止其他原因导致多次析构，还需要手动构造。

## const std::string & 与 std::string_view

[在什么时候使用 string_view 代替 string](https://zhuanlan.zhihu.com/p/655420022)

## 分文件逻辑

- #pragma once 与 inline
- 默认实参
- 限定符 const, noexcept ...

## std::exception_ptr

`std::exception_ptr` 是一个只能管理 `std::current_exception()` 捕获的异常类的共享指针，但是不能通过指针获取到异常类

## 重载函数导致函数模板推导失败

## 

## 难点

- `logger` 与 `router` 在 `HttpConn` 类中需要使用，又不想设计成单例，于是定义成 `HttpConn` 类的静态成员变量，又由于 `WebServer` 类中也要使用，于是将 `WebServer` 作为 `HttpConn` 的友元
- `parse_request` 返回值应该用 `StatusCode` 还是独自定义状态，用 `StatusCode` 会导致枚举有不属于正常状态码该有的状态，独自定义在 `process` 中又会导致状态对应很难看，所以决定返回 `pair`

## todo

- [ ] 请求报文头解析采用字典树