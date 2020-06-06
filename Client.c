#include "Client.h"

void do_client(int DBManagerIdReceived, int QueryLoggerIdReceived, int sharedMemoryIdReceived, int clientDBManagerMsgQIdReceived, int clientNumberReceived, int DBSharedMemoryIdReceived, int loggerMsgQIdReceived, int LoggerIdReceived, int queryLoggerMsgQIdReceived)
{

    initializeClient(DBManagerIdReceived, QueryLoggerIdReceived, sharedMemoryIdReceived, clientDBManagerMsgQIdReceived, clientNumberReceived, DBSharedMemoryIdReceived, loggerMsgQIdReceived, LoggerIdReceived, queryLoggerMsgQIdReceived);
    openConfigurationFile();
    char *readWordFromFile = "1";
    while (readWordFromFile != NULL)
    {
        readWordFromFile = readConfigurationFile();
        if (readWordFromFile == NULL)
        {
            break;
        }

        if (strcmp(readWordFromFile, "add") == 0)
        {
            char name[MAXCHAR];
            strcpy(name, readConfigurationFile());
            char salary[MAXCHAR];
            strcpy(salary, readConfigurationFile());
            requestToAdd(name, atoi(salary));
        }
        else if (strcmp(readWordFromFile, "modify") == 0)
        {
            char key[MAXCHAR];
            strcpy(key, readConfigurationFile());
            char modification[MAXCHAR];
            strcpy(modification, readConfigurationFile());
            int sign = strcmp(modification, "+") == 0 ? ADD_MODIFICATION : SUBTRACT_MODIFICATION;
            char value[MAXCHAR];
            strcpy(value, readConfigurationFile());
            requestToModify(atoi(key), sign, atoi(value));
        }
        else if (strcmp(readWordFromFile, "acquire") == 0)
        {
            char key[MAXCHAR];
            strcpy(key, readConfigurationFile());
            requestToAcquire(atoi(key));
        }
        else if (strcmp(readWordFromFile, "release") == 0)
        {
            char key[MAXCHAR];
            strcpy(key, readConfigurationFile());
            requestToRelease(atoi(key));
        }
        else if (strcmp(readWordFromFile, "query") == 0)
        {
            int queryType, searchedSalary;
            char *searchedString;
            getQueryRequestParameters(&queryType, &searchedSalary, &searchedString);
            requestToQuery(queryType, searchedSalary, &searchedString);
        }
        sleep(1);
    }
    fclose(fp);
    exit(1);
}
void getQueryRequestParameters(int *queryType, int *searchedSalary, char **searchedString)
{

    char *queryNameType = readConfigurationFile();
    if (strcmp(queryNameType, "full") == 0)
    {
        *queryType = QUERY_BY_FULL_TABLE;
        *searchedString = NULL;
        *searchedSalary = 0;
    }
    else if (strcmp(queryNameType, "name") == 0)
    {
        queryNameType = readConfigurationFile();
        if (strcmp(queryNameType, "startswith") == 0)
        {
            *queryType = QUERY_BY_PART_OF_NAME;
            *searchedString = readConfigurationFile();
        }
        else if (strcmp(queryNameType, "exact") == 0)
        {
            *queryType = QUERY_BY_EXACT_NAME;
            *searchedString = readConfigurationFile();
        }
        *searchedSalary = 0;
    }
    else if (strcmp(queryNameType, "salary") == 0)
    {
        queryNameType = readConfigurationFile();
        if (strcmp(queryNameType, "exact") == 0)
        {
            *queryType = QUERY_BY_EXACT_SALARY;
        }
        else if (strcmp(queryNameType, ">") == 0)
        {
            *queryType = QUERY_BY_GREATER_THAN_SALARY;
        }
        else if (strcmp(queryNameType, "<") == 0)
        {
            *queryType = QUERY_BY_LESS_THAN_SALARY;
        }
        else if (strcmp(queryNameType, ">=") == 0)
        {
            *queryType = QUERY_BY_GREATER_THAN_OR_EQUAL_SALARY;
        }
        else if (strcmp(queryNameType, "<=") == 0)
        {
            *queryType = QUERY_BY_LESS_THAN_OR_EQUAL_SALARY;
        }

        *searchedSalary = atoi(readConfigurationFile());
    }
    else if (strcmp(queryNameType, "hybrid") == 0)
    {
        *queryType = QUERY_BY_EXACT_NAME_AND_SALARY_EXACT_HYBRID;
        *searchedString = readConfigurationFile();
       // *searchedSalary = atoi(readConfigurationFile());
    }
}
void initializeClient(int DBManagerIdReceived, int QueryLoggerIdReceived, int sharedMemoryIdReceived, int clientDBManagerMsgQIdReceived, int clientNumberReceived, int DBSharedMemoryIdReceived, int loggerMsgQIdReceived, int LoggerIdReceived, int queryLoggerMsgQIdReceived)
{
    DBManagerId = DBManagerIdReceived;
    QueryLoggerId = QueryLoggerIdReceived;
    sharedMemoryId = sharedMemoryIdReceived;
    clientDBManagerMsgQId = clientDBManagerMsgQIdReceived;
    clientNumber = clientNumberReceived;
    messageClient.mtype = DBManagerId;
    signal(SIGUSR1, handlingSIGUSR1_and_IgnoringSigStop);
    DBtableView = (struct DBrecord *)shmat(DBSharedMemoryIdReceived, (void *)0, SHM_RDONLY);
    loggerMsgQIdClient = loggerMsgQIdReceived;
    clientLogger = (struct Log *)shmat(sharedMemoryId, (void *)0, 0);
    LoggerId = LoggerIdReceived;
    queryLoggerMsgQIdClient = queryLoggerMsgQIdReceived;
}
void openConfigurationFile()
{
    char buffer[50];
    sprintf(buffer, "%d", clientNumber);
    filename = strcat(buffer, ".txt");
    fp = fopen(filename, "r");
}
char *readConfigurationFile()
{
    //read config
    if (fp == NULL)
    {
        printf("Could not open file %s\n", filename);
        return 0;
    }

    if (fscanf(fp, "%s", word) != EOF)
    {
        return word;
    }
    else
    {
        return NULL;
    }
}
void requestToAdd(char *name, int salary)
{
    messageClient.destinationProcess = MESSAGE_TYPE_ADD;
    strcpy(messageClient.name, name);
    messageClient.salary = salary;
    messageClient.callingProcessID = getpid();
    messageClient.mtype = DBManagerId;
    send_val = msgsnd(clientDBManagerMsgQId, &messageClient, sizeof(messageClient) - sizeof(messageClient.mtype), !IPC_NOWAIT);
    if (send_val == -1)
    {
        perror("error in send msg");
    }
    else
    {
        logAdd(name, salary);
    }
}
void logAdd(char *name, int salary)
{
    messageLoggerClient.mtype = LoggerId;
    messageLoggerClient.PID = getpid();
    messageLoggerClient.destinationProcess = MESSAGE_TYPE_ACQUIRE;

    send_val = msgsnd(loggerMsgQIdClient, &messageLoggerClient, sizeof(messageLoggerClient) - sizeof(messageLoggerClient.mtype), !IPC_NOWAIT);

    raise(SIGTSTP);
    signal(SIGTSTP, SIG_DFL);

    char clientNumberString[5];
    sprintf(clientNumberString, "%d", clientNumber);
    strcpy(clientLogger->number, clientNumberString);

    char msgArray[MAXCHAR] = "I require to add ";
    strcat(msgArray, name);

    char salaryString[MAXCHAR];
    sprintf(salaryString, "%d", salary);

    strcat(msgArray, " with salary ");
    strcat(msgArray, salaryString);
    strcpy(clientLogger->message, msgArray);
    messageLoggerClient.destinationProcess = MESSAGE_TYPE_RELEASE;
    messageLoggerClient.mtype = LoggerId;
    send_val = msgsnd(loggerMsgQIdClient, &messageLoggerClient, sizeof(messageLoggerClient) - sizeof(messageLoggerClient.mtype), !IPC_NOWAIT);
}
void requestToModify(int key, int modification, int value)
{
    messageClient.destinationProcess = MESSAGE_TYPE_MODIFY;
    messageClient.key = key;
    messageClient.modification = value * modification;
    messageClient.callingProcessID = getpid();
    messageClient.mtype = DBManagerId;

    send_val = msgsnd(clientDBManagerMsgQId, &messageClient, sizeof(messageClient) - sizeof(messageClient.mtype), !IPC_NOWAIT);
    if (send_val == -1)
    {
        perror("error in send msg");
    }
    else
    {
        logModify(key, modification, value);
    }
}
void logModify(int key, int modification, int value)
{
    messageLoggerClient.mtype = LoggerId;
    messageLoggerClient.PID = getpid();
    messageLoggerClient.destinationProcess = MESSAGE_TYPE_ACQUIRE;
    send_val = msgsnd(loggerMsgQIdClient, &messageLoggerClient, sizeof(messageLoggerClient) - sizeof(messageLoggerClient.mtype), !IPC_NOWAIT);

    raise(SIGTSTP);
    signal(SIGTSTP, SIG_DFL);

    char clientNumberString[5];
    sprintf(clientNumberString, "%d", clientNumber);
    strcpy(clientLogger->number, clientNumberString);
    char msgArray[MAXCHAR] = "I require to modify key: ";
    char keyString[MAXCHAR];
    sprintf(keyString, "%d", key);
    strcat(msgArray, keyString);

    char valueString[MAXCHAR];
    sprintf(valueString, "%d", modification * value);

    strcat(msgArray, " with value ");
    strcat(msgArray, valueString);
    strcpy(clientLogger->message, msgArray);
    messageLoggerClient.destinationProcess = MESSAGE_TYPE_RELEASE;
    messageLoggerClient.mtype = LoggerId;
    send_val = msgsnd(loggerMsgQIdClient, &messageLoggerClient, sizeof(messageLoggerClient) - sizeof(messageLoggerClient.mtype), !IPC_NOWAIT);
}
void requestToAcquire(int key)
{
    messageClient.destinationProcess = MESSAGE_TYPE_ACQUIRE;
    messageClient.key = key;
    messageClient.callingProcessID = getpid();
    messageClient.mtype = DBManagerId;
    send_val = msgsnd(clientDBManagerMsgQId, &messageClient, sizeof(messageClient) - sizeof(messageClient.mtype), !IPC_NOWAIT);
    if (send_val == -1)
    {
        perror("error in send msg");
    }
    else
    {
        struct message messageClient2;
        int rec = msgrcv(clientDBManagerMsgQId, &messageClient2, (sizeof(messageClient2) - sizeof(messageClient2.mtype)), getpid(), !IPC_NOWAIT);

        if (rec != -1)
        {
            logAcquire(key);
        }
        else
        {
            perror("error in send msg");
        }
    }
}
void logAcquire(int key)
{
    messageLoggerClient.mtype = LoggerId;
    messageLoggerClient.PID = getpid();
    messageLoggerClient.destinationProcess = MESSAGE_TYPE_ACQUIRE;
    send_val = msgsnd(loggerMsgQIdClient, &messageLoggerClient, sizeof(messageLoggerClient) - sizeof(messageLoggerClient.mtype), !IPC_NOWAIT);

    raise(SIGTSTP);
    signal(SIGTSTP, SIG_DFL);

    char clientNumberString[5];
    sprintf(clientNumberString, "%d", clientNumber);
    strcpy(clientLogger->number, clientNumberString);
    char msgArray[MAXCHAR] = "I acquire key: ";
    char keyString[MAXCHAR];
    sprintf(keyString, "%d", key);
    strcat(msgArray, keyString);

    strcpy(clientLogger->message, msgArray);
    messageLoggerClient.destinationProcess = MESSAGE_TYPE_RELEASE;
    messageLoggerClient.mtype = LoggerId;
    send_val = msgsnd(loggerMsgQIdClient, &messageLoggerClient, sizeof(messageLoggerClient) - sizeof(messageLoggerClient.mtype), !IPC_NOWAIT);
}
void requestToRelease(int key)
{
    messageClient.destinationProcess = MESSAGE_TYPE_RELEASE;
    messageClient.key = key;
    messageClient.callingProcessID = getpid();
    messageClient.mtype = DBManagerId;
    send_val = msgsnd(clientDBManagerMsgQId, &messageClient, sizeof(messageClient) - sizeof(messageClient.mtype), !IPC_NOWAIT);
    if (send_val == -1)
    {
        perror("error in send msg");
    }
    else
    {
        logRelease(key);
    }
}
void logRelease(int key)
{
    messageLoggerClient.mtype = LoggerId;
    messageLoggerClient.PID = getpid();
    messageLoggerClient.destinationProcess = MESSAGE_TYPE_ACQUIRE;
    send_val = msgsnd(loggerMsgQIdClient, &messageLoggerClient, sizeof(messageLoggerClient) - sizeof(messageLoggerClient.mtype), !IPC_NOWAIT);

    raise(SIGTSTP);
    signal(SIGTSTP, SIG_DFL);

    char clientNumberString[5];
    sprintf(clientNumberString, "%d", clientNumber);
    strcpy(clientLogger->number, clientNumberString);
    char msgArray[MAXCHAR] = "I release key: ";
    char keyString[MAXCHAR];
    sprintf(keyString, "%d", key);
    strcat(msgArray, keyString);

    strcpy(clientLogger->message, msgArray);
    messageLoggerClient.destinationProcess = MESSAGE_TYPE_RELEASE;
    messageLoggerClient.mtype = LoggerId;
    send_val = msgsnd(loggerMsgQIdClient, &messageLoggerClient, sizeof(messageLoggerClient) - sizeof(messageLoggerClient.mtype), !IPC_NOWAIT);

  }
