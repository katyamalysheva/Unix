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

int flag = 0;

void sign_handler(int signum) {
	printf("Сигнал %d пойман! \n", signum);
	flag = 1;
}

int Daemon(char* argv[]) {
	signal(SIGINT, sign_handler);
	printf("Демон был вызван \n");
	int fd; // файловый дискриптор
	int str_len;
	char buf[128];
	fd = open(argv[1], O_RDONLY, S_IRWXU); // только для чтения
	if (fd == -1) {
		printf("Ошибка open()");
		exit(1);
	}
	str_len = read(fd, buf, 128);
	close(fd);
	buf[str_len - 1] = '\0';
	printf("В файле была команда %s\n", buf);
	while (1) {
		pause();// приостанавливаем процесс, пока не получим любой сигнал
		if (flag) { // если был пойман SIGINT, то работаем дальше, если другой, то снова приостанавливаем
			fd = open("daemon_output.txt", O_RDWR | O_CREAT, S_IRWXU); //создаем или открываем файл, (S_IRWXU - имеем права на чтение, запись и выполнение)
			int saveStdout = dup(1); // сохраняем stdout
			dup2(fd, 1); // записываем вместо stdout файловый дескриптор файла вывода		
			char* argv2[] = { argv[1], NULL };
			execve(buf, argv2, NULL); //выполняем программу buf (при успешном выполнении execve не возвращается)
			dup2(saveStdout, 1); 
			
		}
		return 0;}
}

int main(int argc, char* argv[])
{
	pid_t parpid;
	if ((parpid = fork()) == -1) { // создаем дочерний процесс
		printf("\nне работает fork");
		exit(1);
	}

	else if (parpid != 0) // fork() возращает родителю pid, а потомку 0
		exit(0);// если родитель, то завершаем процесс   
	
	setsid();
	printf("Привет, я демон и мой pid: %i\n", getpid());
	Daemon(argv);          
	return 0;
}
