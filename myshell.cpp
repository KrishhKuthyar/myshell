#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>


#define INPUT_SIZE 80
#define PATH_SIZE 255

using namespace std;
pid_t p_id;
pid_t c_id;
int num_files;
int num_directory;
std::vector<int> time_intervals;


void excute();
string read_input();
string command_judge(string input);
void shell_command(string input);
void SplitString(const string& input, vector<string>& v, const string& delim);
void shell_exit();
void shell_pwd();
void shell_cd(string path);
void shell_tree(string path,int depth);
void shell_mymtimes(string path);
void shell_external(string command);
char** parse(char * input,char * delim);
void sig_handler(int signum);
int redirect_cout(string line);
void shell_redirect(string command);
void shell_pipe(string line);

// use function excute() to excute;
int main(void){

    excute();

    return 0;
}

//according to the read of input, judge the typeof input and call different functions;
void excute(){
    string input;
    while(1){
        cout<<"mytoolkit shell$ ";
        input=read_input();
        string type;
        type=command_judge(input);

        if(type=="command"){
            shell_command(input);
        }
        else if(type=="redirection"){
            shell_redirect(input);
        }
        else if(type=="pipe"){
            shell_pipe(input);
        }
    }
}

//read the input and store it in the 'temp'
string read_input(){
    string result;
    int temp=0;   

    while (temp!='\n') {
        // Read in the char
       temp=getchar();
        //Exit when input EOF(control+D)
        if(temp==EOF){
            cout<<"\n mytoolkit shell is terminated! by EOF \n";
            exit(EXIT_FAILURE);           
        }
        //End input when have a return
       else{
            result.push_back(temp);    
        }
        //Error in more than 80 characters
        if(result.size()>=80){
            cerr<< "mytoolkit shell: Max size of your input line is 80!"<<endl;
            exit(EXIT_FAILURE);
        }
    }
    result.pop_back();
    return result;
}

//from excute() function, to judge the input is pipe/IO/command;
string command_judge(string input){
    if(input.find("|")!=string::npos)
        return "pipe";

    if(input.find("<")!=string::npos || input.find(">")!=string::npos)
        return "redirection";

    return "command";    
    
}

//If the input is the type of command, judge the command content and call different functions.
void shell_command(string input){
    p_id=getpid();
    vector<string> tokens;

    //call functions to split string and store it into tokens.
    SplitString(input,tokens," ");
    if(tokens[0]=="myexit")
        shell_exit();
    else if(tokens[0]=="mypwd")
        shell_pwd();
    else if(tokens[0]=="mycd")
        shell_cd(tokens[1]);
    else if(tokens[0]=="mytree"){
        num_files=0;
        num_directory=0;
        std::string path;
        if(tokens.size()==1)
            path=".";
        else
            path=tokens[1];
        shell_tree(path,0);    
        std::cout<<num_directory<<" directory, "<<num_files<<" files"<<std::endl;     
    }
    else if(tokens[0]=="mytime"){
        
        //clock_t Used for system times in clock ticks or CLOCKS_PER_SEC
        //The <sys/times.h> header shall define the structure tms, which is returned by times() and includes at least the following members：
            //clock_t  tms_utime  User CPU time. 
            //clock_t  tms_stime  System CPU time. 
            //clock_t  tms_cutime User CPU time of terminated child processes. 
            //clock_t  tms_cstime System CPU time of terminated child processes.
        static clock_t start_time;
        static clock_t end_time;
        static struct tms start_cpu;
        static struct tms end_cpu;
           
        start_time = times(&start_cpu);
        string new_input;
        for(int i=1;i<(int)tokens.size();i++)
            new_input+=tokens[i]+" ";

        shell_command(new_input);
          
        end_time = times(&end_cpu);


        printf("Real Time: %jd, User Time %jd, System Time %jd\n",
            (intmax_t)(end_time - start_time), 
            (intmax_t)(end_cpu.tms_utime - start_cpu.tms_utime), 
            (intmax_t)(end_cpu.tms_stime - start_cpu.tms_stime));  

    }
    else if(tokens[0]=="mymtimes"){
        //build a vector time_intervals to show every 24 hours.
        time_intervals.resize(24,0);
        string path;

        //determine the path of dir that we want to report
        if (tokens.size()==1)
            path=".";
        else
           path=tokens[1];

        shell_mymtimes(path);
                time_t current_time;  //time_t Used for time in seconds.
        time(& current_time);

        for(int i=23;i>=0;i--){
           time_t temp=current_time-i*3600;
           string result(ctime(&temp));
           result.pop_back();
           cout<<result<<": "<<time_intervals[i]<<endl;
        }  
        
    }
    else if(tokens[0]=="mytimeout"){
        int time_counter=stoi(tokens[1]);
        string new_path;

        //new a new_path to store last two thing of command and time
        for(int i=2;i<(int)tokens.size();i++)
            new_path+=tokens[i]+" ";
        new_path.pop_back();  //there has an " "in the last.

        //when we request the alarm of the time we input, it would generate SIGALRM signal, and do the behavior sig_handler.
        signal(SIGALRM,sig_handler);
        alarm(time_counter);
        
        //pid_t data type stands for process identification and it is used to represent process ids. 
        //When we want to declare a variable that is going to be deal with the process ids we can use pid_t data type.
        pid_t pid;
        pid = fork();  //fork() call (parent process),Fork system call is used for creating a new process, which is called child process
        if (pid == 0) {
        // Child process
        shell_command(new_path);
        } else if (pid < 0) {
        // Error forking
        cerr<<"mytoolkit shell: error forking!"<<endl;
        } 
        else {
        // Parent process
        c_id=pid;
        waitpid(pid, 0, 0);
        }

        //shell_command(new_path);
        alarm(0);
    }
    else{
        shell_external(input);
    }


}  

