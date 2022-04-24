/*
 -- Systems Programming Project -- 
 ** Khushboo Chabra - 202112014
 ** Pranav Prajapati - 202112050
 ** Hamza Zaveri - 202112061
 ** Mihir Popat - 202112073
 */

//--- Client ---


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <netdb.h>

#define PORT "34490" // the port client will be connecting to
#define NAMELEN 100  // player max names length
#define SERVERIP "127.0.0.1"

/*
 * send and recv function with error checking
 */
int myrecv(int sockfd, void *buff, int size, char *err_msg);

int mysend(int sockfd, void *msg, int len, char *err_msg);

int main(int argc, char *argv[])
{

        int client_sockfd;
        char receive = '0';
        char name[NAMELEN];
        struct addrinfo hints, *servinfo;


        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int rv;
        if ((rv = getaddrinfo(SERVERIP, PORT, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                return 1;
        }

        if ((client_sockfd = socket(servinfo->ai_family, servinfo->ai_socktype,
                                    servinfo->ai_protocol)) == -1) {
                perror("client: socket");
                exit(1);
        }

        if (connect(client_sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
                perror("client: connect");
                close(client_sockfd);
                exit(1);
        }

        printf("Connected\n");
        printf("Welcome to the number guessing game, please login to play\n");
        printf("Player name: ");
        scanf("%s", name);

        mysend(client_sockfd, name, (int) strlen(name), "name");

        myrecv(client_sockfd, &receive, 1, "receive start"); // Wait for start message
        if (receive != 'S') {
                printf("Error occur at Start message");
                exit(1);
        }

        printf("\nThe game is starting \n You have to guess the number from 1 to 100\n");
        int guessed_number, converted_number;
        while (1) {
                myrecv(client_sockfd, &receive, 1, "ROUND");

                printf("Guess a number: ");
                scanf("%d", &guessed_number);

                converted_number = htonl(guessed_number); // convert host byte order to network byte order
                mysend(client_sockfd, &converted_number, sizeof(uint32_t), "guessed number");

                myrecv(client_sockfd, &receive, 1, "return value");
                switch (receive) {
                        case '=':
                                printf("Well done, your answer is correct\n");
                                printf("The game is over\n");
                                sleep(2);
                                close(client_sockfd);
                                exit(0);
                        case '<':
                                printf("Too low, try a bigger number\n");
                                break;
                        case '>':
                                printf("Too high, try a smaller number\n");
                                break;

                        case '-':
                                printf("The game is over\n");
                                close(client_sockfd);
                                exit(0);

                        default:
                                printf("error in switch case: %c\n", receive);
                                close(client_sockfd);
                                exit(1);
                }

        }
}

int mysend(int sockfd, void *msg, int len, char *err_msg)
{
        int bytes_send;
        if ((bytes_send = (int) send(sockfd, msg, (size_t) len, 0)) == -1) {
                perror("send");
                printf("%s at %d\n", err_msg, __LINE__);
                exit(0);
        }
        return bytes_send;
}

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
