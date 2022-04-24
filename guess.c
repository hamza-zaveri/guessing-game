/*
 -- Systems Programming Project -- 
 ** Khushboo Chabra - 202112014
 ** Pranav Prajapati - 202112050
 ** Hamza Zaveri - 202112061
 ** Mihir Popat - 202112073
 */

//--- Server ---

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>

int MIN_NUM = 0;
int MAX_NUM = 3000;
int guessed_num = -1;/**/
int next_round = 1;/**/
int num_guess = 0;/**/
int parent_catch_sigint = 0;/**/

pid_t sender_pid;
pid_t recv_pid;



#define PORT "34490"  // the port users will be connecting to
#define PLAYERCOUNT 1
#define NAMELEN 100   // player max names length

void sighandler(int);
void sig1handler(int);
void sig2handler(int);
void sigpinthandler(int);
void sigcinthandler(int);
int check_input(int n);
int is_correct(char *str);
int valid_str(char *str);
int connection_handler(int sockfd, char *buff);
void game_loop(int *players_fd, char players_names[PLAYERCOUNT][NAMELEN]);
int mysend(int sockfd, char *msg, int len, char *err_msg);
int myrecv(int sockfd, void *buff, int len, char *err_msg);
void sighandler(int sig);
void sig1handler(int signum);
void sig2handler(int signum);
void sigpinthandler(int signum);
void sigcinthandler(int signum);


int server_sockfd;

//User will be giving the secret number 
//Used Signal, Fork, Pipe
void SecretNumber() {
    system("clear");
	int secret_num = 0;/**/

	int ret = fork();
	if (ret < 0) {
		perror("fork");
		exit(1);
	}
	// Child process
	if (ret == 0) {

		sender_pid = getppid();
		recv_pid = getpid();

		printf("Input secret number from 0 - 3000 (inclusive): ");
		scanf("%d", &secret_num);

		while(check_input(secret_num) != 1) {
			printf("Invalid input. Please choose between 0 - 3000 (inclusive): ");
			scanf("%d", &secret_num);
		}

		kill(sender_pid, SIGCONT);

		int sig;
		char user_input[8] = {'s', 't', 'a', 'r', 't', '\0', '\0', '\0'};
		while (is_correct(user_input) != 1) {
			
			sigset_t waitset;
			// Initialize wait set.
			sigemptyset(&waitset);
			sigaddset(&waitset, SIGCONT);/* sigaddset is used for adding the corresponding signal mask to that sigset_t variable. */
			sigprocmask(SIG_BLOCK, &waitset, NULL);

			sigwait(&waitset, &sig);

			printf("Hint (hi, lo, correct): ");
			scanf("%s", user_input);
			int i;
			for(i=0;i<=strlen(user_input);i++){
			if(user_input[i]>=65&&user_input[i]<=90)
				user_input[i]=user_input[i]+32;
			}	
			while (valid_str(user_input) != 1) {
				printf("Not a correct input, Please enter from this -> (hi, lo, correct) : ");
				scanf("%s", user_input);
				int i;
				for(i=0;i<=strlen(user_input);i++){
				if(user_input[i]>=65&&user_input[i]<=90)
					user_input[i]=user_input[i]+32;
				}
			}
			if (strcmp(user_input, "hi") == 0) {
				if(kill(sender_pid, SIGCONT) < 0) {
					perror("kill");
				}
				if(kill(sender_pid, SIGUSR1) < 0) {
					perror("kill");
				}
			} else if (strcmp(user_input, "lo") == 0) {
				if(kill(sender_pid, SIGCONT)) {
					perror("kill");
				}
				if(kill(sender_pid, SIGUSR2)){
					perror("kill");
				}
			}

		}

		if(kill(sender_pid, SIGINT)) {
			perror("kill");
		}
		if(kill(sender_pid, SIGCONT)) {
			perror("kill");
		}
		signal(SIGINT, sigcinthandler);


	} else { // parent process.

		int sig;

		sender_pid = ret;
		recv_pid = getpid();

		sigset_t waitset;/*The sigset_t data type is used to represent a signal set.*/
		// Initialize wait set.
		sigemptyset(&waitset); /*Initialize a signal mask to exclude all signals.*/
		sigaddset(&waitset, SIGCONT);/*sigaddset is used for adding the corresponding signal mask to that sigset_t variable.*/
		sigprocmask(SIG_BLOCK, &waitset, NULL);/* The sigprocmask() function examines, or changes, or both examines and changes the signal mask of the calling thread.*/

		sigwait(&waitset, &sig);/* The sigwait() function suspends execution of the calling thread until one of the signals specified in the signal set set becomes pending.*/

		signal(SIGUSR1, sig1handler);
		signal(SIGUSR2, sig2handler);
		signal(SIGINT, sigpinthandler);

		int ran_num = MIN_NUM + rand() / (RAND_MAX / (MAX_NUM - MIN_NUM + 1) + 1);
        while (1){
			if (next_round) {
				sig = 0;/**/
				sigset_t waitset;
				// Initialize wait set.
				sigemptyset(&waitset);
				sigaddset(&waitset, SIGCONT);
				sigprocmask(SIG_BLOCK, &waitset, NULL);
			
				ran_num = MIN_NUM + rand() / (RAND_MAX / (MAX_NUM - MIN_NUM + 1) + 1);

				guessed_num = ran_num;
				printf("Guess: %d\n", guessed_num);

				next_round = 0;

				kill(sender_pid, SIGCONT);
				sigwait(&waitset, &sig);
			} else if (parent_catch_sigint == 1) {
				if(kill(sender_pid, SIGINT)) {
					perror("kill");
				}
			}
		}
	}

}

