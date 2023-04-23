#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>

#define SHM_SIZE 1024
#define NUM_SEMAPHORES 2
#define SEMAPHORES_NAME "/semaphore"
#define SHARED_MEMORY_NAME "/shared_memory_numbers"
#define ARRAY_SIZE 20


sem_t *semaphores[NUM_SEMAPHORES];
int shared_memory;

// Обработчик сигнала прерывания (Ctrl+C)
void sigint_handler(int sig) {
    printf("\nProgram was interrupted by user.\n");
    sem_close(semaphores);
    sem_unlink(SEMAPHORES_NAME);
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
    for (int i = 0; i < NUM_SEMAPHORES; i++) {
        semaphores[i] = sem_open(SEMAPHORES_NAME, O_CREAT, 0666, 0);
        if (semaphores[i] == SEM_FAILED) {
            perror("sem_open");
            exit(1);
        }
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
        int num_pairs = 0;
        for (int i = 0; i < 20; i += 2) {
            if (i == 18) {
                // Если пары не достается, то удваиваем число
                shm_ptr[num_pairs] = nums[i] * 2;
            } else {
                // Объединение пары энергий
                shm_ptr[num_pairs] = (rand() % 2 == 0) ? nums[i] : nums[i+1];
            }
            num_pairs++;
        }

        // Ожидание окончания раунда
        for (int i = 0; i < num_pairs; i++) {
            sem_wait(semaphores);
        }

        // Проверка победителя
        if (round == 10) {
            printf("Winner have score: %d\n", shm_ptr[0]);
            break;
        }

        // Обновление массива чисел
        for (int i = 0; i < num_pairs; i++) {
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
    sem_close(semaphores);
    sem_unlink(SEMAPHORES_NAME);
    munmap(shared_memory, sizeof(int) * ARRAY_SIZE);
    shm_unlink(SHARED_MEMORY_NAME);

    return 0;
}
