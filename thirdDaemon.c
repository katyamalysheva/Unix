#include <stdio.h> 
#include <string.h> 
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <resolv.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>

int flagDo = 0;
int flagStop = 0;
sem_t* semp; // указатель на семафор 
char* semName = "/sem"; //имя семафора (имя всегда должно начинаться с /)

void sigIntHandler(int signum) {
	printf("Сигнал %d пойман! \n", signum);
	flagStop = 1;
}

void sigHupHandler(int signum) {
	printf("Сигнал %d пойман! \n", signum);
	flagDo = 1;
}

void sigChildHandler(int signum) {
	sem_post(semp); // разблокируем семафор после завершения дочернего процесса
}

int Daemon(char* argv[]) {
	
	signal(SIGHUP, sigHupHandler);// сигнал для активации демона
	signal(SIGINT, sigIntHandler); //сигнал для прерывания демона
	signal(SIGCHLD, sigChildHandler); // сигнал для разблокировки семафора как только завершится дочерний процесс

	printf("Демон был вызван\n");
	pid_t pid;
	char* argv2[] = { argv[1], NULL };
	semp = sem_open(semName, O_CREAT); //создаем семафор для синхронизации процессов
	sem_post(semp); //на всякий случай разблокируем семафор

	while (1) {
		pause(); // приостанавливаем процесс, пока не дождемся какого либо сигнала
		if (flagDo == 1) { // если сигнал SIGHUP был пойман, то
			
			int fd = open(argv[1], O_RDONLY, S_IRWXU); // открываем файл с командами, только для чтения
			if (fd == -1) {
				printf("Ошибка открытия файла\n");
				exit(1);
			}
			// считываем команды из файла
			char buf[1024];
			read(fd, buf, sizeof(buf));
			close(fd);
			char* commands[1024];
			int commands_cnt = 0;

			char* command = strtok(buf, "\n");
			
			while (command != NULL) {
				commands[commands_cnt] = command;
				commands[commands_cnt][strlen(command)] = '\0';
				command = strtok(NULL, "\n");
				commands_cnt++;
			}

			fd = open("daemon_output.txt", O_RDWR | O_CREAT, S_IRWXU);
			ftruncate(fd, 0); // чистим выходной файл

			for (int i = 0; i < commands_cnt; i++) {//для каждой команды в файле
				pid = fork(); // создаем дочерний процесс
				if (pid == 0) {// если дочерний процесс, то
					sem_wait(semp); // ждем, пока освободится семафор и блокируем его
					lseek(fd, 0, SEEK_END); // сдвигаемся на конец файла
					dup2(fd, 1); // перенаправлеяем вывод с stdout в наш файл 
					
					execve(commands[i], argv2, NULL); // запускаем команду, как только процесс выполнения закончится, sigChildHandler разблокирует семафор
				}
				
			}
			close(fd);
		}
		flagDo = 0; //  обнуляем флаг, чтобы можно было использовать демона несколько раз

		if (flagStop == 1) { // если пойман сигнал на прерывание демона
			sem_wait(semp); // ждем, пока разблокируется семафор и блокируем его
			sem_post(semp); 
			sem_unlink(semName); // удаляем семафор через имя
			printf("Демон изгнан\n");
			exit(0);
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{
	pid_t parpid;
	if ((parpid = fork()) ==-1 ) {
		printf("\nне могу работать");
		exit(1);
	}
	else if (parpid != 0) {
		
		exit(0); 
	}
	setsid(); // дисконектим с shell
	printf("Здрасте, я Демон, и мой pid: %i\n", getpid());
	Daemon(argv); // вызываем демона
	return 0;
}
