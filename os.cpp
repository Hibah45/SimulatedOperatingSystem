#include "hardware.h"
#include "process.h"
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "Usage: " << argv[0] << " membytes [files]" << endl;
        exit(1);
    }
    sem_init(&critical, 0, 1);
    pthread_t *pid = new pthread_t[argc - 2];
    for (int i = 2; i < argc; i++)
    {
        cout << "\nRunning " << argv[i] << "..." << endl;
        pthread_create(&pid[i - 2], NULL, runFile, (void *)argv[i]);
        sleep(1);
    }
    for (int i = 2; i < argc; i++)
    {
        pthread_join(pid[i - 2], NULL);
    }
    return 0;
}