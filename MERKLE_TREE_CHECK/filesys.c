#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/sha.h>
#include "filesys.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#define number_of_blocks 128000/64
static int filesys_inited = 1;
long int k=0;
struct merkle_tree* root_list[8];
int counter=0;
long int size_array[8];
int global_ckeck=1;
/* returns 20 bytes unique hash of the buffer (buf) of length (len)
 * in input array sha1.
 */
void get_sha1_hash (const void *buf, int len, const void *sha1)
{
	SHA1 ((unsigned char*)buf, len, (unsigned char*)sha1);
}

/* Build an in-memory Merkle tree for the file.
 * Compare the integrity of file with respect to
 * root hash stored in secure.txt. If the file
 * doesn't exist, create an entry in secure.txt.
 * If an existing file is going to be truncated
 * update the hash in secure.txt.
 * returns -1 on failing the integrity check.
 */
int file_descriptior_error(int fd1,const char *pathname){
	if (fd1 == -1) {
		printf ("Unable to open %s\n",pathname);
		return -1;
	}
	return 1;
}
long int find_len(const char file_name[]){
	FILE* fp = fopen(file_name, "r"); 
    if (fp == NULL) { 
        printf("File Not Found!\n"); 
        return -1; 
    } 
    fseek(fp,0, SEEK_END); 
    long int res = ftell(fp); 
    fclose(fp);
    return res; 
}
ssize_t allocate_block(char *buf,int fd1,int index){

	//lseek(fd1,index*64,SEEK_SET);
	ssize_t size=read(fd1,buf,64);//8*sizeof(buf));
	if(size==-1){
		
		printf("error while allocating block in s_open\n");
	}
	return size;
}
struct merkle_tree* build_merkle(struct merkle_tree **container,int n){
	if(n==1){
		return container[0];
	}
	int k=n/2;
	int flag=0;
	if((k)%2!=0&&(n!=2)){
		flag=1;
		k=k+1;
	}
	struct merkle_tree *container1[k];
	for(int i=0;i<k;i++){
		container1[i]=malloc(sizeof(struct merkle_tree));
	}
	int j=0;
	for(int i=0;i<n;i+=2){
		struct merkle_tree* parent=malloc(sizeof(struct merkle_tree));
		//parent->value=malloc(20);
		container[i]->parent=parent;
		container[i+1]->parent=parent;
		parent->left=container[i];
		parent->right=container[i+1];
		char temp[40];
		 for (int i1 = 0;i1<20; i1++){
	 	temp[i1]=container[i]->value[i1];
	 	}
	    for (int j1 = 20; j1<40; j1++){
	    	temp[j1]=container[i+1]->value[j1-20];
	    }
		get_sha1_hash(temp,40,parent->value);
		container1[j]=parent;
		j++;
		if(flag==1&&j==(k-1)){
		container1[j]=parent;
		}
	}

	return build_merkle(container1,k);
}
int init_container(struct merkle_tree **container,int fd1,long int k){
	for(int i=0;i<k;i++){
		container[i]=malloc(sizeof(struct merkle_tree));
		//container[i]->value=malloc(20);
	}
	for(int i=0;i<k;i++){
		char *buf=malloc(64*sizeof(char));
		ssize_t temp_size=allocate_block(buf,fd1,i);
		if(temp_size==-1){
			return 0;
		}
		//unsigned char* hashed_val=malloc(20);
		get_sha1_hash(buf,64,(char *)container[i]->value);
		//strcpy((char *)container[i]->value,(char *)hashed_val);
	}
	return 1;
}
int s_open(const char *pathname, int flags, mode_t mode)
{
	assert (filesys_inited);
	int fd2=open(pathname,flags,mode);
	int flag=file_descriptior_error(fd2,pathname);
	if (flag==-1) {
		return -1;
	}
	int fd1=open(pathname,O_RDONLY, mode);
	flag=file_descriptior_error(fd1,pathname);
	if (flag==-1) {
		return -1;
	}
	k=find_len(pathname);
	long int si=k;
	if(k!=0){
	struct merkle_tree* root=malloc(sizeof(struct merkle_tree));
	k=k/64;
	struct merkle_tree *container[k];
	for(int i=0;i<k;i++){
		container[i]=malloc(sizeof(struct merkle_tree));
		//container[i]->value=malloc(20);
	}
	for(int i=0;i<k;i++){
		char *buf=malloc(64*sizeof(char));
		ssize_t temp_size=allocate_block(buf,fd1,i);
		if(temp_size==-1){
			return 0;
		}
		//unsigned char* hashed_val=malloc(20);
		get_sha1_hash(buf,64,(char *)container[i]->value);
		if(i==0){
			// printf("%s\n",hashed_val);
		}
		//strcpy((char *)container[i]->value,(char *)hashed_val);
	}
	root=build_merkle(container,k);
	root->filename=malloc(10);
	size_array[counter]=si;
	strcpy(root->filename,pathname);
	root_list[counter]=malloc(sizeof(struct merkle_tree));
	root_list[counter]=root;
	counter++;
	lseek(fd1,0,SEEK_SET);
	// printf("successful\n");
	char * pch;
	char fileName[32];
	strcpy(fileName,"secure.txt");
  	FILE* file = fopen(fileName, "r");
  	char line[256];
  	// fgets(line, sizeof(line), file);
  	int flagu = 0;//0 means not found
  	int integrity_check = 0;

    while(fgets(line, sizeof(line), file)){
	    pch = strtok (line," ");

	    while (pch != NULL)
	    {
	    	if(strcmp("\n",pch) == 0){
	    		break;
	    	}
            else{
                if(strcmp(pch,pathname)==0){
                	pch = strtok (NULL, " ");
                	flagu = 1;//file found
                	if(strcmp((char*)root->value,pch)==0){
                		integrity_check = 1;
                	}
                }
            }
            pch = strtok (NULL, " ");
	    }
	    if(integrity_check==1){
	    	break;
	    }
    }
    fclose(file);
    if(flagu==1&&integrity_check==0){
    	return -1;
    }
    if(flagu == 0){
    	FILE* fptr = fopen("secure.txt", "a");
    	char* str1=malloc(strlen(pathname)+23);
    	strcpy(str1,pathname);
    	strcat(str1," ");
    	strcat(str1,(char*)root->value);
    	strcat(str1," \n");
    	fprintf(fptr,"%s", str1);
   		fclose(fptr);
    }
}
close(fd1);
return fd2;
}
int checkfile(int fd1,int fd2){
		struct stat a,b;
		fstat(fd1, &a);
		fstat(fd2, &b);
		if(!((!(a.st_dev==b.st_dev))||(!(a.st_ino==b.st_ino)))){
			return 1;
		}
		else{
			return 0;
		}
}
/* SEEK_END should always return the file size 
 * updated through the secure file system APIs.
 */
