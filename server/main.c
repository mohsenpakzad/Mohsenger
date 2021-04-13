#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ws2tcpip.h>
#include<time.h>
#include"sqlite3.h"

#define MaxBufSize 1000 + 1
#define MaxListElements 1000
#define WaitTOSendMessage 50

#define ACCEPT "1"
#define DECLINE "0"

#define TRUE 1
#define FALSE 0

#define IsNumberChar(n) ( n >= '0' && n <= '9')

SOCKET listening;
SOCKET clientSocket;

sqlite3 *Database;

int Listening_port = 54000;

const char database_dir[] = "Database";
const char DatabaseFile_name[] = "Database.db";

char buffer[MaxBufSize];
char UserName[MaxBufSize];
char list[MaxListElements][MaxBufSize];

int is_doneOnce = FALSE;


//--------------------------------------------------------------------------------------------------------------------------------------
// General functions
int streq(const char *str1, const char *str2) {

    for (int i = 0; str1[i] != '\0' || str2[i] != '\0'; i++) {

        if (str1[i] != str2[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

int strstart(const char *main_str, const char *start_str) {

    for (int i = 0; start_str[i] != '\0'; i++) {

        if (main_str[i] != start_str[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

int atoi(const char *str) {

    int result = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        result = result * 10 + (str[i] - '0');
    }
    return result;
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Get time and date
void get_messageTime(char *destination, int size) {

    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(destination, size, "%I:%M%p", timeinfo);
}

void get_createDate(char *destination, int size) {

    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(destination, size, "%B%e,%G", timeinfo);
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Connecting to client
void makeMainSocket() {

    // Initialze winsock
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);

    int wsOk = WSAStartup(ver, &wsData);
    if (wsOk != 0) {
        printf("Can't Initialize winsock! Quitting \n");
        return;
    }

    // Create a socket
    listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == INVALID_SOCKET) {
        printf("Can't create a socket! Quitting \n");
        return;
    }

    // Bind the ip address and port to a socket
    struct sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(Listening_port);
    hint.sin_addr.S_un.S_addr = INADDR_ANY; // Could also use inet_pton ....

    bind(listening, (struct sockaddr *) &hint, sizeof(hint));

    // Tell Winsock the socket is for listening
    listen(listening, SOMAXCONN);

}

void waitConnection() {
    // Wait for a connection
    struct sockaddr_in client;
    int clientSize = sizeof(client);

    printf("Try to connecting to client...");
    clientSocket = accept(listening, (struct sockaddr *) &client, &clientSize);
    printf("\rClient connected!                                       \n");


    char host[NI_MAXHOST];        // Client's remote name
    char service[NI_MAXSERV];    // Service (i.e. port) the client is connect on

    ZeroMemory(host, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
    ZeroMemory(service, NI_MAXSERV);

    /*if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
        printf("%s connected on port %s \n", host, service);
    }

    else {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        printf("%s connected on port %s \n", host, ntohs(client.sin_port));
    }*/



}

//--------------------------------------------------------------------------------------------------------------------------------------
// Database
void openDatabase() {

    int res;
    char temp[MaxBufSize];

    sprintf(temp, "./%s/%s", database_dir, DatabaseFile_name);
    res = sqlite3_open(temp, &Database);

    if (res != 0) {
        printf("Database: Can't open \"%s\": %s\n", DatabaseFile_name, sqlite3_errmsg(Database));
    } else {
        printf("Database: \"%s\" Opened successfully\n", DatabaseFile_name);
    }

    // Data table
    /*
    "CREATE TABLE [*Data*]("
    "[ID] TEXT PRIMARY KEY NOT NULL UNIQUE, "
    "[isUser] BOOL DEFAULT 0, "
    "[isGroup] BOOL DEFAULT 0, "
    "[isChannel] BOOL DEFAULT 0,"
    "[Password] TEXT);"
    */
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Send and receiveData functions
void sendData(const char *serverOutput) {

    int sendResult;

    printf("Server sendData: \"%s\" \n", serverOutput);
    sendResult = send(clientSocket, serverOutput, strlen(serverOutput) + 1, 0);

    if (sendResult == SOCKET_ERROR) {
        // start again
        printf("Client disconnected!\n");
        int main();
        main();
        exit(0);
    }
}

void receiveData(char *serverInput) {

    int bytesReceived;

    ZeroMemory(serverInput, MaxBufSize);
    bytesReceived = recv(clientSocket, serverInput, MaxBufSize, 0);
    printf("Client sendData: \"%s\" \n", serverInput);

    if (bytesReceived == SOCKET_ERROR) {
        // start again
        printf("Client disconnected!\n");
        int main();
        main();
        exit(0);
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Is exist in Data
int exist_callback(void *data, int argsNum, char **value, char **columnName) {
    *(int *) data = TRUE;
    return 0;
}

int is_exist(const char *ID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT * FROM [*Data*] "
            "WHERE ID = '%s'", ID
    );

    int isExist = FALSE;
    int *isExist_pt = &isExist;

    res = sqlite3_exec(Database, command, exist_callback, isExist_pt, &errMsg); // is exist?
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
        return -1;
    }
    if (isExist == FALSE) {
        return FALSE;
    }
    return TRUE;
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Signin fucntions
void addUser(const char *user_name) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "INSERT INTO [*Data*] (ID, isUser) VALUES"
            "('%s',1);", user_name
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }

}

void give_userPassword(const char *user_name, const char *password) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "UPDATE [*Data*] "
            "SET Password = '%s' "
            "WHERE ID = '%s';", password, user_name
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }
}

void createUser_table(const char *user_name) {

    char command[MaxBufSize], date[MaxBufSize];
    char *errMsg;
    int res;

    get_createDate(date, MaxBufSize);

    sprintf(command,

            "CREATE TABLE [%s]("
            "[SubIDs] TEXT NOT NULL,"
            "[isContact] VARCHAR(1) DEFAULT 'N',"
            "[isPvChat] VARCHAR(1) DEFAULT 'N',"
            "[isGroup] VARCHAR(1) DEFAULT 'N',"
            "[isChannel] VARCHAR(1) DEFAULT 'N',"
            "[isMessage] VARCHAR(1) DEFAULT 'N',"
            "[MessageNumber] INT,"
            "[Time] VARCHAR(10),"
            "[Message] TEXT); ", user_name

    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }


    sprintf(command,
            "INSERT INTO [%s] (SubIDs, isMessage, MessageNumber, Time, Message) VALUES"
            "('*system*', 'Y', 0, '0', 'Saved Messages created on %s');", user_name, date
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }
}

void signin() {

    char password[MaxBufSize];
    int res;
    do {
        receiveData(UserName);
        res = is_exist(UserName);

        if (res == TRUE) {
            sendData(DECLINE);
        } else if (res == FALSE) {
            sendData(ACCEPT);
        }

    } while (res == TRUE);

    addUser(UserName);

    receiveData(password);
    give_userPassword(UserName, password);
    createUser_table(UserName);

    sendData(ACCEPT); // user successfully created

}

//--------------------------------------------------------------------------------------------------------------------------------------
// Login functions
int whatis_callback(void *data, int argsNum, char **value, char **columnName) {
    *(char *) data = (*value)[0]; // as known, all isUser,isGroup and isChannel is boolean
    return 0;
}

int is_userID(const char *ID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT (isUser) FROM [*Data*] "
            "WHERE ID = '%s'", ID
    );

    char isUser;
    char *isUser_pt = &isUser;

    res = sqlite3_exec(Database, command, whatis_callback, isUser_pt, &errMsg); // now, give the password
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
        return -1;
    }

    if (isUser == '0') { // these are diffrent from accept and decline!!!
        return FALSE;
    } else if (isUser == '1') {
        return TRUE;
    }
}

int password_callback(void *data, int argsNum, char **value, char **columnName) {
    strcpy((char *) data, *value);
    return 0;
}

void get_UserPassword(const char *user_name, char *password_buf) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT (Password) FROM [*Data*] "
            "WHERE ID = '%s'", user_name
    );

    res = sqlite3_exec(Database, command, password_callback, password_buf, &errMsg); // now, give the password
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }

}

void login() {

    char password[MaxBufSize], input_password[MaxBufSize];
    int res;

    do {
        receiveData(UserName);
        res = is_exist(UserName) && is_userID(UserName);

        if (res == FALSE) {
            sendData(DECLINE);
        } else if (res == TRUE) {
            sendData(ACCEPT);
        }

    } while (res == FALSE);

    get_UserPassword(UserName, password);

    do {
        receiveData(input_password);
        res = streq(password, input_password);

        if (res == FALSE) {
            sendData(DECLINE);
        } else {
            sendData(ACCEPT);
        }

    } while (res == FALSE);

    sendData(ACCEPT); // user successfully logged in
}

//--------------------------------------------------------------------------------------------------------------------------------------
//  Create
void insert_member(const char *pv_group_channel_ID, const char *subID, const char *isMember_flag) {

    char command[MaxBufSize];
    char *errMsg;
    int res;


    sprintf(command,
            "INSERT INTO [%s] (SubIDs, isMember) VALUES"
            "('%s','%s');", pv_group_channel_ID, subID, isMember_flag
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }

}

int is_ExistInUser(const char *ID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT * FROM [%s] "
            "WHERE SubIDs = '%s'", UserName, ID
    );

    int isExist = FALSE;
    int *isExist_pt = &isExist;

    res = sqlite3_exec(Database, command, exist_callback, isExist_pt, &errMsg); // is exist?
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
        return -1;
    }
    if (isExist == FALSE) {
        return FALSE;
    }
    return TRUE;

}

