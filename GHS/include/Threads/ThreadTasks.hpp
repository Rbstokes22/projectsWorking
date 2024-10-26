#ifndef THREADTASKS_HPP
#define THREADTASKS_HPP

namespace ThreadTask {

void netTask(void* parameter);
void DHTTask(void* parameter);
void AS7341Task(void* parameter);
void soilTask(void* parameter);
void relayTask(void* parameter);

}

#endif // THREADTASKS_HPP