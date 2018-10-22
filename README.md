# Labs from CS111: Operating Systems Principles 
### Taken with Professor Mark Kampe in Spring 2018


**Lab 0: Warmup**

Basic C programming warmup using the getopt_long(3) API to make a simple CLI application that copies the standard input to standard output with support for --segfault and --catch to raise a segmentation fault.

**Lab 1A: Terminal I/O and Inter-Process Communication**

A multi-process telnet-like client and server. Supports character-at-a-time full duplex terminal I/O with Polled I/O in passing input and output between two processes. 

**Lab 1B: Compressed Network Communication**

A continuation of Lab 1A. We pass input and output over a TCP socket and use the zlib library to compress communication between the client and the server.

**Lab 2A: Races and Synchronization**

A multithreaded application in C using pthreads to update a shared variable. We demonstrate that there is a race condition in the provided `add` routine and we must address it with different synchronization methods such as mutexes, spin locks, test-and-sets, and compare-and-swap. Deals with conflicting read-modify-write operations on single variables and complex data structures (an ordered linked list)/ 

**Lab 2B: Lock Granularity and Performance**

Executed performance instrumentation and measurement through a profiling tool (gprof)
to determine the cause of a problem. Divide the linked-list in Lab 2A into smaller sublists and support finer granularity synchronization and allowing parallel access to the original list.

**Lab 3A: File System Interpretation**

Lab to understand the on-disk data format of the Linux EXT2 file system. A program to analyze a file system image and output a summary to standard output that describes the super block, groups, free-lists, inodes, indirect blocks, and directories in a CSV file.

**Lab 3B: File System Consistency Analysis**

A program to analyze a file system summary (a .csv file in the same format produced for the previous file system project) and report on all discovered inconsistencies. Because this project involves trivial text processing, we used Python.

**Lab 4A: Beaglebone Bring-Up**

A simple project to set up Wifi/Bluetooth and SSH capabilities on a Beaglebone so that students have a working development environment and have the ability to transfer files between machines. 

**Lab 4B: Sensors Input**

A program to take temperature measuerements from the Beaglebone sensors with support for options to change the temperature units, the output period, start/stop buttons, and writing outputs to a log file. 

**Lab 4C: Internet Of Things Security**

A project that applies the principles of Cryptography, Distributed Systems Security, and Secure Socket Layer Encryption. We establish communication with a logging server with our Beaglebone and write to both a TCP server log and an Authenticated TLS session log. 

| Lab           | Score         |
| :-------------: |:-------------:| 
| Lab 0         | 100/100       | 
| Lab 1A        | 98/100        |   
| Lab 1B        | 100/100       |    
| Lab 2A        | 96/100        |   
| Lab 2B        | 97/100        |   
| Lab 3A        | 96/100        |    
| Lab 3B        | 97/100        |    
| Lab 4A        | 100/100       | 
| Lab 4B        | 100/100       |   
| Lab 4C        | 99/100        |    
