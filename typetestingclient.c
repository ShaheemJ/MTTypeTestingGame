#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>

#define MAXLEN 4096

static struct termios orig; // og settings
static void restore(void) {
	tcsetattr(STDIN_FILENO, TCSANOW, &orig);
}

static void raw_mode(void) { // put terminal in raw mode so that we can get every keystroke
	tcgetattr(STDIN_FILENO, &orig);
	atexit(restore); // restore terminal if exit

	struct termios raw = orig;
	raw.c_lflag &= ~(ECHO | ICANON | ISIG);
	raw.c_iflag &= ~(IXON | ICRNL);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}
static void die(const char *s) {
	perror(s);
	exit(1);
}

int contains(char *h, size_t len, char *buf) {
	size_t n = strlen(buf);
	if(n == 0 || len < n) return 0;

	for(size_t i = 0; i <= len-n; i++) {
		if(memcmp(h+i, buf, n) == 0) return 1;
	}
	return 0;
}

int main(int argc, char**argv) {
	int sockfd = 0, n = 0;
	char recvBuff[4096];
	struct sockaddr_in serv_addr;

	if(argc!=3) {
		fprintf(stderr,"Usage: %s <ip> <port>\n",argv[0]);
		return 1;
	}
	signal(SIGPIPE, SIG_IGN);

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) die("socket");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) die("inet_pton");
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) die("connect");

	printf("Enter your username: ");
	fflush(stdout);
	if (!fgets(recvBuff, sizeof(recvBuff), stdin)) return 0;
	write(sockfd, recvBuff, strlen(recvBuff));

	raw_mode();
	puts("Connected Succesfully!");

	fd_set set;
	int maxfd = (sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO);

	char linebuf[4096];
	int llen = 0, in_play = 0, typed = 0;

	while(1) {
		FD_ZERO(&set);
		FD_SET(STDIN_FILENO, &set);
		FD_SET(sockfd, &set);

		if (select(maxfd + 1, &set, NULL, NULL, NULL) < 0) {
			if(errno == EINTR) continue;
			die("select");
		}

		if (FD_ISSET(sockfd, &set)) {
			n = read(sockfd, recvBuff, sizeof(recvBuff));
			if (n <= 0) break;

			if(contains(recvBuff, n, "-- START! --")) {
				in_play = 1;
				typed = 0;
				write(STDOUT_FILENO, "\n> ", 3);
			}

			if (contains(recvBuff, n, "-- TIME'S UP! --")) in_play = 0;
			write(STDOUT_FILENO, recvBuff, n);
		}

		if (FD_ISSET(STDIN_FILENO, &set)) {
			n = read(STDIN_FILENO, recvBuff, sizeof(recvBuff));
			if(n <= 0) continue;

			for(int i = 0; i < n; i++) {
				char ch = recvBuff[i];

				if(in_play) {
					if (ch == 8 || ch == 127) {
						if(typed) {
							--typed;
							write(STDOUT_FILENO, "\b \b", 3);
						}
						write(sockfd, &ch, 1);
						continue;
					}

					write(sockfd, &ch, 1);
					write(STDOUT_FILENO, &ch, 1);
					++typed;
					continue;
				}

				if (ch == 8 || ch == 127) {
					if(llen) {
						--llen;
						write(STDOUT_FILENO, "\b \b", 3);
					}
					continue;
				}

				if (ch == '\r') ch = '\n';
				if (ch == '\n') {
					linebuf[llen++] = '\n';
					if (write(sockfd, linebuf, llen) < 0) die("write");
					write(STDOUT_FILENO, "\n", 1);
					llen = 0;
					continue;
				}
				if (llen < MAXLEN -1) {
					linebuf[llen++] = ch;
					write(STDOUT_FILENO, &ch, 1);
				}
			}
		}
	}
	puts("\nDisconnected.");
	close(sockfd);
	return 0;
}