void SplitString(const std::string& input, std::vector<std::string>& v, const std::string& delim){
    std::string::size_type pos1, pos2;
    pos2 = input.find(delim);
    pos1 = 0;
    while(std::string::npos != pos2)
    {
        v.push_back(input.substr(pos1, pos2-pos1));
        
        pos1 = pos2 + delim.size();
        pos2 = input.find(delim, pos1);
    }
    if(pos1 != input.length())
        v.push_back(input.substr(pos1));
}

void shell_exit(){
 std::cerr<<"mytoolkit shell: the shell is teminated!"<<std::endl;
 exit(EXIT_SUCCESS);
}

void shell_pwd(){
    
    char path[PATH_SIZE];
    if (getcwd(path, PATH_SIZE) != 0) 
        std::cout<<path<<std::endl;
}

void shell_cd(string path){
    if(chdir(path.c_str())!=0)
        cerr<<"mytoolkit shell: wrong path"<<endl;
}

void shell_tree(string path,int depth){
    
    DIR *d;
    struct dirent *w;
    struct stat buf;
    
    // new a 'depth' to output the number of dir and file
    if(depth==0)
        cout<<path<<endl;

    depth++;

    //path is not exist
    if ((d = opendir(path.c_str())) == NULL) {
        cerr<<"This is not supposed to happen."<<endl;
        exit(EXIT_FAILURE);
    }

    //judge the w which store the path
    while ((w = readdir(d)) != NULL) {
        if(w->d_name[0]!='.'){
            string file_name(w->d_name);
            string file_path=path+"/"+file_name;
            if (lstat(file_path.c_str(), &buf) <0) {
                cerr<<"stat "<<file_name<<" failed (probably file does not exist)."<<endl;
                exit(EXIT_FAILURE);
            }

            if (S_ISREG(buf.st_mode)){
                //++depth;
                for(int i=0;i<depth;i++){
                    cout<<"  ";  
                }
                cout<<w->d_name<<endl;
                num_files++;
            }
            else if (S_ISDIR(buf.st_mode)){
                ++depth;
                for(int i=0;i<depth;i++){
                    cout<<"  ";
                }
                cout<<w->d_name<<endl;
                num_directory++;
                shell_tree(file_path,depth);
            }
        }
    }  

}

