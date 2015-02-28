/*
 * Copyright (c) 2015 Justin Liu
 * Author: Justin Liu <rssnsj@gmail.com>
 * https://github.com/rssnsj/network-feeds
 */

#ifndef __P2PCAT_H
#define __P2PCAT_H

#include <sys/types.h>
#include <stddef.h>
#include <netdb.h>
#include <fcntl.h>

#define bool  char
#define true  1
#define false 0
#define __be32 uint32_t
#define __be16 uint16_t
#define __u8   uint8_t

#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })

#define P2PCAT_UUID_SIZE  16

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

enum {
	P2PVTUN_MSG_NOOP,
	P2PVTUN_MSG_IPDATA,
	P2PVTUN_MSG_DISCONNECT,
};

struct p2pvtun_msg {
	struct {
		__u8 ver;  /* Unused */
		__u8 opcode;
		char src_uuid[P2PCAT_UUID_SIZE];
		char dst_uuid[P2PCAT_UUID_SIZE];
	}  __attribute__((packed)) hdr;
	union {
		struct {
			__u8 rsv;
		} __attribute__((packed)) noop;
		struct {
			__be16 proto;   /* ETH_P_IP or ETH_P_IPV6 */
			__be16 ip_dlen; /* Total length of IP/IPv6 data */
			char data[1024 * 8];
		} __attribute__((packed)) ipdata;
		struct {
			__u8 rsv;
		} __attribute__((packed)) disconnect;
	};
} __attribute__((packed));

#define P2PVTUN_MSG_BASIC_HLEN  (sizeof(((struct p2pvtun_msg *)0)->hdr))
#define P2PVTUN_MSG_IPDATA_OFFSET  (offsetof(struct p2pvtun_msg, ipdata.data))

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

static inline char *ipv4_htos(uint32_t u, char *s)
{
	static char ss[20];
	if (!s) s = ss;
	sprintf(s, "%d.%d.%d.%d",
		(int)(u >> 24) & 0xff, (int)(u >> 16) & 0xff,
		(int)(u >> 8) & 0xff, (int)u & 0xff);
	return s;
}

static inline bool is_public_ip(uint32_t ip)
{
	if ((ip & 0xff000000) == 0x00000000)
		return false;
	if ((ip & 0xff000000) == 0x0a000000)
		return false;
	if ((ip & 0xff000000) == 0x7f000000)
		return false;
	if ((ip & 0xfff00000) == 0xac100000)
		return false;
	if ((ip & 0xe0000000) == 0xe0000000)
		return false;
	if ((ip & 0xffff0000) == 0xc0a80000)
		return false;
	return true;
}

/* is_valid_bind_sin - Valid local 'sockaddr_in' for bind() */
static inline bool is_valid_bind_sin(struct sockaddr_in *addr)
{
	return (addr->sin_family == AF_INET && addr->sin_port);
}

/* is_valid_host_sin - Valid host 'sockaddr_in' for connect() and sendto() */
static inline bool is_valid_host_sin(struct sockaddr_in *addr)
{
	return (addr->sin_family == AF_INET &&
			addr->sin_addr.s_addr && addr->sin_port);
}

static inline int v4pair_to_sockaddr(const char *pair, char sep, struct sockaddr_in *addr)
{
	char host[64], *portp;
	struct addrinfo hints, *result;
	int rc;

	/* Only getting an INADDR_ANY address. */
	if (pair == NULL) {
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = 0;
		addr->sin_port = 0;
		return 0;
	}

	strncpy(host, pair, sizeof(host));
	host[sizeof(host) - 1] = '\0';

	if (!(portp = strchr(host, sep)))
		return -EINVAL;
	*(portp++) = '\0';

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;  /* For wildcard IP address */
	hints.ai_protocol = 0;        /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	if ((rc = getaddrinfo(host, portp, &hints, &result)))
		return -EINVAL;

	/* Get the first resolution. */
	*addr = *(struct sockaddr_in *)result->ai_addr;
	freeaddrinfo(result);
	return 0;
}

static inline int set_nonblock(int sockfd)
{
	if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1)
		return -1;
	return 0;
}

static inline void hexdump(void *d, size_t len)
{
	unsigned char *s;
	for (s = d; len; len--, s++)
		printf("%02x ", (unsigned int)*s);
	printf("\n");
}

#endif /* __P2PCAT_H */