//User will be guessing the secret number 
//Used Signal, Socket programming
int GuessNumber(void)
{
		system("clear");
        signal(SIGINT, sighandler);

        int player_sockfd[PLAYERCOUNT];
        char player_name[PLAYERCOUNT][NAMELEN];
        struct addrinfo hints, *servinfo;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE; // use my IP

        int rv;
        if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                return 1;
        }
        while (1) {  // main loop
                if ((server_sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
                        close(server_sockfd);
                        perror("server: socket");
                        exit(1);
                }

                if (bind(server_sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
                        close(server_sockfd);
                        perror("server: bind");
                        exit(1);
                }

                if (listen(server_sockfd, PLAYERCOUNT) == -1) {
                        close(server_sockfd);
                        perror("server: listen");
                        exit(1);
                }

                printf("\n\nThe game will be started, waiting for %d player \n", PLAYERCOUNT);

                for (int i = 0; i < PLAYERCOUNT; i++) {
                        player_sockfd[i] = connection_handler(server_sockfd, player_name[i]); // get the player sockfd
                }
                close(server_sockfd); // no need anymore

                game_loop(&player_sockfd[0], player_name); // game start

                for (int i = 0; i < PLAYERCOUNT; i++) // close player sockfds
                        close(player_sockfd[0]);
        }

        return 0;
}

int main(){
    int choice;
    system("clear");
    printf("-- Game Options -- \n");
    printf("1. You guess the secret number\n");
    printf("2. You give the secret number and let computer guess it\n");
    printf("Enter your choice : ");
    scanf("%d",&choice);
    if(choice == 1){
        GuessNumber();
    }else if(choice==2){
        SecretNumber();
    }
}


int check_input(int n) { /*function to check user input is between valid range or not */

	if (n >= 0 && n <= 3000) {
		return 1; // true
	}
	return 0; //return false
}

int is_correct(char *str) {

	if (strcmp(str, "correct") == 0) {
		return 1;
	}
	return 0;

}

int valid_str(char *str) {

    if ( (strcmp(str, "correct") == 0) || (strcmp(str, "hi") == 0) || (strcmp(str, "lo") == 0) ) {
        return 1;
    }
    return 0;

}


/* get the server sockfd, fill the buff with player name, return player sockfd */
int connection_handler(int sockfd, char *buff)
{
        struct sockaddr_storage their_addr; // connector's address information
        socklen_t sin_size = sizeof(their_addr);
        int new_fd; //sockfd of incoming conneciton

        new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
        if (new_fd == -1) {
                perror("accept");
        }

        int numbytes;
        printf("Connection accepted..\n");

        // get player name
        numbytes = myrecv(new_fd, buff, NAMELEN - 1, "get player name");

        buff[numbytes] = '\0';
        printf("%s logged in\n", buff);
        return new_fd;

}

/* game play in game_loop */
void game_loop(int *players_fd, char players_names[PLAYERCOUNT][NAMELEN])
{
        printf("The game is starting \n");
        srand(time(NULL));

        int random_number = (rand() % 99) + 1;
        int guessed_number = 0, converted_number;
        mysend(players_fd[0], "S", 1, "p1 S send"); /* inform player 1 to game is starting */

        sleep(1);
        int round = 1;

        while (1) {

                printf("\n");

                        mysend(players_fd[0], "R", 1, "p1 R send"); // send R char to wake player 1

                        myrecv(players_fd[0], &guessed_number, sizeof(int), "player guessed number");
                        converted_number = ntohl(guessed_number);
                        printf("%s guessed %d\n", players_names[0], converted_number);

                        if (converted_number == random_number) {
                                printf("%s won the game\n", players_names[0]);
                                mysend(players_fd[0], "=", 1, "= send");
                                return;
                        } else if (converted_number > random_number) {
                                mysend(players_fd[0], ">", 1, " > send");
                        } else if (converted_number < random_number) {
                                mysend(players_fd[0], "<", 1, " < send");
                        } else {
                                printf("error at %d\n", __LINE__);
                                exit(1);
                        }
                }
        

}

/* send function with error checking */
int mysend(int sockfd, char *msg, int len, char *err_msg)
{
        int bytes_send = 0;
        if ((bytes_send = (int) send(sockfd, msg, (size_t) len, 0)) == -1) {
                perror("send");
                printf("%s at %d\n", err_msg, __LINE__);
                exit(0);
        }
        return bytes_send;
}

/* recv function with error checking */
int myrecv(int sockfd, void *buff, int len, char *err_msg)
{
        int numbytes;
        if ((numbytes = (int) recv(sockfd, buff, (size_t) len, 0)) == -1) {
                perror("recv");
                printf("%s at %d\n", err_msg, __LINE__);
                exit(1);
        }
        return numbytes;
}

/* Interrupt handlers to exit with CTRL- C*/
void sighandler(int sig)
{
        printf("closing server socket: %d\n", server_sockfd);
        close(server_sockfd);
        exit(0);
}


void sig1handler(int signum) { /**/
	//fprintf(stderr, "%d # SIGUSR1 # %d\n", sender_pid, recv_pid);
	MAX_NUM = guessed_num - 1;/**/
	num_guess++;
	next_round = 1;

}

void sig2handler(int signum) {/**/
	//fprintf(stderr, "%d # SIGUSR2 # %d\n", sender_pid, recv_pid);
	MIN_NUM = guessed_num + 1;/**/
	num_guess++;
	next_round = 1;
}

void sigpinthandler(int signum) {/**/
	//fprintf(stderr, "%d # SIGINT # %d\n", sender_pid, recv_pid);
	printf("Number of guess: %d\n", num_guess);
	parent_catch_sigint = 1;/**/
}

void sigcinthandler(int signum) {/**/

	//fprintf(stderr, "%d # SIGINT # %d\n", sender_pid, recv_pid);
	kill(sender_pid, SIGKILL);
	exit(0);
}