int s_lseek (int fd, long offset, int whence)
{
	assert (filesys_inited);
	
	if((whence==SEEK_END)){
		int root_index=-1;
		for(int i=0;i<counter;i++){
			int fd1=open(root_list[i]->filename, O_WRONLY, 0);
			if(checkfile(fd,fd1)==1){
				root_index=i;
			}
			close(fd1);
		}
		if((root_index!=-1)){
		long size=lseek (fd,size_array[root_index],SEEK_SET);
		return size;
		}
	}
	return lseek (fd, offset, SEEK_SET);
}

/* read the blocks that needs to be updated
 * check the integrity of the blocks
 * modify the blocks
 * update the in-memory Merkle tree and root in secure.txt
 * returns -1 on failing the integrity check.
 */

ssize_t s_write (int fd, const void *buf, size_t count)
{
	
	assert (filesys_inited);
	int position = lseek(fd, 0, SEEK_CUR);
	double p=position+1.0;
	double index=ceil(p/64.0);
	int root_index=-1;
	for(int i=0;i<counter;i++){
		int fd1=open(root_list[i]->filename, O_WRONLY, 0);
		if(checkfile(fd,fd1)==1){
			root_index=i;
		}
		close(fd1);
	}
	if(root_index==-1){
		return write (fd, buf, count);
	}
	else{
		struct merkle_tree* temp=root_list[root_index];
		while(temp->left!=NULL){
			temp=temp->left;
		}
		int fd1=open(root_list[root_index]->filename, O_RDONLY, 0);
		lseek(fd1,((int)index-1)*64, SEEK_SET);
		char *buffer=malloc(64);
		allocate_block(buffer,fd1,0);
		lseek(fd1,0,SEEK_SET);
		char* hash=malloc(20);
		get_sha1_hash(buffer,64,hash);
		close(fd1);
		if(strcmp((char *)hash,(char *)(temp+((int)index-1)*sizeof(struct merkle_tree))->value)!=0){
			return -1;

		}
		int size=write (fd, buf, count);
		int fd2=open(root_list[root_index]->filename,O_RDONLY, S_IRUSR|S_IWUSR);
		int flag=file_descriptior_error(fd2,root_list[root_index]->filename);
		if (flag==-1) {
			return -1;
		}
		k=find_len(root_list[root_index]->filename);
		size_array[root_index]=k;
		k=k/64;
    	struct merkle_tree *container[k];
		if(init_container(container,fd1,k)==0){
			return -1;
	    }
		struct merkle_tree* root=build_merkle(container,k);
		root->filename=root_list[root_index]->filename;
		root_list[root_index]=root;
		close(fd2);
		char * pch;
	  	FILE* file = fopen("secure.txt", "r+");
	  	char line[256];
	  	int fkag=0;
	    while(fgets(line, sizeof(line), file)){
		    pch = strtok (line," ");

		    while (pch != NULL)
		    {
		    	if(strcmp("\n",pch) == 0){
		    		break;
		    	}
	            else{
	                if(strcmp(pch,root_list[root_index]->filename)==0){
	                	fseek(file,ftell(file)-32,SEEK_SET);
	                	char* str1=malloc(strlen(root_list[root_index]->filename)+23);
				    	strcpy(str1,root_list[root_index]->filename);
				    	strcat(str1," ");
				    	strcat(str1,(char*)root_list[root_index]->value);
				    	strcat(str1," \n");
				    	fprintf(file,"%s", str1);
				    	fkag=1;
				    	break;
	                }
	            }
	            pch = strtok (NULL, " ");
		    
		    }
		    if(fkag==1){
		    	break;
		    }
	    }
	   fclose(file);
	   return size;
	}
	return write (fd, buf, count);
}