// Add pvChat
int is_pvChatExistInUser(const char *userName1, const char *userName2) {

    char possible_ID1[MaxBufSize];
    char possible_ID2[MaxBufSize];

    sprintf(possible_ID1, "%s&%s", userName1, userName2);
    sprintf(possible_ID2, "%s&%s", userName2, userName1);

    int res;

    res = is_ExistInUser(possible_ID1);
    if (res == 1) {
        return TRUE;
    }

    res = is_ExistInUser(possible_ID2);
    if (res == 1) {
        return TRUE;
    }
    return FALSE;

}

void add_pvChat(const char *userName1, const char *userName2) {

    char command[MaxBufSize], date[MaxBufSize];
    char *errMsg;
    int res;

    get_createDate(date, MaxBufSize);

    sprintf(command,

            "CREATE TABLE [%s&%s]("
            "[SubIDs] TEXT NOT NULL,"
            "[isMember] VARCHAR(1) DEFAULT 'N',"
            "[isMessage] VARCHAR(1) DEFAULT 'N',"
            "[MessageNumber] INT,"
            "[Time] VARCHAR(10),"
            "[Message] TEXT); ", userName1, userName2
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
        return;

    }

    sprintf(command, "%s&%s", userName1, userName2);
    insert_member(command, userName1, "Y");
    insert_member(command, userName2, "Y");


    sprintf(command,
            "INSERT INTO [%s&%s] (SubIDs, isMessage, MessageNumber, Time, Message) VALUES"
            "('*system*', 'Y', 0, '0', '%s created the private chat with %s on %s');", userName1, userName2, userName1,
            userName2, date
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }

}