void shell_mymtimes(string path){

    DIR *d;
    struct dirent *w;
    struct stat buf;

    if ((d = opendir(path.c_str())) == NULL) {
        cerr<<"This is not supposed to happen."<<endl;
        exit(EXIT_FAILURE);
    }

    while ((w = readdir(d)) != NULL) {

        if(w->d_name[0]!='.'){
            string file_name(w->d_name);
            string file_path=path+"/"+file_name;
            if (lstat(file_path.c_str(), &buf) <0) {
                cerr<<"stat "<<file_name<<" failed (probably file does not exist)."<<endl;
                exit(EXIT_FAILURE);
            }

            if (S_ISREG(buf.st_mode)){
                time_t modify_time=buf.st_mtime;
                time_t current_time;
                time(& current_time);
                int hour=(current_time-modify_time)/3600;
                if(hour<24){
                    time_intervals[hour]++;
                }
            }
            else if (S_ISDIR(buf.st_mode)){
                shell_mymtimes(file_path);
            }
        }
    }

}

void shell_external(string command){

    char comd_temp[INPUT_SIZE];
    strcpy(comd_temp, command.c_str());
    char del[] =" ";
    char **ex_command=parse(comd_temp,del);
    pid_t pid;
    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(ex_command[0], ex_command) == -1) {
            cerr<<"mytoolkit shell: no such command!"<<endl;
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        cerr<<"mytoolkit shell: error forking!"<<endl;
    } else {
        // Parent process
        waitpid(pid, 0, 0);
    }

  
}

char** parse(char * input,char * delim){

    char** argv=(char**)malloc(INPUT_SIZE * sizeof(char *));
    int argc=0;
    char* temp = strtok(input,delim);
    while(temp){
        argv[argc]=temp;
        argc++;
        temp = strtok(NULL,delim);
    }
    argv[argc]=NULL;
    return argv;
}

void sig_handler(int signum){
        cout<<"mytoolkit shell: the command is timed out!"<<endl;
        kill(c_id,SIGTERM);
}

int redirect_cout(string line){
    
    bool flag1=false;
    bool flag2=false;

    if(line.find(">")!=std::string::npos)
        flag1=true;
    if(line.find("<")!=std::string::npos)
        flag2=true;

    // 'command > file' means do commond and store result in file.
    // 'command < file' means use command on this file 
    if(flag1&&!flag2)
        return 1;
    else if(!flag1&&flag2)
        return 2;
    else if(flag1&&flag2) 
        return 3;
    else 
        return 4;
}

void shell_redirect(string command){
    int counter=0;
    counter=redirect_cout(command);
    char * c_command = new char[command.length() + 1];
    strcpy(c_command, command.c_str()); //store the command content into c_command
    char space[]=" ";
    if(counter==1){
        char del[]=">";
        char **line=parse(c_command, del);
        char **command1=parse(line[0],space);
        char **command2=parse(line[1],space);
        int pid = fork();
        
        if (pid == -1) {
            cerr<<"error in fork"<<endl;
        } else if (pid == 0) {
            int fd1 = creat(command2[0], 0644);
            dup2(fd1, STDOUT_FILENO);
            close(fd1);
            execvp(command1[0], command1);  
            fprintf(stderr, "Failed to exec %s\n", command1[0]);
            
        } else {
            waitpid(pid, 0, 0);
            free(command1);
            free(command2);
        }
    }
    else if(counter==2){
        char del[]="<";
        char **line=parse(c_command, del);
        char **command1=parse(line[0],space);
        char **command2=parse(line[1],space);
        int pid = fork();
        
        if (pid == -1) {
            perror("fork");
        } else if (pid == 0) {
            int fd0 = open(command2[0], O_RDONLY);
            dup2(fd0, STDIN_FILENO);
            close(fd0);
            execvp(command1[0], command1);
            fprintf(stderr, "Failed to exec %s\n", command1[0]);
            
        } else {
            waitpid(pid, 0, 0);
            free(command1);
            free(command2);
        }
    }
    else if(counter==3){
        char del[]="<>";
        char **line=parse(c_command, del);
        char **command1=parse(line[0],space);
        char **command2=parse(line[1],space);
        char **command3=parse(line[2],space);
        int pid1 = fork();
        if (pid1 == -1) {
            perror("fork");
        } else if (pid1 == 0) {
            int fd0 = open(command2[0], O_RDONLY);
            int fd1 = creat(command3[0], 0644);
            dup2(fd0, STDIN_FILENO);
            dup2(fd1, STDOUT_FILENO);
            close(fd0);
            close(fd1);
            execvp(command1[0], command1);
            fprintf(stderr, "Failed to exec %s\n", command1[0]);
            
        } else {
            waitpid(pid1, 0, 0);
            free(command1);
            free(command2);
            free(command3);
        }

    }
}

