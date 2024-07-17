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