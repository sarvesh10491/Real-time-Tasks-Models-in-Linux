ifeq ($(TEST_TARGET), Galileo2)
	CC=/opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux/i586-poky-linux-gcc
	
else
	CC=gcc

endif


APP = task1

all:
	$(CC) -Wall -o $(APP) task1.c -pthread -lm 

clean:
	
	rm -f *.o
	rm -f $(APP) 
