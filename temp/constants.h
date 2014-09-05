#include <map>
using namespace std;

typedef enum State
{
    RUNNING, READY, SLEEPING, SUSPENDED
};

//map<int,int> first;

typedef struct Statistics
{
    int threadId;
    State state;
    int numberOfBursts;
    int totalExecutionTime;
    int totalRequestedSleepingTime;
    int averageExecutionTimeQuantum;
    int averageWaitingTime;
};