void add_pvChatToUsers(const char *userName1, const char *userName2) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "INSERT INTO [%s] (SubIDs, isPvChat) VALUES"
            "('%s&%s','Y');", userName1, userName1, userName2
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }

    sprintf(command,
            "INSERT INTO [%s] (SubIDs, isPvChat) VALUES"
            "('%s&%s','Y');", userName2, userName1, userName2
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }
}

void create_pvChat() {

    char destination_user[MaxBufSize];
    int res;

    do {
        receiveData(destination_user);
        if (streq(destination_user, UserName)) {
            sendData("N"); // Privet chat with self notice
            return;
        }

        res = is_exist(destination_user) && is_userID(destination_user);

        if (res == FALSE) {
            sendData(DECLINE);
        } else if (res == TRUE) {
            sendData(ACCEPT);
        }

    } while (res == FALSE);

    res = is_pvChatExistInUser(UserName, destination_user);

    if (res == TRUE) { // if exist, return
        sendData(DECLINE);
        return;
    } else if (res == FALSE) {
        sendData(ACCEPT);
    }

    add_pvChat(UserName, destination_user);
    add_pvChatToUsers(UserName, destination_user);
    sendData(ACCEPT); // privet chat created successfully
}

//  Add group
void add_group(const char *group_id) {

    char command[MaxBufSize], date[MaxBufSize];
    char *errMsg;
    int res;

    get_createDate(date, MaxBufSize);

    sprintf(command,

            "CREATE TABLE [%s]("
            "[SubIDs] TEXT NOT NULL,"
            "[isMember] VARCHAR(1) DEFAULT 'N',"
            "[isMessage] VARCHAR(1) DEFAULT 'N',"
            "[MessageNumber] INT,"
            "[Time] VARCHAR(10),"
            "[Message] TEXT); ", group_id
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
        return;

    }

    insert_member(group_id, UserName, "C");

    sprintf(command,

            "INSERT INTO [%s] (SubIDs, isMessage, MessageNumber, Time, Message) VALUES"
            "('*system*', 'Y', 0, '0', '%s created the group on %s');", group_id, UserName, date
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }
}

