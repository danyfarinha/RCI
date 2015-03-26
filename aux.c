#include "aux.h"

void print_interface(node * self, int n){
	switch (n){
		case 0:
			printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
			printf("|                   DDT - Commands                       |\n");
			printf("|--------------------------------------------------------|\n");
			printf("|join x i                                                |\n");
			printf("|join x i succi succi.IP succi.TCP                       |\n");
			printf("|leave                                                   |\n");
			printf("|show                                                    |\n");
			printf("|search k                                                |\n");
			printf("|exit                                                    |\n");
			printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		case 1:
			sprintf(message, "*************************VERBOSE**************************\n");
			print_verbose(message, self->verbose);
			break;
		case 2:
			printf( "**********************************************************\n");
			break;
		default:
			break;
	}
	
	
}

struct sockaddr_in getIP(char * ip, int port){
	struct hostent *h;
	struct in_addr *a, *b;
	struct sockaddr_in addr;
	
	if((h = gethostbyname(ip))==NULL){
		exit(1);
	}	
	
	a=(struct in_addr*)h->h_addr_list[0];
	
	memset((void*)&addr,(int)'\0', sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr= *a;
	addr.sin_port=htons(port);

	return addr;
}

node Init_Node(char ** argv, int argc){
	node new;
	int i, n;
	int verbose = 0;
	char buffer[_SIZE_MAX_];
	char bootip[_SIZE_MAX_] = "tejo.tecnico.ulisboa.pt";
	int bootport = 58000;
	int ringport = 8000; //voltar ao -1 mais tarde
	//Trata argumentos
	
	
	for(i = 1; i < argc; i++){
		if (strcmp(argv[i],"-t")==0){
			if(i+1 == argc) continue;
			if(argv[i+1][0] == '-') continue;
			n = sscanf(argv[i+1], "%d", &ringport);
			if (n != 1) exit(2);
		}
		if (strcmp(argv[i], "-i") == 0){
			if(i+1 == argc) continue;
			if(argv[i+1][0] == '-') continue;
			strcpy(bootip, argv[i+1]);
		}
		if (strcmp(argv[i], "-p") == 0){
			if(i+1 == argc) continue;
			if(argv[i+1][0] == '-') continue;
			n = sscanf(argv[i+1], "%d", &bootport);
			if (n != 1) exit(2);
		}
		if (strcmp(argv[i], "-v") == 0){
			verbose = 1;
		}		
	}

	// Inicialização
	
	if(gethostname(buffer, _SIZE_MAX_) == -1) printf("error: %s\n", strerror(errno));
	new.id.addr = getIP(buffer, ringport);
	new.id.id = -1;
	new.predi.id = -1;
	new.succi.id = -1;
	new.udp_server = getIP(bootip, bootport);
	
	new.ring = -1;	// Inicialização do numero do anel a -1
	new.boot = 0;
	new.verbose = verbose;
	
	new.fd.keyboard = 0;
	new.fd.predi = -1;
	new.fd.succi = -1;
	
	
	
	return new;
}

int dist(int k, int id){
	if (id >= k) return (id - k);
	else return (64 + id - k);
}

void print_verbose(char * message, int mode){
	if (mode) printf("%s", message);
}

int search(node * self, int k){
	int n;
	char buffer[_SIZE_MAX_];
	
	if (dist(k, self->id.id) < dist(self->predi.id, self->id.id)){
		sprintf(message, "I'm responsible for %d\n", k);
		print_verbose(message, self->verbose);
		sprintf(message, "The Node %d is responsible for %d\n", self->id.id, k);
		print_verbose(message, self->verbose);
		return 1;
	}else{
		sprintf(buffer, "QRY %d %d\n", self->id.id, k);
		n = write(self->fd.succi, buffer, _SIZE_MAX_);
		return 0;
	}
}

int join_succi(node * self, int new){
	int err, addrlen;
	char buffer[_SIZE_MAX_];	
	
	self->fd.succi = socket(AF_INET, SOCK_STREAM, 0);
	if(self->fd.succi == -1) exit(1);

	addrlen = sizeof(self->succi.addr);
	err = connect(self->fd.succi, (struct sockaddr *) &self->succi.addr, (socklen_t) addrlen);
	sprintf(message, "Open Socket %d (succi)\n", self->fd.succi);
	print_verbose(message, self->verbose);
	
	if (err == -1){
		printf("Impossible to connect to %s %hu\n", inet_ntoa(self->id.addr.sin_addr), ntohs(self->id.addr.sin_port));
		close(self->fd.succi);
		sprintf(message, "Close Socket %d (succi)\n", self->fd.succi);
		print_verbose(message, self->verbose);
		self->succi.id = -1;
		self->id.id = -1;
		return 30;
	}
	
	memset((void *) buffer, (int) '\0', _SIZE_MAX_);
	
	if(new) 
		sprintf(buffer, "NEW %d %s %hu\n", self->id.id, inet_ntoa(self->id.addr.sin_addr), ntohs(self->id.addr.sin_port));
	else
		sprintf(buffer, "ID %d\n", self->id.id);
		
	write(self->fd.succi, buffer, (size_t) _SIZE_MAX_);
	
	sprintf(message, "Enviado para o nó %d %s %hu a mensagem %s", self->succi.id, inet_ntoa(self->succi.addr.sin_addr), ntohs(self->succi.addr.sin_port), buffer);
	print_verbose(message, self->verbose);
	return(0);
	
}

int join(node * self, int x){
	int fd, addrlen, n, j, tcp, err;
	char command[_SIZE_MAX_], ip[_SIZE_MAX_];
	char buffer[_SIZE_MAX_];	
	
	fd=socket(AF_INET, SOCK_DGRAM,0);
	if(fd==-1)exit(1);
	
	sprintf(buffer, "BQRY %d\n", x);
  	n=sendto(fd, buffer,50,0,(struct sockaddr*)&self->udp_server, sizeof(self->udp_server));
	if(n==-1)exit(1);
	sprintf(message, "Enviei ao servidor UDP a mensagem %s", buffer);
	print_verbose(message, self->verbose);
	
	memset(buffer, '\0', _SIZE_MAX_);

	addrlen=sizeof(self->udp_server);
	n = recvfrom(fd,buffer,_SIZE_MAX_,0,(struct sockaddr*)&self->udp_server,&addrlen);
	if(n==-1)exit(1);
	sprintf(message, "O servidor respondeu com um %s\n", buffer);
	print_verbose(message, self->verbose);

	
	if(strcmp(buffer, "EMPTY")==0){
		sprintf(buffer, "REG %d %d %s %hu\n", x, self->id.id, inet_ntoa(self->id.addr.sin_addr),ntohs(self->id.addr.sin_port));
		n=sendto(fd, buffer,50,0,(struct sockaddr*)&self->udp_server, sizeof(self->udp_server));
		if(n==-1)exit(1);
		sprintf(message, "Enviei ao servidor UDP a mensagem %s", buffer);
		print_verbose(message, self->verbose);
		
		memset(buffer, '\0', _SIZE_MAX_);
		n = recvfrom(fd,buffer,_SIZE_MAX_,0,(struct sockaddr*)&self->udp_server,&addrlen);
		if(n==-1)exit(1);
		sprintf(message, "O servidor respondeu com um %s\n", buffer);
		print_verbose(message, self->verbose);

		if (strcmp(buffer, "OK") == 0){
			printf("Ring %d created\n", x);
			self->ring = x;
			self->boot = 1;
			close(fd);
			return 0;
		}
	}else{
		sprintf(message, "Ring %d already exists\n", x);
		print_verbose(message, self->verbose);
		n = sscanf(buffer, "%s %d %d %s %d", command, &x, &j, ip, &tcp);
		if(n != 5) return 1;
		if(strcmp(command, "BRSP") == 0){
			if(self->id.id == j){
				printf("Already a node\n");
				close(fd);
				return 1;
			}else{
			self->succi.id = j;
			self->succi.addr = getIP(ip, tcp);
			self->ring = x;
			err = join_succi(self, 0);
			}
		}
	}
	close(fd);
	return err;
}

int show(node * self){
	print_interface(self, 2);
	
	if (self->verbose){
		if(self->boot) printf("BOOT NODE\n");
		if(self->ring != -1){
			printf("Ring: %d\n", self->ring);
			printf("Node: %d     [%s:%hu]\n", self->id.id, inet_ntoa(self->id.addr.sin_addr), ntohs(self->id.addr.sin_port));
		}else{
			printf("Ring: NULL\n");
		}
		if(self->predi.id != -1){
			printf("Predi: %d     [%s:%hu]\n", self->predi.id, inet_ntoa(self->predi.addr.sin_addr), ntohs(self->predi.addr.sin_port));
		}else{
			printf("Predi: NULL\n");
		}
		if(self->succi.id != -1){
			printf("Predi: %d     [%s:%hu]\n", self->succi.id, inet_ntoa(self->succi.addr.sin_addr), ntohs(self->succi.addr.sin_port));
		}else{
			printf("SUCCI: NULL\n");
		}
	}else{
		if(self->ring != -1){
			printf("Ring: %d\n", self->ring);
			printf("Node: %d\n", self->id.id);
		}else{
			printf("Ring: NULL\n");
		}

		if(self->predi.id != -1){
			printf("Predi: %d     [%s:%hu]\n", self->predi.id, inet_ntoa(self->predi.addr.sin_addr), ntohs(self->predi.addr.sin_port));
		}else{
			printf("Predi: NULL\n");
		}
		if(self->succi.id != -1){
			printf("Succi: %d     [%s:%hu]\n", self->succi.id, inet_ntoa(self->succi.addr.sin_addr), ntohs(self->succi.addr.sin_port));
		}else{
			printf("Succi: NULL");
		}
	}
	return 0;
}

int leave(node * self){	
	int fd, addrlen, n;
	char buffer[_SIZE_MAX_];
	
	// Caso o nó não esteja em nenhum anel 
	
	if (self->ring == -1){
		sprintf(message, "This node is not inserted in any ring\n");
		print_verbose(message, self->verbose);
		return 1;
	}

	fd=socket(AF_INET, SOCK_DGRAM,0);
	if(fd==-1)exit(1);
		
	if(self->succi.id == -1){
		// Sou único?
		sprintf(message, "This node is not connected with any node\n");
		print_verbose(message, self->verbose);
		memset(buffer, '\0', _SIZE_MAX_);
		sprintf(buffer, "UNR %d\n", self->ring);
		n=sendto(fd, buffer,50,0,(struct sockaddr*)&self->udp_server, sizeof(self->udp_server));
		if(n==-1)exit(1);
		sprintf(message, "Sent to UDP %s\n", buffer);
		print_verbose(message, self->verbose);	
		
		memset(buffer, '\0', _SIZE_MAX_);
		n = recvfrom(fd,buffer,_SIZE_MAX_,0,(struct sockaddr*)&self->udp_server,&addrlen);
		if(n==-1)exit(1);
		sprintf(message, "UDP Server replied %s\n", buffer);
		print_verbose(message, self->verbose);
		
		if(strcmp(buffer, "OK")==0){
			sprintf(message, "Ring %d erased\n", self->ring);
			print_verbose(message, self->verbose);
			self->ring = -1;
			self->id.id = -1;
			self->boot = 0;
			close(fd);
			return 0;
		}else return 1;
	}else{ 
		sprintf(message, "This node is connected with other node(s)\n");
		print_verbose(message, self->verbose);
		if(self->boot){				
		// Sou BOOT?
			sprintf(message, "BOOT\n");
			print_verbose(message, self->verbose);
			memset(buffer, '\0', _SIZE_MAX_);
			sprintf(buffer, "REG %d %d %s %d\n", self->ring, self->succi.id, inet_ntoa(self->succi.addr.sin_addr), ntohs(self->succi.addr.sin_port));
			n=sendto(fd, buffer,50,0,(struct sockaddr*)&self->udp_server, sizeof(self->udp_server));
			if(n==-1)exit(1);
			sprintf(message, "Sent to UDP %s\n", buffer);
			print_verbose(message, self->verbose);
			
			
			memset(buffer, '\0', _SIZE_MAX_);
			addrlen=sizeof(self->udp_server);
			n = recvfrom(fd,buffer,_SIZE_MAX_,0,(struct sockaddr*)&self->udp_server,&addrlen);
			if(n==-1)exit(1);
			sprintf(message, "UDP Server replied %s\n", buffer);
			print_verbose(message, self->verbose);
			
			n = write(self->fd.succi, "BOOT\n", _SIZE_MAX_);
			if(n==-1)exit(1);
			sprintf(message, "Sent to SUCCI: BOOT\n");
			print_verbose(message, self->verbose);
			self->boot = 0;
		}
				
		memset(buffer, '\0', _SIZE_MAX_);
		sprintf(buffer, "CON %d %s %d\n", self->succi.id, inet_ntoa(self->succi.addr.sin_addr), ntohs(self->succi.addr.sin_port));
		n = write(self->fd.predi, buffer, _SIZE_MAX_);
		if(n==-1)exit(1);
		sprintf(message, "Sent to PREDI:%s\n", buffer);
		print_verbose(message, self->verbose);
		close(self->fd.succi);
		close(self->fd.predi);
		printf("Close Socket: %d (predi)\n", self->fd.predi);
		printf("Close Socket: %d (succi)\n", self->fd.succi);
		self->fd.predi = -1;
		self->fd.succi = -1;
		self->predi.id = -1;
		self->succi.id = -1;
		self->ring = -1;
		self->id.id = -1;
					
	}
	
	sprintf(message,"This node is not inserted in any ring\n");
	print_verbose(message, self->verbose);
	return 0;
}

void exit_app(node * self){
	leave(self);
	// Fazer frees à memória alocada
	sprintf(message, "Exit.\n");
	print_verbose(message, self->verbose);
	exit(0);
}

int switch_listen(char * command, int fd, node * self){
	char buffer[_SIZE_MAX_], id_ip[_SIZE_MAX_];	
	int n, id, id_tcp, k, j, err;
		
	n = sscanf(command, "%s", buffer);
	if(n != 1) return 1;  //codigo de erro
	
	if(strcmp(buffer, "NEW") == 0){
		n = sscanf(command, "%*s %d %s %d", &id, id_ip, &id_tcp);
		if (n != 3) return 1; //codigo de erro
		
		
		if(self->predi.id != -1){
			memset((void*)buffer, (int)'\0', _SIZE_MAX_);
			sprintf(buffer, "CON %d %s %d\n", id, id_ip, id_tcp);
			write(self->fd.predi, buffer, _SIZE_MAX_);
			sprintf(message, "Sent to node %d %s %hu the message %s\n", self->predi.id, inet_ntoa(self->predi.addr.sin_addr), ntohs(self->predi.addr.sin_port), buffer);
			print_verbose(message, self->verbose);
			sprintf(message, "Close Socket: %d (predi)\n", self->fd.predi);
			print_verbose(message, self->verbose);
			close(self->fd.predi);
		}
		self->predi.id = id;
		self->predi.addr = getIP(id_ip, id_tcp);
		self->fd.predi = fd;
		sprintf(message,"Open socket: %d (predi)\n", self->fd.predi);
		print_verbose(message, self->verbose);
		
		if(self->succi.id == -1){
			self->succi.id = id;
			self->succi.addr = getIP(id_ip, id_tcp);
			err = join_succi(self, 1);
		}
	}	
	if(strcmp(buffer, "CON") == 0){
		n = sscanf(command, "%*s %d %s %d", &id, id_ip, &id_tcp);
		if (n != 3) return 1; //codigo de erro
		if(self->id.id == id){
			close(self->fd.succi);
			close(self->fd.predi);
			sprintf(message, "Close Socket: %d (predi)\n", self->fd.predi);
			print_verbose(message, self->verbose);
			sprintf(message, "Close Socket: %d (succi)\n", self->fd.succi);
			print_verbose(message, self->verbose);
			self->succi.id = -1;
			self->predi.id = -1;
			self->fd.predi = -1;
			self->fd.succi = -1;
		}else{
			self->succi.id = id;
			self->succi.addr = getIP(id_ip, id_tcp);
			sprintf(message, "Close Socket: %d (succi)\n", self->fd.succi);
			print_verbose(message, self->verbose);
			close(self->fd.succi);
			err = join_succi(self, 1);
		}
	}	
	if(strcmp(buffer, "QRY") == 0){
		n = sscanf(command, "%*s %d %d", &id, &k);
		if (n != 2) return 1; //codigo de erro
		if (dist(k, self->id.id) < dist(self->predi.id, self->id.id)){
			sprintf(message, "I'm responsible for %d\n", k);
			print_verbose(message, self->verbose);
			sprintf(buffer, "RSP %d %d %d %s %d\n", id, k, self->id.id, inet_ntoa(self->id.addr.sin_addr), ntohs(self->id.addr.sin_port));
			n = write(self->fd.predi, buffer, _SIZE_MAX_);
		}else{
			n = write(self->fd.succi, command, _SIZE_MAX_);
		}
	}	
	if(strcmp(buffer, "RSP") == 0){
		n = sscanf(command, "%*s %d %d %d %s %d", &j, &k, &id, id_ip, &id_tcp);
		if (n != 5) return 1; //codigo de erro
		if(j == self->id.id){
			if(fd == -1){
				sprintf(message, "The Node %d is responsible for %d\n", id, k);
				print_verbose(message, self->verbose);
				err = 0;
			}else{
				sprintf(buffer, "SUCC %d %s %d\n", id, id_ip, id_tcp);
				n = write(fd, buffer, _SIZE_MAX_);
				err = 12;
				sprintf(message, "Sent: SUCC %s\n", buffer);
				print_verbose(message, self->verbose);

			}
		}else{
			n = write(self->fd.predi, command, _SIZE_MAX_);
			err = 0;
		}
	}		
	if(strcmp(buffer, "ID") == 0){
		if(self->succi.id == -1){
			sprintf(buffer, "SUCC %d %s %d\n", self->id.id, inet_ntoa(self->id.addr.sin_addr),ntohs(self->id.addr.sin_port));
			n = write(fd, buffer, _SIZE_MAX_);
		}else{
			n = sscanf(command, "%*s %d", &k);
			if(n != 1) return -1; //codigo de erro
		
			if(search(self, k) == 1){
				sprintf(buffer, "SUCC %d %s %d\n", self->id.id, inet_ntoa(self->id.addr.sin_addr),ntohs(self->id.addr.sin_port));
				n = write(fd, buffer, _SIZE_MAX_);
			}else{
				err = -10;
			}
		}
	}	
	if(strcmp(buffer, "SUCC") == 0){
		n = sscanf(command, "%*s %d %s %d", &id, id_ip, &id_tcp);
		if (n != 3) return 1; //codigo de erro
		if(self->id.id == id){
			printf("Already a node\n");
					
			self->succi.id = -1;
			self->ring = -1;
			close(self->fd.succi);
			sprintf(message, "Close Socket: %d (succi)\n", self->fd.succi);
			print_verbose(message, self->verbose);
			self->fd.succi = -1;
		}else{
			self->succi.id = id;
			self->succi.addr = getIP(id_ip, id_tcp);
			sprintf(message, "Close Socket: %d (succi)\n", self->fd.succi);
			print_verbose(message, self->verbose);
			close(self->fd.succi);
			err = join_succi(self, 1);
		}
	}	
	if(strcmp(buffer, "BOOT") == 0){
		self->boot = 1;
		close(self->fd.predi);
		printf(message, "Close Socket: %d (predi)\n", self->fd.predi);
		print_verbose(message, self->verbose);
		self->fd.predi = -1;
		self->predi.id = -1;
		err = 0;
	}

	return err;
}

int switch_cmd(char * command, node * self){
	char buffer[_SIZE_MAX_], succiIP[_SIZE_MAX_];
	int n, err, x, succi, succiTCP;
	
	n = sscanf(command, "%s %d %d %d %s %d", buffer, &x, &self->id.id, &succi, succiIP, &succiTCP);
	if(self->id.id >= 64){
		printf("Error: Node > 64\n");
		self->id.id = -1;
		return 20;
	}
	switch(n){
		case(1):
			if(strcmp(buffer, "leave") == 0){
				err = leave(self);
			}else{
				if(strcmp(buffer, "show") == 0){
					err = show(self);
				}else{
					if(strcmp(buffer, "exit") == 0){
						exit_app(self);
					}else{
						printf("No command found '%s' or missed arguments\n", buffer);
					}
				}
			}
			break;
		case(2):
			if(strcmp(buffer, "search") == 0){
				err = search(self, x);
			}else{
				printf("No command found '%s' or missed arguments\n", buffer);
			}
			break;
		case(3):
			if(strcmp(buffer, "join") == 0){
				if(self->ring == -1){
					err = join(self, x);
				}else{
					printf("Node already in the ring %d\n", self->ring);
					err = 3;
				}
			}else{
				printf("No command found '%s' or missed arguments\n", buffer);
			}
			break;
		case(6):
			if(strcmp(buffer, "join") == 0){
				if(self->ring == -1){
					self->succi.id = succi;
					self->succi.addr = getIP(succiIP, succiTCP);
					self->ring = x;
					err = join_succi(self, 1);
				}else{
					printf("Node already in the ring %d\n", self->ring);
					err = 3;
				}
			}else{
				printf("No command found '%s' or missed arguments\n", buffer);
			}
			break;
		default:
			printf("No command found '%s' or missed arguments\n", buffer);
			break;
	}
	return err;
}

