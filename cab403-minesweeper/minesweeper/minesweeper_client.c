#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

	#define NUM_TILES_X 9
	#define NUM_TILES_Y 9
	#define NUM_MINES 10
	#define ADJ_TILES 8
	#define RETURNED_ERROR -1
  #define PORT 12345
	#define MAXDATASIZE 100

	void play_game(int socket_identifier);
	void recieve_game(int socket_identifier);
	void get_input(int socket_identifier, int game_over);

void main_menu(int socket_identifier) {
	char selection[30];
	int selection_int = 0;
	uint16_t selection_send;

	printf("\n\nWelcome to the Minesweeper gaming system.\n\n");
	printf("\n\nPlease enter a selection:\n");
	printf("<1> Play Minesweeper\n");
	printf("<2> Show Leaderboard\n");
	printf("<0> Quit\n\n");
	printf("Selection option (0-2):");

	scanf(" %s", selection);

  if (strcmp(selection, "1") == 0) {
		selection_int = 1;
		selection_send = htons(selection_int);
		write(socket_identifier, &selection_send, sizeof(selection_send));
		play_game(socket_identifier);
	} else if (strcmp(selection, "2") == 0) {
		selection_int = 2;
		selection_send = htons(selection_int);
		write(socket_identifier, &selection_send, sizeof(selection_send));
	} else if (strcmp(selection, "0") == 0) {
    printf("\nQuiting game...\n\n");
		close(socket_identifier);
    exit(1);
	} else {
		printf("\nPlease input a valid option...\n\n");
		main_menu(socket_identifier);
	}
}

void play_game(int socket_identifier) {
	int game_over = 0;
	uint16_t game_over_send;

	while (game_over == 0) {
		recieve_game(socket_identifier);
		get_input(socket_identifier, game_over);

		read(socket_identifier, &game_over_send, sizeof(game_over_send));
		game_over = ntohs(game_over_send);
	}
	if (game_over != 3) {
		recieve_game(socket_identifier);
	}

	if (game_over == 1) {
		printf("\nGame over! You hit a mine.\n");

		uint16_t send_game_time;
		read(socket_identifier, &send_game_time, sizeof(send_game_time));
		int game_time_send = ntohs(send_game_time);

		printf("Play time: %d seconds.\n", game_time_send);
		close(socket_identifier);
		exit(1);

	} else if (game_over == 2) {
		//read time to complete
		printf("\nCongratulations! You have located all the mines.\n");

		uint16_t send_game_time;
		read(socket_identifier, &send_game_time, sizeof(send_game_time));
		int game_time_send = ntohs(send_game_time);
		printf("\nYou have won in %d seconds.\n", game_time_send);

		main_menu(socket_identifier);
	} else {
		uint16_t send_game_time;
		read(socket_identifier, &send_game_time, sizeof(send_game_time));
		int game_time_send = ntohs(send_game_time);
		main_menu(socket_identifier);
	}
}

void recieve_game(int socket_identifier) {
	//RECV NUM MINES
	uint16_t num_mines_send;
	read(socket_identifier, &num_mines_send, sizeof(num_mines_send));
	int num_mines = ntohs(num_mines_send);

	const char* ch_arr[] = {"  A | ","  B | ","  C | ",
                          "  D | ","  E | ","  F | ",
                          "  G | ","  H | ","  I | ",};
  printf("\nRemaining mines: %d\n\n", num_mines );
	printf("      1 2 3 4 5 6 7 8 9\n  ---------------------\n");
  for (int i = 0; i < NUM_TILES_X; i++) {
		printf("%s", ch_arr[i]);
		//SEND X
		uint16_t x_send = htons(i);
		write(socket_identifier, &x_send, sizeof(x_send));
    for (int j = 0; j <  NUM_TILES_Y; j++) {
			//SEND Y
			uint16_t y_send = htons(j);
			write(socket_identifier, &y_send, sizeof(y_send));

			//RECV TILE values
			//NOT REVEALED = 101
			//REVEALED = adjmines
			//FLAG = 102
			//MINE = 103
			uint16_t tile_data_send;
			read(socket_identifier, &tile_data_send, sizeof(tile_data_send));
			int tile_data = ntohs(tile_data_send);
			// Switch statement based on recived ints from server
			switch(tile_data) {
				case 103: printf("X "); break;
				case 102: printf("F "); break;
				case 101: printf("  "); break;
				default: printf("%d ", tile_data);
			}
    }
		printf("\n");
  }
  printf("\n\n");
}

