#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define MOB_NAME ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET

void init(void);
int get_random(int);
int roll_d20(void);
int roll_d10(void);
int roll_d8(void);
int roll_d6(void);
void roll_player(void);
void roll_mob(void);
char getinput(void);
void player_move(void);
void mob_move(void);
void mob_attack(void);
void player_attack(void);
bool mob_next_to_player(void);
bool player_hits_mob(void);
bool mob_hits_player(void);
int player_hit_damage(void);
int mob_hit_damage(void);
void player_inventory(void);
void player_stats(void);
bool play_again(void);
bool mob_has_initative(void);
void build_rooms(void);
void check_mob_alive(void);
void check_player_alive(void);
void show_room(void);
void game_message(const char *f, ...);

void *mob_control(void *);
void *player_control(void *);
void *get_command(void *);

#define THING_ORB 0
#define THING_SHORT_SWORD 1

#define ROOMS_X 20
#define ROOMS_Y 20

#define MAX_ROOMS ROOMS_X*ROOMS_Y
#define MAX_THINGS 10
#define MAX_NAME 30
#define MAX_DESCRIPTION 255

typedef struct
{
	char name[MAX_NAME];
	char description[MAX_DESCRIPTION];
	int dmg;
	int tohit;	
} thing;

typedef struct
{
	char name[MAX_NAME];
	char description[MAX_DESCRIPTION];
	int player_was_here;
	thing *things[MAX_THINGS];
	int thingcount;
} room;

room rooms[MAX_ROOMS];
thing things[] = {
	{"orb", "A strange glowing orb lies here.", 0, 0},
	{"short sword", "A unimpressive short sword lies here.", 5, 5}
};
room *starting_room;

#define MAX_COMMAND 20

char command_buffer[MAX_COMMAND];
char *command_index;
char player_command[MAX_COMMAND];


typedef struct
{
	char name[MAX_NAME];
	int s;
	int d;
	int i;
	int dmg;
	int tohit;
	bool alive;
	int hitpoints;
	int armorclass;
	char c;
	thing *things[MAX_THINGS];
	room *r;
} being;

being player;
being mob;
being mobs[] = {
	{"Kuza", 5, 5, 5, 5, 5, false, 10, 10, 'w'},
	{"Muza", 5, 5, 5, 5, 5, false, 10, 10, 'w'},
	{"Tuza", 5, 5, 5, 5, 5, false, 10, 10, 'w'},
	{"Yuza", 5, 5, 5, 5, 5, false, 10, 10, 'w'},
	{"Luza", 5, 5, 5, 5, 5, false, 10, 10, 'w'},
	{"Puza", 5, 5, 5, 5, 5, false, 10, 10, 'w'}
};

pthread_t mob_t, player_t, cmd_t;
bool quit;
bool win;
long t;
bool redisplay_prompt;

int main(void)
{
	init();

	do
	{
		check_mob_alive();
		check_player_alive();
		if (!player.alive) 
		{ 
			if (play_again()) roll_player(); 
		}
	} while (!quit && !win);

	if (quit) game_message("So long, and thanks for nothing.\n");
	if (win) game_message("You have saved the universe from destruction by collecting the orb, and returning it to me.  Thank you.\n");
}

void init(void)
{
	quit = false;
	win = false;
	redisplay_prompt = true;

	srandomdev();

	build_rooms();
	roll_player();
	mob.alive = false;

	game_message("A voice shouts out. \"You're here.  Hurry!  You must retrieve the orb, and return it to this room.  You are our last hope.\"\n");
	show_room();

	if (pthread_create(&player_t, NULL, player_control, (void *)t)) {game_message("Fatal error.\n"); exit(0);}
	if (pthread_create(&mob_t, NULL, mob_control, (void *)t)) {game_message("Fatal error.\n"); exit(0);}
	if (pthread_create(&cmd_t, NULL, get_command, (void *)t)) {game_message("Fatal error.\n"); exit(0);}
}

void check_mob_alive(void)
{
	if (mob.alive && mob.hitpoints <= 0)
	{
		game_message("\nThe " MOB_NAME " dies.\n", mob.name);
		mob.alive = false;
	}
}

