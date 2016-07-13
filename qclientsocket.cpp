#include "qclientsocket.h"

//QClientSocket *QClientSocket::client_pthis;
QClientSocket::QClientSocket()
{
//  client_pthis = this;

  frame = new _frame_info_t;
  frame->frame_type = 20;

  //初始化主命令cmdtype的链表头
  head_cmd_node = new cmd_link_t;
  head_cmd_node->no = 0;
  head_cmd_node->cmd_type = 0;
  head_cmd_node->subcmd_head = NULL;
  head_cmd_node->next = NULL;

  //添加主命令到链表中
  append_cmd_link_node(NET_TCP_TYPE_CTRL);
  append_cmd_link_node(NET_TCP_TYPE_FILE);
  append_cmd_link_node(NET_TCP_TYPE_VID);
  append_cmd_link_node(NET_TCP_TYPE_FILE);
  /*
   * 新增通讯协议主命令
   append_cmd_link_node(NET_TCP_TYPE_XXX);
   */

  //发送缓冲区初始化
  send_sliding = new QSlidingWindow();
  send_buffer = (char *)malloc(sizeof(char) * PROTOCOL_BUFFER_LEN);
  send_sliding->sliding_init(PROTOCOL_BUFFER_LEN,send_buffer);
  send_sliding->consume_linklist_append(SEND_USER);
  //发送缓冲区消费者--取协议进行网络发送
  send_consume = send_sliding->consume_linklist_getConsume(SEND_USER);
  //接受缓冲区初始化
  recv_sliding = new QSlidingWindow();
  recv_buffer = (char *)malloc(sizeof(char) * PROTOCOL_BUFFER_LEN);
  recv_sliding->sliding_init(PROTOCOL_BUFFER_LEN,recv_buffer);
  recv_sliding->consume_linklist_append(RECV_USER);
  //接受缓冲区消费者--将从网络接受到的协议进行解析
  recv_consume = recv_sliding->consume_linklist_getConsume(RECV_USER);


  //将回调函数初始化为空
  set_protocol_ack_callback(NET_TCP_TYPE_CTRL,NET_CTRL_HEART,NULL);
  set_protocol_ack_callback(NET_TCP_TYPE_CTRL,NET_CTRL_LOGIN,NULL);
  set_protocol_ack_callback(NET_TCP_TYPE_CTRL,NET_CTRL_LOGOUT,NULL);
  set_protocol_ack_callback(NET_TCP_TYPE_FILE,NET_FILE_START,NULL);
  set_protocol_ack_callback(NET_TCP_TYPE_FILE,NET_FILE_SEND,NULL);
  set_protocol_ack_callback(NET_TCP_TYPE_FILE,NET_FILE_PATH,NULL);
  set_protocol_ack_callback(NET_TCP_TYPE_FILE,NET_FILE_LIST,NULL);
}
/*
 * 定时器处理函数
 *    发送心跳包
 */

void *QClientSocket::run_timer_pthread(void *ptr)
{
  QClientSocket *pthis = (QClientSocket *)ptr;
  int i = 0;
  while(pthis->quite == 0)
  {

    time_t timep;
    time(&timep);
    struct tm *ptime;
    ptime = localtime(&timep);

    u32 len = sizeof(app_net_ctrl_heart);
    char *buffer = (char *)malloc(sizeof(char) * len);
    app_net_ctrl_heart *heart = (app_net_ctrl_heart *)buffer;
    heart->yy = htons(1900+ptime->tm_year);
    heart->MM = ptime->tm_mon;
    heart->dd = ptime->tm_mday;
    heart->hh = ptime->tm_hour;
    heart->mm = ptime->tm_min;
    heart->ss = ptime->tm_sec;
  //  pthis->send_protocol(NET_TCP_TYPE_CTRL,NET_CTRL_HEART,buffer,len);
    //pthis->itimer_cnt = 0;
    printf("==heart:%d:%d:%d\n",heart->hh,heart->mm,heart->ss);
      select(0,NULL,NULL,NULL,&pthis->timer);
    //  if(pthis->send_consume->IsEmpty() == 1)
    //  if(pthis->itimer_cnt >= 5000)
      {
        time_t timep;
        time(&timep);
        struct tm *ptime;
        ptime = localtime(&timep);

        u32 len = sizeof(app_net_ctrl_heart);
        char *buffer = (char *)malloc(sizeof(char) * len);
        app_net_ctrl_heart *heart = (app_net_ctrl_heart *)buffer;
        heart->yy = htons(1900+ptime->tm_year);
        heart->MM = ptime->tm_mon;
        heart->dd = ptime->tm_mday;
        heart->hh = ptime->tm_hour;
        heart->mm = ptime->tm_min;
        heart->ss = ptime->tm_sec;
        pthis->send_protocol(NET_TCP_TYPE_CTRL,NET_CTRL_HEART,buffer,len);
        //pthis->itimer_cnt = 0;
        printf("heart:%d:%d:%d\n",heart->hh,heart->mm,heart->ss);
      }
      //pthis->itimer_cnt++;
      //i++;
      //printf("timer i:%d\n",i);
    //  else
    //    printf("no insert heart\n");

  }
}
/*
 * 连接服务器
 */
