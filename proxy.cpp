#include <assert.h>
#include <WinSock2.h>
#include "socket.h"
#include "proxy.h"

#define	ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#pragma comment(lib, "ws2_32.lib")

struct proxy {
	struct socket	*s;
	char		ip[32];
	unsigned short	port;
	char		usr[256];
	char		pwd[256];
};

#pragma pack(push, 1)
//http://www.rfc-editor.org/rfc/rfc1928.txt
//http://www.rfc-editor.org/rfc/rfc1929.txt

struct socket5_select_request {
	char ver;		// version --> 05
	char method_nr;
	char method_tbl[255];
};

struct socket5_select_response {
	char ver;
	char method;	//X'00'不需要认证;
	//X'01'GSSAPI;
	//X'02用户名/密码;
	//X'03'--X'7F'由IANA分配;
	//X'80'--X'FE'为私人方法所保留;
	//X'FF'没有可接受的方法
};

//515 array
#define	SOCKET5_AUTH_SIZE		515
#define	SOCKET5_AUTH_VER(a)		(a[0])
#define	SOCKET5_AUTH_USR_LEN(a)		(a[1])
#define	SOCKET5_AUTH_USR(a)		(a[2])
#define	SOCKET5_AUTH_PWD_LEN(a)		(a[a[1] + 2])
#define	SOCKET5_AUTH_PWD(a)		(a[a[1] + a[a[1] + 2] + 2])
#define	SOCKET5_AUTH_LEN(a)		(3 + SOCKET5_AUTH_USR_LEN(a) + SOCKET5_AUTH_PWD_LEN(a))

#define	SOCKET5_AUTH_RES_SIZE		2
#define	SOCKET5_AUTH_RES_VER(a)		(a[0])
#define	SOCKET5_AUTH_RES_STATUS(a)	(a[1])

//连接请求
struct socket5_connect_request {
	char		ver;		//X'05'
	char		cmd;		//X'01' CONNECT
					//X'02' BIND
					//X'03' UDP ASSOCIATE
	char		rsv;		//保留, 填0
	char		atyp;		//后面IP地址类型
					//X'01' IPV4
					//X'03' 域名
					//X'04' 暂时只支持IPV4
	unsigned long	dst_ip;		//IPV4是4个字节
					//IPV6是6个字节
	unsigned short	dst_port;
};

struct socket5_connect_response {
	char ver;			//X'05'
	char err_code;			//Error code
	//  X'00' succeeded
	//  X'01' general SOCKS server failure
	//  X'02' c//nnecti//n n//t all//wed by ruleset
	//  X'03' Netw//rk unreachable
	//  X'04' H//st unreachable
	//  X'05' C//nnecti//n refused
	//  X'06' TTL expired
	//  X'07' C//mmand n//t supp//rted
	//  X'08' Address type n//t supp//rted
	//  X'09' t// X'FF' unassigned
	char rsv;
	char atyp;			//address type of following address
	unsigned long dst_ip;
	unsigned short dst_port;
};

#pragma pack(pop)

static int auth(struct socket *s, const char *usr, const char *pwd)
{
	int err;
	int len;
	char buff[SOCKET5_AUTH_SIZE];

	SOCKET5_AUTH_VER(buff) = 1;
	SOCKET5_AUTH_USR_LEN(buff) = strlen(usr);
	strcpy(&SOCKET5_AUTH_USR(buff), usr);

	SOCKET5_AUTH_PWD_LEN(buff) = strlen(pwd);
	strcpy(&SOCKET5_AUTH_PWD(buff), pwd);

	len = SOCKET5_AUTH_LEN(buff);

	err = socket_send_data(s, buff, len);
	if (err < 0)
		return err;

	err = socket_recv_data(s, buff, SOCKET5_AUTH_RES_SIZE);
	if (err < 0)
		return err;

	if (SOCKET5_AUTH_RES_STATUS(buff) != 0)
		return -1;

	return 0;
}

