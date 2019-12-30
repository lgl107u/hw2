#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<signal.h>
#include<fcntl.h>
#include<ctype.h>
#include<errno.h>
#include<pthread.h>
#define PORT 8080


pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t match_mutex = PTHREAD_MUTEX_INITIALIZER;
void *sendsock(void *arg);
void *recvsock(void *arg);
void playing(int socket);

int sock;
int threadend = 0;
int data_flag = 0;
int match_flag = 0;
int nextturn = 0;
char data[128];
char playername[128];
char username[128];

static void sig_handler(int sig)
{
	char cmd[128];
	memset(cmd, 0, sizeof(cmd));
	if(sig==SIGINT){
		printf("\n\tExit the game\n\n");
		strcpy(cmd, "q");
		write(sock, cmd, sizeof(cmd));
		close(sock);
		exit(0);
	}
}
int max(int a, int b)
{
	return a>b?a:b;
}

int file_set(int fd, int blocking)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if(flags==-1) return 0;
	if(blocking) flags &= ~O_NONBLOCK;
	else flags |= O_NONBLOCK;
	return fcntl(fd, F_SETFL, flags)!=-1;
}

int main()
{
	int connectok;
	struct sockaddr_in serverAddress;
	const char *serverIP = "127.0.0.1";
	unsigned short serverPort = PORT;
	struct sigaction sa;
	system("clear");

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock<0){
		fprintf(stderr, "Error: socket\n");
		exit(EXIT_FAILURE);
	}

	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(serverIP);
	serverAddress.sin_port = htons(serverPort);

	sa.sa_handler = sig_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	connectok = connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
	if(sigaction(SIGINT, &sa, NULL)==-1){
		fprintf(stderr, "Error: sigaction\n");
		exit(EXIT_FAILURE);
	}

	pthread_t recvsock_t, sendsock_t;
	//pthread_attr_t attr;
	//pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		
	pthread_create(&recvsock_t, NULL, recvsock, (void*)&sock);
	pthread_create(&sendsock_t, NULL, sendsock, (void*)&sock);
	pthread_join(recvsock_t, NULL);

	pthread_mutex_destroy(&data_mutex);

	close(sock);
	return 0;
}

void *recvsock(void *arg)
{
	int sock = *(int*)arg;
	char *temp;
	int n;
    int i;

	while(1){
		pthread_mutex_lock(&data_mutex);
		n = read(sock, data, sizeof(data));
		if(strcmp(data, "q")==0) break;
		else if(strncmp(data, "Invite", 6)==0){
			write(fileno(stdout), "\n", sizeof("\n"));
			write(fileno(stdout), data, sizeof(data));
			temp = strtok(data, " ");
			temp = strtok(NULL, " ");
			pthread_mutex_lock(&match_mutex);
			strcpy(playername, temp);
			match_flag=1;		
			pthread_mutex_unlock(&match_mutex);
			usleep(1000);
		}
		else if(strncmp(data, "Start", 5)==0){
			system("clear");
			match_flag = 1;
			// for(i=0; i<40; i++) printf("");
			printf("\n");
			temp = strtok(data, ";");
			printf("\t%s\n", temp);
			temp = strtok(NULL, ";");
			nextturn=0;
			if(strcmp(temp, username)==0) nextturn=1;
			temp = strtok(NULL, ";");
			printf("%s\n", temp);
		}
		else if(strncmp(data, "Reject", 6)==0){
			printf("%s\n\n", data);
		}
		else if(strcmp(data, "Win")==0){
			printf("\n\tYou Win !!\n\n");
		}
		else if(strcmp(data, "Lose")==0){
			printf("\n\tYou Lose !!\n\n");
		}
		else if(strncmp(data, "Even", 4)==0){
			printf("\n\tThe game ended in tie !!\n\n");
		}
		else if(strncmp(data, "Leave", 5)==0){
			temp = strtok(data, ";");
			temp = strtok(NULL, ";");
			printf("%s\n", temp);
		}
		else if(strncmp(data, "username", 8)==0){
			temp = strtok(data, " ");
			temp = strtok(NULL, " ");
			memset(username, 0, sizeof(temp));
			strcpy(username, temp);
		}
		else if(strncmp(data, "Busy", 4)==0){
			printf("%s\n", data);
		}
		else if(strcmp(data, "error")==0){
			printf("\n\tWrong name !!\n\n");
		}
		else if(strcmp(data, "Error")==0){
			printf("\n\tIncorrect input !!\n\n"); 
		}

		pthread_mutex_unlock(&data_mutex);
		usleep(1000);
	}
}

