
/*
* Copyright (c), 2018-2022
*
* File Name:    main.cpp
* Author:       zhou
* Mail:         
* Created Time: 2021-05-24
*
* Description:
*
* History
*   1. Data:
*      Author:
*      Modification:
*   2. ...
*/

#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "./include/libmodbus/modbus.h"

// ModbusTcp从站（服务端）
void *ModbusTcp_Slave(void *arg);
// ModbusTcp主站（客户端）
void *ModbusTcp_Host(void *arg);

int main(void) 
{
    pthread_t slave_thread_id, host_thread_id;

    // 网口1的IP地址为192.168.1.100
    // 网口2的IP地址为192.168.2.100
    // 程序内部通讯配置与网口的IP地址相关，若改变IP地址则程序也许要对应修改

    // 创建ModbusTcp从站（服务端）线程
    pthread_create(&slave_thread_id, NULL, ModbusTcp_Slave, NULL);
    // 创建ModbusTcp主站（客户端）线程
    pthread_create(&host_thread_id, NULL, ModbusTcp_Host, NULL);

    while(1) {
        sleep(10);
    }

}

// ModbusTcp从站（服务端）
void *ModbusTcp_Slave(void *arg)
{
    modbus_t *ctx;                  // modbusTcp的句柄
    modbus_mapping_t *mb_mapping;   // 数据区
    int server_socket = -1;         // 通讯Socket

    // 初始化Modbus Tcp 句柄指针
    // 此处的IP地址是指绑定本机的IP地址（使用哪个网口）
    // 此IP为网口1的IP地址，数据通讯走网口1
    ctx = modbus_new_tcp("192.168.1.100", 502);

    // 设置是否打印调试信息
    modbus_set_debug(ctx, FALSE);

    // 申请内存区用于存放寄存器数据(每种类型的数据都是500个，起始点号为0)
    mb_mapping = modbus_mapping_new(500, 500, 500, 500);
    if(mb_mapping == NULL)  {
        fprintf(stderr, "Failed mapping:%s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return NULL;
    }

    // 寄存器初始数据填充
    mb_mapping->tab_registers[0] = 0;   // 将0写入地址为0的保持寄存器
    mb_mapping->tab_registers[1] = 1;   // 将1写入地址为1的保持寄存器
    mb_mapping->tab_registers[2] = 2;   // 将2写入地址为2的保持寄存器
    mb_mapping->tab_registers[3] = 3;   // 将3写入地址为3的保持寄存器
    mb_mapping->tab_registers[4] = 4;   // 将4写入地址为4的保持寄存器
    mb_mapping->tab_registers[5] = 5;   // 将5写入地址为5的保持寄存器
    mb_mapping->tab_registers[6] = 6;   // 将6写入地址为6的保持寄存器
    mb_mapping->tab_registers[7] = 7;   // 将7写入地址为7的保持寄存器
    mb_mapping->tab_registers[8] = 8;   // 将8写入地址为8的保持寄存器
    mb_mapping->tab_registers[9] = 9;   // 将9写入地址为9的保持寄存器

    // 开始监听Modbus句柄
    server_socket = modbus_tcp_listen(ctx, 1);
    if(server_socket == -1)  {
        fprintf(stderr,"Unable to listen TCP.\n");
        modbus_free(ctx);
        return NULL;
    }

    // 等待ModbusTcp主站连接
    modbus_tcp_accept(ctx, &server_socket);

    while(1)  {
        uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];   // 存放接收数据的报文
        int rc;                                     // 存放执行结果

        rc = modbus_receive(ctx, query);            // 获取查询报文
        // 收到查询报文
        if(rc >= 0)  {
            // 应发送对接收到的请求的响应。分析参数中给出的请求req ，然后使用 modbus 上下文ctx的信息构建和发送响应。
            // 如果请求指示读取或写入值，则操作将根据被操作数据的类型在 modbus 映射mb_mapping中完成。
            // 如果发生错误，将发送异常响应。
            modbus_reply(ctx, query, rc, mb_mapping); // 回复响应报文
        }
        // 接收报文异常 - 连接被客户端关闭或错误
        else {
            printf("Connection Closed.\n");
            // 关闭Modbus句柄
            modbus_close(ctx);
            // 等待新的ModbusTcp主站连接
            modbus_tcp_accept(ctx, &server_socket);
        }
    }

    printf("Quit the loop:%s\n",modbus_strerror(errno));

    modbus_mapping_free(mb_mapping);
    modbus_close(ctx);
    modbus_free(ctx);
}


// ModbusTcp主站（客户端）
void *ModbusTcp_Host(void *arg)
{
    modbus_t *ctx;          // modbusTcp的句柄
    int rc;                 // 命令执行结果

    int addr = 0;           // 要读的数据起始地址
    int nb = 10;            // 要读的数据个数
    uint16_t tab_reg[20];   // 用于存放读回来的数据

    // 初始化Modbus Tcp 句柄指针
    // 此处的IP地址是指连接的从站（服务端）的IP地址
    // 此地址与网口2的IP地址在同一个网段中，则数据通过网口2通讯
    ctx = modbus_new_tcp("192.168.2.101", 502);

    // 设置是否打印调试信息
    modbus_set_debug(ctx, FALSE);

    // 建立ModbusTcp连接
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed:%d\n", modbus_strerror(errno));
        modbus_free(ctx);
        return NULL;
    }

    // 设置从站ID
    rc = modbus_set_slave(ctx, 1);
    if (rc == -1) {
        fprintf(stderr, "Invalid slave ID\n");
        modbus_free(ctx);
        return NULL;
    }

    while(1) {
        // 读写寄存器/数据
        rc = modbus_read_registers(ctx, addr, nb, tab_reg);
        if (rc == -1) {
            fprintf(stderr, "%s\n", modbus_strerror(errno));
            continue;
        }
        // 打印读回来的数据
        for (int i = 0; i < nb; i++) {
            printf("reg[%d]=%d \n", i, tab_reg[i]);
        }
        sleep(5);
    }
    
    modbus_close(ctx);
    modbus_free(ctx);
}
