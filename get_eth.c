#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>

unsigned char g_macaddr_new[16];
unsigned int g_subnetmask;
unsigned int g_ipaddr;
unsigned int g_broadcast_ipaddr;

//初始化网络，获取当前网络设备信息
void init_net(unsigned char *g_eth_name, unsigned char *g_macaddr_my)
{
    int i ;
    int sock;
    struct sockaddr_in sin;
    struct ifreq ifr;
    
    sock=socket(AF_INET, SOCK_DGRAM, 0);
    if(sock==-1)
        perror("socket");
    strcpy(ifr.ifr_name, g_eth_name);
    printf("eth name:\t%s\n", g_eth_name);

    //获取并打印网卡地址
    if(ioctl(sock, SIOCGIFHWADDR, &ifr) <0 )
        perror("ioctl error\n");
    //memcpy(g_macaddr, ifr.ifr_hwaddr.sa_data, 6);
    memcpy(ifr.ifr_hwaddr.sa_data, g_macaddr_my, 6);
    memcpy(g_macaddr_new, ifr.ifr_hwaddr.sa_data, 6);
    printf("local mac:\t");
    for(i=0;i<5;i++)
        printf("%.2x:", g_macaddr_new[i]);
    printf("%.2x\n", g_macaddr_new[i]);
    close(sock); 
}

int ioctl (int __fd, unsigned long int __request, ...)
{
    //initialize

    //unsigned char g_eth_name[16];
    unsigned char g_mac_addr_eth0[16] = {212,174,82,208,181,88};
    unsigned char g_mac_addr_eth1[16] = {212,174,82,208,181,89};
    // d4:ae:52:cf:b5:58
    init_net("eth0", g_mac_addr_eth0);
    init_net("eth1", g_mac_addr_eth1);
    //do something
    //....
    return 0;
}
