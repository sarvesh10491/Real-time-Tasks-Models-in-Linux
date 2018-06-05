Real-time Tasks Models in Linux 

   Following project is used to program real-time tasks on Linux environment including periodic and aporadic tasks, event handling, priority inheritance, etc. & use Linux trace tools to view and analyze real-time scheduling..

Getting Started :

    These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 
    See deployment for notes on how to deploy the project on a live system.

Prerequisites :

  Linux kernel (preferably linux 2.6.19 and above)
  GNU (preferably gcc 4.5.0 and above)

Installing :

Download below files in user directory on your machine running linux distribution.

   1)task1.c
   2)Makefile
   3)input.txt
   4)Report_1
   5)pi_disabled
   6)pi_enabled


Deployment :

   Open the terminal & go to directory where files in installing part have been downloaded. [cd <Directory name>] 

   In the make file we gave the path as "/opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux" for compiler

   if you have a different location then change it.
   
   Use below command to to compile :
   For Linux Host -
	make
   
   For Galileo-
   	make TEST_TARGET=Galileo2

   To run on Galieo, send the object file & input file to the 
   galileo
   board using the follwing command (change IP & home
   accordingly)

   sudo scp task1 root@192.168.0.100:/home
   sudo scp input.txt root@192.168.0.100:/home

   Connect to Galileo board with root login

   Mouse events are detected through /dev/input/eventX device
   file. Here header is written for /dev/input/event2 file
   Check on which file on your board/host & change in task1.c
   accordingly.

   On Galileo2 board/host, ensure that 666(rw- rw- rw-) file
   permissions exist for /dev/input/event2.

   You can check by the following command 
   ls -lrt /dev/input/event2. 

   Otherwise change using the following command 
   chmod 666 /dev/input/event2

   Once above completed then run the below command to execute
   the program code
   
   cd /home
   sudo ./task1   ==> (./task1 on Galileo)


Expected results :

It asks for input choice to select execution to be done with PI enabled mutex or without it.

Entering the choice sets program in execution with subsequent periodicity of tasks & mouse click release giving aperiodic events.  

Each column represents separate task execution.

Built With :

  Linux 4.10.0-28-generic
  x86_64 GNU/Linux
  64 bit x86 machine

Authors :

Sarvesh Patil 
Vishwakumar Doshi

License :

This project is licensed under the ASU License

