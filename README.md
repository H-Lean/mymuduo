## 运行环境

-   Ubuntu18.04
-   gcc 7.5.0
-   cmake 3.10.2



## 执行

### 项目编译

执行`autobuild.sh`

### 项目测试

#### 服务端

```bash
cd example
make clean
make
./testserver
```

#### 客户端

```c++
telnet 127.0.0.1 8000
```



## 功能

简单的回响服务器，主要实现muduo网络库



## 文件描述

CMakeLists.txt是cmake脚本，生成**lib**文件夹包含.so动态库

autobuild.sh自动编译，生成**build**文件夹，调用make



## 技术描述