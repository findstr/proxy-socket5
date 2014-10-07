#include <assert.h>
#include <WinSock2.h>
#include "socket.h"

#define	ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#pragma comment(lib, "ws2_32.lib")

struct socket {
	SOCKET		s;
};

struct socket *socket_create()
{
	struct socket *s;

	s = (struct socket *)malloc(sizeof(*s));

	if (s == NULL)
		return NULL;

	memset(s, 0, sizeof(*s));

	s->s = INVALID_SOCKET;

	return s;
}

int socket_free(struct socket *self)
{
	assert(self);

	if (self->s != INVALID_SOCKET)
		closesocket(self->s);

	free(self);

	return 0;
}

int socket_connect(struct socket *self, const char *dst_ip, unsigned short dst_port)
{
	int err;
	SOCKET s;
	struct sockaddr_in sa;

	assert(self);
	assert(dst_ip);

	s = socket(AF_INET, SOCK_STREAM, 0);

	sa.sin_addr.s_addr = inet_addr(dst_ip);
	sa.sin_port = htons(dst_port);
	sa.sin_family = AF_INET;

	err = connect(s, (SOCKADDR *)&sa, sizeof(sa));
	if (err < 0)
		return -1;

	self->s = s;
	return 0;
}

int socket_disconnect(struct socket *self)
{
	assert(self->s != INVALID_SOCKET);

	closesocket(self->s);

	self->s = INVALID_SOCKET;

	return 0;
}

int socket_send_data(struct socket *self, const char *buff, int len)
{
	int err;

	assert(self);
	assert(buff);
	assert(len);
	assert(self->s != INVALID_SOCKET);

	do {
		err = send(self->s, buff, len, 0);
		if (err < 0)
			break;

		len -= err;
		buff += err;
	} while (len);

	if (len == 0)
		return 0;
	else
		return -1;
}

int socket_recv_data(struct socket *self, char *buff, int len)
{
	int err;

	assert(self);
	assert(buff);
	assert(len);
	assert(self->s != INVALID_SOCKET);

	do {
		err = recv(self->s, buff, len, 0);
		if (err <= 0)
			break;

		len -= err;
		buff += err;
	} while (len);

	if (len == 0)
		return 0;
	else
		return -1;
}

int socket_send_line(struct socket *self, const char *buff)
{
	int err;
	int len;

	assert(self);
	assert(self->s != INVALID_SOCKET);
	assert(buff);

	len = 0;

	while (buff[len++] != '\n')
		;

	err = socket_send_data(self, buff, len);

	return err;
}

int socket_recv_line(struct socket *self, char *buff, int len)
{
	int once;

	assert(self);
	assert(self->s != INVALID_SOCKET);
	assert(buff);

	memset(buff, 0, len);

	do {
		once = recv(self->s, buff, len, 0);
		if (once <= 0)
			break;
		
		len -= once;
		buff += once;

	} while (buff[-1] != '\n' && len > 0);

	if (once <= 0)
		return -1;
	else
		return 0;
}
