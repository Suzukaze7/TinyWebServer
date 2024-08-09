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

## 链接

Linux 使用手动编译的高版本 `gcc` 编译程序运行后报错 

```
/lib/x86_64-linux-gnu/libstdc++.so.6: version `GLIBCXX_3.4.32' not found
/lib/x86_64-linux-gnu/libstdc++.so.6: version `GLIBCXX_3.4.31' not found
```

#### 编译选项

- `-l`: 指定想要链接的库名，链接库都命名为 `libxxx.yy`，指定时只能指定 `xxx`，如果有多个有一个特殊的规则
- `-L`: 指定额外去找库的目录路径
- `-Wl,rpath`: 指定程序运行时额外去找库的目录路径

#### 库搜索路径

链接器在搜索库文件时通常会遵循以下顺序：

1. 命令行指定的目录（使用 `-L` 选项）
2. 环境变量 `LD_LIBRARY_PATH` 指定的目录
3. 链接器默认的系统库目录（例如 `/lib`、`/usr/lib`、`/usr/local/lib` 等）
4. 动态链接器配置文件中的路径（例如 `/etc/ld.so.conf` 和 `/etc/ld.so.conf.d/` 中的配置文件）

当程序运行时，动态链接器会按以下顺序查找共享库：

1. 使用 `rpath` 编译选项指定的目录
2. 使用 `LD_LIBRARY_PATH` 环境变量指定的目录
3. 使用 `ld.so.cache` 文件中列出的目录
4. 默认系统库目录

#### 默认链接库

gcc 会默认链接一些库

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

## benchmark

#### 编译 webbench

- 报错
  
    ```
    cc -Wall -ggdb -W -O   -c -o webbench.o webbench.c
    webbench.c:21:10: fatal error: rpc/types.h: No such file or directory
    21 | #include <rpc/types.h>
       |          ^~~~~~~~~~~~~
    compilation terminated.
    make: *** [<builtin>: webbench.o] Error 1
    ```

  打开文件，将 `rpc/types.h` 改为 `sys/types.h`

- 报错

    ```
    cc -Wall -ggdb -W -O  -o webbench webbench.o  
    ctags *.c
    /bin/sh: 1: ctags: not found
    make: [Makefile:12: tags] Error 127 (ignored)
    ```

    下载 `sudo apt install universal-ctags`

#### 优化过程

测试 url 为 /，返回 `judge.html` 文件

```
Document Path:          /
Document Length:        586 bytes
```

#### 初始(1st)

```
Concurrency Level:      1000
Time taken for tests:   6.080 seconds
Complete requests:      999
Failed requests:        0
Total transferred:      685685 bytes
HTML transferred:       586586 bytes
Requests per second:    164.32 [#/sec] (mean)
Time per request:       6085.806 [ms] (mean)
Time per request:       6.086 [ms] (mean, across all concurrent requests)
Transfer rate:          110.14 [Kbytes/sec] received

Connection Times (ms)
            min  mean[+/-sd] median   max
Connect:        0    3   5.2      0      18
Processing:  2945 3055  59.1   3076    3141
Waiting:        0 1461 857.4   1450    2937
Total:       2947 3058  59.0   3078    3141

Percentage of the requests served within a certain time (ms)
50%   3078
66%   3099
75%   3108
80%   3110
90%   3123
95%   3126
98%   3136
99%   3139
100%   3141 (longest request)
```

连接方式改为保持连接后

```
Concurrency Level:      1000
Time taken for tests:   4.390 seconds
Complete requests:      50000
Failed requests:        0
Keep-Alive requests:    50000
Total transferred:      34500000 bytes
HTML transferred:       29300000 bytes
Requests per second:    11388.66 [#/sec] (mean)
Time per request:       87.807 [ms] (mean)
Time per request:       0.088 [ms] (mean, across all concurrent requests)
Transfer rate:          7674.00 [Kbytes/sec] received

Connection Times (ms)
            min  mean[+/-sd] median   max
Connect:        0    0   0.6      0      19
Processing:    10   59 225.8     31    2826
Waiting:        5   59 225.5     31    2822
Total:         10   59 226.1     31    2828

Percentage of the requests served within a certain time (ms)
50%     31
66%     32
75%     33
80%     35
90%     43
95%     47
98%     69
99%   1436
100%   2828 (longest request)
```

继续加压

```
Concurrency Level:      2000
Time taken for tests:   25.696 seconds
Complete requests:      500000
Failed requests:        0
Keep-Alive requests:    500000
Total transferred:      345000000 bytes
HTML transferred:       293000000 bytes
Requests per second:    19458.59 [#/sec] (mean)
Time per request:       102.782 [ms] (mean)
Time per request:       0.051 [ms] (mean, across all concurrent requests)
Transfer rate:          13111.74 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   1.6      0    1056
Processing:     0   83 305.3     31    7848
Waiting:        0   83 305.1     31    7842
Total:          0   83 305.5     31    7853

Percentage of the requests served within a certain time (ms)
  50%     31
  66%     47
  75%     47
  80%     47
  90%     78
  95%    297
  98%    532
  99%    614
 100%   7853 (longest request)
```

