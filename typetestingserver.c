#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>

#define MAXUSERS 100
#define MAXLEN 10000

char userids[MAXUSERS][MAXLEN];
int clientfds[MAXUSERS];
int numUsers=0;

pthread_mutex_t users_lock = PTHREAD_MUTEX_INITIALIZER;

void pexit(char *errmsg) {
	fprintf(stderr, "%s\n", errmsg);
	exit(1);
}

void broadcast(const char *msg, int user) {
	printf("Broadcast message from %s: %s", userids[user], msg);
	pthread_mutex_lock(&users_lock);
	for(int i = 0; i < MAXUSERS; i++) {
		int fd = clientfds[i];
		if (fd >= 0 && i != user) write(fd, msg, strlen(msg));
	}
	pthread_mutex_unlock(&users_lock);
}

void record(const char *userID, double wpm) {
	double old = 0.0;
	FILE *f = fopen("highscore.txt", "r");
	if(f) {
		fscanf(f, "%*[^0-9] %lf", &old);
		fclose(f);
	}
	if( wpm <= old) return;
	if(wpm > old) {
		FILE *fwrite = fopen("highscore.txt", "w");
		if(!fwrite) return;

		time_t t = time(NULL);
		struct tm *tm = localtime(&t);
		char info[64];
		strftime(info, sizeof(info), "%c", tm);
		fprintf(fwrite, "%s's record is %.1f achieved on %s\n", userID, wpm, info);
		fclose(fwrite);
	}
}

void send_record(int connfd) {
	FILE *f = fopen("highscore.txt", "r");
	char buffer[1024];
	if(!f) {
		const char *inv = "No scores found!\n";
		write(connfd, inv, strlen(inv));
		return;
	}
	while(fgets(buffer, sizeof(buffer), f)) {
		write(connfd, buffer, strlen(buffer));
	}
	fclose(f);
}
void send_list(int connfd) {
	pthread_mutex_lock(&users_lock);
	for(int i = 0; i < MAXUSERS; i++) {
		if(clientfds[i] >= 0) {
			write(connfd, userids[i], strlen(userids[i]));
			write(connfd, "\n", 1);
		}
	}
	pthread_mutex_unlock(&users_lock);
}

static void set_blocking(int fd, int blocking) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (blocking) {
		flags &= ~O_NONBLOCK;
	} else {
		flags |=  O_NONBLOCK;
	}
	fcntl(fd, F_SETFL, flags);
}
char *words[] = {
    "alani","monster","ghost","energy","drinks",
    "quick","brown","fox","jumps","over","lazy","dog",
    "lorem","ipsum","grovyle","sit","amet","consectetur","adipiscing","elit",
    "sed","do","charmander","india","torchic","utd","mavs","dallas",
    "dolore","magna","aliqua","enim","ad","minim","veniam",
    "quis","nostrud","exercitation","ullamco","laboris","nisi",
    "aliquip","ex","ea","commodo","consequat","duis","aute","irure",
    "reprehenderit","voluptate","velit","esse","cillum","eu","fugiat",
    "nulla","pariatur","excepteur","sint","occaecat","cupidatat",
    "non","proident","sunt","in","culpa","qui","officia","deserunt",
    "mollit","anim","id","est","laborum"
};

const int wordSize = sizeof(words)/sizeof(words[0]);