int QClientSocket::connect_server(char *ip,int port)
{
    struct sockaddr_in serv_addr;
    fd_set fdr, fdw;
    struct timeval timeout;
    int flags,res;
    unsigned long ul;
    printf("connect to Ipcamera.\n\r");

#ifdef __WIN32
    WORD sockVersion = MAKEWORD(2,2);
    WSADATA data;

    if(WSAStartup(sockVersion, &data) != 0)
    {
        printf("ESAStart error\n");
        return -1;
    }
    if(LOBYTE(data.wVersion)!=2 || HIBYTE(data.wVersion)!=2)
    {
        WSACleanup();
        printf("Invalid WinSock version!\n");
        return -1;
    }
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(client_socket == INVALID_SOCKET)
    {
        printf("socket create error %ld\n",GetLastError());
        printf("create socket failed!\n");
        return -1;
    }
#else
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("create socket failed!\n");
        return -1;
    }
#endif
#ifdef _WIN32
    ul = 0;

    if((flags = ioctlsocket(client_socket, FIONBIO, &ul)) < 0) {

#else
    ul = 1;
    if((flags = ioctl(client_socket,FIONBIO,&ul)) < 0) {
#endif
        perror("Netwrok test...\n");

        close(client_socket);
        return -1;
    }

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
#ifdef _WIN32
    serv_addr.sin_addr.S_un.S_addr = inet_addr(ip);
#else
    if (inet_pton(AF_INET,ip,&serv_addr.sin_addr) <= 0)
    {
        printf("ip error!\n");
        close(client_socket);
        return -1;
    }
#endif

    if(::connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        if(errno != EINPROGRESS)
        { // EINPROGRESS

            printf("Network test1...\n");
            close(client_socket);
            return -1;
        }
    }
    else {
        printf("Connected1!\n");
    }
    FD_ZERO(&fdr);
    FD_ZERO(&fdw);
    FD_SET(client_socket, &fdr);
    FD_SET(client_socket, &fdw);

    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;


    res = select(client_socket + 1, &fdr, &fdw, NULL, &timeout);
    if(res < 0) {
        perror("Network test...\n");
        close(client_socket);
        return -1;
    }
    if(res == 0) {
        printf("Connect server timeout\n");
        close(client_socket);
        return -1;
    }
    if(res == 1) {
        if(FD_ISSET(client_socket, &fdw))
        {
            printf("Connected2!\n");
        }

    }

    /* Not necessary */
    if(res == 2) {
        printf("Connect server timeout\n");
        close(client_socket);
        return -1;
    }
    ul = 0;
#ifdef _WIN32
    ioctlsocket(client_socket,FIONBIO,&ul);
#else
    ioctl(client_socket, FIONBIO, &ul); //重新将socket设置成阻塞模式
#endif

  return client_socket;
}

void QClientSocket::start()
{
  //连接成功，启动线程
  start_send();
  start_recv();
  start_treasmit();
}
/*
 * 设置协议的应答处理函数
 */
void QClientSocket::set_protocol_ack_callback(u16 cmd_type,u32 sub_cmd_type,void (*function)(char *data,u32 len))
{
    append_subcmd_link_node(cmd_type,sub_cmd_type,function);
}
/*
 * 搜索子命令sub_cmd_type在链表中的节点
 */
