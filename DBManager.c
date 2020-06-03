#include "DBManager.h"
//----------------------------------------------------------------
int respondToAdd(char* name, int salary, int lastKey)
{
    struct DBrecord newRecord;
    strcpy(newRecord.name,name);
    newRecord.salary = salary;
    newRecord.key = lastKey+1;
    DBtable[lastKey+1]=newRecord;
    printf("New record added to DB with name: %s and Salary: %d with key: %d \n",DBtable[lastKey+1].name,DBtable[lastKey+1].salary,DBtable[lastKey+1].key);
    return lastKey+1;
}
void respondToModify(int keyOfTheRecordToBeModified, int modificationValue)
{
    int lowerLimit=0;
    int currentIndex=0;
    for(;DBtable[currentIndex].key < keyOfTheRecordToBeModified;currentIndex++);
    if(currentIndex == keyOfTheRecordToBeModified )
    {
      //  printf("New record will be modified from DB with name: %s and Salary: %d with key: %d \n",DBtable[currentIndex].name,DBtable[currentIndex].salary,currentIndex);
        DBtable[currentIndex].salary += modificationValue;
        if(DBtable[currentIndex].salary < lowerLimit) DBtable[currentIndex].salary = lowerLimit;
        printf("New record modified succ in DB with name: %s and new Salary: %d with key: %d \n",DBtable[currentIndex].name,DBtable[currentIndex].salary,currentIndex);
        respondToRelease(keyOfTheRecordToBeModified,pointersOfWaitingQueuesForRecordKeys[keyOfTheRecordToBeModified]);
    }

}
void respondToAcquire(int requiredRecordKey, int CallingProccessPID, struct waitingQueue* waitingQueueOfThePassedKey)
{
    if(DBsemaphores[requiredRecordKey] == SEMAPHORE_OCCUPIED)
    {
        addToWaitingQueue(waitingQueueOfThePassedKey, CallingProccessPID);
       // printf("I am db manager nanannana %d \n",DBsemaphores[50]);

    }
    else
    {
        DBsemaphores[requiredRecordKey] == SEMAPHORE_OCCUPIED;
     //   printf("I am db manager wake up %d \n",CallingProccessPID);
      /*  kill(CallingProccessPID,SIGUSR1);
        kill(CallingProccessPID,SIGCONT);*/
    }
}
void respondToRelease(int releasedRecordKey, struct waitingQueue* waitingQueueOfThePassedKey)
{
    DBsemaphores[releasedRecordKey] == SEMAPHORE_AVAILABLE;
    int releasedProcessPID = removeFromWaitingQueue(waitingQueueOfThePassedKey);
   // printf("released \n");
   // kill(releasedProcessPID,SIGCONT);
}
void respondToQuery(int queryType, int searchedSalary, char searchedName[], int callingProcessID) {
    if(lastKey+1>0) {
        struct message sendSearchResult;
        int notReturnedKey = -1;
        int checkedRecordIndex=0;
        int returnedKeyIndex=0;
        memset(sendSearchResult.queryKeys, notReturnedKey, lastKey+1);
        if(queryType == QUERY_BY_FULL_TABLE) {
            for(;checkedRecordIndex<=lastKey;checkedRecordIndex++) {
                sendSearchResult.queryKeys[returnedKeyIndex++] = checkedRecordIndex;
            }
        }
        else if(queryType == QUERY_BY_EXACT_NAME) {
            for(;checkedRecordIndex<=lastKey;checkedRecordIndex++) {
                if(DBtable[checkedRecordIndex].name == searchedName){
                    sendSearchResult.queryKeys[returnedKeyIndex++] = checkedRecordIndex;
                }
            }
        }
        else if(queryType == QUERY_BY_PART_OF_NAME) {
            for(;checkedRecordIndex<=lastKey;checkedRecordIndex++) {
                if(strlen(DBtable[checkedRecordIndex].name) >= strlen(searchedName)){
                    int checkedChar = 0;
                    for(;checkedChar<strlen(searchedName)&&(DBtable[checkedRecordIndex].name[checkedChar]==searchedName[checkedChar]);checkedChar++);
                    if(checkedChar==strlen(searchedName))sendSearchResult.queryKeys[returnedKeyIndex++] = checkedRecordIndex;
                }
            }
        }
        else if(queryType==QUERY_BY_EXACT_SALARY) {
            for(;checkedRecordIndex<=lastKey;checkedRecordIndex++) {
                if(DBtable[checkedRecordIndex].salary == searchedSalary){
                    sendSearchResult.queryKeys[returnedKeyIndex++] = checkedRecordIndex;
                }
            }
        }
        else if(queryType==QUERY_BY_LESS_THAN_SALARY) {
            for(;checkedRecordIndex<=lastKey;checkedRecordIndex++) {
                if(DBtable[checkedRecordIndex].salary < searchedSalary){
                    sendSearchResult.queryKeys[returnedKeyIndex++] = checkedRecordIndex;
                }
            }
        }
        else if(queryType==QUERY_BY_GREATER_THAN_SALARY) {
            for(;checkedRecordIndex<=lastKey;checkedRecordIndex++) {
                if(DBtable[checkedRecordIndex].salary > searchedSalary){
                    sendSearchResult.queryKeys[returnedKeyIndex++] = checkedRecordIndex;
                }
            }
        }
        else if(queryType==QUERY_BY_LESS_THAN_OR_EQUAL_SALARY) {
            for(;checkedRecordIndex<=lastKey;checkedRecordIndex++) {
                if(DBtable[checkedRecordIndex].salary <= searchedSalary){
                    sendSearchResult.queryKeys[returnedKeyIndex++] = checkedRecordIndex;
                }
            }
        }
        else if(queryType==QUERY_BY_GREATER_THAN_OR_EQUAL_SALARY) {
            for(;checkedRecordIndex<=lastKey;checkedRecordIndex++) {
                if(DBtable[checkedRecordIndex].salary >= searchedSalary){
                    sendSearchResult.queryKeys[returnedKeyIndex++] = checkedRecordIndex;
                }
            }
        }
        else if(queryType==QUERY_BY_EXACT_NAME_AND_SALARY_EXACT_HYBRID)
        {
            for(;checkedRecordIndex<=lastKey;checkedRecordIndex++) {
                if(DBtable[checkedRecordIndex].salary == searchedSalary && DBtable[checkedRecordIndex].name == searchedName){
                    sendSearchResult.queryKeys[returnedKeyIndex++] = checkedRecordIndex;
                }
            }
        }

        // note always send not only if found
        if (returnedKeyIndex>0) {
            sendSearchResult.mtype=callingProcessID;
            msgsnd(messageQueueID, &sendSearchResult, sizeof(sendSearchResult)-sizeof(sendSearchResult.mtype), !IPC_NOWAIT);
        }
    }
    
}
void initializeDBManager(int messageQueueIdReceived, int sharedMemoryIdReceived,int DBSharedMemoryIdReceived, int loggerMsgQIdReceived){
    messageQueueID=messageQueueIdReceived;
    DBManagerPID = getpid();
    sharedMemoryId = sharedMemoryIdReceived;
    memset(DBsemaphores, SEMAPHORE_AVAILABLE, MAX_NUMBER_OF_RECORDS);
    DBtable = (struct DBrecord *)shmat(DBSharedMemoryIdReceived, (void *)0, 0);
    loggerMsgQIdDBManager=loggerMsgQIdReceived;
}
void do_DBManager(int sharedMemoryIdReceived, int clientDBManagerMsgQIdReceived, int DBSharedMemoryIdReceived, int loggerMsgQIdReceived)
{
    initializeDBManager(clientDBManagerMsgQIdReceived, sharedMemoryIdReceived, DBSharedMemoryIdReceived,loggerMsgQIdReceived);
    while(1)
    {
        msgrcv(messageQueueID, &receivedMessage,(sizeof(struct message) - sizeof(receivedMessage.mtype)),DBManagerPID,!IPC_NOWAIT);
                        printf("querryyyyyyyyyyyzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz \n");
        int messageType = receivedMessage.destinationProcess;
        if(messageType == MESSAGE_TYPE_ADD) {
            lastKey = respondToAdd(receivedMessage.name,receivedMessage.salary,lastKey);
            pointersOfWaitingQueuesForRecordKeys[lastKey] = createWaitingQueue();
        }
        else if (messageType == MESSAGE_TYPE_MODIFY) {
            respondToModify(receivedMessage.key, receivedMessage.modification);
        }
        else if (messageType == MESSAGE_TYPE_ACQUIRE) {
            respondToAcquire(receivedMessage.key, receivedMessage.callingProcessID, pointersOfWaitingQueuesForRecordKeys[receivedMessage.key]);
        }
        else if (messageType == MESSAGE_TYPE_RELEASE) {
            respondToRelease(receivedMessage.key, pointersOfWaitingQueuesForRecordKeys[receivedMessage.key]);
        }
        else if (messageType == MESSAGE_TYPE_QUERY) {

            respondToQuery(receivedMessage.queryType, receivedMessage.searchedSalary, receivedMessage.searchedString, receivedMessage.callingProcessID);
        }
    }
}