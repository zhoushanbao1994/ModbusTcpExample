# ModbusTcpExample
基于libmodbus的ModbusTcp例子



## 文件说明

- Lib目录下是例程编译运行所需要的库文件，需要拷贝至开发编译环境中
- Prj目录下是例程的代码



## 编译说明

1. 将整个文件夹拷贝到开发编译环境内。

2. 拷贝“Lib”目录下的库文件“libmodbus.so.5.0.5”到开发编译环境的“/opt/am4372/lib/lib/”路径下（文件夹不存在则手动创建）

3. 为“libmodbus.so.5.0.5”创建软连接

   ```sheell
   ln -s libmodbus.so.5.0.5 libmodbus.so.5
   ln -s libmodbus.so.5.0.5 libmodbus.so
   ```

4. 进入“Prj”目录下，使用“Make”命令编译工程

   ```shell
   root@ubuntu# make
   /opt/am4372/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-g++   -pthread -c -o main.o main.cpp
   /opt/am4372/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-g++ main.o -pthread -lmodbus -o test_bin -L /opt/am4372/lib/lib/
   ```

5. 生成“test_bin”可执行文件

6. 将可执行文件传输至CCU控制器，运行文件。
