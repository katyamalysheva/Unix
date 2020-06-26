#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
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

int flag = 0;

void sign_handler(int signum) {
	printf("Сигнал %d пойман! \n", signum);
	flag = 1;
}

int Daemon() {
	signal(SIGINT, sign_handler);// если пойман сигнал SIGINT, то вызывается функция sign_handler
	printf("Демон был вызван \n");
	int fd; // файловый дескриптор
	char text[] = "Привет, я демон, и я тут поработал\n";
	while (1) { 
		pause();// приостанавливаем процесс, пока не получим любой сигнал
		if (flag) { // если был пойман SIGINT, то работаем дальше, если другой, то снова приостанавливаем
			fd = open("daemon_output.txt", O_RDWR | O_CREAT, S_IRWXU); //создаем или открываем файл, (S_IRWXU - имеем права на чтение, запись и выполнение)
			lseek(fd, 0, SEEK_END); // настраеваем смещение на конец файла
			write(fd, text, sizeof(text)); //пишем текст в файл
			exit(0); 
		}
	}
	return 0;
}

int main()
{
	pid_t parpid;
	if ((parpid = fork()) == -1) { // создаем дочерний процесс
		printf("\nне работает fork");
		exit(1);
	}
	else if (parpid != 0) // fork() возращает родителю pid, а потомку 0
		exit(0); // если родитель, то завершаем процесс          
	setsid();  // уходим в фон
	printf("Привет, я демон и мой pid: %i\n", getpid());
	Daemon(); //вызываем демона         
	return 0;
}
