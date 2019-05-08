#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

	/* DEFINES */
	#define RANDOM_NUMBER_SEED 42
	#define NUM_TILES_X 9
	#define NUM_TILES_Y 9
	#define NUM_MINES 10
	#define ADJ_TILES 8
	#define BACKLOG 10     /* how many pending connections queue will hold */
	#define RETURNED_ERROR -1
  #define PORT 12345

	int sockfd, new_fd;

	time_t time(time_t *t);

	typedef struct {
		int adjacent_mines;
		bool revealed;
		bool is_mine;
		bool is_flag;
	} Tile;

	typedef struct {
		// ...Additional Fields...
		bool game_leave;
		bool game_over;
		int remaining_mines;
		Tile tiles[NUM_TILES_X][NUM_TILES_Y];
	} GameState;

	// Prototypes
	void check_auth(int socket_id, char user[30], char password[30]);
	void recv_credentials(int socket_id);
	void server_shutdown(int sig);
	void game_init(int socket_id);
	void create_mines(GameState *game);
	void place_mines(GameState *game);
	void adj_mines(GameState *game);
	void zero_clear(GameState *game);
	void game_data(GameState *game, int socket_id);
	void update_game(GameState *game, int socket_id);
	void recv_menuOption(int socket_id);


void game_init(int socket_id) {
// Main Game Loop
	time_t game_start;
	time_t game_time;
	game_start = time(NULL);

	GameState game;
	game.remaining_mines = NUM_MINES;
	game.game_over = false;
	game.game_leave = false;
	srand(RANDOM_NUMBER_SEED);
	create_mines(&game);
	place_mines(&game);
	adj_mines(&game);
	while (!game.game_over) {
		game_data(&game, socket_id);
		update_game(&game, socket_id);
		zero_clear(&game);
	}
	//Time
	game_time = time(NULL) - game_start;
	if (!game.game_leave) {
		game_data(&game, socket_id);
	}

	int game_time_send = game_time;
	uint16_t send_game_time = htons(game_time_send);
	write(socket_id, &send_game_time, sizeof(send_game_time));

	printf("\nTime played: %d seconds\n", game_time_send);
	recv_menuOption(socket_id);
}

void create_mines(GameState *game) {
	// Initial tile setup
  for (int x = 0; x < NUM_TILES_X; x++) {
    for (int y = 0; y < NUM_TILES_Y; y++) {
      game->tiles[x][y].is_mine = false;
      game->tiles[x][y].is_flag = false;
      game->tiles[x][y].revealed = false;
      game->tiles[x][y].adjacent_mines = 0;
    }
  }
}

void place_mines(GameState *game) {
  for (int i = 0; i < NUM_MINES; i++) {
    int x, y;
    do {
			// Rand based on the SEED
      x = rand() % NUM_TILES_X;
      y = rand() % NUM_TILES_Y;
    } while (game->tiles[x][y].is_mine);
    // place mine at (x, y)
    game->tiles[x][y].is_mine = true;
  }
}