struct sub_cmd_link_t *QClientSocket::search_subcmd_node(u16 cmd_type,u32 sub_cmd_type)
{
  struct sub_cmd_link_t *head_subcmd = search_subcmd_head(cmd_type);
  struct sub_cmd_link_t *p1;
  if(head_subcmd->next == NULL)
    return NULL;
  p1 = head_subcmd->next;
  while((p1->next != NULL) && (p1->sub_cmd_type != sub_cmd_type))
    p1 = p1->next;

  if(p1->next == NULL)
  {
    if(p1->sub_cmd_type == sub_cmd_type)
      return p1;
    else
      return NULL;
  }
  else if(p1->sub_cmd_type == sub_cmd_type)
  {
    return p1;
  }
}
/*
 * 添加主命令cmd_type的节点到链表中
 */
void QClientSocket::append_cmd_link_node(u16 cmd_type)
{
  struct sub_cmd_link_t *head_subcmd = new sub_cmd_link_t;
  head_subcmd->no = 0;
  head_subcmd->sub_cmd_type = 0;
  head_subcmd->callback = NULL;
  head_subcmd->next = NULL;


  struct cmd_link_t *new_cmd_link = new cmd_link_t;
  new_cmd_link->no = cmd_type;
  new_cmd_link->cmd_type = cmd_type;
  new_cmd_link->subcmd_head = head_subcmd;
  new_cmd_link->next = NULL;

  struct cmd_link_t *p1;
  if(head_cmd_node->next == NULL)
  {
      head_cmd_node->next = new_cmd_link;
      return;
  }
  p1 = head_cmd_node->next;
  while((p1->next != NULL) && (p1->cmd_type != cmd_type))
    p1 = p1->next;
  if(p1->cmd_type == cmd_type)
  {
    return;
  }
  else if(p1->next == NULL)
  {
    p1->next = new_cmd_link;
  }

  return;
}
/*
 * 搜索子命令sub_cmd_type的头节点
 */
struct sub_cmd_link_t *QClientSocket::search_subcmd_head(u16 cmd_type)
{
  struct cmd_link_t *p1;
  if(head_cmd_node->next == NULL)
      return NULL;
  p1 = head_cmd_node->next;
  while((p1->next != NULL) && (p1->cmd_type != cmd_type))
    p1 = p1->next;

  if(p1->next == NULL)
  {
    if(p1->cmd_type == cmd_type)
        return p1->subcmd_head;
    else
        return NULL;
  }
  else if(p1->cmd_type == cmd_type)
    return p1->subcmd_head;
  else
    return NULL;
}
/*
 * 添加子命令sub_cmd_type节点
 */
void QClientSocket::append_subcmd_link_node(u16 cmd_type,u32 sub_cmd_type,void (*function)(char *data,u32 len))
{
  struct sub_cmd_link_t *head_subcmd = search_subcmd_head(cmd_type);
  struct sub_cmd_link_t *p1;
  struct sub_cmd_link_t *new_subcmd_node = new sub_cmd_link_t;
  new_subcmd_node->no = sub_cmd_type;
  new_subcmd_node->sub_cmd_type = sub_cmd_type;
  new_subcmd_node->callback = function;
  new_subcmd_node->next = NULL;

  if(head_subcmd->next == NULL)
  {
      head_subcmd->next = new_subcmd_node;
      return;
  }
  p1 = head_subcmd->next;
  while((p1->next != NULL) && (p1->sub_cmd_type != sub_cmd_type))
      p1 = p1->next;
  if(p1->next == NULL)
  {
      if(p1->sub_cmd_type == sub_cmd_type)
         p1->callback = function;
      else
          p1->next = new_subcmd_node;
  }
  else if(p1->sub_cmd_type == sub_cmd_type)
  {
      p1->callback = function;
      return;
  }
  else
    return;
}
/*
 * 发送通讯协议－主命令
 */
void QClientSocket::send_protocol(u16 cmd_type,u32 sub_cmd_type,char *data,u32 len)
{
    switch(cmd_type)
    {
      case NET_TCP_TYPE_CTRL:
        do_ctrl_protocol(sub_cmd_type,data,len);
      break;
      case NET_TCP_TYPE_FILE:
        do_file_protocol(sub_cmd_type,data,len);
      break;
      case NET_TCP_TYPE_AID:
        do_aid_protocol(sub_cmd_type,data,len);
      break;
      case NET_TCP_TYPE_VID:
        do_vid_protocol(sub_cmd_type,data,len);
      break;
      /*
       *添加新的协议: NET_TCP_TYPE_xxx
       case NET_TCP_TYPE_XXX:
         do_xxx_protocol(sub_cmd_type,data,len);
       break;
       */
      default:
      break;
    }
}
/*
 * NET_CTRL_xxx类处理函数－协议包生成
 */