static int conn(struct socket *s, const char *dst_ip, unsigned short port)
{
	int err;
	struct socket5_connect_request	req;
	struct socket5_connect_response	res;

	assert(s);
	assert(dst_ip);
	assert(port);

	req.ver = 0x05;
	req.cmd = 0x01;		//const
	req.atyp = 0x01;	//IPV4
	req.rsv = 0x00;	
	req.dst_ip = inet_addr(dst_ip);
	req.dst_port = htons(port);

	err = socket_send_data(s, (char *)&req, sizeof(req));
	if (err < 0)
		return err;
	
	err = socket_recv_data(s, (char *)&res, sizeof(res));
	if (err < 0)
		return err;

	if (res.err_code != 0)
		return -1;

	return 0;
}

struct proxy *proxy_create()
{
	struct proxy *p;

	p = (struct proxy *)malloc(sizeof(*p));
	if (p == NULL)
		return NULL;

	memset(p, 0, sizeof(*p));

	p->s = socket_create();
	if (p->s == NULL) {
		free(p);
		p = NULL;
	}

	return p;
}
int proxy_free(struct proxy *self)
{
	assert(self);
	assert(self->s);

	socket_free(self->s);
	
	free(self);

	return 0;
}

int proxy_set(
	struct proxy *self,
	const char *proxy_ip,
	unsigned short proxy_port,
	const char *usr,
	const char *pwd
	)
{
	assert(self);
	assert(proxy_ip);

	strncpy(self->ip, proxy_ip, ARRAY_SIZE(self->ip));
	self->port = proxy_port;

	if (usr)
		strncpy(self->usr, usr, ARRAY_SIZE(self->usr));

	if (pwd)
		strncpy(self->pwd, pwd, ARRAY_SIZE(self->pwd));

	return 0;
}

int proxy_connect(struct proxy *self, const char *dst_ip, unsigned short dst_port)
{
	int err;

	err = socket_connect(self->s, self->ip, self->port);
	if (err < 0)
		return err;

	struct socket5_select_request	request;
	struct socket5_select_response	response;

	assert(self->ip);
	assert(self->usr);
	assert(self->pwd);

	err = socket_connect(self->s, self->ip, self->port);
	if (err < 0)
		return err;

	request.ver = 0x05;
	request.method_nr = 2;
	request.method_tbl[0] = 0x00;	/* no auth */
	request.method_tbl[2] = 0x02;	/* user pwd */

	err = socket_send_data(
				self->s,
				(char *)&request,
				2 + request.method_nr * sizeof(request.method_tbl[0])
				);

	if (err < 0)
		return err;

	err = socket_recv_data(
				self->s,
				(char *)&response,
				sizeof(response)
				);

	if (err < 0)
		return err;

	if (response.method == 0x02)
		err = auth(self->s, self->usr, self->pwd);

	if (err < 0)
		return err;

	err = conn(self->s, dst_ip, dst_port);
	
	return err;
}
int proxy_disconnect(struct proxy *self)
{
	socket_disconnect(self->s);

	return 0;
}

int proxy_send_data(struct proxy *self, const char *data, int len)
{
	int err;

	assert(self);
	assert(self->s);

	err = socket_send_data(self->s, data, len);

	return err;
}

int proxy_recv_data(struct proxy *self, char *data, int len)
{
	int err;

	assert(self);
	assert(self->s);

	err = socket_recv_data(self->s, data, len);

	return err;
}

int proxy_send_line(struct proxy *self, const char *buff)
{
	int err;

	assert(self);
	assert(self->s);

	err = socket_send_line(self->s, buff);

	return err;
}

int proxy_recv_line(struct proxy *self, char *buff, int len)
{
	int err;

	assert(self);
	assert(self->s);

	err = socket_recv_line(self->s, buff, len);

	return err;
}


static int socket5_connect_proxy(
	const char *proxy_ip,
	unsigned long proxy_port,
	const char *usr,
	const char *pwd
	)
{


}

