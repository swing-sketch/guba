#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <string>
#include <sys/types.h>  
#include <sys/stat.h>  
#include <sys/time.h>
#include <dirent.h>  
#include <vector>
#include <queue>
#include <iostream>

using namespace std;

///////////////////////////////////////////////////////////

struct timeval begin_time, end_time;
double run_time_ms;

size_t name_max = 1024;  

long long count[26];   //存储26个字母出现次数数组

vector<vector<long>> threads_num_letter; //存各线程统计到26个字母次数的二维容器vector

vector<bool> threads_state(4); //线程的状态  这也会涉及伪共享的问题

queue <string> paths; //存文件以及文件夹路径的队列
pthread_mutex_t queue_lock;  //队列锁
pthread_mutex_t vector_lock;  //容器锁
pthread_mutex_t threads_state_lock;  //容器锁



///////////////////////////////////////////////////////////


//注意“伪共享”问题 cache line
void count_in_file(char *path,vector<long> &thread_num_letter)   //vector<long> thread_num_letter 是当前线程统计得到的26个字母出现次数的vector
{
	FILE *search;  
	char buffer[256], *buf_ptr;  
	
	search = fopen(path, "r");
	if(search == NULL)  {
	   printf("unable to open file %s:\n", path);       
	}			   
	else {  
	
		while((buf_ptr = fgets(buffer, sizeof(buffer), search)) != NULL) {  
			char *c = &buffer[0]; //遍历当前行的字母
			while (*c != '\0' && *c != '\n') {    
				if (*c >= 'a' && *c <= 'z') {	 //不区分大小写			
					thread_num_letter[*c - 'a']++; 			
				}
				else if (*c >= 'A' && *c <= 'Z') {   //不区分大小写			
					thread_num_letter[*c - 'A']++; 
				}
				
				c++;
			}			
	   }  
   }   
   
   fclose(search);
}  


void count_in_dir(char *path)
{	
	DIR *directory;  
	struct dirent *entry; 
	struct dirent *result;  
	char new_path[1024];
	int status;
	
	directory = opendir(path);  
	if(directory == NULL) {  
		printf("unable to open directory %s\n", path);  
		exit(1);  
	}  
	
	entry = (struct dirent*)malloc(sizeof(struct dirent) + name_max);  
	if(entry ==  NULL) {
		printf("malloc error\n");
		exit(1);
	}	
	
	while(1) {  
		status = readdir_r(directory, entry, &result);    //读取下一个目录项
		if(status != 0){  
			printf("unable to open directory %s\n", path);              
			break;  
		}  
		if(result == NULL) {
			break;
		}
		
		if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {  //跳过刚开始的..与.目录项
			continue;
		}
		
		strcpy(new_path, path);  
		strcat(new_path, "/");  
		strcat(new_path, entry->d_name);  //拼接成完整的相对路径
				
		pthread_mutex_lock(&queue_lock);  //只能一个线程访问队列，于是对队列加锁
		paths.push(new_path);             //将该相对路径加入队列
		pthread_mutex_unlock(&queue_lock); //给队列解锁
		
	} 
	
	free(entry);
	closedir(directory); 
} 


void *count_thread(void *arg)
{    
int id =*(int *)arg;
int flag=0; 
vector<long> thread_num_letter(26); //每个线程都会有自己存的
    
    while (1){
        string  path;
        bool isEmpty= true;
        pthread_mutex_lock(&queue_lock);   //给队列加锁，只能一个线程访问队列
        
        if (!paths.empty()){       //对列不为空
        
           
            path=paths.front();   //获取队列第一个文件或文件夹路径
            paths.pop();          //出队列
         
            threads_state[id]=true; //表示有任务处理
          
            
            isEmpty= false;
           
        }
        else{
            threads_state[id]=false;  //队列为空，则是等待态s   
        }
        
         pthread_mutex_unlock(&queue_lock);    //给队列解锁s  
    
        
            
        
       
        
        
        if (!isEmpty){    //队列不为空
            int status;
            char* c_path=const_cast<char*>(path.c_str());      //path.c_str() 转为char*
            
            struct stat filestat;
            status = lstat(c_path, &filestat);  
            
            
            
            //判断当前队列第一个元素是文件还是文件夹
            if(S_ISDIR(filestat.st_mode)) {  //文件夹
                count_in_dir(c_path);        //文件夹下的目录项全部存入
            }
            else if(S_ISREG(filestat.st_mode)) {   //文件
                count_in_file(c_path,thread_num_letter);  //对该文件的字母进行统计
            }
        }  //如果队列为空，在全部线程都没有工作时候退出
          //serial_empty_time 代表该线程连续没有任务的次数，如果次数大于3，代表所有线程都没有工作。
          else{
          
       
          
          //判断四个false即可
         
            
 
            
          if(threads_state[0]==false && threads_state[1]==false && threads_state[2]==false && threads_state[3]==false){   //若都是，则退出线程
            break; //结束
          }
          
          }
        
    }
    
   //全部线程都没有处理任务时， 将该线程下统计到的26字母出现次数的vector放入全局二维vector threads_num_letter
    //pthread_mutex_lock(&vector_lock);    //threads_num_letter是全局二维vector，需互斥访问，加锁
    //for(int i=0;i<26;i++){
    //threads_num_letter[id][i]=thread_num_letter[i];  //将当前线程的统计到的26字母出现次数的vector放入threads_num_letter末端
    //}
    //pthread_mutex_unlock(&vector_lock);   //访问结束，解锁
    pthread_mutex_lock(&vector_lock);    //threads_num_letter是全局二维vector，需互斥访问，加锁
    threads_num_letter.push_back(thread_num_letter);  //将当前线程的统计到的26字母出现次数的vector放入threads_num_letter末端
    pthread_mutex_unlock(&vector_lock);   //访问结束，解锁
    
}




int main(int argc, char *argv[])  
{  
	int i, status;	
	pthread_t tid[4];   //定义4个线程.
	char path[1024];
	
	if (argc != 2) {
		printf("Usage: %s path\n", argv[0]);
		return -1;
	}		
	
	strcpy(path, argv[1]);
	
	for (i=0; i<26; i++)  //初始化存储count存储
		count[i] = 0;
	
	for(int m=0;m<4;m++){
	threads_state[m]=false;
	}
	//首先需放入第一个文件路径（即输入文件）
	paths.push(path);
	
	gettimeofday(&begin_time,nullptr);
	
       for (i = 0; i <4; i++){    //创建4个线程，count_thread 是要执行的函数。
        pthread_create(&tid[i], nullptr,count_thread, (void *)(&i));
    	}
    	
    	
    	for (i = 0; i <4; i++){
        //等待四条线程完成
        pthread_join(tid[i],nullptr);
    	}
		

	
	
        
        for(int i=0;i<26;i++){   
             count[i]=threads_num_letter[0][i]+threads_num_letter[1][i]+threads_num_letter[2][i]+threads_num_letter[3][i];
        	
            //printf("%c: %lld\n", 'a'+i, count[i]);//各字母出现次数打印.
        	
        }
        
        gettimeofday(&end_time,nullptr);
	run_time_ms =
        (end_time.tv_sec - begin_time.tv_sec)*1000 +
        (end_time.tv_usec - begin_time.tv_usec)*1.0/1000;
        
        printf("run_time = %lfms\n", run_time_ms);
        
         for(int i=0;i<26;i++){
        printf("%c: %lld\n", 'a'+i, count[i]);//各字母出现次数打印.
        
	}
	return 0;  
}  