void add_groupToData(const char *group_id) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "INSERT INTO [*Data*] (ID, isGroup) VALUES"
            "('%s',1);", group_id
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }
}

void add_groupToUser(const char *group_id, const char *status) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "INSERT INTO [%s] (SubIDs, isGroup) VALUES"
            "('%s','%s');", UserName, group_id, status
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }
}

void create_group() {

    char group_id[MaxBufSize];
    int res;
    do {
        receiveData(group_id);
        res = is_exist(group_id);

        if (res == TRUE) { // the same id found, so bad!
            sendData(DECLINE);
        } else if (res == FALSE) { // no same id found
            sendData(ACCEPT);
        }

    } while (res == TRUE);

    add_group(group_id);
    add_groupToData(group_id);
    add_groupToUser(group_id, "C"); // as a creator
    sendData(ACCEPT); // group successfully created

}

// Add channel
void add_channel(const char *channel_id) {

    char command[MaxBufSize], date[MaxBufSize];
    char *errMsg;
    int res;

    get_createDate(date, MaxBufSize);

    sprintf(command,

            "CREATE TABLE [%s]("
            "[SubIDs] TEXT NOT NULL,"
            "[isMember] VARCHAR(1) DEFAULT 'N',"
            "[isMessage] VARCHAR(1) DEFAULT 'N',"
            "[MessageNumber] INT,"
            "[Time] VARCHAR(10),"
            "[Message] TEXT); ", channel_id
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
        return;

    }

    insert_member(channel_id, UserName, "C");

    sprintf(command,

            "INSERT INTO [%s] (SubIDs, isMessage, MessageNumber, Time, Message) VALUES"
            "('*system*', 'Y', 0, '0', '%s created the channel on %s');", channel_id, UserName, date
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }
}

void add_channelToData(const char *channel_id) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "INSERT INTO [*Data*] (ID, isChannel) VALUES"
            "('%s',1);", channel_id
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }
}

void add_channelToUser(const char *channel_id, const char *status) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "INSERT INTO [%s] (SubIDs, isChannel) VALUES"
            "('%s','%s');", UserName, channel_id, status
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }
}

void create_channel() {

    char channel_id[MaxBufSize];
    int res;
    do {
        receiveData(channel_id);
        res = is_exist(channel_id);

        if (res == TRUE) { // the same id found, so bad!
            sendData(DECLINE);
        } else if (res == FALSE) { // no same id found
            sendData(ACCEPT);
        }

    } while (res == TRUE);

    add_channel(channel_id);
    add_channelToData(channel_id);
    add_channelToUser(channel_id, "C");
    sendData(ACCEPT); // channel successfully created

}
// --------------------------------------------------------------------------------------------------------------------------------------
//  Join

// Join group
int is_groupID(const char *ID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT (isGroup) FROM [*Data*] "
            "WHERE ID = '%s'", ID
    );

    char isGroup;
    char *isGroup_pt = &isGroup;

    res = sqlite3_exec(Database, command, whatis_callback, isGroup_pt, &errMsg); // now, give the password
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
        return -1;
    }

    // these are diffrent from accept and decline!!!
    if (isGroup == '0') {  // if not group
        return FALSE;
    } else if (isGroup == '1') { // if is group
        return TRUE;
    }
}

void joinGroup_message(const char *group_ID, const char *subID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "INSERT INTO [%s] (SubIDs, isMessage, MessageNumber, Time, Message) VALUES"
            "('*system*', 'Y', 0, '0', '%s joined the group');", group_ID, subID
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }

}

void join_group() {

    char group_id[MaxBufSize];
    int res;
    do {
        receiveData(group_id);
        res = is_ExistInUser(group_id);
        if (res == TRUE) {
            sendData("D"); // duplicate join request!
            return;
        }

        res = is_exist(group_id) && is_groupID(group_id);

        if (res == FALSE) { // the id is not found, so bad!
            sendData(DECLINE);
        } else if (res == TRUE) { // id found
            sendData(ACCEPT);
        }

    } while (res == FALSE);

    insert_member(group_id, UserName, "M");
    joinGroup_message(group_id, UserName);
    add_groupToUser(group_id, "M");
    sendData(ACCEPT); // group successfully created
}

