#include<stdio.h>
#include<stdlib.h>

void clean() {
    while (getchar() != '\n');
}

int main() {

    char buf[100];
    int start_listingPort = 5400;

    printf("***** Welcome to client management, please press enter to create new client\n");

    while (1) {

        clean();

        sprintf(buf, "start client_side.exe %d", start_listingPort);
        system(buf);

        sprintf(buf, "start server_side.exe %d", start_listingPort);
        system(buf);


        start_listingPort++;
        printf("Client added!\n");
    }

    return 0;
}