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
int strEquality(const char *str1, const char *str2) {

    for (int i = 0; str1[i] != '\0' || str2[i] != '\0'; i++) {

        if (str1[i] != str2[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

int strStart(const char *main_str, const char *start_str) {

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
void getMessageTime(char *destination, int size) {

    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(destination, size, "%I:%M%p", timeinfo);
}

void getCreateDate(char *destination, int size) {

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
int existCallback(void *data, int argsNum, char **value, char **columnName) {
    *(int *) data = TRUE;
    return 0;
}

int isExist(const char *ID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT * FROM [*Data*] "
            "WHERE ID = '%s'", ID
    );

    int isExist = FALSE;
    int *isExist_pt = &isExist;

    res = sqlite3_exec(Database, command, existCallback, isExist_pt, &errMsg); // is exist?
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

void giveUserPassword(const char *user_name, const char *password) {

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

void createUserTable(const char *user_name) {

    char command[MaxBufSize], date[MaxBufSize];
    char *errMsg;
    int res;

    getCreateDate(date, MaxBufSize);

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

void signup() {

    char password[MaxBufSize];
    int res;
    do {
        receiveData(UserName);
        res = isExist(UserName);

        if (res == TRUE) {
            sendData(DECLINE);
        } else if (res == FALSE) {
            sendData(ACCEPT);
        }

    } while (res == TRUE);

    addUser(UserName);

    receiveData(password);
    giveUserPassword(UserName, password);
    createUserTable(UserName);

    sendData(ACCEPT); // user successfully created

}

//--------------------------------------------------------------------------------------------------------------------------------------
// Login functions
int whatisCallback(void *data, int argsNum, char **value, char **columnName) {
    *(char *) data = (*value)[0]; // as known, all isUser,isGroup and isChannel is boolean
    return 0;
}

int isUserId(const char *ID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT (isUser) FROM [*Data*] "
            "WHERE ID = '%s'", ID
    );

    char isUser;
    char *isUser_pt = &isUser;

    res = sqlite3_exec(Database, command, whatisCallback, isUser_pt, &errMsg); // now, give the password
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

int passwordCallback(void *data, int argsNum, char **value, char **columnName) {
    strcpy((char *) data, *value);
    return 0;
}

void getUserPassword(const char *user_name, char *password_buf) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT (Password) FROM [*Data*] "
            "WHERE ID = '%s'", user_name
    );

    res = sqlite3_exec(Database, command, passwordCallback, password_buf, &errMsg); // now, give the password
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }

}

void login() {

    char password[MaxBufSize], input_password[MaxBufSize];
    int res;

    do {
        receiveData(UserName);
        res = isExist(UserName) && isUserId(UserName);

        if (res == FALSE) {
            sendData(DECLINE);
        } else if (res == TRUE) {
            sendData(ACCEPT);
        }

    } while (res == FALSE);

    getUserPassword(UserName, password);

    do {
        receiveData(input_password);
        res = strEquality(password, input_password);

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
void insertMember(const char *pv_group_channel_ID, const char *subID, const char *isMember_flag) {

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

int isExistInUser(const char *ID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT * FROM [%s] "
            "WHERE SubIDs = '%s'", UserName, ID
    );

    int isExist = FALSE;
    int *isExist_pt = &isExist;

    res = sqlite3_exec(Database, command, existCallback, isExist_pt, &errMsg); // is exist?
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
int isPvChatExistInUser(const char *userName1, const char *userName2) {

    char possible_ID1[MaxBufSize];
    char possible_ID2[MaxBufSize];

    sprintf(possible_ID1, "%s&%s", userName1, userName2);
    sprintf(possible_ID2, "%s&%s", userName2, userName1);

    int res;

    res = isExistInUser(possible_ID1);
    if (res == 1) {
        return TRUE;
    }

    res = isExistInUser(possible_ID2);
    if (res == 1) {
        return TRUE;
    }
    return FALSE;

}

void addPvChat(const char *userName1, const char *userName2) {

    char command[MaxBufSize], date[MaxBufSize];
    char *errMsg;
    int res;

    getCreateDate(date, MaxBufSize);

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
    insertMember(command, userName1, "Y");
    insertMember(command, userName2, "Y");


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

void addPvChatToUsers(const char *userName1, const char *userName2) {

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

void createPvChat() {

    char destination_user[MaxBufSize];
    int res;

    do {
        receiveData(destination_user);
        if (strEquality(destination_user, UserName)) {
            sendData("N"); // Privet chat with self notice
            return;
        }

        res = isExist(destination_user) && isUserId(destination_user);

        if (res == FALSE) {
            sendData(DECLINE);
        } else if (res == TRUE) {
            sendData(ACCEPT);
        }

    } while (res == FALSE);

    res = isPvChatExistInUser(UserName, destination_user);

    if (res == TRUE) { // if exist, return
        sendData(DECLINE);
        return;
    } else if (res == FALSE) {
        sendData(ACCEPT);
    }

    addPvChat(UserName, destination_user);
    addPvChatToUsers(UserName, destination_user);
    sendData(ACCEPT); // privet chat created successfully
}

//  Add group
void addGroup(const char *group_id) {

    char command[MaxBufSize], date[MaxBufSize];
    char *errMsg;
    int res;

    getCreateDate(date, MaxBufSize);

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

    insertMember(group_id, UserName, "C");

    sprintf(command,

            "INSERT INTO [%s] (SubIDs, isMessage, MessageNumber, Time, Message) VALUES"
            "('*system*', 'Y', 0, '0', '%s created the group on %s');", group_id, UserName, date
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }
}

void addGroupToData(const char *group_id) {

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

void addGroupToUser(const char *group_id, const char *status) {

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

void createGroup() {

    char group_id[MaxBufSize];
    int res;
    do {
        receiveData(group_id);
        res = isExist(group_id);

        if (res == TRUE) { // the same id found, so bad!
            sendData(DECLINE);
        } else if (res == FALSE) { // no same id found
            sendData(ACCEPT);
        }

    } while (res == TRUE);

    addGroup(group_id);
    addGroupToData(group_id);
    addGroupToUser(group_id, "C"); // as a creator
    sendData(ACCEPT); // group successfully created

}

// Add channel
void addChannel(const char *channel_id) {

    char command[MaxBufSize], date[MaxBufSize];
    char *errMsg;
    int res;

    getCreateDate(date, MaxBufSize);

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

    insertMember(channel_id, UserName, "C");

    sprintf(command,

            "INSERT INTO [%s] (SubIDs, isMessage, MessageNumber, Time, Message) VALUES"
            "('*system*', 'Y', 0, '0', '%s created the channel on %s');", channel_id, UserName, date
    );

    res = sqlite3_exec(Database, command, NULL, NULL, &errMsg);
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);

    }
}

void addChannelToData(const char *channel_id) {

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

void addChannelToUser(const char *channel_id, const char *status) {

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

void createChannel() {

    char channel_id[MaxBufSize];
    int res;
    do {
        receiveData(channel_id);
        res = isExist(channel_id);

        if (res == TRUE) { // the same id found, so bad!
            sendData(DECLINE);
        } else if (res == FALSE) { // no same id found
            sendData(ACCEPT);
        }

    } while (res == TRUE);

    addChannel(channel_id);
    addChannelToData(channel_id);
    addChannelToUser(channel_id, "C");
    sendData(ACCEPT); // channel successfully created

}
// --------------------------------------------------------------------------------------------------------------------------------------
//  Join

// Join group
int isGroupId(const char *ID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT (isGroup) FROM [*Data*] "
            "WHERE ID = '%s'", ID
    );

    char isGroup;
    char *isGroup_pt = &isGroup;

    res = sqlite3_exec(Database, command, whatisCallback, isGroup_pt, &errMsg); // now, give the password
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

void joinGroupMessage(const char *group_ID, const char *subID) {

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

void joinGroup() {

    char group_id[MaxBufSize];
    int res;
    do {
        receiveData(group_id);
        res = isExistInUser(group_id);
        if (res == TRUE) {
            sendData("D"); // duplicate join request!
            return;
        }

        res = isExist(group_id) && isGroupId(group_id);

        if (res == FALSE) { // the id is not found, so bad!
            sendData(DECLINE);
        } else if (res == TRUE) { // id found
            sendData(ACCEPT);
        }

    } while (res == FALSE);

    insertMember(group_id, UserName, "M");
    joinGroupMessage(group_id, UserName);
    addGroupToUser(group_id, "M");
    sendData(ACCEPT); // group successfully created
}

// Join channel
int isChannelId(const char *ID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT (isChannel) FROM [*Data*] "
            "WHERE ID = '%s'", ID
    );

    char isChannel;
    char *isChannel_pt = &isChannel;

    res = sqlite3_exec(Database, command, whatisCallback, isChannel_pt, &errMsg); // now, give the password
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

void joinChannel() {

    char channel_id[MaxBufSize];
    int res;
    do {
        receiveData(channel_id);
        res = isExistInUser(channel_id);
        if (res == TRUE) {
            sendData("D"); // duplicate join request!
            return;
        }

        res = isExist(channel_id) && isChannelId(channel_id);

        if (res == FALSE) { // the id is not found, so bad!
            sendData(DECLINE);
        } else if (res == TRUE) { // id found
            sendData(ACCEPT);
        }

    } while (res == FALSE);

    insertMember(channel_id, UserName, "M");
    addChannelToUser(channel_id, "M");
    sendData(ACCEPT); // group successfully created
}
//--------------------------------------------------------------------------------------------------------------------------------------
// Start Chatting

// find message number
int messageNumCallback(void *data, int argsNum, char **value, char **columnName) {

    *(int *) data = atoi(*value);
    return 0;
}

int findUserLastMessageNum(const char *user_pv_group_channel_id) {

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

    res = sqlite3_exec(Database, command, messageNumCallback, messageNum_pt, &errMsg);

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }
    return messageNum;

}

char getUserStatus(const char *pv_group_channel_id, const char *userID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT isMember FROM [%s] "
            "WHERE isMessage = 'N' AND SubIDs = '%s'", pv_group_channel_id, userID
    );

    char status;
    char *status_pt = &status;

    res = sqlite3_exec(Database, command, whatisCallback, status_pt, &errMsg);

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }

    return status;
}

// sendData message
int sendMessagesCallback(void *data, int argsNum, char **value, char **columnName) {

    char ID[MaxBufSize], time[MaxBufSize], message[MaxBufSize];
    char status;
    int messageNum;

    int res;
    res = isUserId((const char *) data); // data here is name of user_pv_group_channel
    if (res == TRUE) {
        status = 'Y';
    } else if (value[0][0] == '*') { // if system message
        status = 'N';
    } else if (res == FALSE) {
        status = getUserStatus((const char *) data, value[0]);
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

void sendAllMessages(const char *user_pv_group_channel_id) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT SubIDs, MessageNumber, Time, Message FROM [%s] "
            "WHERE isMessage = 'Y'",
            user_pv_group_channel_id  // this only for see all messages , for see only user message user use WHERE SubIDs = UserName
    );

    res = sqlite3_exec(Database, command, sendMessagesCallback, (void *) user_pv_group_channel_id, &errMsg);
    sendData(DECLINE); // that is it!

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }
}

// sendData list callback
int sendListCallback(void *data, int argsNum, char **value, char **columnName) {

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
void addMessage(const char *user_pv_group_channel_id, const char *input_message, int messageNum) {

    char command[MaxBufSize], time[MaxBufSize];
    char *errMsg;
    int res;

    getMessageTime(time, MaxBufSize);

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
void deleteMessage(const char *user_pv_group_channel_id, const char *input_message) {

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

void editMessage(const char *user_pv_group_channel_id, const char *input_message) {

    char updated_text[MaxBufSize] = "";
    int messageNum = 0;
    sscanf(input_message, "%d %[^\n]s", &messageNum, updated_text);

    if (messageNum <= 0) { // Nope!
        return;
    }
    if (strEquality(updated_text, "")) {
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

void doCommand(const char *input_message, const char *user_pv_group_channel_id) {


    if (strStart(input_message, "/delete ")) {

        deleteMessage(user_pv_group_channel_id,
                      input_message + 8); //example: /delete 1234 --> should ignore 8 characters
    } else if (strStart(input_message, "/edit ")) {

        editMessage(user_pv_group_channel_id, input_message + 6);
    }
    // members list
    // promit to admin
    // admins can delete member message
}

// Saved Messages
void savedMessages() {

    char input_message[MaxBufSize];
    int startMessageNum = findUserLastMessageNum(UserName) + 1;

    while (!strEquality(input_message, DECLINE)) {

        sendAllMessages(UserName);

        receiveData(input_message);
        if (strEquality(input_message, DECLINE)) {
            return;
        } else if (strEquality(input_message, "")) {
            continue;
        } else if (input_message[0] == '/' && input_message[1] == '/') {
            addMessage(UserName, input_message + 1, startMessageNum);
            startMessageNum++;
        } else if (input_message[0] == '/') {
            doCommand(input_message, UserName);
        } else {
            addMessage(UserName, input_message, startMessageNum);
            startMessageNum++;
        }
    }
}

// Pv chats
void findPvOpponent(const char *full_pvChatID, char *opponent_destination) {

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

    if (strEquality(user1, UserName)) {
        strcpy(opponent_destination, user2);
    } else if (strEquality(user2, UserName)) {
        strcpy(opponent_destination, user1);
    }


}

int sendPvChatListCallback(void *data, int argsNum, char **value, char **columnName) {

    char opponent_ID[MaxBufSize];
    findPvOpponent(*value, buffer);
    sprintf(opponent_ID, ">%s", buffer);

    int index = (*(int *) data);
    strcpy(list[index], *value);
    (*(int *) data)++;

    Sleep(WaitTOSendMessage); // some wait to client can clearly recive previous message
    sendData(opponent_ID);
    return 0;
}

int sendPvChatList() {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT SubIDs FROM [%s] "
            "WHERE isPvChat = 'Y'", UserName
    );

    int pvChatsNum = 0;
    int *pvChatsNum_pt = &pvChatsNum;

    res = sqlite3_exec(Database, command, sendPvChatListCallback, pvChatsNum_pt, &errMsg);
    sendData(DECLINE); // that is it!

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }

    return pvChatsNum;
}

void gotoPvChat(const char *pvChat_id) {

    Sleep(WaitTOSendMessage);
    sendData(pvChat_id);

    char input_message[MaxBufSize];
    int startMessageNum = findUserLastMessageNum(pvChat_id) + 1;

    while (1) {

        sendAllMessages(pvChat_id);

        receiveData(input_message);
        if (strEquality(input_message, DECLINE)) {
            return;
        } else if (strEquality(input_message, "")) {
            continue;
        } else if (input_message[0] == '/' && input_message[1] == '/') {
            addMessage(pvChat_id, input_message + 1, startMessageNum);
            startMessageNum++;
        } else if (input_message[0] == '/') {
            doCommand(input_message, pvChat_id);
        } else {
            addMessage(pvChat_id, input_message, startMessageNum);
            startMessageNum++;
        }
    }
}

void pvChats() {

    int PvChatsNum, mood;
    PvChatsNum = sendPvChatList();

    receiveData(buffer);

    while (!strEquality(buffer, DECLINE)) {

        mood = atoi(buffer);
        gotoPvChat(list[mood - 1]);

        receiveData(buffer);
    }
}

// Groups
int sendGroupsList() {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT SubIDs FROM [%s] "
            "WHERE isGroup <> 'N'", UserName
    );

    int groupsNum = 0;
    int *groupsNum_pt = &groupsNum;

    res = sqlite3_exec(Database, command, sendListCallback, groupsNum_pt, &errMsg);
    sendData(DECLINE); // that is it!

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }

    return groupsNum;
}

void gotoGroup(const char *group_id) {

    Sleep(WaitTOSendMessage);
    sendData(group_id);

    char input_message[MaxBufSize];
    int startMessageNum = findUserLastMessageNum(group_id) + 1;

    while (1) {

        sendAllMessages(group_id);

        receiveData(input_message);
        if (strEquality(input_message, DECLINE)) {
            return;
        } else if (strEquality(input_message, "")) {
            continue;
        } else if (input_message[0] == '/' && input_message[1] == '/') {
            addMessage(group_id, input_message + 1, startMessageNum);
            startMessageNum++;
        } else if (input_message[0] == '/') {
            doCommand(input_message, group_id);
        } else {
            addMessage(group_id, input_message, startMessageNum);
            startMessageNum++;
        }
    }
}

void groups() {

    int groupsNum, mood;
    groupsNum = sendGroupsList();

    receiveData(buffer);

    while (!strEquality(buffer, DECLINE)) {

        mood = atoi(buffer);
        gotoGroup(list[mood - 1]);

        receiveData(buffer);
    }

}

// Channels
int sendChannelsList() {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT SubIDs FROM [%s] "
            "WHERE isChannel <> 'N'", UserName
    );

    int channelsNum = 0;
    int *channelsNum_pt = &channelsNum;

    res = sqlite3_exec(Database, command, sendListCallback, channelsNum_pt, &errMsg);
    sendData(DECLINE); // that is it!

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }

    return channelsNum;
}

void gotoChannel(const char *channel_id) {

    Sleep(WaitTOSendMessage);
    sendData(channel_id);

    char status = getUserStatus(channel_id, UserName);
    sprintf(buffer, "%c", status);
    sendData(buffer);

    char input_message[MaxBufSize];
    int startMessageNum = findUserLastMessageNum(channel_id) + 1;

    while (1) {

        sendAllMessages(channel_id);

        receiveData(input_message);
        if (strEquality(input_message, DECLINE)) {
            return;
        } else if (strEquality(input_message, "")) {
            continue;
        } else if (input_message[0] == '/' && input_message[1] == '/') {
            addMessage(channel_id, input_message + 1, startMessageNum);
            startMessageNum++;
        } else if (input_message[0] == '/') {
            doCommand(input_message, channel_id);
        } else {
            addMessage(channel_id, input_message, startMessageNum);
            startMessageNum++;
        }
    }
}

void channels() {

    int channelsNum, mood;
    channelsNum = sendChannelsList();

    receiveData(buffer);

    while (!strEquality(buffer, DECLINE)) {

        mood = atoi(buffer);
        gotoChannel(list[mood - 1]);

        receiveData(buffer);
    }
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Contacts
int sendContactListCallback(void *data, int argsNum, char **value, char **columnName) {

    char ID[MaxBufSize];
    sprintf(ID, ">%s", *value);

    Sleep(WaitTOSendMessage); // some wait to client can clearly recive previous message
    sendData(ID);
    return 0;
}

void sendContactsList() {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT SubIDs FROM [%s] "
            "WHERE isContact = 'Y'", UserName
    );

    res = sqlite3_exec(Database, command, sendContactListCallback, NULL, &errMsg);
    sendData(DECLINE); // that is it!

    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
    }
}

int isContactExistInUser(const char *ID) {

    char command[MaxBufSize];
    char *errMsg;
    int res;

    sprintf(command,
            "SELECT * FROM [%s] "
            "WHERE isContact = 'Y' AND SubIDs = '%s'", UserName, ID
    );

    int is_contactExist = FALSE;
    int *is_contactExist_pt = &is_contactExist;

    res = sqlite3_exec(Database, command, existCallback, is_contactExist_pt, &errMsg); // is exist?
    if (res != SQLITE_OK) {
        printf("Database: SQL error: %s\n", errMsg);
        return -1;
    }
    if (is_contactExist == FALSE) {
        return FALSE;
    }
    return TRUE;

}

void addContactToUser(const char *destination_userName) {

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

void addContact() {

    char destination_contact[MaxBufSize];
    int res;

    do {
        receiveData(destination_contact);
        res = isContactExistInUser(destination_contact);
        if (res == TRUE) {
            sendData("D"); // duplicate join request!

            receiveData(buffer); // stay for user to accept commands
            if (strEquality(buffer, ACCEPT)) {
                return;
            }
        }

        res = isExist(destination_contact) && isUserId(destination_contact);

        if (res == FALSE) {
            sendData(DECLINE);
        } else if (res == TRUE) {
            addContactToUser(destination_contact);
            sendData(ACCEPT);
        }

    } while (res == FALSE);


    receiveData(buffer); // stay for user to accept commands
    if (strEquality(buffer, ACCEPT)) {
        return;
    }


}

void deleteContactFromUser(const char *destination_userName) {

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

void deleteContact() {

    char destination_contact[MaxBufSize];
    int res;

    do {
        receiveData(destination_contact);
        res = isContactExistInUser(destination_contact);

        if (res == FALSE) {
            sendData(DECLINE);
        } else if (res == TRUE) {
            deleteContactFromUser(destination_contact);
            sendData(ACCEPT);
        }

    } while (res == FALSE);


    receiveData(buffer); // stay for user to accept commands
    if (strEquality(buffer, ACCEPT)) {
        return;
    }


}

void contacts() {

    int mood;

    while (1) {

        sendContactsList();

        receiveData(buffer);
        if (strEquality(buffer, DECLINE)) {
            return;
        } else if (strEquality(buffer, "R")) { // retry
            continue;
        }


        mood = atoi(buffer);
        if (mood == 1) {
            addContact();
        } else if (mood == 2) {
            deleteContact();
        }

    }
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Process response
int processResponse() {

    if (strEquality(buffer, "signup")) {
        signup();
    } else if (strEquality(buffer, "login")) {
        login();
    } else if (strEquality(buffer, "createPvChat")) {
        createPvChat();
    } else if (strEquality(buffer, "createGroup")) {
        createGroup();
    } else if (strEquality(buffer, "createChannel")) {
        createChannel();
    } else if (strEquality(buffer, "joinGroup")) {
        joinGroup();
    } else if (strEquality(buffer, "joinChannel")) {
        joinChannel();
    } else if (strEquality(buffer, "savedMessages")) {
        savedMessages();
    } else if (strEquality(buffer, "pvChats")) {
        pvChats();
    } else if (strEquality(buffer, "groups")) {
        groups();
    } else if (strEquality(buffer, "channels")) {
        channels();
    } else if (strEquality(buffer, "contacts")) {
        contacts();
    } else if (strEquality(buffer, "off")) {
        return FALSE;
    } else {
        printf("There is an invalid response\n");
    }

    return TRUE;
}

//--------------------------------------------------------------------------------------------------------------------------------------
// Main loop
void mainLoop() {

    int serverRunning = TRUE;

    do {

        receiveData(buffer);
        serverRunning = processResponse(); //answer the response

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
    mainLoop();


    // make ready to program ending....
    closesocket(listening); // Close listening socket
    closesocket(clientSocket); // Close the socket
    WSACleanup(); // Cleanup winsock



    return 0;
}
