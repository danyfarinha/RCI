#include "aux1.h"

/******************************************************************************
 * print_verbose()
 *
 * Arguments:	message				- String to be printed in the terminal
 * 
 * Returns: (void)
 * 
 * Side-Effects: none
 *
 * Description: 	Prints the message in the terminal if in verbose mode. 
 * 
 *****************************************************************************/

void print_verbose(char * message){
	if (verbose) printf("%s", message);
}

/******************************************************************************
 * Print_Interface()
 *
 * Arguments:	n				- Control integer
 * 
 * Returns: (void)
 * 
 * Side-Effects: none
 *
 * Description: 	If the control integer is 0, the menu interface is printed.
 * 					Otherwise auxiliary divisions are printed
 * 					 
 *****************************************************************************/
 
void print_interface(int n){
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
			break;
		case 1:
			sprintf(message, "*************************VERBOSE**************************\n");
			print_verbose(message);
			break;
		case 2:
			printf( "************\n");
			break;
		default:
			break;
	}
}

/******************************************************************************
 * getIP()
 *
 * Arguments:	ip				- IP address in user format
 * 				port			- Port in user format
 * 
 * Returns: (struct sockaddr_in)
 * 
 * Side-Effects: none
 *
 * Description: 	Function used to get the ip address in network format 
 * 
 *****************************************************************************/

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

/******************************************************************************
 * Init_Node()
 * 
 * Arguments:	argv				- 	Arguments on program call
 * 				argc 				-	Number of arguments on program call
 * 
 * Returns: (node)
 * 
 * Side-Effects: none
 *
 * Description: 	Initialize node with its respective data
 * 
 *****************************************************************************/

node Init_Node(char ** argv, int argc){
	node new;
	int i, n;
	char buffer[_SIZE_MAX_];
	char bootip[_SIZE_MAX_] = "tejo.tecnico.ulisboa.pt";
	int bootport = 58000;
	int ringport = -1; 
	
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
		if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
			printf("\nFunction is called with:\n");
			printf("./ddt [-i BootIP] [-p BootPort] [-t RingPort] [-v]\n\n");
			printf("By default:\n");
			printf("BootIP = <tejo.tecnico.ulisboa.pt> and BootPort = <58000>\n");
			printf("RingPort must be set\n\n");
			printf("Opcional: -v enables Verbose\n\n");
			exit(0);
		}		
	}

	if(ringport == -1){
		printf("RingPort not set. Aborting program run.\n");
		printf("Program must be called with ./ddt [-t RingPort]\n");
		printf("\nFor more informations run: ./ddt [-h | --help]\n\n");
		exit(0);
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
	
	new.fd.keyboard = 0;
	new.fd.predi = -1;
	new.fd.succi = -1;
	
	return new;
}

/******************************************************************************
 * dist()
 *
 * Arguments:	k				- Value of the identifier that we want to find
 * 				id				- Value of identifier of the inserted node
 * 
 * Returns: (int)
 * 
 * Side-Effects: none
 *
 * Description: 	 Returns the distance between two nodes in a ring with 64 nodes
 * 
 *****************************************************************************/

int dist(int k, int id){
	if (id >= k) return (id - k);
	else return (64 + id - k);
}

/******************************************************************************
 * show()
 *
 * Arguments:	self			- Pointer to struct with node informations
 * 
 * Returns: (int)
 * 
 * Side-Effects: none
 *
 * Description: 	Prints in the terminal most of the node's information
 * 
 *****************************************************************************/

int show(node * self){
	print_interface(2);
	
	if (verbose){
		if(self->boot) printf("BOOT NODE\n");
		if(self->ring != -1){
			printf("Ring:  %4d\n", self->ring);
			printf("Node:  %4d     [%s:%hu]\n", self->id.id, inet_ntoa(self->id.addr.sin_addr), ntohs(self->id.addr.sin_port));
		}else{
			printf("Ring:  NULL\n");
		}
		if(self->predi.id != -1){
			printf("Predi: %4d     [%s:%hu]\n", self->predi.id, inet_ntoa(self->predi.addr.sin_addr), ntohs(self->predi.addr.sin_port));
		}else{
			printf("Predi: NULL\n");
		}
		if(self->succi.id != -1){
			printf("Succi: %4d     [%s:%hu]\n", self->succi.id, inet_ntoa(self->succi.addr.sin_addr), ntohs(self->succi.addr.sin_port));
		}else{
			printf("Succi: NULL\n");
		}
	}else{
		if(self->ring != -1){
			printf("Ring:  %4d\n", self->ring);
			printf("Node:  %4d\n", self->id.id);
		}else{
			printf("Ring:  NULL\n");
			printf("Node:  NULL\n");
		}

		if(self->predi.id != -1){
			printf("Predi: %4d\n", self->predi.id);
		}else{
			printf("Predi: NULL\n");
		}
		if(self->succi.id != -1){
			printf("Succi: %4d\n", self->succi.id);
		}else{
			printf("Succi: NULL\n");
		}
	}
	
	print_interface(2);
	return 0;
}

