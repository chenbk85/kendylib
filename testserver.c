
#include "KendyNet.h"
#include "Connection.h"
#include <stdio.h>
#include <stdlib.h>
#include "thread.h"
#include "SocketWrapper.h"
#include "SysTime.h"
#include "Acceptor.h"
#include <stdint.h>
uint32_t packet_recv = 0;
uint32_t packet_send = 0;
uint32_t send_request = 0;
uint32_t tick = 0;
uint32_t now = 0;
uint32_t s_p = 0;
uint32_t bf_count = 0;
uint32_t clientcount = 0;
uint32_t last_send_tick = 0;
uint32_t recv_count = 0;

#define MAX_CLIENT 1000
static struct connection *clients[MAX_CLIENT];

void init_clients()
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT;++i)
		clients[i] = 0;
}

void add_client(struct connection *c)
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i] == 0)
		{
			clients[i] = c;
			break;
		}
	}
}

void send2_all_client(rpacket_t r)
{
	uint32_t i = 0;
	wpacket_t w;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i])
		{
			w = wpacket_create_by_rpacket(0,0,r);
			++send_request;
			connection_push_packet(clients[i],w);
		}
	}
}

void remove_client(struct connection *c)
{
	uint32_t i = 0;
	for(; i < MAX_CLIENT; ++i)
	{
		if(clients[i] == c)
		{
			clients[i] = 0;
			break;
		}
	}
}

void on_process_packet(struct connection *c,rpacket_t r)
{
	send2_all_client(r);
	++send_request;
	rpacket_destroy(&r);
	++packet_recv;	
}

void accept_callback(HANDLE s,void *ud)
{
	HANDLE *engine = (HANDLE*)ud;
	struct connection *c = connection_create(s,0,on_process_packet,remove_client);
	add_client(c);
	printf("cli fd:%d\n",s);
	setNonblock(s);
	//������һ��������
	connection_recv(c);
	Bind2Engine(*engine,s,RecvFinish,SendFinish);
}


const char *ip;
uint32_t port;


void *_Listen(void *arg)
{
	acceptor_t a = create_acceptor(ip,port,&accept_callback,arg);
	while(1)
		acceptor_run(a,100);
	return 0;
}
uint32_t iocp_count = 0; 
int main(int argc,char **argv)
{

	HANDLE engine;
	uint32_t n;
	
	ip = argv[1];
	port = atoi(argv[2]);
	signal(SIGPIPE,SIG_IGN);
	if(InitNetSystem() != 0)
	{
		printf("Init error\n");
		return 0;
	}	

	uint32_t i = 0;
	//getchar();
	//init_wpacket_pool(100000);
	//init_rpacket_pool(50000);
	//buffer_init_maxbuffer_size(2000);
	//buffer_init_64(2000);
	init_clients();

	engine = CreateEngine();
	thread_run(_Listen,&engine);
	tick = GetSystemMs();
	while(1)
	{
		EngineRun(engine,15);
		now = GetSystemMs();
		if(now - tick > 1000)
		{
			//printf("recv:%u,send:%u,s_req:%u,pool_size:%u,bf:%u,sp:%u,rpk_size:%u\n",packet_recv,packet_send,send_request,wpacket_pool_size(),bf_count,s_p,rpacket_pool_size());
			tick = now;
			packet_recv = 0;
			packet_send = 0;
			send_request = 0;
			s_p = 0;
			iocp_count = 0;
			recv_count = 0;
		}
		if(now - last_send_tick > 50)
		{
			//����,ÿ50ms���з�һ�ΰ�
			last_send_tick = now;
			for(i=0; i < MAX_CLIENT; ++i)
			{
				if(clients[i])
				{
					//++send_request;
					connection_send(clients[i],0,0);
				}
			}
		}
		
	}
	return 0;
}