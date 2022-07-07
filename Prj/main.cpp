
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
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
    //pthread_create(&host_thread_id, NULL, ModbusTcp_Host, NULL);

    while(1) {
        sleep(10);
    }

}

static modbus_t *slave_ctx;                  // modbusTcp的句柄
static modbus_mapping_t *slave_mb_mapping;   // 数据区
static int slave_server_socket = -1;         // 通讯Socket

static void close_sigint(int dummy)
{
    if (slave_server_socket != -1) {
        close(slave_server_socket);
    }
    modbus_free(slave_ctx);
    modbus_mapping_free(slave_mb_mapping);

    exit(dummy);
}

// ModbusTcp从站（服务端）
void *ModbusTcp_Slave(void *arg)
{
    //modbus_t *ctx;                  // modbusTcp的句柄
    //modbus_mapping_t *mb_mapping;   // 数据区
    //int server_socket = -1;         // 通讯Socket

    // 初始化Modbus Tcp 句柄指针
    // 此处的IP地址是指绑定本机的IP地址（使用哪个网口）
    // 此IP为网口1的IP地址，数据通讯走网口1
    //ctx = modbus_new_tcp("192.168.1.100", 502);
    slave_ctx = modbus_new_tcp("127.0.0.1", 502);

    // 设置是否打印调试信息
    modbus_set_debug(slave_ctx, FALSE);

    // 申请内存区用于存放寄存器数据(每种类型的数据都是500个，起始点号为0)
    slave_mb_mapping = modbus_mapping_new(1500, 1500, 1500, 1500);
    if(slave_mb_mapping == NULL)  {
        fprintf(stderr, "Failed mapping:%s\n", modbus_strerror(errno));
        modbus_free(slave_ctx);
        return NULL;
    }

    // 寄存器初始数据填充
    for(int i = 0; i < 1500; i++) {
        slave_mb_mapping->tab_bits[i]             = i;    // 将0写入地址为0的线圈状态
        slave_mb_mapping->tab_input_bits[i]       = i;    // 将0写入地址为0的离散输入
        slave_mb_mapping->tab_input_registers[i]  = i;    // 将0写入地址为0的只读寄存器
        slave_mb_mapping->tab_registers[i]        = i;    // 将0写入地址为0的保持寄存器
    }

    // 开始监听Modbus句柄
    slave_server_socket = modbus_tcp_listen(slave_ctx, 1);
    if(slave_server_socket == -1)  {
        fprintf(stderr,"Unable to listen TCP.\n");
        modbus_free(slave_ctx);
        return NULL;
    }

    // 等待ModbusTcp主站连接
    modbus_tcp_accept(slave_ctx, &slave_server_socket);

    uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];   // 存放接收数据的报文
    int rc;                                     // 存放执行结果
    fd_set refset;
    fd_set rdset;
    /* Maximum file descriptor number  最大文件描述符数 */
    int fdmax;
    int master_socket;

    signal(SIGINT, close_sigint);

    /* Clear the reference set of socket  清除socket的引用集 */
    FD_ZERO(&refset);
    /* Add the server socket 添加服务器套接字*/
    FD_SET(slave_server_socket, &refset);

    /* Keep track of the max file descriptor 跟踪最大文件描述符 */
    fdmax = slave_server_socket;


    while(1)  {
        rdset = refset;
        if (select(fdmax+1, &rdset, NULL, NULL, NULL) == -1) {
            perror("Server select() failure.");
            close_sigint(1);
        }

        /* Run through the existing connections looking for data to be read */
        /* 运行现有连接以查找要读取的数据 */
        for (master_socket = 0; master_socket <= fdmax; master_socket++) {
            if (!FD_ISSET(master_socket, &rdset)) {
                continue;
            }
            if (master_socket == slave_server_socket) {
                /* A client is asking a new connection */
                /* 一个客户端正在请求一个新的连接 */
                socklen_t addrlen;
                struct sockaddr_in clientaddr;
                int newfd;

                /* Handle new connections  处理新连接 */
                addrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, sizeof(clientaddr));
                newfd = accept(slave_server_socket, (struct sockaddr *)&clientaddr, &addrlen);
                if (newfd == -1) {
                    perror("Server accept() error");
                } 
                else {
                    FD_SET(newfd, &refset);
                    if (newfd > fdmax) {
                        /* Keep track of the maximum 跟踪最大值 */
                        fdmax = newfd;
                    }
                    printf("New connection from %s:%d on socket %d\n",
                           inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port, newfd);
                }
            } 
            else {
                modbus_set_socket(slave_ctx, master_socket);
                rc = modbus_receive(slave_ctx, query);
                if (rc > 0) {
                    modbus_reply(slave_ctx, query, rc, slave_mb_mapping);
                }
                else if (rc == -1) {
                    /* This example server in ended on connection closing or any errors. */
                    /* 此示例服务器在连接关闭或任何错误时结束。*/
                    printf("Connection closed on socket %d\n", master_socket);
                    close(master_socket);

                    /* Remove from reference set 从参考集中移除*/
                    FD_CLR(master_socket, &refset);

                    if (master_socket == fdmax) {
                        fdmax--;
                    }
                }
            }
        }
    }

    printf("Quit the loop:%s\n",modbus_strerror(errno));

    modbus_mapping_free(slave_mb_mapping);
    modbus_close(slave_ctx);
    modbus_free(slave_ctx);
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