/* check the integrity of blocks containing the 
 * requested data.
 * returns -1 on failing the integrity check.
 */
int integrity_check(int fd,size_t size){
	int position = lseek(fd, 0, SEEK_CUR);
	double p=position+1.0;
	double index=ceil(p/64.0);
	int root_index=-1;
	for(int i=0;i<counter;i++){
		int fd1=open(root_list[i]->filename, O_WRONLY, 0);
		if(checkfile(fd,fd1)==1){
			root_index=i;
		}
		close(fd1);
	}
	if(root_index==-1){
		if(!global_ckeck){
		return -1;
		}
	}
	else{
		for(int i=0;i<size/64;i++){
		struct merkle_tree* temp=root_list[root_index];
		while(temp->left!=NULL){
			temp=temp->left;
		}
		int fd1=open(root_list[root_index]->filename, O_RDONLY, 0);
		lseek(fd1,((int)index-1)*64, SEEK_SET);
		char *buffer=malloc(64);
		allocate_block(buffer,fd1,0);
		lseek(fd1,0,SEEK_SET);
		char* hash=malloc(20);
		get_sha1_hash(buffer,64,hash);
		close(fd1);
		if((char *)hash==(char *)(temp+((int)index-1)*sizeof(struct merkle_tree))->value){
			return -2;
		}
		index+=1;
	}
	}
	return 0;
}
ssize_t s_read (int fd, void *buf, size_t count)
{

	assert (filesys_inited);
	int k=integrity_check(fd,count);
	if(k==0||k==-2){
		return read (fd, buf, count);
    }
    else{
    	return -1;
    }
}

/* destroy the in-memory Merkle tree */
int s_close (int fd)
{
	assert (filesys_inited);
	return close (fd);
}

/* Check the integrity of all files in secure.txt
 * remove the non-existent files from secure.txt
 * returns 1, if an existing file is tampered
 * return 0 on successful initialization
 */
int filesys_init (void)
{
	filesys_inited = 1;
	char * pch;
	char filename[32];
	char *fileNames[8];
	char *hashvalues[8];
	strcpy(filename,"secure.txt");
	if( access(filename, F_OK ) == -1 ) {
		FILE* file2= fopen(filename, "a");
		fclose(file2);
	}
	else{
	FILE* file = fopen(filename, "r");
	FILE* file1 = fopen("secure1.txt", "a");
	char line[256];
	int flag=file_descriptior_error(fileno(file),"secure.txt");
	if (flag==-1) {
		return -1;
	}
	int flag1=file_descriptior_error(fileno(file1),"secure1.txt");
	if (flag1==-1) {
		return -1;
	}
	int count=0;
	while(fgets(line, sizeof(line), file)){
	    pch = strtok (line," ");
	    while (pch != NULL)
	    {
	    	if(strcmp("\n",pch) == 0){
	    		break;
	    	}
            else{
            		
                	if( access(pch, F_OK ) != -1 ) {
                			fileNames[count]=malloc(32);
		                	strcpy(fileNames[count],pch);
		                	pch = strtok (NULL, " ");
		                	hashvalues[count]=malloc(20);
		                	strcpy(hashvalues[count],pch);
		                	char* str1=malloc(strlen(fileNames[count])+23);
					    	strcpy(str1,fileNames[count]);
					    	strcat(str1," ");
					    	strcat(str1,hashvalues[count]);
					    	strcat(str1," \n");
					    	fprintf(file1,"%s", str1);
					    	count++;
					} 
					else{
						pch = strtok (NULL, " ");
					}
					
                }
                pch = strtok (NULL, " ");
        }
            
	}
	remove("secure.txt");
    fclose(file);
    rename("secure1.txt", "secure.txt");
    fclose(file1);
	// printf("duplicates removed\n");
	int integrity_flag=0;
    for(int i=0;i<count;i++){
    	struct merkle_tree* root=malloc(sizeof(struct merkle_tree));
    	int fd1=open(fileNames[i],O_RDONLY, S_IRUSR|S_IWUSR);
		int flag=file_descriptior_error(fd1,fileNames[i]);
		if (flag==-1) {
			return -1;
		}
		k=find_len(fileNames[i]);
		k=k/64;
    	struct merkle_tree *container[k];
		if(init_container(container,fd1,k)==0){
			return -1;
	    }
		root=build_merkle(container,k);
		lseek(fd1,0,SEEK_SET);
		if(strcmp((char *)root->value,hashvalues[i])!=0){
 			printf("%s\n","integrity failed");
			close(fd1);
			integrity_flag=1;
			break;
		}
		close(fd1);
    }
    if(fileNames[0]==NULL){
	// printf("fine1");
    }
    if(hashvalues[0]==NULL){
    	// printf("fine2");
    }
	return integrity_flag;
}
return 0;
}
