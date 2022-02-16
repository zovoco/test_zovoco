#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc,char * argv[])//argv[1]为cmd，为1表示查看服务器中的文件列表,2表示下载服务器中的文件,3表示上传文件
                                  //argv[2]为文件名                        
{
    int cmd;
    cmd = atoi(argv[1]);//从程序命令行获取cmd，知道客户端想要干什么(字符串处理操作)
    if(argc != 3 && (cmd == 2||cmd == 3))
    {
        printf("输入错误\n");
        return 0;
    }
    else if(argc != 2 && cmd == 1)
    {
        printf("输入错误\n");
        return 0;
    }
    
    int sockfd = socket(AF_INET,SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        perror("socket error");
        return 0;
    }

      
    struct sockaddr_in addr;
    //void *memset(void *s, int c, size_t n); //用于设置一块内存为指定值c
	memset(&addr,0,sizeof(addr));//把addr变量内存所有字节设置为 0
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);//指定端口号，并进行字节序的转换
	inet_aton("192.168.100.221",&addr.sin_addr);//指定服务端IP地址

    //建立连接
    int r = connect(sockfd,(struct sockaddr *)&addr,sizeof(addr));
    if(r==-1)
    {
        perror("connect error");
        close(sockfd);
        return 0;
    }

    
    /*
        命令数据包参考格式如下
				命令号(1字节) 参数长度(1字节) 参数(不固定长度)

				2 5 '1' '.' 't' 'x' 't' 		->下载文件名为 1.txt的文件
					
				1 0 							ls命令，没有参数

    **/
    if(cmd == 1)
    {        
        write(sockfd,&cmd,4);
        char buf[1000]="";
        read(sockfd,buf,1000);
        
        printf("%s\n",buf);       
        
    }
    else if(cmd == 2)//下载文件
    {
        int filenamelen = strlen(argv[2]);
        write(sockfd,&cmd,4);
        write(sockfd,&filenamelen,sizeof(int));//将文件名长度发送给服务端
        write(sockfd,argv[2],strlen(argv[2]));//将文件名发送到服务器

        unsigned char resv;
        read(sockfd,&resv,1);
        if(resv == 0)
        {
            printf("服务器没有该文件\n");
            return 0;
        }
        
        //开始接收文件内容
        int filefd = open(argv[2],O_WRONLY | O_CREAT | O_TRUNC,0764);//如果没有就创建一个这个文件
        if(filefd == -1)
        {
            perror("open file error");    
        }

        char buf[1024];    
        while (1)
        {   
            r = read(sockfd,buf,1024);//建立通信后，读和写操作都在连接描述符文件中完成
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
    
    else if(cmd == 3)//向服务器上传文件
    {   
        int filenamelen = strlen(argv[2]);
        write(sockfd,&cmd,4);
        write(sockfd,&filenamelen,sizeof(int));//将文件名长度发送给服务端
        write(sockfd,argv[2],strlen(argv[2]));//将文件名发送到服务器

        int filefd = open(argv[2],O_RDONLY);
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
                write(sockfd,buf,sizeof(buf));
                break;
            }
            else
            {
                buf[0]=1;
            }               
            write(sockfd,buf,sizeof(buf));//建立通信后，读和写操作都在连接描述符文件中完成
            
        }
        printf("本次文件内容已上传完毕\n");
        close(filefd); 
    }
    
    close(sockfd);
    
}

