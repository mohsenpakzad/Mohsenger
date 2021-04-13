#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ws2tcpip.h>
//#pragma comment (lib, "ws2_32.lib") // For building with visual studio

#define MaxBufSize 1000 + 1
#define MaxListSize 1000 * 1000 + 1
#define WaitTOSendMessage 50

#define ValidID(c) ( (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c == '_') )

#define ACCEPT "1"
#define DECLINE "0"

#define TRUE 1
#define FALSE 0

SOCKET sock;
struct sockaddr_in hint;

int Listening_port = 54000;  // Listening port # on the server

char list[MaxListSize];
char buffer[MaxBufSize];
char UserName[MaxBufSize];

int is_doneOnce = FALSE;

//--------------------------------------------------------------------------------------------------------------------------------------
// General functions
void clean() {

    while (getchar() != '\n');
}

void getstr(char *destination) {

    char c;
    int i = 0;

    while ((c = getchar()) != '\n') {

        destination[i] = c;
        i++;
    }
    destination[i] = '\0';
}

int getID(char *destination) {

    char c;
    int i = 0;

    while ((c = getchar()) != '\n') {

        if (!ValidID(c)) {
            clean();
            return 0;
        }
        destination[i] = c;
        i++;
    }
    if (i == 0) {
        return 0;
    }
    destination[i] = '\0';
    return 1;
}

int streq(const char *str1, const char *str2) {

    for (int i = 0; str1[i] != '\0' || str2[i] != '\0'; i++) {

        if (str1[i] != str2[i]) {
            return 0;
        }
    }
    return 1;
}

void strapp(char *destination, const char *source) {

    int i;
    for (i = 0; destination[i] != '\0'; i++);

    for (int j = 0; source[j] != '\0'; j++) {

        destination[i] = source[j];
        i++;
    }
    destination[i] = '\0';
}

int atoi(const char *str) {

    int result = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        result = result * 10 + (str[i] - '0');
    }
    return result;
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Connecting to server
void retrySecond(int second) {

    while (second != -1) {

        printf("Can't connect to server, try again in %d seconds...", second);
        Sleep(1000);
        second--;
        system("cls");
    }
}

int inet_pton(int af, const char *src, void *dst) {
    struct sockaddr_storage ss;
    int size = sizeof(ss);
    char src_copy[INET6_ADDRSTRLEN + 1];

    ZeroMemory(&ss, sizeof(ss));
    /* stupid non-const API */
    strncpy(src_copy, src, INET6_ADDRSTRLEN + 1);
    src_copy[INET6_ADDRSTRLEN] = 0;

    if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *) &ss, &size) == 0) {
        switch (af) {
            case AF_INET:
                *(struct in_addr *) dst = ((struct sockaddr_in *) &ss)->sin_addr;
                return 1;
            case AF_INET6:
                *(struct in6_addr *) dst = ((struct sockaddr_in6 *) &ss)->sin6_addr;
                return 1;
        }
    }
    return 0;
}

void makeSocket() {

    const char ipAddress[] = "127.0.0.1";        // IP Address of the server
    // const int port = 54000;					// Listening port # on the server


    // Initialize WinSock
    struct WSAData data;
    WORD ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);

    if (wsResult != 0) {
        printf("Can't start Winsock, Err #%s \n", wsResult);
        return;
    }

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Can't create socket, Err #%s \n", WSAGetLastError());
        WSACleanup();
        return;
    }

    // Fill in a hint structure
    hint.sin_family = AF_INET;
    hint.sin_port = htons(Listening_port);
    inet_pton(AF_INET, ipAddress, &hint.sin_addr);

    // Connect to server
    int connResult;
    do {
        connResult = connect(sock, (struct sockaddr *) &hint, sizeof(hint));

        if (connResult == SOCKET_ERROR) {
            retrySecond(3);
        }

    } while (connResult == SOCKET_ERROR);
    system("cls");
    printf("Connected!\n");
    Sleep(500);
    system("cls");

}

//--------------------------------------------------------------------------------------------------------------------------------------
// sendData and receiveData
void sendData(const char *userInput) {

    int sendResult;

    sendResult = send(sock, userInput, strlen(userInput) + 1, 0); // Send the text

    if (sendResult == SOCKET_ERROR) {
        // start again
        printf("You are disconnected to server\n");
        Sleep(1000);
        system("cls");
        int main();
        main();
        exit(0);
    }
}

