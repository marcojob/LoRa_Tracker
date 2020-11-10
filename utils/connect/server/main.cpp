#include <iostream>
#include <unistd.h>
#include <pthread.h>

void *hello(void*) {
    std::cout << "hello from thread " << std::endl;
    while(true) {
        std::cout << "running" << std::endl;
        sleep(1);
    }
}

int main() {
    pthread_t thread;
    pthread_create(&thread, NULL, hello, NULL);

    void* result;
    pthread_join(thread,&result);

    return 0;
}