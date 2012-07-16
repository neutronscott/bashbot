#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>


#define	BLOCK_SIZE	1024

void usage(char *self)
{
	printf(
		"%s send <file>\n"
		"%s get <host> <ip>\n",
		self, self
	);

	exit(-1);
}

int make_listener(int port)
{
	int sock;
	struct sockaddr_in sin;
	socklen_t slen = sizeof(struct sockaddr_in);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		return -1;
	}
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = port;
	if (bind(sock, (struct sockaddr *)&sin, slen) < 0) {
		perror("bind");
		return -1;
	}
	if (listen(sock, 1) < 0) {
		perror("listen");
		return -1;
	}

	return sock;
}

int dcc_send(char *file)
{
	struct sockaddr_in sin;
	struct stat st;
	struct timeval tv;
	char buf[BLOCK_SIZE], a[4];
	int lsock, sock, len, retval;
	int slen = sizeof(struct sockaddr_in);
	int32_t ack = 0;
	u_int32_t sent = 0;
	fd_set fds;
	FILE *fp;

	fp = fopen(file, "rb");
	if (fp == NULL) {
		perror("fopen");
		puts("NOFILE"); fflush(stdout);
		return -1;
	}

	/* get file size */
	if (fstat(fileno(fp), &st) < 0) {
		perror("fstat");
		puts("NOFILE"); fflush(stdout);
		return -1;
	}

	lsock = make_listener(0);
	if (lsock < 0)
		return -1;

	/* what port did we get? */
	if (getsockname(lsock, (struct sockaddr *)&sin, &slen) < 0) {
		perror("getsockname");
		return -1;
	}

	printf("%d %d %d\nWAIT\n", 0, ntohs(sin.sin_port), st.st_size);
	fflush(stdout);

	/* timeout */
	tv.tv_sec = 300;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(lsock, &fds);
	if (select(lsock+1, &fds, NULL, NULL, &tv) < 0) {
		perror("select");
		puts("ERROR"); fflush(stdout);
		return -1;
	}
	if (FD_ISSET(lsock, &fds) == 0) {
		puts("TIMEOUT"); fflush(stdout);
		return -1;
	}
	sock = accept(lsock, (struct sockaddr *)&sin, &slen);
	if (sock < 0) {
		perror("accept");
		puts("ERROR"); fflush(stdout);
		return -1;
	}

	printf("CONNECTED %s:%d\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
	fflush(stdout);

	while ((len = fread(buf, 1, BLOCK_SIZE, fp)) > 0) {
		if ((retval = send(sock, buf, len, 0)) < 0) {
			puts("ERROR SEND"); fflush(stdout);
			perror("send");
			return -1;
		}
		sent += retval;
		retval = recv(sock, (char *)&ack, sizeof(int32_t), 0);
		ack = ntohl(ack);
		if (retval == -1) {
			puts("ERROR RECV"); fflush(stdout);
			perror("recv");
			return -1;
		} else if (retval == 0) {
			puts("CLOSED"); fflush(stdout);
			return -1;
		}
		printf("SENT %lu %.1f%% ACK %lu %.1f%%\n",
			sent, 100.0 * sent / st.st_size,
			ack,  100.0 * ack  / st.st_size);
		fflush(stdout);
	}

	puts("COMPLETED"); fflush(stdout);
	return 0;
}

int dcc_get(unsigned long ip, long port)
{
	return 0;
}

int main(int argc, char **argv)
{
	long a;
	unsigned long b;

	if (argc < 2) {
		usage(argv[0]);
	} else if (strcasecmp(argv[1], "send") == 0) {
		if (argc != 3)
			usage(argv[0]);
		return dcc_send(argv[2]);
	} else if (strcasecmp(argv[1], "get") == 0) {
		if (argc != 4)
			usage(argv[0]);
		a = strtol(argv[2], NULL, 0);
		b = strtol(argv[3], NULL, 0);
		if ((a <= 0) || (b <= 0))
			usage(argv[0]);
		return dcc_get(a, b);
	} else {
		usage(argv[0]);
	}

	/* never reached */
	return 0;
}
