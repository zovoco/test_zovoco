#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>

void download(int linkfd)
{    
    unsigned char resv=1;
    write(linkfd,&resv,1);
    
    int filenamelen;
    read(linkfd,&filenamelen,4);//读取客户端文件名长度  
    printf("len:%d\n",filenamelen);
    
    char filename[100]="";
    read(linkfd,filename,filenamelen);//读取客户端所发来的文件名
    printf("name:%s\n",filename);
    
    int filefd = open(filename,O_RDONLY);
    if(filefd == -1)
    {
        perror("open file error");    
    }
    //printf("打开文件名所对应的文件成功\n");
    
    //开始传输数据
    char buf[1024];    
    while (1)
    {   
        int r = read(filefd,buf+1,1023);
        if(r<1023)
        {
            buf[0]=0; 
            write(linkfd,buf,r+1);
            break;
        }
        else
        {
            buf[0]=1;
        }               
        write(linkfd,buf,r+1);//建立通信后，读和写操作都在连接描述符文件中完成
        
    }

    printf("本次文件内容已发送完毕");
    close(filefd);

}

void upload(int linkfd)
{
    int filenamelen;
    read(linkfd,&filenamelen,4);//读取客户端文件名长度  
    printf("len:%d\n",filenamelen);
    
    char filename[100]="";
    read(linkfd,filename,filenamelen);//读取客户端所发来的文件名
    printf("name:%s\n",filename);
    
    //开始接收文件内容
    int filefd = open(filename,O_WRONLY | O_CREAT | O_TRUNC,0764);//如果没有就创建一个这个文件
    if(filefd == -1)
    {
        perror("open file error");    
    }

    char buf[1024];    
    while (1)
    {   
        int r = read(linkfd,buf,1024);//建立通信后，读和写操作都在连接描述符文件中完成
        if(buf[0]==1)
        {
            write(filefd,buf+1,r-1);
        }        
        else//即buf[0]==0
        {
            write(filefd,buf+1,r-1);
            break;
        }
    }

    printf("本次接收完毕\n");
    close(filefd);


}

void ls(int linkfd)
{
   DIR *pdir = opendir ("./");
   if(pdir == NULL)
    {
        perror("opendir error");
        return ;
    }
   
   struct dirent * p; 
   char buf[1000]="";
   while(1)
    {
        
        p = readdir(pdir);
    	if(p==NULL)
        {   
            strcat(buf,"\0");
    		break; 
        }
       
       if(strcmp(p->d_name,".") == 0 || strcmp(p->d_name,"..") ==0 )
       {
            continue;
       }
                 
       strcat(buf, p->d_name);
       strcat(buf,"\t");
    }
   
    int filenamelen = strlen(buf);

    write(linkfd,buf,filenamelen);
    closedir(pdir);

}


int main()//自动，不需要手动输入文件名和自己的IP地址，假设IP地址固定
{
    int sockfd = socket(AF_INET,SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        perror("socket error");
        return 0;
    }

    int on =1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(int));//允许地址复用
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEPORT,&on,sizeof(int));//允许端口复用


    struct sockaddr_in addr;
    //void *memset(void *s, int c, size_t n); //用于设置一块内存为指定值c
	memset(&addr,0,sizeof(addr));//把addr变量内存所有字节设置为 0
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);//指定端口号，并进行字节序的转换
	inet_aton("192.168.100.221",&addr.sin_addr);//指定IP地址

    //绑定服务端自己的ip
    int r = bind(sockfd,(struct sockaddr *)&addr,sizeof(addr));
    if(r==-1)
    {
        perror("bind error");
        close(sockfd);
        return 0;
    }

    //建立连接
    r = listen(sockfd,3);//在服务端中，sockfd(套接字描述符)只负责用来监听，不用于读和写操作
    if(r==-1)
    {
        perror("listen error");
        close(sockfd);
        return 0;
    }

    //开始文件传输
    int linkfd;//每个客户端都有属于自己的连接符
    while(1)//不断等待其他客户端来请求文件传输
    {
        int addrlen = sizeof(addr);
        struct sockaddr_in caddr;
            
        int linkfd = accept(sockfd,(struct sockaddr*)&caddr,&addrlen);
        if(linkfd == -1)
        {
            perror("accept error");
            close(sockfd);
            return 0;
        }
        printf("IP为%s,端口号为%hu的client与我建立连接\n",inet_ntoa(addr.sin_addr),ntohs(caddr.sin_port));
        int cmd;    
            
        pid_t pid = fork();
        if(pid == 0)//子进程
        {   
            int r = read(linkfd,&cmd,4);//读取cmd，知道客户端需要做什么
            printf("cmd = %d\n",cmd);
            
            if(cmd == 1)//查看服务器文件
            {
                printf("查看服务器文件列表\n");
                ls(linkfd);
            }
            else if(cmd == 2)//下载服务器文件
            {
                printf("下载服务器中的文件\n");
                download(linkfd);
                
            }
            
            else if(cmd == 3)//上传服务器文件
            {
                printf("向服务器上传文件\n");
                upload(linkfd);
            }
            else 
            {
                printf("输入命令号错误\n");
                return 0;
            }

            close(linkfd);
            
            exit(0);
        }
        
        else if(pid > 0)//父进程
        {   
            close(linkfd);//父进程不需要与客户端通信的，所以要关闭linkfd
        }
        else 
        {
            perror("fork error");    
        }
      
    }
    close(sockfd);//关闭套接字连接符,结束监听
    return 0;
}


