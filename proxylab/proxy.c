#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define LRU_MAGIC_NUMBER 9999
#define CACHE_OBJS_COUNT 10

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_hdr = "Proxy-Connection: close\r\n";
static const char *host_hdr_format = "Host: %s\r\n";
static const char *requestlint_hdr_format = "GET %s HTTP/1.0\r\n";
static const char *endof_hdr = "\r\n";

static const char *connection_key = "Connection";
static const char *user_agent_key= "User-Agent";
static const char *proxy_connection_key = "Proxy-Connection";
static const char *host_key = "Host";

// following function
void *thread(void *vargp);
void doit(int connfd);
void build_http_header(char *http_header,char *hostname,char *path,int port,rio_t *client_rio);
void parse_uri(char *uri, char *hostname, char *path,int *port);
int connect_endServer(char *hostname,int port,char *http_header);

/*cache function*/
void cache_init();
int cache_find(char *url);
int cache_eviction();
void cache_LRU(int index);
void cache_uri(char *uri,char *buf);
void readerPre(int i);
void readerAfter(int i);


// data struct for cache,  reader_first
typedef struct {
	char cache_obj[MAX_OBJECT_SIZE];
	char cache_url[MAXLINE];
	int LRU;
	int isEmpty;

	int readCnt;            /*count of readers*/
	sem_t wmutex;           /*protects accesses to cache*/
	sem_t rdcntmutex;       /*protects accesses to readcnt*/

//	int writeCnt;          /*count of writer*/
//	sem_t wtcntMutex;      /*protects accesses to writeCnt*/
//	sem_t queue;       /*protects writer first*/

}cache_block;

typedef struct {
	cache_block cacheobjs[CACHE_OBJS_COUNT];  /*ten cache blocks*/
	int cache_num;
}Cache;

Cache cache;

int main(int argc, char **argv)
{
	int listenfd = 0, connfd = 0;
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	pthread_t tid;
    printf("%s", user_agent_hdr);
    struct sockaddr_storage clientaddr;	/* generic sockaddr struct which is 28 Byte*/
	cache_init();

    /* check command line arguments */
	if(argc != 2) {
		fprintf(stderr, "usage :%s <port> \n", argv[0]);
		exit(1);
	}
	// important
	Signal(SIGPIPE, SIG_IGN);

	listenfd = Open_listenfd(argv[1]);
	while(1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

		/*print accepted message*/
		Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
		printf("Accepted connection from (%s %s).\n", hostname, port);

		Pthread_create(&tid, NULL, thread, (void *)connfd);
		/*sequential handle the client transaction*/

	}

	return 0;
}

void *thread(void *vargp) {
	printf("Thread %d processing \n", Pthread_self());
	int connfd = (int)vargp;
	Pthread_detach(pthread_self());
	doit(connfd);
	Close(connfd);
}

/*handle the client HTTP transaction
 * doit函数中对于客户端请求的HTTP Header进行处理，首先获request header（例：GET http://www.zhihu.com HTTP/1.1）部分，
 * 判断是否是请求类型（GET）。然后对于请求URL进行分析，获取需要连接的服务器的hostname，port。
 * 修改客户端的HTTP Header，让proxy充当客户端将信息转发给正确的服务器，接受服务器的返回并转发给正真的请求客户端。 
 * 其中解析request line需要调用parse_uri函数，修改客户端的HTTP Header 调用build_http_header函数
 * */
