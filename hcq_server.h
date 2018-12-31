#ifndef HCQ_SERVER_H
#define HCQ_SERVER_H

#define MAX_BACKLOG 5
#define BUFSIZE 30

#define NOT_CONNECTED 0
#define WAITING_FOR_THE_NAME 1
#define WAITING_FOR_THE_ROLE 2
#define TYPING_COURSE 3
#define TYPING_COMMANDS_AS_STU 4
#define TYPING_COMMANDS_AS_TA 5

#define ASK_FOR_NAME 1
#define ASK_FOR_ROLE 2
#define INVALID_ROLE 3
#define PRESENT_TA_COMMANDS 4
#define FULL_QUEUE 5
#define ASK_FOR_COURSE 6
#define ALREADY_IN_THE_QUEUE 7
#define INVALID_COURSE 8
#define PRESENT_STU_COMMANDS 9
#define CURRENTLY_SERVING 10
#define INCORRECT_SYNTAX 11
#define KICKED_OUT 12
#define TAKEN_BY_TA 13

#define IS_TA 0
#define IS_STU 1
#define UNKONWN 2

typedef struct buffer {
  	 char *buf;		 // buffer with BUFSIZE
     int inbuf;       // How many bytes currently in buffer?
     int room;		 // How many bytes remaining in buffer?
     char *after;     // Pointer to position after the data in buf
}Buffer;

typedef struct user {
    int sock_fd;
    char *username;
	int role;
	int state;
	Buffer buffer;
	struct user *next;
}User;

/*
 * Helper function for the user initailization
 */
void initialize_users(User *user);


/*
 *	Helper function to release closed user
 */
void release_user(User *user_to_release);

/*
 * This function takes in a message and a user pointer.
 * According the message, write instruction to that user. 
 */
void write_to(User *user, int message);

/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int n);

/*
 *  Handles the commands from the user.
 *  It can take care of 5 cases: 
 *  Case 1: read in user's name and ask user for the role.  
 *  Case 2: read in user's role. If user is a TA, present ta's commands. If user is a student, 
 *  ask for the course code. 
 *  Case 3: read in the course if user is a Student.
 *  Case 4: read in user's commands if the user is a TA.
 *  Case 5: read in user's commands if the user is a Student.
 */
void commands_handler(User *user);

/*
 * Read information from given user. If read full line, implements user's commands.
 * Return this user's fd, if this user's fd is closed or user wrote things more than 30
 * characters.
 */
void read_from(User *user);

/* 
 * Add a user to the queue with given sock_fd.
 */
void add_user(int sock_fd);
#endif