void *sendsock(void *arg)
{
	int sock = *(int*)arg;
	char cmd[128];
	char response[128];
	fd_set readset;
	int flags;
	int n;
	int maxfdp;
	FILE *fp = stdin;
	pthread_detach(pthread_self());

	printf("\n\t\t\t\tWelcome to the Tic-Tac-Toe Game\t\t\t\t\n");
	printf("Game Menu\n");
	printf("(menu)  Show the cmd you can use\n");
	printf("(list)  Who is online\n");
	printf("(match) Create a game\n");
	printf("(q)     Quit\n");
	printf("\n");
	
	while(1){
		printf("cmd : ");
		scanf("%s", cmd);
		char *ptr = cmd;
		char *qtr = cmd;
		while(*ptr!='\0')
		{
			*qtr = tolower(*ptr);
			ptr++;
			qtr++;
		}
		if(strcmp(cmd, "q")==0 || strcmp(cmd, "quit")==0 ){
			write(sock, cmd, sizeof(cmd));
			printf("\n\n\tEnd game\n\t");
			break;
		}
		else if(strcmp(cmd, "list")==0){
			write(sock, cmd, sizeof(cmd));
			pthread_mutex_lock(&data_mutex);
			//read(sock, data, sizeof(data));
			data[strlen(data)-1]='\0';
			printf("\nplayer(s) online\n");	
			printf("%s\n\n\n", data);
			pthread_mutex_unlock(&data_mutex);
			usleep(1000);
		}
		else if(strcmp(cmd, "menu")==0){
			printf("\nMenu\n");
			printf("(menu)  Show cmd you can use\n");
			printf("(list)  Check online player\n");
			printf("(match) Choose matching player\n");
			printf("(q)     Quit\n\n");
		}
		else if(strcmp(cmd, "match")==0){
			printf("Choose a player : ");
			scanf("%s", playername);
			strcat(cmd, " ");
			strcat(cmd, playername);
			write(sock, cmd, sizeof(cmd));
			printf("\t\n Hold on !!\n");
			pthread_mutex_lock(&data_mutex);
			pthread_mutex_unlock(&data_mutex);
			usleep(1000);
			if(match_flag) playing(sock);
			match_flag = 0;
		}
		else if(strcmp(cmd, "y")==0){
			int enterflag = 0;
			pthread_mutex_lock(&match_mutex);
			if(match_flag){
				memset(response, 0, sizeof(response));
				strcpy(response, "Accept ");
				strcat(response, playername);
				write(sock, response, sizeof(response));
				enterflag = 1;
			}
			else printf("\tError!!\n");
			pthread_mutex_unlock(&match_mutex);
			usleep(1000);
			
			if(enterflag) playing(sock);
			match_flag = 0;
		}
		else if(strcmp(cmd, "n")==0){
			pthread_mutex_lock(&match_mutex);
			if(match_flag){
				memset(response, 0, sizeof(response));	
				strcpy(response, "Reject ");
				strcat(response, playername);
				write(sock, response, sizeof(response));
			}
			else printf("\tError\n");
			pthread_mutex_unlock(&match_mutex);
			usleep(1000);
		}
		else{
			printf("\tError\n");
		}
	}
}

void playing(int socket)
{
	int next;
	char temp[2];
	char response[128];

	while(1){
		pthread_mutex_lock(&data_mutex);
		if(strcmp(data, "q")==0 || strcmp(data, "Win")==0 || strcmp(data, "Lose")==0 || strncmp(data, "Even", 4)==0 || strncmp(data, "Leave", 5)==0){
			pthread_mutex_unlock(&data_mutex);
			break;
		}
		else if(nextturn){
			while(1){
				printf("You turn (-1 is quit) : ");
				scanf("%d", &next);
				if(next>=1 && next<=9 || next==-1) break;
				else printf("Input a number(1 to 9) !!\n\n");
			}
			if(next ==-1 || strcmp(data, "q")==0){
				printf("\n\tYou left the game !!\n");
				memset(response, 0, sizeof(response));
				strcpy(response, "Leave");
				write(socket, response, sizeof(response));
				pthread_mutex_unlock(&data_mutex);
				break;
			}
			memset(response, 0, sizeof(response));
			temp[1]='\0';
			temp[0]=next+48;
			strcpy(response, "Next;");
			strcat(response, temp);
			write(socket, response, sizeof(response));
		}
		pthread_mutex_unlock(&data_mutex);
		usleep(1000);
	}
}