void adj_mines(GameState *game) {
	// Check tiles adjacent mines
	int mine_check[8][2] =  {{-1,-1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
	for (int i = 0; i < NUM_TILES_X; i++) {
		for (int j = 0; j < NUM_TILES_Y; j++) {
			for (int tile = 0; tile < ADJ_TILES; tile++) {
				int ti = i + mine_check[tile][0];
				int tj = j + mine_check[tile][1];
				if (ti < 0 || ti >= NUM_TILES_X || tj < 0 || tj >= NUM_TILES_Y) {
					continue; //continues to next iteration
				}
				if (game->tiles[ti][tj].is_mine) {
					game->tiles[i][j].adjacent_mines += 1;
				}
			}
		}
	}
}

void zero_clear(GameState *game) {
	// Check tiles for adjacent 0's
	int mine_check[8][2] =  {{-1,-1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
	int count = 0;
	for (int i = 0; i < NUM_TILES_X; i++) {
		for (int j = 0; j < NUM_TILES_Y; j++) {
			for (int tile = 0; tile < ADJ_TILES; tile++) {
				int ti = i + mine_check[tile][0];
				int tj = j + mine_check[tile][1];
				if (ti < 0 || ti >= NUM_TILES_X || tj < 0 || tj >= NUM_TILES_Y) {
					continue; //continues to next iteration
				}
				if ((game->tiles[i][j].adjacent_mines == 0) && (game->tiles[i][j].revealed) && !(game->tiles[i][j].is_mine)) {
					if (!game->tiles[ti][tj].revealed && !game->tiles[ti][tj].is_flag) {
						count++;
						game->tiles[ti][tj].revealed = true;
					}
				}
			}
		}
	}
	if (count != 0) {
		// Recursively check the rest of the tiles
		zero_clear(game);
	}
}

void game_data(GameState *game, int socket_id) {
	//SEND NUM MINES
	int num_mines = game->remaining_mines;
	uint16_t num_mines_send = htons(num_mines);
	write(socket_id, &num_mines_send, sizeof(num_mines_send));
	//RECIEVE X Y
	for (int i = 0; i < NUM_TILES_X; i++) {
		//RECV X
		uint16_t x_send;
		read(socket_id, &x_send, sizeof(x_send));
		int x = ntohs(x_send);
		for (int j = 0; j <  NUM_TILES_Y; j++) {
			//RECV Y
			uint16_t y_send;
			read(socket_id, &y_send, sizeof(y_send));
			int y = ntohs(y_send);

			//check XY Tile
			//NOT REVEALED = -1
			//REVEALED = adjmines
			//FLAG = -2
			//MINE = -3
			int return_value;
			if (game->tiles[x][y].revealed) {
				if (game->tiles[x][y].is_mine) {
					return_value = 103;
				} else {
					return_value = game->tiles[x][y].adjacent_mines;
				}

			} else if (game->tiles[x][y].is_flag) {
				return_value = 102;
			} else {
				return_value = 101;
			}

			//send tile game_data
			// printf("Sending return value: %d\n", return_value);
			uint16_t tile_data_send = htons(return_value);
			write(socket_id, &tile_data_send, sizeof(tile_data_send));
		}
	}
}

void update_game(GameState *game, int socket_id) {
	char option_return[512];
	int get_option_return;

	get_option_return = recv(socket_id,option_return,sizeof(option_return),0);
	int y = option_return[2] - '0';
	int x = option_return[1] - '0';
	x--;
	y--;
	// DEBUGGING INPUT
	// printf("coordinates: X:%d, Y:%d\n", x,y);

	// REVEALING A TILE
	if (strncmp(option_return, "R", 1) == 0) {
		printf("Reveal location: %c, %c\n", option_return[2], option_return[1]);
		if (!(game->tiles[x][y].revealed) && !(game->tiles[x][y].is_flag)) {
			if (game->tiles[x][y].is_mine) {
				game->game_over = true;
				//send gameG_over signal
				uint16_t game_over_send = htons(1);
				write(socket_id, &game_over_send, sizeof(game_over_send));

				for (int i = 0; i < NUM_TILES_Y; i++) {
					for (int j = 0; j <  NUM_TILES_X; j++) {
						if (game->tiles[i][j].is_mine) {
							game->tiles[i][j].revealed = true;
						}
					}
				}
			} else{
				//send gameG_over signal
				uint16_t game_over_send = htons(0);
				write(socket_id, &game_over_send, sizeof(game_over_send));
			}
			game->tiles[x][y].revealed = true;
		} else {
			//Attempting to Reveal a Flag
			uint16_t game_over_send = htons(0);
			write(socket_id, &game_over_send, sizeof(game_over_send));
		}
		// PLACING A FLAG
	} else if (strncmp(option_return, "P", 1) == 0) {
		printf("Flag location: %c, %c\n", option_return[2], option_return[1]);
		if (!(game->tiles[x][y].revealed) && !(game->tiles[x][y].is_flag)) {
			if (game->tiles[x][y].is_mine) {
				game->remaining_mines--;
			}
			game->tiles[x][y].is_flag = true;
			if (game->remaining_mines != 0) {
				//send gameG_over signal
				uint16_t game_over_send = htons(0);
				write(socket_id, &game_over_send, sizeof(game_over_send));
			}
		} else {
			//Attempting to Place a Flag on a Flagged Tile
			uint16_t game_over_send = htons(0);
			write(socket_id, &game_over_send, sizeof(game_over_send));
		}
		//reveal ALL tiles - win
		if (game->remaining_mines == 0) {
			game->game_over = true;
			//send gameG_over signal - win
			uint16_t game_over_send = htons(2);
			write(socket_id, &game_over_send, sizeof(game_over_send));

			for (int i = 0; i < NUM_TILES_Y; i++) {
				for (int j = 0; j <  NUM_TILES_X; j++) {
					game->tiles[i][j].revealed = true;
				}
			}
		}
	} else {
		// Quits game and resets the board
		printf("\nQuitting game\n");
		game->game_over = true;
		game->game_leave = true;
		uint16_t game_over_send = htons(3);
		write(socket_id, &game_over_send, sizeof(game_over_send));
	}
}


void check_auth(int socket_id, char user[30], char password[30]){
		// Checks username and password input sent from client
		FILE *p;
    char file_user[30], file_password[30];
		int auth_id = 0;
		uint16_t send_auth_id;

		// Error checking
		if ((p = fopen("./Authentication.txt", "r")) == NULL) {
			perror("fopen failed");
			printf("file open fail\n");
		}

		while (fgetc(p) != '\n'); /* Move past the column titles in the file. */

		/* Each line in the file contains the username and password separated by a space. */
		while (fscanf(p, "%s %s\n", file_user, file_password) > 0) {
			/* Check if the username matches one in the file, and if the password matches for that username. */
			if (strcmp(file_user, user) == 0 && strcmp(file_password, password) == 0) {
				auth_id = 1;
			}
		}
		// Send a code to client 1 == auth 0 == incorrect login
		send_auth_id = htons(auth_id);
		write(socket_id, &send_auth_id, sizeof(send_auth_id));
		fclose(p);
}

void recv_menuOption(int socket_id) {

	//Recieve menu option from the client
	uint16_t selection_send;
	int selection_int = 0;

	read(socket_id, &selection_send, sizeof(selection_send));
	selection_int = ntohs(selection_send);
	if (selection_int == 1) {
		game_init(socket_id);
	} else {
		//FINISH LEADERBOARD
	}
}

void recv_credentials(int socket_id) {

	// Recieve username and password from the client
	char user[30],password[30];
	int get_pass, get_user;

	get_user = recv(socket_id,user,20,0);
	get_pass = recv(socket_id,password,20,0);

	printf("\n\nUsername recieved: %s", user);
	printf("\nPassword recieved: %s\n\n", password);

	// Send User / Pass to be checked via auth.txt
	check_auth(socket_id, user, password);
	recv_menuOption(socket_id);
}

void server_shutdown(int sig)
{
	// Close sockets to end server
    printf("\n\nGracefully closing server...\n");
		close(sockfd);
		close(new_fd);
		exit(1);
}

int main(int argc, char *argv[]) {

	/* Thread and thread attributes */
	pthread_t client_thread;
	pthread_attr_t attr;

	 /* listen on sock_fd, new connection on new_fd */
	struct sockaddr_in my_addr;    /* my address information */
	struct sockaddr_in their_addr; /* connector's address information */
	socklen_t sin_size;
	int port;

	// SIGINT CALL
	signal(SIGINT, server_shutdown);

	/* Get port number for server to listen on */
	if (argc < 1) {
		fprintf(stderr,"usage: client port_number\n");
		exit(1);
	}
  if (argc > 1) {
    port = atoi(argv[1]);
  } else port = PORT;

	/* generate the socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	/* generate the end point */
	my_addr.sin_family = AF_INET;         /* host byte order */
	my_addr.sin_port = htons(port);     /* short, network byte order */
	my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
		/* bzero(&(my_addr.sin_zero), 8);   ZJL*/     /* zero the rest of the struct */

	/* bind the socket to the end point */
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) \
	== -1) {
		perror("bind");
		exit(1);
	}

	/* start listnening */
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("server starts listnening ...\n");


	/* repeat: accept, send, close the connection */
	/* for every accepted connection, use a sepetate process or thread to serve it */
	while(1) {  /* main accept() loop */
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, \
		&sin_size)) == -1) {
			perror("accept");
			continue;
		}
		printf("server: got connection from %s\n", \
			inet_ntoa(their_addr.sin_addr));

		//Create a thread to accept client

		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_create(&client_thread, &attr, recv_credentials, new_fd);

		pthread_join(client_thread,NULL);

	}
}
