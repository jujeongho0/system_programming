#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1
#define GPIO_RED 17 
#define GPIO_GREEN 27
#define GPIO_BLUE  22
#define VALUE_MAX    256 

static int GPIOExport(int pin) 
{
#define BUFFER_MAX 3
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;
	
	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) 
	{
		fprintf(stderr, "Failed to open export for writing!\n");
		return(-1);
	}
	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

static int GPIOUnexport(int pin) 
{
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;
	
	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) 
	{
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return(-1);
	} 
	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

static int GPIODirection(int pin, int dir) 
{
#define DIRECTION_MAX 45
	static const char s_directions_str[] = "in\0out";
	char path[DIRECTION_MAX]="/sys/class/gpio/gpio%d/direction";
	int fd;
	
	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) 
	{
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return(-1);
	}
	if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) 
	{
		fprintf(stderr, "Failed to set direction!\n");
		return(-1);
	}
	close(fd);
	return(0);
}

static int GPIOWrite(int pin, int value) 
{
	static const char s_values_str[] = "01";
	char path[VALUE_MAX];
	int fd;
	
	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) 
	{
		close(fd);
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return(-1);
	}
	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) 
	{
		close(fd);
		fprintf(stderr, "Failed to write value!\n");
		return(-1);		
	}
	close(fd);
}

void error_handling( char *message)
{
	fputs(message,stderr);
	fputc( '\n',stderr);
	exit( 1);
}

int main(int argc, char *argv[]) 
{
	int sock;
	struct sockaddr_in serv_addr;
	char msg[4];
	int str_len;
	int brightness = 0;
	if(argc!=3)
	{
		printf("Usage : %s <IP> <port>\n",argv[0]);
		exit(1);
	}
	
	if(-1 == GPIOExport(GPIO_RED))
		return(1);
		
	if(-1 == GPIOExport(GPIO_GREEN))
		return(1);
		
	if(-1 == GPIOExport(GPIO_BLUE))
		return(1);
		
	if(-1 == GPIODirection(GPIO_RED,OUT))
		return(2);
		
	if(-1 == GPIODirection(GPIO_GREEN,OUT))
		return(2);
		
	if(-1 == GPIODirection(GPIO_BLUE,OUT))
		return(2);
		
	while(1){
		sock = socket(PF_INET, SOCK_STREAM, 0);
		if(sock == -1)	error_handling("socket() error");
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
		serv_addr.sin_port = htons(atoi(argv[2]));
		if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)	
			error_handling("connect() error");
		str_len = read(sock, msg, sizeof(msg));
		if(str_len == -1) error_handling("read() error");		
		brightness = atoi(msg);
		printf("brightness : %d\n",brightness);
		if(brightness < 400){ //dark	
			printf("LED ON!\n");			
			if(brightness%7 == 0){//RGB
				if(-1 == GPIOWrite(GPIO_RED,1))	return 3;					
				if(-1 == GPIOWrite(GPIO_GREEN,1)) return 3;					
				if(-1 == GPIOWrite(GPIO_BLUE,1)) return 3;
			}
			else if(brightness%7 == 1){//RG
				if(-1 == GPIOWrite(GPIO_RED,1)) return 3;					
				if(-1 == GPIOWrite(GPIO_GREEN,1)) return 3;				
			}
			else if(brightness%7 == 2){//R
				if(-1 == GPIOWrite(GPIO_RED,1))	return 3;				
			}
			else if(brightness%7 == 3){//GB
				if(-1 == GPIOWrite(GPIO_RED,1))	return 3;					
				if(-1 == GPIOWrite(GPIO_GREEN,1)) return 3;			
			}
			else if(brightness%7 == 4)//G
				if(-1 == GPIOWrite(GPIO_GREEN,1)) return 3;
			else if(brightness%7 == 5){//RB
				if(-1 == GPIOWrite(GPIO_RED,1))	return 3;					
				if(-1 == GPIOWrite(GPIO_BLUE,1)) return 3;				
			}
			else//B					
				if(-1 == GPIOWrite(GPIO_BLUE,1)) return 3;
		}
		else{//bright			
			if(-1 == GPIOWrite(GPIO_RED,0))	return 3;
			if(-1 == GPIOWrite(GPIO_GREEN,0)) return 3;
			if(-1 == GPIOWrite(GPIO_BLUE,0)) return 3;			
		}
		usleep(500 * 100);
		close(sock);
	}
	
	if(-1 == GPIOUnexport(GPIO_RED))
		return 4;
	if(-1 == GPIOUnexport(GPIO_GREEN))
		return 4;
	if(-1 == GPIOUnexport(GPIO_BLUE))
		return 4;
		
	return(0);
}
