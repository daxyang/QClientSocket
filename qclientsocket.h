#ifndef QCLIENTSOCKET_H
#define QCLIENTSOCKET_H

#include "qclientsocket_global.h"
#include "net_protocol.h"

#include "qslidingwindow.h"
#include "qslidingwindowconsume.h"
#include "pthread.h"
#include "sys/time.h"
#include "signal.h"

#define PROTOCOL_BUFFER_LEN (2 * 1024 * 1024)
#define SEND_USER 1
#define RECV_USER 2


struct sub_cmd_link_t
{
  int no;
  u32 sub_cmd_type;
  void (*callback)(char *data,u32 len);
  struct sub_cmd_link_t *next;
};

struct cmd_link_t
{
    int no;
    u16 cmd_type;
    struct sub_cmd_link_t *subcmd_head;
    struct cmd_link_t *next;
};

class QCLIENTSOCKETSHARED_EXPORT QClientSocket
{

public:
    QClientSocket();
    int connect_server(char *ip,int port);  //连接到服务器
    void set_protocol_ack_callback(u16 cmd_type,u32 sub_cmd_type,void (*function)(char *data,u32 len)); //添加协议处理函数
    void send_protocol(u16 cmd_type,u32 sub_cmd_type,char *data,u32 len);
    void start_heart();
    void stop_heart();
    void start();

private:

    int client_socket;
    //static QClientSocket *client_pthis;
    int itimer_cnt;
    struct cmd_link_t *head_cmd_node;
    void append_cmd_link_node(u16 cmd_type);
    struct sub_cmd_link_t *search_subcmd_head(u16 cmd_type);
    void append_subcmd_link_node(u16 cmd_type,u32 sub_cmd_type);
    struct sub_cmd_link_t *search_subcmd_node(u16 cmd_type,u32 sub_cmd_type);
    void append_subcmd_link_node(u16 cmd_type,u32 sub_cmd_type,void (*function)(char *data,u32 len));

    pthread_t send_pthread_id;
    pthread_t recv_pthread_id;
    pthread_t treasmit_pthread_id;
    pthread_t timer_pthread_id;

    int quite;

    static void *run_send_pthread(void *ptr);
    static void *run_recv_pthread(void *ptr);
    static void *run_treasmit_pthread(void *ptr);
    static void *run_timer_pthread(void *ptr);

    void start_send();
    void start_recv();
    void start_treasmit();

    int WRITE(int sk, char *buf, int len);
    int READ(int sk, char *buf, int len);
    QSlidingWindow *send_sliding;
    QSlidingWindow *recv_sliding;
    QSlidingWindowConsume *send_consume;
    QSlidingWindowConsume *recv_consume;
    char *send_buffer;
    char *recv_buffer;
    struct _frame_info_t *frame;

    struct timeval timer;

    void do_ctrl_protocol(u32 sub_cmd_type,char *data,u32 len);
    void do_file_protocol(u32 sub_cmd_type,char *data,u32 len);
    void do_aid_protocol(u32 sub_cmd_type,char *data,u32 len);
    void do_vid_protocol(u32 sub_cmd_type,char *data,u32 len);
    /*
     * 添加新的协议处理函数
    void do_xxx_protocol(u32 sub_cmd_type,char *data,u32 len);
     */




};

#endif // QCLIENTSOCKET_H
