#ifndef _PROXY_H
#define	_PROXY_H

struct proxy;

struct proxy *proxy_create();
int proxy_free(struct proxy *self);

int proxy_set(
	struct proxy *self,
	const char *proxy_ip,
	unsigned short proxy_port,
	const char *usr,
	const char *pwd
	);

int proxy_connect(struct proxy *self, const char *dst_ip, unsigned short dst_port);
int proxy_disconnect(struct proxy *self);

int proxy_send_data(struct proxy *self, const char *data, int len);
int proxy_recv_data(struct proxy *self, char *data, int len);

int proxy_send_line(struct proxy *self, const char *buff);
int proxy_recv_line(struct proxy *self, char *buff, int len);


#endif // !_PROXY_H
