#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <sys/types.h>  
#include <sys/stat.h>  
#include <dirent.h>  

///////////////////////////////////////////////////////////

struct timeval begin_time, end_time;
double run_time_ms;

size_t name_max = 1024;  

long count[26];

///////////////////////////////////////////////////////////

void count_in_file(char *path)
{
	FILE *search;  
	char buffer[256], *buf_ptr;  
	
	search = fopen(path, "r");
	if(search == NULL)  {
	   printf("unable to open file %s: %d (%s)\n", path);       
	}			   
	else {  
		while((buf_ptr = fgets(buffer, sizeof(buffer), search)) != NULL) {  
			char *c = &buffer[0];
			while (*c != '\0' && *c != '\n') {
				if (*c >= 'a' && *c <= 'z') {
					count[*c - 'a']++; 
				}
				else if (*c >= 'A' && *c <= 'Z') {
					count[*c - 'A']++; 
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
		status = readdir_r(directory, entry, &result);  
		if(status != 0){  
			printf("unable to open directory %s\n", path);              
			break;  
		}  
		if(result == NULL) {
			break;
		}
		
		if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		
		strcpy(new_path, path);  
		strcat(new_path, "/");  
		strcat(new_path, entry->d_name);  
				
		struct stat filestat;
		status = lstat(new_path, &filestat);  
		if(S_ISDIR(filestat.st_mode)) {
			count_in_dir(new_path);
		}
		else if(S_ISREG(filestat.st_mode)) { 
			count_in_file(new_path);
		}
	} 
	
	free(entry);
	closedir(directory); 
} 


int main(int argc, char *argv[])  
{  
	int i, status;	
	
	char path[1024];
	
	if (argc != 2) {
		printf("Usage: %s path\n", argv[0]);
		return -1;
	}		
	
	strcpy(path, argv[1]);
	
	for (i=0; i<26; i++)
		count[i] = 0;
	  
	gettimeofday(&begin_time);
	 
	count_in_dir(path);	
	
	gettimeofday(&end_time);
	run_time_ms =
        (end_time.tv_sec - begin_time.tv_sec)*1000 +
        (end_time.tv_usec - begin_time.tv_usec)*1.0/1000;
    
	for (i=0; i<26; i++) {
		printf("%c: %ld\n", 'a'+i, count[i]);
	}
	
	printf("run_time = %lfms\n", run_time_ms);
	
	return 0;  
}  