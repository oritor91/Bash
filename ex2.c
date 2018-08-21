#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <pwd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

static int cmdLength =0;
static int numOfCmd = 0;

void sig_handler(int);
void free_2d_array(char**,int);
int find_index_of(char**,char*,int);
void execute(char **,int);
char * split(char *);
void print_location(char *,char*);
int find_redirect_case(char **,int);
void execute_redirect(char **,int,int,int);
void execute_pipe(char **,int,int,bool,char*,int);


void main(int argc, char **argv)
{

	uid_t id = 0;	
	struct passwd *pwd; // struct to get the username
	char userName[20];
	
	char path[256]; 
	while(true)
	{
		signal(SIGINT,sig_handler);
		signal(SIGCHLD,sig_handler);
		print_location(userName,path);
		char copy[510]; // a copy of the first command to check if its a done command.
		char command[510]; 
		fgets(command,510,stdin); // takes the next command 
		strcpy(copy,command); // making a copy of the original command for further use.
		char *str2 = strtok(copy," \n");
		int arr_length=0;
		while(str2!=NULL) // finding the length of the array 
		{
			arr_length++;
			str2 = strtok(NULL," \n");
		}
		char ** newCommand = (char**)malloc(sizeof(char*)*arr_length+1);
		char *str = strtok(command," \n");
		int i=0; 
		while(str!=NULL) // spliting each word into rows 
		{
			newCommand[i++] = split(str);
			str = strtok(NULL," \n");
		}
		newCommand[i] = NULL;

		if(newCommand[0]==NULL)
		{
			free(newCommand);
			continue;
		}

		int redirect_case = find_redirect_case(newCommand,arr_length);
		if(redirect_case!=-1)
		{
			int redirect_index;
			if( redirect_case == 1 ) // case 1 is ">"
				 redirect_index = find_index_of(newCommand,">",arr_length);
			else if( redirect_case == 2 ) // case 2 is ">>"
				 redirect_index = find_index_of(newCommand,">>",arr_length);
			else if( redirect_case == 3 ) // case 3 is "2>"
				 redirect_index = find_index_of(newCommand,"2>",arr_length);
			else if( redirect_case == 4 )
				 redirect_index = find_index_of(newCommand,"<",arr_length);

			execute_redirect(newCommand,arr_length,redirect_index,redirect_case);
			free_2d_array(newCommand,arr_length);
			continue;
		}

		int pipe_index = find_index_of(newCommand,"|",arr_length);
		if(  pipe_index != -1 )
		{
			execute_pipe(newCommand,arr_length,pipe_index,false,NULL,-1);
			free_2d_array(newCommand,arr_length);
			continue;
		}

		else
			execute(newCommand,arr_length);

		free_2d_array(newCommand,arr_length);
	}


}
void execute_pipe(char **arr,int arr_length,int pipe_index,bool is_piped,char * file_name,int case_num)
{
	int status;
	char **left_arr , **right_arr;
	int i,left_size=0,right_size=0;
	for(i=0;i<arr_length;i++)
	{
		if(strcmp(arr[i],"|")<0)
			left_size++; // checks the size of the left command and the right command 	
		else
		{
			right_size = arr_length-left_size-1;
			break;
		}
	}
	left_arr = (char**)malloc(left_size*sizeof(char*)+1);
	right_arr = (char**)malloc(right_size*sizeof(char*)+1);
	int j=0;
	for(i=0;i<arr_length;i++) // inserts the splits commands into their arrays
	{
		if(i<left_size)
			left_arr[i] = arr[i];
		else
		{
			if(i == pipe_index)
				continue;
			right_arr[j++]=arr[i];
		}
	}
	right_arr[right_size]= NULL;
	left_arr[left_size]= NULL;
	int fd[2];
	if(pipe(fd)<0)
	{
		perror("FD Failed\n");
		free_2d_array(arr,arr_length);
		free(left_arr);
		free(right_arr);
		exit(1);
	}
	pid_t left_pid,right_pid;
	int file_fd;
	
	if((left_pid= fork() )== 0)
	{
		close(fd[0]);
		dup2(fd[1],STDOUT_FILENO);
		if(execvp(*left_arr,left_arr)<0)
		{
			printf("execvp failed\n");
			free_2d_array(arr,arr_length);
			free(left_arr);
			free(right_arr);
			exit(1);
		}
	}
	if( (right_pid=fork()) == 0)
	{
		close(fd[1]);
		dup2(fd[0],STDIN_FILENO);
		if(is_piped)
		{
			file_fd = open(file_name, O_WRONLY|O_CREAT|O_TRUNC,0600);
			if(fd<0)
			{
				perror("file open failed \n");
				free_2d_array(arr,arr_length);
				free(left_arr);
				free(right_arr);
				exit(1);
			}
			else
				dup2(file_fd,STDOUT_FILENO);
		}

		if(execvp(*right_arr,right_arr)<0)
		{
			printf("execvp failed\n");
			free_2d_array(arr,arr_length);
			free(left_arr);
			free(right_arr);
			exit(1);
		}
	}
	else
	{

		if(is_piped)
			close(file_fd);
		close(fd[0]);
		close(fd[1]);
		wait(NULL);
		wait(NULL);
		free(left_arr);
		free(right_arr);
	}

}
void sig_handler(int signo)
{
	while(waitpid(-1,NULL,WNOHANG)>0);
}
void execute(char **str,int arr_length)
{
	int status;
	pid_t p;
	int run_in_bg;
	if(strcmp(*str,"cd")==0)
	{
		int check = chdir(*(str+1));
		if(check!=0)
			printf("ERR\n");
		return;
	}
	cmdLength+= (int)strlen(*str);
	numOfCmd++;
	if(strcmp(*str,"done")==0)
	{
		printf("Num of cmd:%d\n",numOfCmd-1);
		printf("Cmd length:%d\n",cmdLength-4);
		printf("Bye\n");
		free_2d_array(str,arr_length);
		exit(0);
	}
	
	
	
	 if( (p=fork()) == 0 )
	{
		run_in_bg = find_index_of(str,"&",arr_length);
		if(run_in_bg != -1)
		{
			char ** new_arr = (char**)malloc(sizeof(char*)*arr_length);
			for(int i=0;i<arr_length-1;i++)
				new_arr[i] = str[i];
			new_arr[arr_length-1] = NULL;

			if(execvp(*new_arr,new_arr)<0)
			{
				perror("execvp failed\n");
				free(new_arr);
				free_2d_array(str,arr_length);
				exit(1);
			}
			else
				free(new_arr);
		}
		else
		{
			if(execvp(*str,str)<0)
			{
				printf("ERR\n");
				free_2d_array(str,arr_length);
				exit(0);
			}
		}
	}
	else if( p < 0 )
		printf("ERR\n");
	else
	{
		if( run_in_bg == -1 )
			while( wait(NULL) );
			
	}
}
char * split(char *str)
{
	char *temp = (char*)malloc(strlen(str));
	assert(temp!=NULL);
	strcpy(temp,str);	
	return temp;
}
int find_redirect_case(char ** arr,int arr_length)
{
	for(int i=0;i<arr_length;i++)
	{
		if(strcmp(arr[i],">") == 0)
			return 1;
		if(strcmp(arr[i],">>") == 0)
			return 2;
		if(strcmp(arr[i],"2>") == 0)
			return 3;
		if(strcmp(arr[i],"<") == 0)
			return 4;
	}
	return -1;
}
void execute_redirect(char ** arr,int arr_length,int index,int case_num)
{
	char ** temp = (char**) malloc ( sizeof(char*)*index + 1 );
	char file_name[510];
	int fd;
	pid_t p;
	for(int i=0;i<arr_length;i++)
	{
		if(i < index)
			temp[i] = arr[i];
		else
		{
			if(i == index)
				continue;
			strcpy(file_name,arr[i]);
		}
	}
	int pipe_index = find_index_of(temp, "|", index);
	if(pipe_index != -1)
	{
		bool is_piped = true;
		execute_pipe(temp, index, pipe_index,is_piped,file_name,case_num);
		free(temp);
		return;
	}
	else
	{
		temp[index] = NULL;
		if( (p=fork()) == 0 )
		{		
			if( case_num == 1 || case_num == 3 )
			{
				fd = open(file_name,O_WRONLY|O_CREAT|O_TRUNC,0600);
				if(fd<0)
				{
					perror("File open failed\n");
					free(temp);
					free_2d_array(arr,arr_length);
					exit(1);
				}
			}
			else if( case_num == 2 )
			{
				fd = open(file_name,O_WRONLY|O_CREAT|O_APPEND,0600);
				if(fd<0)
				{
					perror("File open failed\n");
					free(temp);
					free_2d_array(arr,arr_length);
					exit(1);
				}
			}
			else if( case_num == 4)
			{
				fd = open(file_name,O_RDONLY,0600);
				if(fd < 0)
				{
					fprintf(stderr,"File open failed\n");
					free(temp);
					free_2d_array(arr,arr_length);
					exit(1);
				}
			}

			if(case_num == 1 || case_num == 2)
				dup2(fd,STDOUT_FILENO);
			else if (case_num == 3)
				dup2(fd,STDERR_FILENO);
			else if (case_num == 4)
				dup2(fd,STDIN_FILENO);

			if(execvp(*temp,temp)<0)
			{
				perror("execvp failed\n");
				free(temp);
				free_2d_array(arr,arr_length);
				close(fd);
				exit(1);
			}
		free(temp);
		close(fd);
		}

		else if(p<0)
		{
			printf("ERR \n");
			free(temp);
			free_2d_array(arr,arr_length);
			exit(1);
		}
		else
			wait(NULL);

	}
}
void free_2d_array(char** arr,int arr_length)
{
		for(int i=0;i<arr_length;i++)
		{
				free(arr[i]);
		}
		free(arr);
}
int find_index_of(char** arr,char *c,int arr_length)
{
	for(int i=0;i<arr_length;i++)
	{
		if(strcmp(arr[i],c)==0)
			return i;
	}
	return -1;
}
void print_location(char *userName,char* path)
{
	struct passwd *pwd;
	pwd = getpwuid(0);
	strcpy(userName,pwd->pw_name);
	getcwd(path,256);
	printf("%s@%s:",userName,path);
}