void doit(int connfd)
{
	int end_serverfd;	/*the end server file descriptor*/

	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char endserver_http_header [MAXLINE];
	/*store the request line arguments*/
	char hostname[MAXLINE], path[MAXLINE];
	int port;

	rio_t rio, server_rio;	/*rio is client's rio, server_rio is endserver's rio*/

	/* Read request line and headers */

	// call this function when opening a new fd every time, 将描述符fd和地址rio处的一个类型为rio_t的读缓冲区联系起来
	Rio_readinitb(&rio, connfd);

	// 从文件rio读出下一个问本行（包括结尾的换行符），将它复制到内存位置buf，并且用null字符来结束这个问本行，最多读 MAXLINE - 1个字符，留一个给NULL
	// buf 存取了数据包的第一行，包括method， url， 和version
	Rio_readlineb(&rio, buf, MAXLINE);

	sscanf(buf,"%s %s %s", method, uri, version); /*read the client request line*/

	if(strcasecmp(method, "GET")) {
		printf("Proxy does not implement the method");
		return;
	}

	char url_store[100];
	strcpy(url_store, uri);  /*store the original url */

	/*the uri is cached ? */
	int cache_index;
	if((cache_index = cache_find(url_store)) != -1) {
		/*response has been in cache then return the cache content， then return because having complete task*/
		printf("get the data from cache!\n");
		readerPre(cache_index);
		Rio_writen(connfd, cache.cacheobjs[cache_index].cache_obj, strlen(cache.cacheobjs[cache_index].cache_obj));
		readerAfter(cache_index);
		return;
	}

	/*parse the uri to get hostname, file path , port */
	parse_uri(uri, hostname, path, &port);

	/*build the http header which will send to the end server*/
	build_http_header(endserver_http_header, hostname, path, port, &rio);

	/*connect to the end server*/
	end_serverfd = connect_endServer(hostname, port, endserver_http_header);

	if(end_serverfd < 0) {
		printf("connection failed\n");
		return;
	}

	Rio_readinitb(&server_rio, end_serverfd);
	/*write the http header to endserver*/
	Rio_writen(end_serverfd, endserver_http_header, strlen(endserver_http_header));

	// because the response is not in the cache, we should cache it for serve next same request
	char cachebuf[MAX_OBJECT_SIZE];
	int sizebuf = 0;
	/*receive message from end server and send to the client*/
	size_t n;
	while((n = Rio_readlineb(&server_rio, buf, MAXLINE)) != 0)
	{
		sizebuf += n;
		if(sizebuf < MAX_OBJECT_SIZE)  strcat(cachebuf, buf);
		printf("proxy received %zu bytes, then send\n",n);
		Rio_writen(connfd, buf, n);
	}
	Close(end_serverfd);

	/*store it*/
	if(sizebuf < MAX_OBJECT_SIZE){
		cache_uri(url_store, cachebuf);
	}
}

/*parse the uri to get hostname,file path ,port*/
void parse_uri(char *uri, char *hostname, char *path, int *port)
{
	*port = 80;
	//The strstr() function locates the first occurrence of the null-terminated string needle in the null-terminated string haystack.
	char* pos = strstr(uri,"//");

	// http:// 可能省略了，所以按照如下处理，最后pos指向hotsname起始位置
	pos = (pos != NULL? pos + 2 : uri); // pos point to the hostname

	// pos2  标识hostname结尾
	char *pos2 = strstr(pos, ":");
	if(pos2 != NULL)
	{
		*pos2 = '\0';
		sscanf(pos, "%s", hostname);
		sscanf(pos2 + 1,"%d%s", port, path);
	} else {
		// 省略了端口号，所以pos2为空
		pos2 = strstr(pos, "/");
		if(pos2 != NULL)
		{
			*pos2 = '\0';
			sscanf(pos, "%s", hostname);
			*pos2 = '/';
			sscanf(pos2, "%s", path);
		}
		else
		{
			sscanf(pos,"%s",hostname);
		}
	}
	return;
}

/*
 * build_http_header函数：构建新的请求头，根据实验指导书的要求，
 * 需要将请求头中Host、User-Agent、Connection、Proxy-Connection等key的value部分修改为指定的信息，并且其他请求头不变
 *
 */

void build_http_header(char *http_header, char *hostname, char *path, int port, rio_t *client_rio)
{
	char buf[MAXLINE], request_hdr[MAXLINE], other_hdr[MAXLINE], host_hdr[MAXLINE];

	/*request line, 写入请求行path的内容*/
	sprintf(request_hdr, requestlint_hdr_format, path);

	/*get other request header for client rio and change it */
	while(Rio_readlineb(client_rio, buf, MAXLINE) > 0)
	{
		// 所有http请求都以\r\n作为结尾
		if(strcmp(buf, endof_hdr) == 0)
			break;/*EOF*/

		// return 0 means equal
		if(!strncasecmp(buf, host_key, strlen(host_key)))/*Host:*/
		{
			//请求有自己的Host行，用原生的
			strcpy(host_hdr, buf);
			continue;
		}

		if(!strncasecmp(buf, connection_key, strlen(connection_key))
		   &&!strncasecmp(buf, proxy_connection_key, strlen(proxy_connection_key))
		   &&!strncasecmp(buf, user_agent_key, strlen(user_agent_key)))
		{
			strcat(other_hdr, buf);
		}
	}
	if(strlen(host_hdr) == 0)
	{
		// 若请求没有host行，按照规定自己造一个
		sprintf(host_hdr, host_hdr_format, hostname);
	}
	sprintf(http_header,"%s%s%s%s%s%s%s",
			request_hdr,
			host_hdr,
			conn_hdr,
			prox_hdr,
			user_agent_hdr,
			other_hdr,
			endof_hdr);

	return ;
}

