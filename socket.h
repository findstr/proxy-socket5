#ifndef _SOCKET_H
#define	_SOCKET_H

struct socket;

struct socket *socket_create();
int socket_free(struct socket *self);

int socket_connect(struct socket *self, const char *dst_ip, unsigned short dst_port);
int socket_disconnect(struct socket *self);


int socket_send_data(struct socket *self, const char *buff, int len);
int socket_recv_data(struct socket *self, char *buff, int len);

int socket_send_line(struct socket *self, const char *buff);
int socket_recv_line(struct socket *self, char *buff, int len);




#endif // !_SOCKET_H
