## 运行环境

-   Ubuntu18.04
-   gcc 7.5.0
-   cmake 3.10.2



## 执行

### 项目编译

执行`autobuild.sh`编译muduo网络库

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



## 技术描述

-   cmake构建将muduo头文件和库文件加载到系统环境变量，支持用户自定义测试文件
-   









