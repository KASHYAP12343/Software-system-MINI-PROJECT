#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/ip.h>
#include <arpa/inet.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<termios.h>
#include<signal.h>

void connectionHandler(int socketFileDescriptor);
void hide_input(char *buffer, int size);

void main()
{
    signal(SIGINT, SIG_IGN); // Ignore Ctrl+C in client
    int socketFileDescriptor;
    int connectStatus;

struct sockaddr_in address;

socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
if (socketFileDescriptor == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
}
printf("Client side socket successfully created!\n");

address.sin_family = AF_INET;
address.sin_port = htons(8081);

// connect to localhost
if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
    perror("inet_pton");
    exit(EXIT_FAILURE);
}

// attempt connection
if (connect(socketFileDescriptor, (struct sockaddr *)&address, sizeof(address)) == -1) {
    perror("connect");
    exit(EXIT_FAILURE);
}

printf("Connected to server!\n");


    // Handling server response
    connectionHandler(socketFileDescriptor);
    exit(0);
}

void connectionHandler(int socketFileDescriptor)
{
    char readBuffer[4096], writeBuffer[4096], tempBuffer[4096];
    int readBytes, writeBytes;

    do
    {
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(socketFileDescriptor, readBuffer, sizeof(readBuffer));
        if(readBytes == -1)
        {
            printf("Unable to read from server\n");
        }
        else if(readBytes == 0)
        {
            printf("Closing the connection\n");
        }
        else
        {
            if (strcmp(readBuffer, "Enter password: ") == 0)
            {
                hide_input(writeBuffer, sizeof(writeBuffer));
            }            
            else
            {
                bzero(writeBuffer, sizeof(writeBuffer));
                bzero(tempBuffer, sizeof(tempBuffer));
                if(strcmp(readBuffer, "Client logging out...\n") == 0)
                {
                    strcpy(writeBuffer, "");
                    close(socketFileDescriptor);
                    return;
                }
                else if(strchr(readBuffer, '^') != NULL)
                {
                    if(strlen(readBuffer) != 1)
                    {
                        strncpy(tempBuffer, readBuffer, strlen(readBuffer) - 1);
                        printf("%s\n", tempBuffer);
                    }
                    strcpy(writeBuffer, "");
                }
                else if(strcmp(strncpy(tempBuffer, readBuffer, 8), "Loan ID:") == 0)
                {
                    printf("%s\n", readBuffer);
                    strcpy(writeBuffer, "");
                }
                else
                {
                    printf("%s\n", readBuffer);
                    scanf("%s", writeBuffer);
                }                
            }
            
            writeBytes = write(socketFileDescriptor, writeBuffer, sizeof(writeBuffer));
            if(writeBytes == -1)
            {
                printf("Unable to write to server\n");
                printf("Closing the connection to the server now!\n");
                break;
            }
        }
    } while(readBytes > 0);

    close(socketFileDescriptor);
}

// Hiding Password
void hide_input(char *buffer, int size) {
    struct termios oldt, newt;
    
    // Get the current terminal settings
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // Disable echoing of characters
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // Read input
    printf("Enter password: ");
    scanf("%s", buffer);

    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}