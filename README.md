# Bash

Program objective: creating a shell and practice the system calls fork,wait,signals
The program include infinity while loop that every iteration get the command from the 
Promt , every command will store into dynamic 2d chars array 
The run function will get the array and run it using execvp so / chdir (cd command)

Compile: enter the folder that contain the files and enter make in the shell
The program supports one pipe commands and redirection command i/o
The program handle SIGINT and SIDCHLD signals
The program support backGround commands with &
Run:  ./<app>
 program Files: ex2.c ,makefile 
input: basic shell commands, one pipe command , backGround command & , redirection command 
output: shell output,and error message if an error occurred