void requestToQuery(int queryType, int searchedSalary, char **searchedString)
{
    messageClient.destinationProcess = MESSAGE_TYPE_QUERY;
    messageClient.queryType = queryType;
    messageClient.searchedSalary = searchedSalary;
    messageClient.mtype = DBManagerId;
    if (*searchedString != NULL)
    {
        strcpy(messageClient.searchedString, *searchedString);
    }
    messageClient.callingProcessID = getpid();
    send_val = -1;
    while (send_val == -1)
    {
        send_val = msgsnd(clientDBManagerMsgQId, &messageClient, sizeof(messageClient) - sizeof(messageClient.mtype), !IPC_NOWAIT);
    }
    if (send_val != -1)
    {

    }
    else
    {

        perror("error in send msg");
    }
    //struct message messageClient2;
    int rec = msgrcv(clientDBManagerMsgQId, &messageClient, (sizeof(messageClient) - sizeof(messageClient.mtype)), getpid(), !IPC_NOWAIT);

    if (rec == -1)
    {
        perror("error in send msg");
    }
    else
    {
        logQuery();
        sendToQueryLogger(queryType);
    }
}
void logQuery()
{
    messageLoggerClient.mtype = LoggerId;
    messageLoggerClient.PID = getpid();
    messageLoggerClient.destinationProcess = MESSAGE_TYPE_ACQUIRE;
    send_val = msgsnd(loggerMsgQIdClient, &messageLoggerClient, sizeof(messageLoggerClient) - sizeof(messageLoggerClient.mtype), !IPC_NOWAIT);

    raise(SIGTSTP);
    signal(SIGTSTP, SIG_DFL);

    char clientNumberString[5];
    sprintf(clientNumberString, "%d", clientNumber);
    strcpy(clientLogger->number, clientNumberString);
    char msgArray[MAXCHAR] = "I requested a query ";

    strcpy(clientLogger->message, msgArray);
    messageLoggerClient.destinationProcess = MESSAGE_TYPE_RELEASE;
    messageLoggerClient.mtype = LoggerId;
    send_val = msgsnd(loggerMsgQIdClient, &messageLoggerClient, sizeof(messageLoggerClient) - sizeof(messageLoggerClient.mtype), !IPC_NOWAIT);

}
void sendToQueryLogger(int queryType)
{

    int sizeOfReturnedRecords = 0;

    char clientNumberString[5];
    char message[MAXCHAR] = "I am client number: ";

    sprintf(clientNumberString, "%d", clientNumber);
    strcat(message, clientNumberString);

    char msgArray[MAXCHAR] = ", I requested a ";
    strcat(message, msgArray);

    switch (queryType)
    {
    case QUERY_BY_FULL_TABLE:
        strcpy(msgArray, "QUERY_BY_FULL_TABLE");
        strcat(message, msgArray);
        break;
    case QUERY_BY_EXACT_NAME:
        strcpy(msgArray, "QUERY_BY_EXACT_NAME");
        strcat(message, msgArray);
        break;
    case QUERY_BY_PART_OF_NAME:
        strcpy(msgArray, "QUERY_BY_PART_OF_NAME");
        strcat(message, msgArray);
        break;
    case QUERY_BY_EXACT_SALARY:
        strcpy(msgArray, "QUERY_BY_EXACT_SALARY");
        strcat(message, msgArray);
        break;
    case QUERY_BY_GREATER_THAN_SALARY:
        strcpy(msgArray, "QUERY_BY_GREATER_THAN_SALARY");
        strcat(message, msgArray);
        break;
    case QUERY_BY_LESS_THAN_SALARY:
        strcpy(msgArray, "QUERY_BY_LESS_THAN_SALARY");
        strcat(message, msgArray);
        break;
    case QUERY_BY_GREATER_THAN_OR_EQUAL_SALARY:
        strcpy(msgArray, "QUERY_BY_GREATER_THAN_OR_EQUAL_SALARY");
        strcat(message, msgArray);
        break;
    case QUERY_BY_LESS_THAN_OR_EQUAL_SALARY:
        strcpy(msgArray, "QUERY_BY_LESS_THAN_OR_EQUAL_SALARY");
        strcat(message, msgArray);
        break;
    case QUERY_BY_EXACT_NAME_AND_SALARY_EXACT_HYBRID:
        strcpy(msgArray, "QUERY_BY_EXACT_NAME_AND_SALARY_EXACT_HYBRID");
        strcat(message, msgArray);
        break;
    default:
        break;
    }
    strcpy(queryLoggerMsgQClient.message, message);
    queryLoggerMsgQClient.mtype = QueryLoggerId;
    queryLoggerMsgQClient.PID = getpid();
    queryLoggerMsgQClient.destinationProcess = MESSAGE_TYPE_ACQUIRE;

    printf("***************************************************************************************client send \n");
    send_val = msgsnd(queryLoggerMsgQIdClient, &queryLoggerMsgQClient, sizeof(queryLoggerMsgQClient) - sizeof(queryLoggerMsgQClient.mtype), !IPC_NOWAIT);
    int rec = msgrcv(queryLoggerMsgQIdClient, &queryLoggerMsgQClient, (sizeof(queryLoggerMsgQClient) - sizeof(queryLoggerMsgQClient.mtype)), getpid(), !IPC_NOWAIT);
    printf("----------------------------------------------------------------------client rec \n");

    FILE *fileOpened;
    char fileToOpen[80];
    fileOpened = fopen("QueryLogger.txt", "a"); //opening file  a

    if (fileOpened == NULL)
    {
        printf("unable to open file\n");
    }
    else
    {

        fprintf(fileOpened, "---------------------------------------------------------------------------------------------------------\n");
        fprintf(fileOpened, "%-15s", "Query  ");
        fprintf(fileOpened, "%-15lu", (unsigned long)time(NULL));
        fprintf(fileOpened, "%-15s", message); //writing data into file
        fprintf(fileOpened, "\n");
        fprintf(fileOpened, "****************************************************\n");

        fprintf(fileOpened, "%-15s", "Number");
        fprintf(fileOpened, "%-15s", "Key");
        fprintf(fileOpened, "%-15s", "Name");
        fprintf(fileOpened, "%-15s", "Salary");

        fprintf(fileOpened, "\n");
        fprintf(fileOpened, "---------------------------------------------------------------------------------------------------------\n");

        for (int i = 0; i < MAX_NUMBER_OF_RECORDS && messageClient.queryKeys[i] != -1; i++)
        {
            fprintf(fileOpened, "%-15d", (i + 1));
            fprintf(fileOpened, "%-15d", DBtableView[messageClient.queryKeys[i]].key);
            fprintf(fileOpened, "%-15s", DBtableView[messageClient.queryKeys[i]].name);
            fprintf(fileOpened, "%-15d \n", DBtableView[messageClient.queryKeys[i]].salary);
        }
    }
    fflush(fileOpened);

    queryLoggerMsgQClient.mtype = QueryLoggerId;
    queryLoggerMsgQClient.destinationProcess = MESSAGE_TYPE_RELEASE;

    send_val = msgsnd(queryLoggerMsgQIdClient, &queryLoggerMsgQClient, sizeof(queryLoggerMsgQClient) - sizeof(queryLoggerMsgQClient.mtype), !IPC_NOWAIT);
    if (send_val == -1)
    {
        perror("error in send msg");
    }
}
void handlingSIGUSR1_and_IgnoringSigStop()
{
    signal(SIGTSTP, SIG_IGN);
}