void check_player_alive(void)
{
	if (player.alive && player.hitpoints <= 0)
	{
		game_message("\nYou have died.\n");
		player.alive = false;
	}
}

void *mob_control(void *t)
{
	time_t next_turn;

	next_turn = time(NULL);
	while (1)
	{
		if (next_turn <= time(NULL))
		{
			next_turn = time(NULL) + 1;
			if (!mob.alive)
			{
				if (player.r != starting_room && roll_d20() >= 19)
				{
					roll_mob();
					if (mob_has_initative()) mob_attack();
				}
			}
			if (mob.alive and player.alive) mob_attack();
		}	
	}
}

void *get_command(void *t)
{
	char c;
	player_command[0] = NULL;
	do
	{
		command_index = &command_buffer[0];
		*command_index = NULL;
		c = 0;
		while (c != '\n' && command_index < &command_buffer[MAX_COMMAND-1]) 
		{
				c = getchar();
				*command_index++ = c; 
				*command_index = NULL; 
		}
		fflush(stdin);
		strcpy(player_command, command_buffer);
	} while (1);
}

void *player_control(void *t)
{
	char c;

	while (1)
	{
		if (redisplay_prompt)
		{
			game_message("(%d) >", player.hitpoints);
			redisplay_prompt = false;
		}

		if (player_command[0] == '\0') continue; 
		c = player_command[0];
		player_command[0] = NULL;

		if (c == 'a') {player_attack(); continue;}
		if (c == 'i') {player_inventory(); continue;}
		if (c == 'c') {player_stats(); continue;}
		if (c == 'l') {show_room(); continue;}
		if (c == 'q') {quit = true; continue;}
		
		if (c == 'n' || c == 's' || c == 'w' || c == 'e')
		{
			if (mob_next_to_player())
			{
				game_message("You can't move, there is a " MOB_NAME " here attacking you.\n", mob.name);
				continue;
			}

			room *r;
			if (c == 'n') {r = player.r - ROOMS_X; game_message("You head north.\n");}
			if (c == 's') {r = player.r + ROOMS_X; game_message("You head south.\n");}
			if (c == 'w') {r = player.r + ROOMS_Y; game_message("You head west.\n");}
			if (c == 'e') {r = player.r - ROOMS_Y; game_message("You head east.\n");}
			if (r < &rooms[0] || r > &rooms[MAX_ROOMS]) {game_message("Oops. You can't move that way.\n"); continue;}
			player.r = r;
			show_room();
			continue;
		}
		game_message("Huh?\b\n");
	}
}

void build_rooms(void)
{
	int i;
	room *r;

	for (i = 0;i < MAX_ROOMS;i++)
	{
		rooms[i].player_was_here = 0;
		rooms[i].thingcount = 0;
		strcpy(rooms[i].name, "Unknown");
		strcpy(rooms[i].description, "You are not quite sure where you are.");
	}

	r = &rooms[get_random(MAX_ROOMS)];
	strcpy(r->name, "Sanctuary");
	strcpy(r->description, "There is a strange force here that seems to be protecting you.");
	starting_room = r;

	r = &rooms[get_random(MAX_ROOMS)];
	r->things[r->thingcount++] = &things[THING_ORB];

	r = &rooms[get_random(MAX_ROOMS)];
	r->things[r->thingcount++] = &things[THING_SHORT_SWORD];
}

void show_room(void)
{
	game_message("%s\n", player.r->name);
	game_message("%s\n", player.r->description);
	for (int i = 0; i < player.r->thingcount; i++) game_message("%s\n", player.r->things[i]->description);
}

bool play_again(void)
{
	game_message("Would you like to re-encarnate (y/n)?");
	if (getinput() == 'y') return true;
	return false;
}

void roll_mob(void)
{	
	int m = roll_d6() - 1;

	strcpy (mob.name, mobs[m].name);
	mob.alive = true;
	mob.s = mobs[m].s;
	mob.d = mobs[m].d;
	mob.i = mobs[m].i;
	mob.tohit = mobs[m].tohit;
	mob.dmg = mobs[m].dmg;
	mob.hitpoints = mobs[m].hitpoints;
	mob.armorclass = mobs[m].armorclass;
	mob.r = player.r;

	game_message("A " MOB_NAME " has arrived.\n", mob.name);
}