```
Concurrency Level:      3000
Time taken for tests:   33.452 seconds
Complete requests:      500000
Failed requests:        0
Keep-Alive requests:    500000
Total transferred:      345000000 bytes
HTML transferred:       293000000 bytes
Requests per second:    14946.86 [#/sec] (mean)
Time per request:       200.711 [ms] (mean)
Time per request:       0.067 [ms] (mean, across all concurrent requests)
Transfer rate:          10071.62 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.5      0      24
Processing:     6  140 479.3     47   16306
Waiting:        0  140 479.1     47   16306
Total:          6  140 479.4     47   16306

Percentage of the requests served within a certain time (ms)
  50%     47
  66%     53
  75%     63
  80%     68
  90%    307
  95%    501
  98%    640
  99%   1103
 100%  16306 (longest request)
```

#### 加上内存池后

```
Concurrency Level:      2000
Time taken for tests:   23.781 seconds
Complete requests:      500000
Failed requests:        0
Keep-Alive requests:    500000
Total transferred:      345000000 bytes
HTML transferred:       293000000 bytes
Requests per second:    21024.87 [#/sec] (mean)
Time per request:       95.125 [ms] (mean)
Time per request:       0.048 [ms] (mean, across all concurrent requests)
Transfer rate:          14167.15 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.4      0      31
Processing:    12   82 244.9     47    6072
Waiting:        0   82 244.7     47    6057
Total:         12   82 245.0     47    6072

Percentage of the requests served within a certain time (ms)
  50%     47
  66%     47
  75%     47
  80%     48
  90%     78
  95%    297
  98%    491
  99%    611
 100%   6072 (longest request)
```

## 正则表达式

语法：[C++正则表达式全攻略：从基础到高级应用](https://blog.csdn.net/Long_xu/article/details/135306358)

C++ [正则表达式库](https://zh.cppreference.com/w/cpp/regex)

对于 `std::regex_search` 的匹配逻辑可以理解成匹配最左侧合法的子串

## 信号

1. 如果是异常产生的信号（比如程序错误，像SIGPIPE、SIGEGV这些），则只有产生异常的线程收到并处理。
2. 如果是用pthread_kill产生的内部信号，则只有pthread_kill参数中指定的目标线程收到并处理。
3. 如果是外部使用kill命令产生的信号，通常是SIGINT、SIGHUP等job control信号，则会遍历所有线程，直到找到一个不阻塞该信号的线程，然后调用它来处理。(一般从主线程找起)，注意只有一个线程能收到。
4. 其次，每个线程都有自己独立的signal mask，但所有线程共享进程的signal action。这意味着，你可以在线程中调用pthread_sigmask(不是sigmask)来决定本线程阻塞哪些信号。但你不能调用sigaction来指定单个线程的信号处理方式。如果在某个线程中调用了sigaction处理某个信号，那么这个进程中的未阻塞这个信号的所有线程在收到这个信号都会按同一种方式处理这个信号。另外，注意子线程的mask是会从主线程继承而来的。

[菜鸟教程 sigaction()](https://www.runoob.com/cprogramming/c-function-sigaction.html)

## mysql

mysql库：`sudo apt install libmysqlclient-dev`

## 异常

#### 异常保证

- 不抛出异常保证（Nothrow exception guarantee）：函数决不抛出异常。
- 强异常保证（Strong exception guarantee）：若函数抛出异常，则程序的状态被回滚到正好在函数调用前的状态。
- 基础异常保证（Basic exception guarantee）：若函数抛出异常，则程序在合法状态。它可能需要清理，但所有不变量都原封不动。
- 无异常保证（No exception guarantee）：若函数抛出异常，则程序可能不在合法状态：可能已经发生了资源泄漏、内存谬误，或其他摧毁不变量的错误。

#### ThreadPool 的异常安全

构造函数抛出异常时，所有已经构造好的成员会调用析构，而自身不会调用析构，所以 `ThreadPool` 在构造成员时是无异常的，而在构造函数体里创建线程时，如果发生异常，需要将 `stop_` 停止标志置 1，并唤醒所有线程，又由于 成员变量 `threads_` 已经构造成功，会调用析构，所以能调用 `std::jthread` 的析构 join 线程。这样就能保证强异常安全

## 内存管理

#### 内存对齐

Q: 一般什么情况下会出现内存对齐超过8
A: 原子(比如 `std::atomic`)，其他的一些线程安全的东西里也可能存在
A: 手动指定的时候(例如：`#pragma pack (n)`)。另外有些 64 位平台上 long double 的对齐是 16

构造函数初始化列表中可以取未初始化成员地址

#### operator new/delete

- 类重载默认 `static`
- `delete` 指向子类的基类指针，没有虚析构是 ub，反之 `delete` 可以找到正确的 `operator delete`

## 难点

- `logger` 与 `router` 在 `HttpConn` 类中需要使用，又不想设计成单例，于是定义成 `HttpConn` 类的静态成员变量，又由于 `WebServer` 类中也要使用，于是将 `WebServer` 作为 `HttpConn` 的友元。(补充)这样设计不好，使得在使用上有限制，整个程序只能有一个 `WebServer` 类，但是给 `HttpConn` 都加个成员变量，开销感觉太大了？
- `parse_request` 返回值应该用 `StatusCode` 还是独自定义状态，用 `StatusCode` 会导致枚举有不属于正常状态码该有的状态，独自定义在 `process` 中又会导致状态对应很难看，所以决定返回 `pair`

## todo

- [ ] 请求报文头解析采用字典树