#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

typedef struct node_data{
	int  id;	
	struct sockaddr_in addr;
} node_data;

typedef struct node{
	node_data id;
	node_data predi;
	node_data succi;
	struct sockaddr_in udp_server;
	int ring;
} node;

struct sockaddr_in getIP(char * bootip, int bootport){
	struct hostent *h;
	struct in_addr *a;
	struct sockaddr_in addr;
	
	if((h = gethostbyname(bootip))==NULL){
		exit(1);
	}	
	a=(struct in_addr*)h->h_addr_list[0];
	
	memset((void*)&addr,(int)'\0', sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr= *a;
	addr.sin_port=htons(bootport);

	return addr;
}

int join(node * self, int x){
	int fd, addrlen, n;
	char buffer[128];
	
	fd=socket(AF_INET, SOCK_DGRAM,0);
	if(fd==-1)exit(1);
	
	sprintf(buffer, "BQRY %d", x);
  	n=sendto(fd, buffer,50,0,(struct sockaddr*)&self->udp_server, sizeof(self->udp_server));
	if(n==-1)exit(1);

	memset(buffer, '\0', 128);

	addrlen=sizeof(self->udp_server);
	n = recvfrom(fd,buffer,128,0,(struct sockaddr*)&self->udp_server,&addrlen);
	if(n==-1)exit(1);
	
	if(strcmp(buffer, "EMPTY")==0){
		printf("EMPTY\nbora fazer um REG\n");
		sprintf(buffer, "REG %d %d %s %hu", x, self->id.id, inet_ntoa(self->id.addr.sin_addr),ntohs(self->id.addr.sin_port));
		n=sendto(fd, buffer,50,0,(struct sockaddr*)&self->udp_server, sizeof(self->udp_server));
		if(n==-1)exit(1);
		memset(buffer, '\0', 128);
		n = recvfrom(fd,buffer,128,0,(struct sockaddr*)&self->udp_server,&addrlen);
		if(n==-1)exit(1);
		if (strcmp(buffer, "OK") == 0){
			printf("Anel %d criado\n", x);
			self->ring = x;
			close(fd);
			return 0;
		}
	}
	close(fd);
	printf("Anel %d já existente\n", x);
	return 1;
}

int leave(node * self){	
	int fd, addrlen, n;
	char buffer[128];
	
	if (self->ring == -1){
		printf("O nó não está inserido em nenhum anel\n");
		return 1;
	}
	
	fd=socket(AF_INET, SOCK_DGRAM,0);
	if(fd==-1)exit(1);
	
	sprintf(buffer, "UNR %d", self->ring);
  	n=sendto(fd, buffer,50,0,(struct sockaddr*)&self->udp_server, sizeof(self->udp_server));
	if(n==-1)exit(1);

	memset(buffer, '\0', 128);

	addrlen=sizeof(self->udp_server);
	n = recvfrom(fd,buffer,128,0,(struct sockaddr*)&self->udp_server,&addrlen);
	if(n==-1)exit(1);
	
	if(strcmp(buffer, "OK")==0){
		printf("Anel %d apagado\n", self->ring);
		self->ring = -1;
		close(fd);
		return 0;
	}
	printf("Ocorreu algum problema\n");
	return 1;
}

void exit_app(node * self){
	leave(self);
	// Fazer frees à memória alocada
	printf("A sair da aplicação.\n");
	exit(0);
}

int switch_cmd(char * command, node * self){
	char buffer[128], succiIP[128], succiTCP[128];
	int n, err, x, succi;
	
	n = sscanf(command, "%s %d %d %d %s %s", buffer, &x, &self->id.id, &succi, succiIP, succiTCP);
	switch(n){
		case(1):
			if(strcmp(buffer, "leave") == 0){
				err = leave(self);
			}else{
				if(strcmp(buffer, "show") == 0){
					// Função de listagem de informações
					printf("Função ainda não implementada\n");
				}else{
					if(strcmp(buffer, "exit") == 0){
						exit_app(self);
					}else{
						printf("%s não é um comando válido, ou faltam argumentos\n", buffer);
					}
				}
			}
			break;
		case(2):
			if(strcmp(buffer, "search") == 0){
				// Função de procura de um identificador k, neste caso, o inteiro x
				printf("Função ainda não implementada\n");
			}else{
				printf("%s não é um comando válido, ou faltam argumentos\n", buffer);
			}
			break;
		case(3):
			if(strcmp(buffer, "join") == 0){
				err = join(self, x);
			}else{
				printf("%s não é um comando válido, ou faltam argumentos\n", buffer);
			}
			break;
		case(6):
			if(strcmp(buffer, "join") == 0){
				// Função de entrada no anel x, como identificador i, sabendo succi
				printf("Função ainda não implementada\n");
			}else{
				printf("%s não é um comando válido, ou faltam argumentos\n", buffer);
			}
			break;
		default:
			printf("%s não é um comando válido, ou faltam argumentos\n", buffer);
			break;
	}
	return 0;
}

int main(int argc, char ** argv){
	int n, i, err, errno, ringport;
	char buffer[128], instruction[128];
	char bootip[128] = "tejo.tecnico.ulisboa.pt";
	int bootport = 58000;
	node self;
		
	//ERROS
	
	if(argc > 7){
		printf("Incorrect number of arguments\n");
		exit(-1);
	}
	
	//Trata argumentos
	
	for(i = 1; i < argc-1; i++){
		if (strcmp(argv[i],"-t")==0){
			if(argv[i+1][0] == '-') continue;
			n = sscanf(argv[i+1], "%d", &ringport);
			if (n != 1) exit(2);
		}
		if (strcmp(argv[i], "-i") == 0){
			if(argv[i+1][0] == '-') continue;
			strcpy(bootip, argv[i+1]);
		}
		if (strcmp(argv[i], "-p") == 0){
			if(argv[i+1][0] == '-') continue;
			n = sscanf(argv[i+1], "%d", &bootport);
			if (n != 1) exit(2);
		}
	}

	self.udp_server = getIP(bootip, bootport);
	if(gethostname(buffer,128)==-1) printf("error: %s\n", strerror(errno));
	self.id.addr = getIP(buffer, ringport);
	
	printf("Esperando um comando (join, leave, show, search, exit)\n");
	
	self.ring = -1;
	while(fgets(instruction, 128, stdin) != NULL){
		err = switch_cmd(instruction, &self);
			if (err == 1) exit(1); //código de erro
	}
	exit(0);
}