void get_input(int socket_identifier, int game_over) {
  bool menu = true;
  bool axis_vld = false;
  char option[512];
	char option_return[512];
  char coords[10] = {0};
  char axisY[9] = "ABCDEFGHI";

  while((menu || !axis_vld) && game_over == 0 ) {
    printf("Choose an option:\n<R> Reveal a tile\n<P> Place a flag\n"
    "<Q> Quit the game\n\nOption (R,P,Q): ");
		fflush(stdin);
    fgets(option, 512, stdin);
		scanf("%s", option);
    strtok(option, "\n");
    if (strcmp(option, "R") == 0 || strcmp(option, "P") == 0) {
			strcpy(option_return, option);
      printf("\nEnter tile coordinates: ");
			scanf(" %s", coords);
      strcat(option, " ");
      strcat(option, coords);
			// DEBUGGING INPUTS
			// printf("coords: %s\n", coords);
			// printf("coordsASCII: %d\n",coords[1]);
			// printf("options+coords: %s\n",option);
      menu = false;
    } else if (strcmp(option, "Q") == 0) {
      printf("\nQuiting game...\n\n");
			strcpy(option_return, option);
      menu = false;
			break;
    } else {
			printf("\nPlease input a valid option...\n\n");
		}

    for (int i = 0; i < sizeof(axisY);i++) {
      if ((option[2] == axisY[i]) && ((coords[1] >= 49) && (coords[1] <= 57 ))) {
				char temp[10];
				// DEBUGGING INPUTS
        // printf("%c to %d\n", option[2], i+1);
				sprintf(temp, "%d",i+1);
				strcat(option_return, temp);
				int x = coords[1] - 48;
				sprintf(temp, "%d",x);
				strcat(option_return, temp);
				// printf("OPTION RETURN: %s\n",option_return);
        axis_vld = true;
      }
    }
  }
	send(socket_identifier,&option_return,sizeof(option_return),0);
}

void recieve_auth(int socket_identifier){
	uint16_t send_auth_id;
	int get_auth_id = 0;

	read(socket_identifier, &send_auth_id, sizeof(send_auth_id));
	get_auth_id = ntohs(send_auth_id);

	if (get_auth_id == 1) {
			main_menu(socket_identifier);
	} else {
		printf("\nIncorrect Incorrect username or password, exiting... \n\n");
		exit(0);
	}
}

void send_login(int socket_id){
	char user[30];
	char password[30];

	printf("\n\n======================================================\n");
	printf("Welcome to the online Minesweeper gaming system\n");
	printf("======================================================\n\n");
	printf("You are required to log on with your registered user name and password.\n\n");

	printf("username: ");
	scanf("%s",user);
	printf("password: ");
	scanf("%s", password);

	send(socket_id,user,20,0);
	send(socket_id,password,20,0);
}

int main(int argc, char *argv[]) {
	int sockfd, numbytes, i=0, port;
	char buf[MAXDATASIZE];
	struct hostent *he;
	struct sockaddr_in their_addr; /* connector's address information */

	if (argc < 2) {
		fprintf(stderr,"usage: client_hostname port_number\n");
		exit(1);
	}
  if (argc >2) {
    port = atoi(argv[2]);

  } else port = PORT;

	if ((he=gethostbyname(argv[1])) == NULL) {  /* get the host info */
		herror("gethostbyname");
		exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}


	their_addr.sin_family = AF_INET;      /* host byte order */
	their_addr.sin_port = htons(port);    /* short, network byte order */
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

	if (connect(sockfd, (struct sockaddr *)&their_addr, \
	sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}

	 //Receive data from server
	send_login(sockfd);
	recieve_auth(sockfd);

		// Receive message back from server
	if ((numbytes=recv(sockfd, buf, MAXDATASIZE, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	buf[numbytes] = '\0';

	close(sockfd);
	return 0;

}