// Join channel
int is_channelID(const char *ID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT (isChannel) FROM [*Data*] "
            "WHERE ID = '%s'", ID
    );

    char isChannel;
    char *isChannel_pt = &isChannel;

    res = sqlite3_exec(Database, command, whatis_callback, isChannel_pt, &errMsg); // now, give the password
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
        return -1;
    }

    // these are diffrent from accept and decline!!!
    if (isChannel == '0') {  // if not channel
        return FALSE;
    } else if (isChannel == '1') { // if is channel
        return TRUE;
    }
}

void join_channel() {

    char channel_id[MaxBufSize];
    int res;
    do {
        receiveData(channel_id);
        res = is_ExistInUser(channel_id);
        if (res == TRUE) {
            sendData("D"); // duplicate join request!
            return;
        }

        res = is_exist(channel_id) && is_channelID(channel_id);

        if (res == FALSE) { // the id is not found, so bad!
            sendData(DECLINE);
        } else if (res == TRUE) { // id found
            sendData(ACCEPT);
        }

    } while (res == FALSE);

    insert_member(channel_id, UserName, "M");
    add_channelToUser(channel_id, "M");
    sendData(ACCEPT); // group successfully created
}
//--------------------------------------------------------------------------------------------------------------------------------------
// Start Chatting

// find message number
int messageNum_callback(void *data, int argsNum, char **value, char **columnName) {

    *(int *) data = atoi(*value);
    return 0;
}

int find_userLastMessageNum(const char *user_pv_group_channel_id) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT MessageNumber FROM [%s] "
            "WHERE isMessage = 'Y' AND SubIDs = '%s'", user_pv_group_channel_id,
            UserName // this for show only user message, for see all messages use WHERE isSavedMessage = 'Y'
    );

    int messageNum = 0; // if user did not sendData anything return 0 to start from 1
    int *messageNum_pt = &messageNum;

    res = sqlite3_exec(Database, command, messageNum_callback, messageNum_pt, &errMsg);

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }
    return messageNum;

}

char get_userStatus(const char *pv_group_channel_id, const char *userID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT isMember FROM [%s] "
            "WHERE isMessage = 'N' AND SubIDs = '%s'", pv_group_channel_id, userID
    );

    char status;
    char *status_pt = &status;

    res = sqlite3_exec(Database, command, whatis_callback, status_pt, &errMsg);

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }

    return status;
}

// sendData message
int sendMessages_callback(void *data, int argsNum, char **value, char **columnName) {

    char ID[MaxBufSize], time[MaxBufSize], message[MaxBufSize];
    char status;
    int messageNum;

    int res;
    res = is_userID((const char *) data); // data here is name of user_pv_group_channel
    if (res == TRUE) {
        status = 'Y';
    } else if (value[0][0] == '*') { // if system message
        status = 'N';
    } else if (res == FALSE) {
        status = get_userStatus((const char *) data, value[0]);
    }

    strcpy(ID, value[0]);
    messageNum = atoi(value[1]);
    strcpy(time, value[2]);
    strcpy(message, value[3]);

    char chat_records[MaxBufSize];
    sprintf(chat_records, "%s %c %d %s %s", ID, status, messageNum, time, message);

    sendData(chat_records);
    Sleep(WaitTOSendMessage); // some wait to client can clearly recive previous message
    //receiveData(buffer);
    return 0;
}

void send_allMessages(const char *user_pv_group_channel_id) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT SubIDs, MessageNumber, Time, Message FROM [%s] "
            "WHERE isMessage = 'Y'",
            user_pv_group_channel_id  // this only for see all messages , for see only user message user use WHERE SubIDs = UserName
    );

    res = sqlite3_exec(Database, command, sendMessages_callback, (void *) user_pv_group_channel_id, &errMsg);
    sendData(DECLINE); // that is it!

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }
}

// sendData list callback
int sendList_callback(void *data, int argsNum, char **value, char **columnName) {

    char ID[MaxBufSize];
    sprintf(ID, ">%s", *value);

    int index = (*(int *) data);
    strcpy(list[index], *value);
    (*(int *) data)++;

    Sleep(WaitTOSendMessage); // some wait to client can clearly recive previous message
    sendData(ID);
    return 0;
}