bool mob_has_initative(void)
{
	if (roll_d20() <= 10) 
	{
		game_message("You suprise the " MOB_NAME ".\n", mob.name);
		return false;
	}
	game_message("You are surprised by the " MOB_NAME ".\n", mob.name);
	return true;
}	

void mob_attack(void)
{
	if (mob_next_to_player())
	{
		game_message("The " MOB_NAME " swings at you and ", mob.name);
		if (mob_hits_player())
		{
			int d = mob_hit_damage();
			game_message("hits for %d damage.\n", d);
			player.hitpoints -= d;
		} else game_message("misses.\n");
	} 
}

void player_inventory(void)
{
	game_message("You don't have anything.\n");
}

void player_stats(void)
{
	game_message("Name: [%s],  Hitpoints: [%d]\n", player.name, player.hitpoints);
	game_message("Str:  [%d],  Dex: [%d],    Int: [%d]\n", player.s, player.d, player.i);
	game_message("AC:   [%d],  To-hit: [%d], Base damage: [%d]\n", player.armorclass, player.tohit, player.dmg);
}

void player_attack(void)
{
	if (mob_next_to_player())
	{
		game_message("You swing at the " MOB_NAME " and ", mob.name);
		if (player_hits_mob())
		{
			int d;
			d = player_hit_damage();
			game_message("hits for %d damage.\n", d);
			mob.hitpoints -= d;
		} else game_message("miss.\n");
	} 
	else game_message("There is nothing here to hit.\n");
}

int player_hit_damage(void)
{
	return (roll_d10() + player.dmg);
}

int mob_hit_damage(void)
{
	return (roll_d6() + mob.dmg);
}

bool mob_next_to_player(void)
{
	if (mob.alive && player.r == mob.r) return true;
	return false;
}

bool player_hits_mob(void)
{
	if (roll_d20() + player.tohit > mob.armorclass) return true;
	return false;
}

bool mob_hits_player(void)
{
	if (roll_d20() + mob.tohit > player.armorclass) return true;
	return false;
}

int get_random(int r)
{
	return (random() % (r + 1));
}
int roll_d20(void)
{
	return (get_random(20));
}

int roll_d6(void)
{
	return (get_random(6));
}

int roll_d10(void)
{
	return(get_random(10));
}

void roll_player(void)
{
	char c;
	while(1)
	{
		game_message("Rolling a new character.\n");
		player.s = roll_d20();
		player.d = roll_d20();
		player.i = roll_d20();
		game_message("str: %d, dex: %d, int: %d\n", player.s, player.d, player.i);
		game_message("Accept or quit (y/n or q)?");
		c = getinput();
		if (c == 'q') {quit = true; return;}
		if (c == 'y')
		{
			c = 0;
			do
			{
				game_message("Choose a class: (w)arrier, (t)hief, wi(z)ard?");
				c = getinput();
			} while (c != 'w' && c != 't' && c != 'z');

			player.c = c;
			player.tohit = roll_d6();
			player.dmg = roll_d6();

			if (player.c=='w') {player.tohit += player.s * .2; player.dmg += player.s * .2; player.hitpoints = roll_d20() + player.dmg;}
			if (player.c=='t') {player.tohit += player.d * .2; player.dmg += player.d * .2; player.hitpoints = roll_d20() + player.dmg;}
			if (player.c=='z') {player.tohit += player.i * .2; player.dmg += player.i * .2; player.hitpoints = roll_d20() + player.dmg;}
			player.r = starting_room;
			player.alive = true;
			return;
		}
	}
}

void game_message(const char *f, ...)
{
	va_list args;

	va_start(args, f);
	vprintf(f, args);
	va_end(args);
	redisplay_prompt = true;
	fflush(stdout);
}

char getinput(void)
{
	char c, t = getchar();
	while (t != '\n') {c = t; t = getchar();}
	return c;
}