void QClientSocket::do_ctrl_protocol(u32 sub_cmd_type,char *data,u32 len)
{
  u32 pkg_len = NET_HEAD_SIZE + len;
  char *buffer = (char *)malloc(sizeof(char) * pkg_len);
  app_net_head_pkg_t *head = (app_net_head_pkg_t *)buffer;
  if(len > 0)
    memcpy((buffer+NET_HEAD_SIZE),data,len);

  switch(sub_cmd_type)
  {
    case NET_CTRL_LOGIN:
        HEAD_PKG(head,NET_TCP_TYPE_CTRL,NET_CTRL_LOGIN,0,pkg_len);
    break;
    case NET_CTRL_LOGOUT:
        HEAD_PKG(head,NET_TCP_TYPE_CTRL,NET_CTRL_LOGOUT,0,pkg_len);
    break;
    case NET_CTRL_HEART:
        HEAD_PKG(head,NET_TCP_TYPE_CTRL,NET_CTRL_HEART,0,pkg_len);
    break;
    /*
     * 添加新的协议:
     case NET_CTRL_XXX:
        HEAD_PKG(head,NET_TCP_TYPE_CTRL,NET_CTRL_XXX,0,pkg_len);
     break;
     */
    default:
    break;
  }
  send_sliding->write_data_to_buffer(pkg_len,buffer,frame);
  free(buffer);
}
/*
 * NET_FILE_xxx类处理函数-协议生成包
 */
void QClientSocket::do_file_protocol(u32 sub_cmd_type,char *data,u32 len)
{
  u32 pkg_len = NET_HEAD_SIZE + len;
  char *buffer = (char *)malloc(sizeof(char) * pkg_len);
  app_net_head_pkg_t *head = (app_net_head_pkg_t *)buffer;
  if(len > 0)
    memcpy(buffer+NET_HEAD_SIZE,data,len);
  switch (sub_cmd_type) {
    case NET_FILE_START:
      HEAD_PKG(head,NET_TCP_TYPE_FILE,NET_FILE_START,0,pkg_len);
    break;
    case NET_FILE_LIST:
      HEAD_PKG(head,NET_TCP_TYPE_FILE,NET_FILE_LIST,0,pkg_len);
    break;
    case NET_FILE_PATH:
      HEAD_PKG(head,NET_TCP_TYPE_FILE,NET_FILE_PATH,0,pkg_len);
    break;
    case NET_FILE_SEND:
      HEAD_PKG(head,NET_TCP_TYPE_FILE,NET_FILE_SEND,0,pkg_len);
    break;
  }
  //itimer_cnt = 0;
  send_sliding->write_data_to_buffer(pkg_len,buffer,frame);
  free(buffer);

}
/*
 * NET_AID_xxx类处理函数－协议包生成
 */
void QClientSocket::do_aid_protocol(u32 sub_cmd_type,char *data,u32 len)
{

}
/*
 * NET_VID_xxx类处理函数－协议包生成
 */
void QClientSocket::do_vid_protocol(u32 sub_cmd_type,char *data,u32 len)
{

}
/*
 * 以下是进行网络通讯部份
 */
/*
 * 将发送缓冲区中的数据进行网络发送
 */
void *QClientSocket::run_send_pthread(void *ptr)
{
  QClientSocket *pthis = (QClientSocket *)ptr;
  pthis->send_consume->read_init();
  char *buffer = (char *)malloc(sizeof(char) * PROTOCOL_BUFFER_LEN);

  while(1)
  {
    int len = pthis->send_consume->read_data_to_buffer(buffer);
    if(len > 0)
    {
      pthis->WRITE(pthis->client_socket,buffer,len);
    }


#if defined(Q_OS_WIN32)
          usleep(1000);
#elif defined(Q_OS_MACX)
          pthread_yield_np();
#elif defined(Q_OS_UNIX)
        //  usleep(5000);
          pthread_yield();
#endif
  }
}
/*
 * 接受线程－接受从网络的协议包，保存在recv_buffer中
 */