/*Connect to the end server*/
inline int connect_endServer(char *hostname, int port, char *http_header){
	char portStr[100];
	sprintf(portStr, "%d", port);
	return Open_clientfd(hostname, portStr);
}


/**************************************
 * Cache Function
 **************************************/

void cache_init() {
	cache.cache_num = 0;
	int i;
	for(i = 0; i < CACHE_OBJS_COUNT; i++) {
		cache.cacheobjs[i].LRU = 0;
		cache.cacheobjs[i].isEmpty = 1;
		Sem_init(&cache.cacheobjs[i].wmutex, 0, 1);
		Sem_init(&cache.cacheobjs[i].rdcntmutex, 0, 1);
		cache.cacheobjs[i].readCnt = 0;
	}
}

void readerPre(int i) {
	P(&cache.cacheobjs[i].rdcntmutex);
	cache.cacheobjs[i].readCnt++;
	if(cache.cacheobjs[i].readCnt==1)
		P(&cache.cacheobjs[i].wmutex); // 读者进来就加锁，只要有一个读者在，写者就要等待，直到所有读完毕

	V(&cache.cacheobjs[i].rdcntmutex);
}

void readerAfter(int i){
	P(&cache.cacheobjs[i].rdcntmutex);
	cache.cacheobjs[i].readCnt--;
	if(cache.cacheobjs[i].readCnt == 0)
		V(&cache.cacheobjs[i].wmutex);
	V(&cache.cacheobjs[i].rdcntmutex);

}

void writePre(int i){
	P(&cache.cacheobjs[i].wmutex);
}

void writeAfter(int i){
	V(&cache.cacheobjs[i].wmutex);
}

/*find url is in the cache or not */
int cache_find(char *url){
	int i;
	for(i = 0; i < CACHE_OBJS_COUNT; i++){
		readerPre(i);
		if((cache.cacheobjs[i].isEmpty == 0) && (strcmp(url, cache.cacheobjs[i].cache_url)==0))
			break;
		readerAfter(i);
	}
	if(i >= CACHE_OBJS_COUNT)
		return -1; /*can not find url in the cache*/
	return i;
}

/*find the empty cacheObj or which cacheObj should be evictioned*/
int cache_eviction() {
	int min = LRU_MAGIC_NUMBER;
	int minindex = 0;
	int i;
	for(i = 0; i < CACHE_OBJS_COUNT; i++)
	{
		readerPre(i);
		if(cache.cacheobjs[i].isEmpty == 1){/*choose if cache block empty */
			minindex = i;
			readerAfter(i);
			break;
		}
		if(cache.cacheobjs[i].LRU < min){    /*if not empty choose the min LRU*/
			minindex = i;
			readerAfter(i);
			continue;
		}
		readerAfter(i);
	}

	return minindex;
}

// 除了新写进来的cache，其他内容lru均减一
/*update the LRU number except the new cache one*/
void cache_LRU(int index){
	int i;
	for(i = 0; i < index; i++)    {
		writePre(i);
		if(cache.cacheobjs[i].isEmpty == 0 && i != index){
			cache.cacheobjs[i].LRU--;
		}
		writeAfter(i);
	}
	i++;
	for(i; i < CACHE_OBJS_COUNT; i++)    {
		writePre(i);
		if(cache.cacheobjs[i].isEmpty == 0 && i != index){
			cache.cacheobjs[i].LRU--;
		}
		writeAfter(i);
	}
}

/*cache the uri and content in cache*/
void cache_uri(char *uri,char *buf){

	int i = cache_eviction();

	writePre(i);/*writer P*/

	strcpy(cache.cacheobjs[i].cache_obj, buf);
	strcpy(cache.cacheobjs[i].cache_url, uri);
	cache.cacheobjs[i].isEmpty = 0;
	cache.cacheobjs[i].LRU = LRU_MAGIC_NUMBER;
	cache_LRU(i);

	writeAfter(i);/*writer V*/
}