void receiveData(char *userOutput) {

    int bytesReceived;

    ZeroMemory(userOutput, MaxBufSize);
    bytesReceived = recv(sock, userOutput, MaxBufSize, 0);

    if (bytesReceived == SOCKET_ERROR) {
        // start again
        printf("You are disconnected to server\n");
        Sleep(1000);
        system("cls");
        int main();
        main();
        exit(0);
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------
// login menu
void user_signin() {

    sendData("signin");

    int res, isContinue = 1;
    do {
        system("cls");
        printf("Welcome to signin menu\n"
               "Please enter the username (You can use a-z, 0-9 and underscores): ");

        res = getID(UserName);
        if (res == 0) {
            printf("Invalid username, please try again...");
            clean();
            continue;
        }

        sendData(UserName);
        receiveData(buffer);

        if (streq(buffer, DECLINE)) {
            printf("This usrname is already token, please try again...");
            clean();
        } else if (streq(buffer, ACCEPT)) {
            isContinue = 0;
        }

    } while (isContinue == 1);

    char password[MaxBufSize];
    system("cls");
    printf("Now, please enter the password for \"%s\" (if you don't want password, just press enter): ", UserName);
    getstr(password);
    sendData(password);


    receiveData(buffer); // receiveData result

    if (streq(buffer, ACCEPT)) {
        printf("The username with ID \"%s\" is created successfully...\n", UserName);
        clean();
    }

}

void user_login() {

    sendData("login");

    int res, isContinue = 1;
    do {
        system("cls");
        printf("Welcome to login menu\n"
               "Please enter your username (You can use a-z, 0-9 and underscores): ");

        res = getID(UserName);
        if (res == 0) {
            printf("Invalid username, please try again...");
            clean();
            continue;
        }

        sendData(UserName);
        receiveData(buffer);

        if (streq(buffer, DECLINE)) {
            printf("Username not exist, please try again...");
            clean();
        } else if (streq(buffer, ACCEPT)) {
            isContinue = 0;
        }

    } while (isContinue == 1);

    isContinue = 1;
    char intput_password[MaxBufSize];
    do {
        system("cls");
        printf("Now, please enter the password of \"%s\": ", UserName);
        getstr(intput_password);

        sendData(intput_password);
        receiveData(buffer);

        if (streq(buffer, DECLINE)) {
            printf("Wrong password, please try again...\n");
            clean();
        } else {
            isContinue = 0;
        }

    } while (isContinue == 1);


    receiveData(buffer); // receiveData result

    if (streq(buffer, ACCEPT)) {
        printf("The username with ID \"%s\" successfully logged in...\n", UserName);
        clean();
    }

}

int login_menu() {

    int mood;
    do {
        system("cls");
        printf(
                "***** Welcome to Mohsenger! *****\n"
                "What do you like to do?\n"
                "1. Log in\n"
                "2. Sign on\n"
                "3. Exit\n"
        );

        scanf("%d", &mood);
        clean();

        if (mood != 1 && mood != 2 && mood != 3) {
            printf("Invalid command please press any key to try again...\n");
            clean();
            system("cls");
        }

    } while (mood != 1 && mood != 2 && mood != 3);

    if (mood == 1) {

        user_login();
    } else if (mood == 2) {

        user_signin();
    } else {

        return 0;
    }
    return 1;
}

// --------------------------------------------------------------------------------------------------------------------------------------
// Create menu
void create_pvChat() {

    sendData("create_pvChat");

    char intended_user[MaxBufSize];
    int res, isContinue = 1;
    do {
        system("cls");
        printf("Please enter your intended Username (You can use a-z, 0-9 and underscores): ");

        res = getID(intended_user);
        if (res == 0) {
            printf("Invalid username, please try again...");
            clean();
            continue;
        }

        sendData(intended_user);
        receiveData(buffer);

        if (streq(buffer, DECLINE)) {
            printf("Username not exist, please try again...");
            clean();
        } else if (streq(buffer, ACCEPT)) {
            isContinue = 0;
        } else if (streq(buffer, "N")) {
            printf("Privet chat with yourself is already created, try Saved Messages...\n");
            clean();
            return;
        }

    } while (isContinue == 1);

    receiveData(buffer); // receiveData pv chat exist result

    if (streq(buffer, DECLINE)) {

        printf("The privet chat with \"%s\" is already exist...\n", intended_user);
        clean();
        return;
    }

    receiveData(buffer); // receiveData pv chat final result

    if (streq(buffer, ACCEPT)) {

        printf("The privet chat with \"%s\" is created successfully...\n", intended_user);
        clean();
    }

}

void create_group() {

    sendData("create_group");

    char group_id[MaxBufSize];

    int res, isContinue = 1;
    do {

        system("cls");
        printf("Please enter the ID for your group (You can use a-z, 0-9 and underscores): ");

        res = getID(group_id);
        if (res == 0) {
            printf("Invalid group ID, please try again...");
            clean();
            continue;
        }

        sendData(group_id);
        receiveData(buffer);

        if (streq(buffer, DECLINE)) {
            printf("This ID is already token, please try again...");
            clean();
        } else if (streq(buffer, ACCEPT)) {
            isContinue = 0;
        }
    } while (isContinue == 1);

    receiveData(buffer); // receiveData result

    if (streq(buffer, ACCEPT)) {
        printf("The group with ID \"%s\" is created successfully...\n", group_id);
        clean();
    }

}

void create_channel() {

    sendData("create_channel");

    char channel_id[MaxBufSize];

    int res, isContinue = 1;
    do {

        system("cls");
        printf("Please enter the ID for your channel (You can use a-z, 0-9 and underscores): ");

        res = getID(channel_id);
        if (res == 0) {
            printf("Invalid channel ID, please try again...");
            clean();
            continue;
        }

        sendData(channel_id);
        receiveData(buffer);

        if (streq(buffer, DECLINE)) {
            printf("This ID is already token, please try again...");
            clean();
        } else if (streq(buffer, ACCEPT)) {
            isContinue = 0;
        }
    } while (isContinue == 1);

    receiveData(buffer); // receiveData result

    if (streq(buffer, ACCEPT)) {
        printf("The channel with ID \"%s\" is created successfully...\n", channel_id);
        clean();
    }
}

void create_menu() {

    int mood;
    while (1) {

        mood = -1;
        system("cls");
        printf(
                "1. New Privet chat\n"
                "2. New Group\n"
                "3. New Channel\n"
                "4. Back to main menu\n"
        );

        scanf("%d", &mood);
        clean();

        if (mood == 1) {

            create_pvChat();
            return;
        } else if (mood == 2) {

            create_group();
            return;
        } else if (mood == 3) {

            create_channel();
            return;
        } else if (mood == 4) {

            return;
        } else {

            printf("Invalid command please press any key to try again...\n");
            clean();
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------------------------
// Join menu
void join_group() {

    sendData("join_group");

    char group_id[MaxBufSize];

    int res, isContinue = 1;
    do {

        system("cls");
        printf("Please enter the group ID that you want join (You can use a-z, 0-9 and underscores): ");

        res = getID(group_id);
        if (res == 0) {
            printf("Invalid group ID, please try again...");
            clean();
            continue;
        }

        sendData(group_id);
        receiveData(buffer);

        if (streq(buffer, DECLINE)) {
            printf("There is no group ID with \"%s\", please try again...", group_id);
            clean();
        } else if (streq(buffer, ACCEPT)) {
            isContinue = 0;
        } else if (streq(buffer, "D")) {
            printf("You already joined the group with ID \"%s\"...\n", group_id);
            clean();
            return;
        }
    } while (isContinue == 1);

    receiveData(buffer); // receiveData result

    if (streq(buffer, ACCEPT)) {
        printf("You joined the group with ID \"%s\" successfully...\n", group_id);
        clean();
    }

}

void join_channel() {

    sendData("join_channel");

    char channel_id[MaxBufSize];

    int res, isContinue = 1;
    do {

        system("cls");
        printf("Please enter the channel ID that you want join (You can use a-z, 0-9 and underscores): ");

        res = getID(channel_id);
        if (res == 0) {
            printf("Invalid channel ID, please try again...");
            clean();
            continue;
        }

        sendData(channel_id);
        receiveData(buffer);

        if (streq(buffer, DECLINE)) {
            printf("There is no channel ID with \"%s\", please try again...", channel_id);
            clean();
        } else if (streq(buffer, ACCEPT)) {
            isContinue = 0;
        } else if (streq(buffer, "D")) {
            printf("You already joined the channel with ID \"%s\"...\n", channel_id);
            clean();
            return;
        }
    } while (isContinue == 1);

    receiveData(buffer); // receiveData result

    if (streq(buffer, ACCEPT)) {
        printf("You joined the channel with ID \"%s\" successfully...\n", channel_id);
        clean();
    }
}

void join_menu() {

    int mood;
    while (1) {

        mood = -1;
        system("cls");
        printf(
                "1. Join Group\n"
                "2. Join Channel\n"
                "3. Back to main menu\n"
        );

        scanf("%d", &mood);
        clean();


        if (mood == 1) {

            join_group();
            return;
        } else if (mood == 2) {

            join_channel();
            return;
        } else if (mood == 3) {

            return;
        } else {

            printf("Invalid command please press any key to try again...\n");
            clean();
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------------------------
// receive and show receiveData messages
void synthesisText(const char *received_message, char *senderName, char *status, char *messageNum, char *time,
                   char *message) {

    int index = 0;
    int i;

    for (i = 0; received_message[index] != ' '; i++) {

        senderName[i] = received_message[index];
        index++;
    }
    index++;
    senderName[i] = '\0';

    *status = received_message[index];
    index += 2;

    for (i = 0; received_message[index] != ' '; i++) {

        messageNum[i] = received_message[index];
        index++;
    }
    index++;
    messageNum[i] = '\0';

    for (i = 0; received_message[index] != ' '; i++) {

        if (received_message[index] == 'A' || received_message[index] == 'P') {
            time[i] = ' ';
            i++;
        }
        time[i] = received_message[index];
        index++;
    }
    index++;
    time[i] = '\0';

    for (i = 0; received_message[index] != '\0'; i++) {

        message[i] = received_message[index];
        index++;
    }
    message[i] = '\0';
}

void show_MessageStyle(const char *received_message) {

    char senderName[MaxBufSize], messageNum[MaxBufSize], time[MaxBufSize], message[MaxBufSize];
    char status;
    synthesisText(received_message, senderName, &status, messageNum, time, message);

    if (senderName[0] == '*') { // so this a system message!

        printf("********** %s **********\n", message);
    } else if (status == 'C') {

        printf("%-10s _Creator_%4s >> %s (%s)\n", senderName, messageNum, message, time);
    } else if (status == 'A') {

        printf("%-10s   _Admin_%4s >> %s (%s)\n", senderName, messageNum, message, time);
    } else {

        printf("%-10s         _%4s >> %s (%s)\n", senderName, messageNum, message, time);
    }

}

void show_previousMessages() {

    char receive_message[MaxBufSize];

    //sendData(ACCEPT);
    receiveData(receive_message);

    while (!streq(receive_message, DECLINE)) {

        show_MessageStyle(receive_message);
        //sendData(ACCEPT); // i am ready to get your next message
        receiveData(receive_message);

    }
}

void show_headStyle(const char *ID, const char type_flag) {

    // X40
    if (type_flag == 'S') {
        printf("________________________________________ %s _______________________________________\n", ID);
    } else if (type_flag == 'P') {
        printf("P________________________________________ %s _______________________________________V\n", ID);
    } else if (type_flag == 'G') {
        printf("G________________________________________ %s _______________________________________P\n", ID);
    } else if (type_flag == 'C') {
        printf("C________________________________________ %s _______________________________________H\n", ID);
    }
    // X20  ____________________
}

// --------------------------------------------------------------------------------------------------------------------------------------
// Start chatting
int get_list(char listType_flag) {

    char list_line[MaxBufSize];
    int count = 1;

    list[0] = '\0'; // at first clean list

    if (listType_flag == 'P') { strapp(list, "Your privet chats list:\n"); }
    if (listType_flag == 'G') { strapp(list, "Your groups list:\n"); }
    if (listType_flag == 'C') { strapp(list, "Your channels list:\n"); }


    while (1) {

        receiveData(buffer);
        if (streq(buffer, DECLINE)) {
            break;
        }

        sprintf(list_line, "%d. %s\n", count, buffer + 1); // first character is extra
        count++;

        strapp(list, list_line);
    }
    sprintf(list_line, "%d. Back to main menu\n", count);
    strapp(list, list_line);

    return count;

}

void show_commandList() {

    printf(
            "________________________________________________________________________________\n"
            "Commands:\n"
            "\"/exit\" for exit this chatroom,\n"
            "\"/delete [Message Number]\" for delete the message,\n"
            "\"/edit [Message Number] [Edited Text]\" for edit the message\n"
            "Notice: if you want to write a slash at the beginning of your message use \"//\"\n"
            "________________________________________________________________________________\n"
    );
}

// Saved Messages
void savedMessages() {

    sendData("savedMessages");
    char message[MaxBufSize];

    while (1) {

        system("cls");
        show_headStyle("Saved Messages", 'S');
        show_previousMessages();

        show_commandList();
        getstr(message);
        if (streq(message, "/exit")) {
            sendData(DECLINE);
            return;
        }
        sendData(message);
    }
}

// Private chats
void goToPvChat() {

    char pvChat_id[MaxBufSize], message[MaxBufSize];
    receiveData(pvChat_id);

    while (1) {

        system("cls");
        show_headStyle(pvChat_id, 'P');
        show_previousMessages();

        show_commandList();
        getstr(message);
        if (streq(message, "/exit")) {
            //Sleep(WaitTOSendMessage);
            sendData(DECLINE);
            return;
        }
        sendData(message);
    }

}

void pvChats() {

    sendData("pvChats");

    int exitBy;
    exitBy = get_list('P');

    int mood;
    while (1) {

        mood = -1;
        system("cls");
        printf(list); // here is groups lists

        scanf("%d", &mood);
        clean();

        if (mood >= 1 && mood < exitBy) {

            sprintf(buffer, "%d", mood);
            sendData(buffer);
            goToPvChat();
        } else if (mood == exitBy) {

            sendData(DECLINE);
            return;
        } else {

            printf("Invalid command please press any key to try again...\n");
            clean();
        }

    }

}

// chat into Groups
void goToGroup() {

    char group_id[MaxBufSize], message[MaxBufSize];
    receiveData(group_id);

    while (1) {

        system("cls");
        show_headStyle(group_id, 'G');
        show_previousMessages();

        show_commandList();
        getstr(message);
        if (streq(message, "/exit")) {
            //Sleep(WaitTOSendMessage);
            sendData(DECLINE);
            return;
        }
        sendData(message);
    }


}

void groups() {

    sendData("groups");

    int exitBy;
    exitBy = get_list('G');

    int mood;
    while (1) {

        mood = -1;
        system("cls");
        printf(list); // here is groups lists

        scanf("%d", &mood);
        clean();

        if (mood >= 1 && mood < exitBy) {

            sprintf(buffer, "%d", mood);
            sendData(buffer);
            goToGroup();
        } else if (mood == exitBy) {

            sendData(DECLINE);
            return;
        } else {

            printf("Invalid command please press any key to try again...\n");
            clean();
        }

    }

}

// chat into channels
void goToChannel() {

    char channel_id[MaxBufSize], message[MaxBufSize];
    receiveData(channel_id);

    receiveData(buffer);
    char userStatus = buffer[0];


    while (1) {

        system("cls");
        show_headStyle(channel_id, 'C');
        show_previousMessages();

        show_commandList();
        getstr(message);
        if (streq(message, "/exit")) {
            //Sleep(WaitTOSendMessage);
            sendData(DECLINE);
            return;
        } else if (streq(message, "")) {
            sendData(message);
        }


        if (userStatus != 'M') {
            sendData(message);
        } else {
            printf("You can not sendData anything to channel until you are member...\n");
            clean();
            sendData("");
        }

    }


}

void channels() {

    sendData("channels");

    int exitBy;
    exitBy = get_list('C');

    int mood;
    while (1) {

        mood = -1;
        system("cls");
        printf(list); // here is groups lists

        scanf("%d", &mood);
        clean();

        if (mood >= 1 && mood < exitBy) {

            sprintf(buffer, "%d", mood);
            sendData(buffer);
            goToChannel();
        } else if (mood == exitBy) {

            sendData(DECLINE);
            return;
        } else {

            printf("Invalid command please press any key to try again...\n");
            clean();
        }

    }


}

// --------------------------------------------------------------------------------------------------------------------------------------
// Contacts
void show_Contactlist() {

    printf("Your contacts list:\n");

    while (1) {

        receiveData(buffer);
        if (streq(buffer, DECLINE)) {
            break;
        }

        printf("%s\n", buffer + 1);
    }

    printf(
            "1. New contact\n"
            "2. Delete contact\n"
            "3. Back to main menu\n"
    );

}

void get_Contactlist() {

    char list_line[MaxBufSize];
    list[0] = '\0'; // at first clean list

    while (1) {

        receiveData(buffer);
        if (streq(buffer, DECLINE)) {
            break;
        }

        sprintf(list_line, "%s\n", buffer + 1); // first character is extra
        strapp(list, list_line);
    }
    sprintf(list_line,
            "1. New contact\n"
            "2. Back to main menu\n");
    strapp(list, list_line);

}

void add_contact() {

    char intended_user[MaxBufSize];
    int res;

    while (1) {
        system("cls");
        printf("Please enter your intended Username (You can use a-z, 0-9 and underscores): ");

        res = getID(intended_user);
        if (res == 0) {
            printf("Invalid username, please try again...");
            clean();
            continue;
        }

        sendData(intended_user);
        receiveData(buffer);

        if (streq(buffer, DECLINE)) {
            printf("Username do not exist, please try again...");
            clean();
        } else if (streq(buffer, ACCEPT)) {
            printf("The contact with ID \"%s\" successfully add in your contact list...\n", intended_user);
            clean();
            sendData(ACCEPT);
            return;
        } else if (streq(buffer, "D")) {
            printf("You already added the contact with ID \"%s\" to your contact list...\n", intended_user);
            clean();
            sendData(ACCEPT);
            return;
        }
    }
}

void delete_contact() {

    char intended_user[MaxBufSize];
    int res;

    while (1) {
        system("cls");
        printf("Please enter your intended Username (You can use a-z, 0-9 and underscores): ");

        res = getID(intended_user);
        if (res == 0) {
            printf("Invalid username, please try again...");
            clean();
            continue;
        }

        sendData(intended_user);
        receiveData(buffer);

        if (streq(buffer, DECLINE)) {
            printf("Username do not exist in your contact list, please try again...");
            clean();
        } else if (streq(buffer, ACCEPT)) {
            printf("The contact with ID \"%s\" successfully delete from your contact list...\n", intended_user);
            clean();
            sendData(ACCEPT);
            return;
        }
    }
}

void contacts() {

    sendData("contacts");

    int mood;
    while (1) {

        mood = -1;
        system("cls");
        show_Contactlist(); // here is contact list

        scanf("%d", &mood);
        clean();

        if (mood == 1) {

            sprintf(buffer, "%d", mood);
            sendData(buffer);
            add_contact();
        } else if (mood == 2) {

            sprintf(buffer, "%d", mood);
            sendData(buffer);
            delete_contact();
        } else if (mood == 3) {

            sendData(DECLINE);
            return;
        } else {

            printf("Invalid command please press any key to try again...\n");
            clean();
            sendData("R"); // retry
        }

    }
}

// --------------------------------------------------------------------------------------------------------------------------------------
// main menu
int main_menu() {

    int mood;
    while (1) {

        mood = -1;
        system("cls");
        printf(

                "********** Welcome %s! **********\n"
                "What\'s up!?\n"
                "1. Saved Messages\n"
                "2. Private chats\n"
                "3. Groups\n"
                "4. Channels\n"
                "5. Contacts\n"
                "6. Create\n"
                "7. Join\n"
                "8. Log out\n"
                "9. Exit\n", UserName);

        scanf("%d", &mood);
        clean();

        if (mood == 1) {

            savedMessages();
        } else if (mood == 2) {

            pvChats();
        } else if (mood == 3) {

            groups();
        } else if (mood == 4) {

            channels();
        } else if (mood == 5) {

            contacts();
        } else if (mood == 6) {

            create_menu();
        } else if (mood == 7) {

            join_menu();
        } else if (mood == 8) {

            return 1; // start from login menu
        } else if (mood == 9) {

            return 0; // exit the programm
        } else {
            printf("Invalid command please press any key to try again...\n");
            clean();
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------
// main, sweet main ;)
int main(int args, char **strs) {

    if (is_doneOnce == FALSE) {
        //Listening_port = atoi(strs[1]);
        is_doneOnce = TRUE;
    }

    makeSocket();

    int loginMenu_continue, mainMenu_continue;
    while (1) {

        loginMenu_continue = login_menu();
        if (loginMenu_continue == 0)break;

        mainMenu_continue = main_menu();
        if (mainMenu_continue == 0)break;

    }

    sendData("off");// off the server

    // At last close down everything
    closesocket(sock);
    WSACleanup();

    return 0;
}
