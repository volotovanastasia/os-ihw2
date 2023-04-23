#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>

#define SHM_SIZE 1024
#define NUM_SEMAPHORES 2
#define SHARED_MEMORY_NAME "/shared_memory_numbers"
#define ARRAY_SIZE 20


sem_t semaphores[NUM_SEMAPHORES];
int shared_memory;

// Обработчик сигнала прерывания (Ctrl+C)
void sigint_handler(int sig) {
    printf("\nProgram was interrupted by user.\n");
    sem_destroy(&semaphores[0]);
    sem_destroy(&semaphores[1]);
    munmap(shared_memory, sizeof(int) * ARRAY_SIZE);
    shm_unlink(SHARED_MEMORY_NAME);
    exit(0);
}

int main() {
    signal(SIGINT, sigint_handler);
    // Создание разделяемой памяти
    shared_memory = shm_open("/shm", O_CREAT | O_RDWR, 0666);
    if (shared_memory == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shared_memory, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    int *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Инициализация семафоров
    if (sem_init(&semaphores[0], 1, 0) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&semaphores[1], 1, 0) == -1) {
        perror("sem_init");
        exit(EXIT_FAILURE);
    }

    // Генерация случайных чисел и заполнение массива
    srand(getpid());
    int nums[20];
    for (int i = 0; i < 20; i++) {
        nums[i] = rand() % 100 + 1;
    }

    // Раунды игры
    int round = 1;
    while (1) {
        int n = 20;
        int num_pairs = 0;
        for (int i = 0; i < n; i += 2) {
            if (i == n - 1) {
                // Если пары не достается, то удваиваем число
                shm_ptr[num_pairs] = nums[i] * 2;
            } else {
                shm_ptr[num_pairs] = (rand() % 2 == 0) ? nums[i] : nums[i+1];
            }
            num_pairs++;
        }

        // Ожидание окончания раунда
        for (int i = 0; i < num_pairs; i++) {
            n = n / 2 + n % 2;
            sem_wait(&semaphores);
        }

        // Проверка победителя
        if (n == 1) {
            printf("Winner have score: %d\n", shm_ptr[0]);
            break;
        }

        // Обновление массива чисел
        for (int i = 0; i < n; i++) {
            nums[i] = shm_ptr[i];
        }

        // Увеличение номера раунда
        round++;
    }

    // Удаление разделяемой памяти
    if (munmap(shm_ptr, SHM_SIZE) == -1) {
        perror("munmap");
    }
    if (shm_unlink("/shm") == -1) {
        perror("shm_unlink");
    }

    // Удаляем семафоры и разделяемую память
    sem_destroy(&semaphores[0]);
    sem_destroy(&semaphores[1]);
    munmap(shared_memory, sizeof(int) * ARRAY_SIZE);
    shm_unlink(SHARED_MEMORY_NAME);

    return 0;
}