// add message
void add_Message(const char *user_pv_group_channel_id, const char *input_message, int messageNum) {

    char command[MaxBufSize], time[MaxBufSize];
    char *errMsg;
    int res;

    get_messageTime(time, MaxBufSize);

    sprintf(command,
            "INSERT INTO [%s] (SubIDs, isMessage, MessageNumber,Time, Message) VALUES"
            "('%s', 'Y', %d, '%s', '%s');", user_pv_group_channel_id, UserName, messageNum, time, input_message
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }

}

// do command
void delete_message(const char *user_pv_group_channel_id, const char *input_message) {

    int messageNum = 0;
    sscanf(input_message, "%d", &messageNum);

    if (messageNum <= 0) { // Nope!
        return;
    }

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,

            "DELETE FROM [%s] "
            "WHERE isMessage = 'Y' AND SubIDs = '%s' AND MessageNumber = %d;", user_pv_group_channel_id, UserName,
            messageNum
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }

}

void edit_message(const char *user_pv_group_channel_id, const char *input_message) {

    char updated_text[MaxBufSize] = "";
    int messageNum = 0;
    sscanf(input_message, "%d %[^\n]s", &messageNum, updated_text);

    if (messageNum <= 0) { // Nope!
        return;
    }
    if (streq(updated_text, "")) {
        return;
    }

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "UPDATE [%s] "
            "SET Message = '%s' "
            "WHERE isMessage = 'Y' AND SubIDs = '%s' AND MessageNumber = %d;", user_pv_group_channel_id, updated_text,
            UserName, messageNum
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }

}

void do_command(const char *input_message, const char *user_pv_group_channel_id) {


    if (strstart(input_message, "/delete ")) {

        delete_message(user_pv_group_channel_id,
                       input_message + 8); //example: /delete 1234 --> should ignore 8 characters
    } else if (strstart(input_message, "/edit ")) {

        edit_message(user_pv_group_channel_id, input_message + 6);
    }
    // members list
    // promit to admin
    // admins can delete member message
}

// Saved Messages
void savedMessages() {

    char input_message[MaxBufSize];
    int startMessageNum = find_userLastMessageNum(UserName) + 1;

    while (!streq(input_message, DECLINE)) {

        send_allMessages(UserName);

        receiveData(input_message);
        if (streq(input_message, DECLINE)) {
            return;
        } else if (streq(input_message, "")) {
            continue;
        } else if (input_message[0] == '/' && input_message[1] == '/') {
            add_Message(UserName, input_message + 1, startMessageNum);
            startMessageNum++;
        } else if (input_message[0] == '/') {
            do_command(input_message, UserName);
        } else {
            add_Message(UserName, input_message, startMessageNum);
            startMessageNum++;
        }
    }
}

// Pv chats
void find_pvOpponent(char *full_pvChatID, char *opponent_destination) {

    char user1[MaxBufSize], user2[MaxBufSize];

    int i, count = 0;

    for (i = 0; full_pvChatID[count] != '&'; i++) {

        user1[i] = full_pvChatID[count];
        count++;
    }
    count++;
    user1[i] = '\0';

    for (i = 0; full_pvChatID[count] != '\0'; i++) {

        user2[i] = full_pvChatID[count];
        count++;
    }
    user2[i] = '\0';

    if (streq(user1, UserName)) {
        strcpy(opponent_destination, user2);
    } else if (streq(user2, UserName)) {
        strcpy(opponent_destination, user1);
    }


}

int sendPvChatList_callback(void *data, int argsNum, char **value, char **columnName) {

    char opponent_ID[MaxBufSize];
    find_pvOpponent(*value, buffer);
    sprintf(opponent_ID, ">%s", buffer);

    int index = (*(int *) data);
    strcpy(list[index], *value);
    (*(int *) data)++;

    Sleep(WaitTOSendMessage); // some wait to client can clearly recive previous message
    sendData(opponent_ID);
    return 0;
}

int send_pvChatList() {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT SubIDs FROM [%s] "
            "WHERE isPvChat = 'Y'", UserName
    );

    int pvChatsNum = 0;
    int *pvChatsNum_pt = &pvChatsNum;

    res = sqlite3_exec(Database, command, sendPvChatList_callback, pvChatsNum_pt, &errMsg);
    sendData(DECLINE); // that is it!

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }

    return pvChatsNum;
}

