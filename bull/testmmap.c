#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
	if (access("test2.txt", F_OK) == -1) {
		unlink("test2.txt");
	}
	int fd = open("test2.txt", O_CREAT | O_RDWR, 0666);
	char buf[256];
	memset(buf, 0, sizeof(buf));
	write(fd, buf, sizeof(buf));
	void *mem = mmap(NULL, sizeof(buf), PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	char *msg = "hello world";
	memcpy(mem, msg, strlen(msg) + 1);
	munmap(mem, sizeof(buf));
	return 0;
}