void shell_pipe(string line){

    vector<string> tokens;
    SplitString(line,tokens,"|");
    int no_para=tokens.size();
    for(auto token: tokens){
        //cout<<"before: "<<token<<endl;
        int begin_flag=0;
        int end_flag=tokens.size()-1;
        for(int i=0;i<(int)token.size();i++){
            if(token[i]!=' '){
                begin_flag=i;
                break;
            }        
        }
        for(int i=(int)token.size();i>=0;i--){
            if(token[i]!=' '){
                end_flag=i;
                break;
            }    
        }
        token=token.substr(begin_flag,end_flag-begin_flag);
    }
    vector<char*> cstrings{};

    for(auto& string : tokens)
        cstrings.push_back(&string.front());
    
    char **pipe_command;
    pipe_command=cstrings.data();
    char space[]=" ";
    if (no_para==2){
        int fds[2];
        char **command1=parse(pipe_command[0],space);
        char **command2=parse(pipe_command[1],space);
        if(pipe(fds)==-1)
            cerr<<"mytoolkits shell: error in pipe"<<endl;
        
        if (fork()== 0) {
            close(1); 
            if(dup(fds[1])==-1)
                cerr<<"mytoolkits shell: error in dup"<<endl;
            close(fds[1]);close(fds[0]);
            execvp(command1[0],command1);
            exit(0);
        }
        if (fork() == 0) {
            close(0); 
            if(dup(fds[0])==-1){
                cerr<<"mytoolkits shell: error in dup"<<endl;
            } /* redirect standard input to fds[1] */
            close(fds[0]);close(fds[1]);
            execvp(command2[0],command2);
            exit(0);
        }
        close(fds[0]); close(fds[1]);
        free(command1);
        free(command2);
        int stat;
        //free(pipe_command);
        wait(&stat);
        wait(&stat);
    }
    if (no_para==3){
    //     //int child[3];
        //cout<< "para 3"<<endl;
        int fds[2];
        char **command1=parse(pipe_command[0],space);
        char **command2=parse(pipe_command[1],space);
        char **command3=parse(pipe_command[2],space);
        if(pipe(fds)==-1)
            cerr<<"mytoolkits shell: error in pipe"<<endl;
        if (fork()== 0) {
            close(1); 
            if(dup(fds[1])==-1){
                cerr<<"mytoolkits shell: error in dup"<<endl;
            }  /* redirect standard output to fds[0] */
            close(fds[1]);close(fds[0]);
            execvp(command1[0],command1);
            exit(0);
        }
        if (fork() == 0) {
            close(1); 
            if(dup(fds[1])==-1){
                cerr<<"mytoolkits shell: error in dup"<<endl;
            }  /* redirect standard output to fds[0] */
            close(0); 
            if(dup(fds[0])==-1){
                cerr<<"mytoolkits shell: error in dup"<<endl;
            } /* redirect standard input to fds[1] */
            close(fds[0]);close(fds[1]);
            execvp(command2[0],command2);
            exit(0);
        }
        if (fork() == 0) {
            close(0); 
            if(dup(fds[0])==-1){
                cerr<<"mytoolkits shell: error in dup"<<endl;
            } /* redirect standard input to fds[1] */
            close(fds[0]);close(fds[1]);
            execvp(command3[0],command3);
            exit(0);
        }
        close(fds[0]); close(fds[1]);
        int stat;
        free(command1);
        free(command2);
        free(command3);
        //free(pipe_command);
        wait(&stat);
        wait(&stat);
        wait(&stat);
        
    }
}