void goto_pvChat(const char *pvChat_id) {

    Sleep(WaitTOSendMessage);
    sendData(pvChat_id);

    char input_message[MaxBufSize];
    int startMessageNum = find_userLastMessageNum(pvChat_id) + 1;

    while (1) {

        send_allMessages(pvChat_id);

        receiveData(input_message);
        if (streq(input_message, DECLINE)) {
            return;
        } else if (streq(input_message, "")) {
            continue;
        } else if (input_message[0] == '/' && input_message[1] == '/') {
            add_Message(pvChat_id, input_message + 1, startMessageNum);
            startMessageNum++;
        } else if (input_message[0] == '/') {
            do_command(input_message, pvChat_id);
        } else {
            add_Message(pvChat_id, input_message, startMessageNum);
            startMessageNum++;
        }
    }
}

void pvChats() {

    int PvChatsNum, mood;
    PvChatsNum = send_pvChatList();

    receiveData(buffer);

    while (!streq(buffer, DECLINE)) {

        mood = atoi(buffer);
        goto_pvChat(list[mood - 1]);

        receiveData(buffer);
    }
}

// Groups
int send_groupsList() {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT SubIDs FROM [%s] "
            "WHERE isGroup <> 'N'", UserName
    );

    int groupsNum = 0;
    int *groupsNum_pt = &groupsNum;

    res = sqlite3_exec(Database, command, sendList_callback, groupsNum_pt, &errMsg);
    sendData(DECLINE); // that is it!

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }

    return groupsNum;
}

void goto_group(const char *group_id) {

    Sleep(WaitTOSendMessage);
    sendData(group_id);

    char input_message[MaxBufSize];
    int startMessageNum = find_userLastMessageNum(group_id) + 1;

    while (1) {

        send_allMessages(group_id);

        receiveData(input_message);
        if (streq(input_message, DECLINE)) {
            return;
        } else if (streq(input_message, "")) {
            continue;
        } else if (input_message[0] == '/' && input_message[1] == '/') {
            add_Message(group_id, input_message + 1, startMessageNum);
            startMessageNum++;
        } else if (input_message[0] == '/') {
            do_command(input_message, group_id);
        } else {
            add_Message(group_id, input_message, startMessageNum);
            startMessageNum++;
        }
    }
}

void groups() {

    int groupsNum, mood;
    groupsNum = send_groupsList();

    receiveData(buffer);

    while (!streq(buffer, DECLINE)) {

        mood = atoi(buffer);
        goto_group(list[mood - 1]);

        receiveData(buffer);
    }

}

// Channels
int send_channelsList() {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT SubIDs FROM [%s] "
            "WHERE isChannel <> 'N'", UserName
    );

    int channelsNum = 0;
    int *channelsNum_pt = &channelsNum;

    res = sqlite3_exec(Database, command, sendList_callback, channelsNum_pt, &errMsg);
    sendData(DECLINE); // that is it!

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }

    return channelsNum;
}

void goto_channel(const char *channel_id) {

    Sleep(WaitTOSendMessage);
    sendData(channel_id);

    char status = get_userStatus(channel_id, UserName);
    sprintf(buffer, "%c", status);
    sendData(buffer);

    char input_message[MaxBufSize];
    int startMessageNum = find_userLastMessageNum(channel_id) + 1;

    while (1) {

        send_allMessages(channel_id);

        receiveData(input_message);
        if (streq(input_message, DECLINE)) {
            return;
        } else if (streq(input_message, "")) {
            continue;
        } else if (input_message[0] == '/' && input_message[1] == '/') {
            add_Message(channel_id, input_message + 1, startMessageNum);
            startMessageNum++;
        } else if (input_message[0] == '/') {
            do_command(input_message, channel_id);
        } else {
            add_Message(channel_id, input_message, startMessageNum);
            startMessageNum++;
        }
    }
}

void channels() {

    int channelsNum, mood;
    channelsNum = send_channelsList();

    receiveData(buffer);

    while (!streq(buffer, DECLINE)) {

        mood = atoi(buffer);
        goto_channel(list[mood - 1]);

        receiveData(buffer);
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Contacts
int sendContactList_callback(void *data, int argsNum, char **value, char **columnName) {

    char ID[MaxBufSize];
    sprintf(ID, ">%s", *value);

    Sleep(WaitTOSendMessage); // some wait to client can clearly recive previous message
    sendData(ID);
    return 0;
}

void send_contactsList() {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT SubIDs FROM [%s] "
            "WHERE isContact = 'Y'", UserName
    );

    res = sqlite3_exec(Database, command, sendContactList_callback, NULL, &errMsg);
    sendData(DECLINE); // that is it!

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }
}