void randPassage(char *buf) {
	buf[0] = '\0';
	strcat(buf, words[rand() % wordSize]);
	strcat(buf, " ");

	for( int i = 0; i < 98; i++) {
		int j = rand() % wordSize;
		strcat(buf, words[j]);
		strcat(buf, " ");
	}
	strcat(buf, words[rand() % wordSize]);
}
//introduce a function for thread -- work as dedicated server for a client
void *dedicatedServer(void *ptr) {
	int clientId = *(int *) ptr;
	free(ptr);
	int connfd;

	pthread_mutex_lock(&users_lock);
	connfd = clientfds[clientId];
	pthread_mutex_unlock(&users_lock);

	char buffer[MAXLEN];
	char cout[MAXLEN];
	int n;

	while((n = read(connfd, buffer, sizeof(buffer) -1)) > 0) {
		buffer[n] = '\0';
		if(buffer[0] == '/') {
			char *command = strtok(buffer + 1, " \n");
			if(!command) continue;
			if (strcmp(command, "msg") == 0) {
				char *target = strtok(NULL, " \n");
				char *message = strtok(NULL, "\n");
				if(!target || !message) {
					char *usage = "Usage: /msg <user> <text>\n";
					write(connfd, usage, strlen(usage));
					continue;
				}
				int found = -1;
				pthread_mutex_lock(&users_lock);

				for(int i = 0; i < MAXUSERS; i++) {
					if(clientfds[i] >= 0 && strcmp(target, userids[i]) == 0) {
						found = i;
						break;
					}
				}
				pthread_mutex_unlock(&users_lock);
				if (found >= 0) {
					snprintf(cout, sizeof(cout), "%s says %s\n", userids[clientId], message);
					write(clientfds[found], cout, strlen(cout));
					char *valid = "Message sent!\n";
					write(connfd, valid, strlen(valid));
				} else {
					char *invalid = "Sorry, that user has not joined yet.\n";
					write(connfd, invalid, strlen(invalid));
				}
			}
			else if (strcmp(command, "list") == 0) {
				send_list(connfd);
			}
			else if (strcmp(command, "broadcast") == 0) {
				char *message = strtok(NULL, "\n");
				if (!message) message = "";
				snprintf(cout, sizeof(cout), "%s says to all: %s\n", userids[clientId], message);
				broadcast(cout, clientId);
				char *b = "Broadcast message sent!\n";
				write(connfd, b, strlen(b));
			}
			else if (strcmp(command, "highscore") == 0) {
				send_record(connfd);
			}
			else if (strcmp(command, "play") == 0) {
				char pasBuf[8480];
				randPassage(pasBuf);

				char *start = "-- YOU HAVE 15 SECONDS! --\n-- START! --\n";
				write(connfd, start, strlen(start));
				write(connfd, pasBuf, strlen(pasBuf));
				write(connfd, "\n", 1);

				set_blocking(connfd, 0); // socket is in non-blocking mode
				size_t pos = 0, correct = 0, total = 0, burst = 0;
				int copied = 0;
				struct timeval last = {0,0};

				time_t startT = time(NULL);
				while(time(NULL) - startT < 15) {
					char buf[256];
					ssize_t r = read(connfd, buf, sizeof(buf));
					if(r > 5){
						if (time(NULL) - startT < 2) copied = 1; // prob c&pasted
					}

					if(r > 0) {
						for (ssize_t i=0; i < r; i++) {
							char ch = buf[i];

							struct timeval tv;
							gettimeofday(&tv, NULL);//super acc timestamp
							long gap = (tv.tv_sec  - last.tv_sec) * 1000L + (tv.tv_usec - last.tv_usec) / 1000L;
						        if (last.tv_sec && gap < 15) {
								if(burst++ > 50) copied = 1; // too fast for human
							} else {
								burst = 0;
							}
							last = tv; // track the keystroke timing

							if(ch == '\b' || ch == 127) { // backspace
								if(total > 0) total--;
								if(pos > 0) pos--;
								continue;
							}
							if (ch == '\r' || ch == '\n') continue;
							if (total < strlen(pasBuf)) {
								if(ch == pasBuf[pos]) correct++;
								pos++;
							}
							total++;
						}
					 } else {
							usleep(20*1000);
						}
					}
				set_blocking(connfd, 1); // restore socket blocking mode
				double wpm = copied ? 0.0 : ((double)correct/5.0) * 4.0;

				char *end = "\n -- TIME'S UP! --\n";
				write(connfd, end, strlen(end));
				char result[128];
				if (copied)
					snprintf(result,sizeof(result), "Paste detected\n");
				else
					snprintf(result, sizeof(result), "Your WPM is %.1f\n", wpm);
				write(connfd, result, strlen(result));
				if (copied == 0) record(userids[clientId], wpm);
			}
			else if (strcmp(command, "quit") == 0) {
				char *bye = "Thanks for joining!\n";
				write(connfd, bye, strlen(bye));
				break;
			}
			else {
				char *unknown = "Unknown command.\n";
				write(connfd, unknown, strlen(unknown));
			}
		}
	}

	close(connfd);
	pthread_mutex_lock(&users_lock);
	clientfds[clientId] = -1;
	userids[clientId][0] = '\0';
	numUsers--;
	pthread_mutex_unlock(&users_lock);
	return NULL;
}

int main(int argc, char *argv[])
{

    for(int i = 0; i < MAXUSERS; i++) {
    	clientfds[i] = -1;
	userids[i][0] = '\0';
    }

    srand(time(NULL) ^ getpid());

    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		pexit("socket() error.");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int port = 4999;
    do {
	port++;
    	serv_addr.sin_port = htons(port);
    } while (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0);
	printf("bind() succeeds for port #%d\n", port);

    if (listen(listenfd, 10) < 0) pexit("listen() error.");

    while(1)
    {
       	        connfd = accept(listenfd, NULL, NULL);
		if(connfd < 0) continue;
		//read the userid here & store userid & connfd in global arrays
		char buffer[MAXLEN];
		int n = read(connfd, buffer, sizeof(buffer)-1);
		if (n <= 0) {
			close(connfd);
			continue;
		}

		buffer[n] = '\0';
		char *nl = strchr(buffer, '\n');
		if (nl) *nl = '\0';

		int idx = -1;
		pthread_mutex_lock(&users_lock);
		for(int i = 0; i < MAXUSERS; i++) {
			if (clientfds[i] < 0) {
				idx = i;
				clientfds[i] = connfd;
				strncpy(userids[i], buffer, MAXLEN-1);
				userids[i][MAXLEN-1] = '\0';
				numUsers++;
				break;
			}
		}
		pthread_mutex_unlock(&users_lock);
		printf("connected to client slot %d.\nActive users: %d\n", idx, numUsers);

		//broadcast to all other users that a new user has joined?
		sprintf(buffer, "%s joined.\n", userids[idx]);
		//send this to all previous clients!
		broadcast(buffer, idx);
		// create a  thread here for dedicated server with current numUsers as parameter?

		int *h = malloc(sizeof(int));
		*h = idx;
		pthread_t tid;
		pthread_create(&tid, NULL, dedicatedServer, h);
		pthread_detach(tid);
	}
        close(listenfd);
	return 0;
}