void *QClientSocket::run_recv_pthread(void *ptr)
{
  QClientSocket *pthis = (QClientSocket *)ptr;
  while(1)
  {
    char *head_buffer = (char *)malloc(sizeof(char) * NET_HEAD_SIZE);
    app_net_head_pkg_t *head = (app_net_head_pkg_t *)head_buffer;
    pthis->READ(pthis->client_socket,head_buffer,NET_HEAD_SIZE);
    u32 len = ntohl(head->Length);
    char *buffer = (char *)malloc(sizeof(char) * len);
    pthis->READ(pthis->client_socket,(buffer+NET_HEAD_SIZE),len - NET_HEAD_SIZE);
    memcpy(buffer,head_buffer,NET_HEAD_SIZE);
    pthis->recv_sliding->write_data_to_buffer(len,buffer,pthis->frame);
    #if defined(Q_OS_WIN32)
              usleep(1000);
    #elif defined(Q_OS_MACX)
              pthread_yield_np();
    #elif defined(Q_OS_UNIX)
            //  usleep(5000);
              pthread_yield();
    #endif
    free(head_buffer);
    free(buffer);
  }
}
/*
 * 接受到网络数据的协议进行解协
 */
void *QClientSocket::run_treasmit_pthread(void *ptr)
{
  QClientSocket *pthis = (QClientSocket *)ptr;
  pthis->recv_consume->read_init();
  char *buffer = (char *)malloc(sizeof(char) * PROTOCOL_BUFFER_LEN);
  while(1)
  {
    int len = pthis->recv_consume->read_data_to_buffer(buffer);
    if(len > 0)
    {
      app_net_head_pkg_t *head = (app_net_head_pkg_t *)buffer;
      u16 cmd_type = ntohs(head->CmdType);
      u32 sub_cmd_type = ntohl(head->CmdSubType);
      u32 len = ntohl(head->Length);

      struct sub_cmd_link_t *subcmd_node = pthis->search_subcmd_node(cmd_type,sub_cmd_type);
      if(subcmd_node->callback != NULL)
          subcmd_node->callback(buffer+NET_HEAD_SIZE,len - NET_HEAD_SIZE);
    }
    #if defined(Q_OS_WIN32)
              usleep(1000);
    #elif defined(Q_OS_MACX)
              pthread_yield_np();
    #elif defined(Q_OS_UNIX)
            //  usleep(5000);
              pthread_yield();
    #endif
  }

}
/*
 * 定长读、写数据
 */
int QClientSocket::WRITE(int sk, char *buf, int len)
{
    int ret;
    int left = len;
    int pos = 0;

    while (left > 0)
    {
        if((ret = send(sk,&buf[pos], left,0))<0)
        {
            printf("write data failed!\n");
            return -1;
        }

        left -= ret;
        pos += ret;
    }

    return 0;
}
int QClientSocket::READ(int sk, char *buf, int len)
{
    int ret;
    int left = len;
    int pos = 0;

    while (left > 0)
    {
        if((ret = recv(sk,&buf[pos], left,0))<0)
        {
            printf("read data failed!ret,left: %d,%d,%s\n",ret,left,strerror(errno));
            return -1;
        }

        left -= ret;
        pos += ret;
    }

    return 0;
}
/*
 * 线程启动函数
 */
void QClientSocket::start_recv()
{
   pthread_attr_t attr;
   pthread_attr_init (&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
   pthread_create(&recv_pthread_id,&attr,run_recv_pthread,this);
   pthread_attr_destroy (&attr);
}
void QClientSocket::start_send()
{
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&send_pthread_id,&attr,run_send_pthread,this);
  pthread_attr_destroy (&attr);
}
void QClientSocket::start_treasmit()
{
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&treasmit_pthread_id,&attr,run_treasmit_pthread,this);
  pthread_attr_destroy (&attr);
}
/*
 * 启动心跳包
 */
void QClientSocket::start_heart()
{

  timer.tv_sec = 1;
  timer.tv_usec = 0;  //1mS
  quite = 0;
  itimer_cnt = 0;
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&timer_pthread_id,&attr,run_timer_pthread,this);
  pthread_attr_destroy (&attr);

}
void QClientSocket::stop_heart()
{
  quite = 1;
}