int is_contactExistInUser(const char *ID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT * FROM [%s] "
            "WHERE isContact = 'Y' AND SubIDs = '%s'", UserName, ID
    );

    int is_contactExist = FALSE;
    int *is_contactExist_pt = &is_contactExist;

    res = sqlite3_exec(Database, command, exist_callback, is_contactExist_pt, &errMsg); // is exist?
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
        return -1;
    }
    if (is_contactExist == FALSE) {
        return FALSE;
    }
    return TRUE;

}

void add_contactToUser(const char *destination_userName) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "INSERT INTO [%s] (SubIDs, isContact) VALUES"
            "('%s','Y');", UserName, destination_userName
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }

}

void add_contact() {

    char destination_contact[MaxBufSize];
    int res;

    do {
        receiveData(destination_contact);
        res = is_contactExistInUser(destination_contact);
        if (res == TRUE) {
            sendData("D"); // duplicate join request!

            receiveData(buffer); // stay for user to accept commands
            if (streq(buffer, ACCEPT)) {
                return;
            }
        }

        res = is_exist(destination_contact) && is_userID(destination_contact);

        if (res == FALSE) {
            sendData(DECLINE);
        } else if (res == TRUE) {
            add_contactToUser(destination_contact);
            sendData(ACCEPT);
        }

    } while (res == FALSE);


    receiveData(buffer); // stay for user to accept commands
    if (streq(buffer, ACCEPT)) {
        return;
    }


}

void delete_contactFromUser(const char *destination_userName) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,

            "DELETE FROM [%s] "
            "WHERE isContact = 'Y' AND SubIDs = '%s';", UserName, destination_userName
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }

}

void delete_contact() {

    char destination_contact[MaxBufSize];
    int res;

    do {
        receiveData(destination_contact);
        res = is_contactExistInUser(destination_contact);

        if (res == FALSE) {
            sendData(DECLINE);
        } else if (res == TRUE) {
            delete_contactFromUser(destination_contact);
            sendData(ACCEPT);
        }

    } while (res == FALSE);


    receiveData(buffer); // stay for user to accept commands
    if (streq(buffer, ACCEPT)) {
        return;
    }


}

void contacts() {

    int mood;

    while (1) {

        send_contactsList();

        receiveData(buffer);
        if (streq(buffer, DECLINE)) {
            return;
        } else if (streq(buffer, "R")) { // retry
            continue;
        }


        mood = atoi(buffer);
        if (mood == 1) {
            add_contact();
        } else if (mood == 2) {
            delete_contact();
        }

    }
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Process responce
int process_responce() {

    if (streq(buffer, "signin")) {

        signin();
    } else if (streq(buffer, "login")) {

        login();
    } else if (streq(buffer, "create_pvChat")) {

        create_pvChat();
    } else if (streq(buffer, "create_group")) {

        create_group();
    } else if (streq(buffer, "create_channel")) {

        create_channel();
    } else if (streq(buffer, "join_group")) {

        join_group();
    } else if (streq(buffer, "join_channel")) {

        join_channel();
    } else if (streq(buffer, "savedMessages")) {

        savedMessages();
    } else if (streq(buffer, "pvChats")) {

        pvChats();
    } else if (streq(buffer, "groups")) {

        groups();
    } else if (streq(buffer, "channels")) {

        channels();
    } else if (streq(buffer, "contacts")) {

        contacts();
    } else if (streq(buffer, "off")) {

        return FALSE;
    } else {

        printf("There is an invalid response\n");
    }

    return TRUE;
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Main loop
void main_loop() {

    int serverRunning = TRUE;

    do {

        receiveData(buffer);
        serverRunning = process_responce(); //answer the response

    } while (serverRunning == TRUE);

}

//--------------------------------------------------------------------------------------------------------------------------------------
// Main... sweet main
int main(int args, char **strs) {

    // Connecting to client
    if (is_doneOnce == FALSE) {
        //Listening_port = atoi(strs[1]);
        openDatabase();
        makeMainSocket();
        is_doneOnce = TRUE;
    }

    waitConnection();
    //main loop
    main_loop();


    // make ready to program ending....
    closesocket(listening); // Close listening socket
    closesocket(clientSocket); // Close the socket
    WSACleanup(); // Cleanup winsock



    return 0;
}
