#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE 1024 // Размер буфера для чтения и записи данных

int bytes_to_write; // Количество байт для записи в файл

void sig_child_handler(int signum) {
    bytes_to_write = BUFFER_SIZE;
}

// Функция для процесса-потомка
void childProcess(int N, const char *output_file) {
    FILE *fp = fopen(output_file, "a"); // Открытие файла для записи
    if (fp == NULL) { // Проверка успешности открытия файла
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    signal(SIGUSR1, sig_child_handler); // Установка обработчика сигнала

    while (1) {
        pause(); // Ожидание сигнала SIGUSR1 от родителя

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE); // Инициализация буфера нулями

        // Генерация N чисел и запись их в буфер
        for (int i = 0; i < N; i++) {
            snprintf(buffer + strlen(buffer), BUFFER_SIZE - strlen(buffer), "%d ", rand() % 100);
        }

        // Запись буфера в файл
        if (fwrite(buffer, sizeof(char), strlen(buffer), fp) < strlen(buffer)) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        fflush(fp); // Сброс буфера

        // Если прочитано достаточно байт, завершаем работу
        if (bytes_to_write <= 0) {
            fclose(fp); // Закрытие файла в процессе-потомке
            exit(EXIT_SUCCESS); // Выход из процесса-потомка
        }
    }
}

// Функция для процесса-родителя
void parentProcess(int M, const char *output_file) {
    pid_t child_pid;
    int status;

    child_pid = fork(); // Создание потомка
    if (child_pid == -1) { // Проверка успешности создания потомка
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (child_pid == 0) {
        childProcess(M, output_file); // Вызов функции для процесса-потомка
    }

    // Установка сигнала SIGALRM для родителя
    signal(SIGALRM, sig_child_handler);

    // Отправляем M сигналов SIGUSR1 потомку
    for (int i = 0; i < M; i++) {
        alarm(1); // Установка таймера на 1 секунду
        pause(); // Ожидание сигнала SIGALRM
        kill(child_pid, SIGUSR1); // Отправить сигнал SIGUSR1 потомку
    }

    usleep(1000000); // Подождать 1 секунду
    kill(child_pid, SIGUSR2); // Отправить сигнал SIGUSR2 для завершения работы потомка

    waitpid(child_pid, &status, 0); // Ожидание завершения работы потомка

    printf("File content:\n");
    // Вывод содержимого файла на экран
    FILE *read = fopen(output_file, "r"); // Открытие файла для чтения
    if (read == NULL) { // Проверка успешности открытия файла для чтения
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = fread(buffer, sizeof(char), BUFFER_SIZE, read)) > 0) {
        fwrite(buffer, sizeof(char), bytes_read, stdout); // Вывод данных файла на экран
    }

    if (ferror(read)) { // Проверка ошибок чтения файла
        perror("fread");
        fclose(read);
        exit(EXIT_FAILURE);
    }

    printf("\n");

    fclose(read); // Закрытие файла после чтения
}

int main(int argc, char *argv[]) {
    if (argc != 3) { // Проверка правильности переданных аргументов
        fprintf(stderr, "Usage: %s <M> <N> <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int M = atoi(argv[1]); // Чтение значения M
    const char *output_file = argv[2]; // Чтение имени выходного файла
    bytes_to_write = M * BUFFER_SIZE; // Инициализация переменной для записи в файл

    parentProcess(M, output_file); // Вызов функции для процесса-родителя

    return EXIT_SUCCESS;
}
