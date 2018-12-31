#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket.h"
#include "hcq_server.h"
#include "hcq.h"

#ifndef PORT
  #define PORT 53355
#endif


// Use global variables so we can have exactly one TA list ,one student list , one user list
// , one course list, num_courses and all_fds. 
Ta *ta_list = NULL;
Student *stu_list = NULL;
User *user_list = NULL;

Course *courses;  
int num_courses;
fd_set all_fds;

  
/* 
 * Helper function 
 * Memory allocation helper function, Checking whether malloc failes. 
 * Return a void pointer 
 */   
void *Malloc(int size, char *message){  
    void *result = malloc(size);  
    if(result == NULL){  
        perror(message);  
        exit(1);      
    }  
    return result;  
}  

/*
 * Helper function for the user initailization
 */
void initialize_users(User *user){ 
	user->sock_fd = -1;
    user->username = NULL;
	user->role = UNKONWN;
	user->state = NOT_CONNECTED;
	//Initialize struct buffer
	user->buffer.buf = Malloc(sizeof(char) * BUFSIZE, "buffer");
	memset(user->buffer.buf,'\0',BUFSIZE);
	user->buffer.inbuf = 0;
	user->buffer.room = BUFSIZE - strlen(user->buffer.buf);
	user->buffer.after = user->buffer.buf;
}


/*
 *	Helper function to release closed user
 */
void release_user(User *user_to_release){
	if(user_to_release->state != NOT_CONNECTED){	
		int pre_fd = user_to_release->sock_fd;
		if(user_to_release->role == IS_TA){
			remove_ta(&ta_list,user_to_release->username);
		}
		// Only release student added to the stu_list
		else if((user_to_release->role == IS_STU)&&(user_to_release->state!=TYPING_COURSE)){
			give_up_waiting(&stu_list,user_to_release->username);
		}

		// Free allocated memory
		if(user_to_release->username !=NULL){
			free(user_to_release->username);
		}
		free(user_to_release->buffer.buf);

		// Remove this fd from all fd sets
		FD_CLR(pre_fd, &all_fds);
		close(pre_fd); // Close this fd.

		// Initialize this user
		initialize_users(user_to_release);
	}
}

/*
 * This function takes in a message and a user pointer.
 * According the message, write instruction to that user. 
 */
void write_to(User *user, int message){
	char *buf;
	int size = 0;
	switch(message){
		case ASK_FOR_NAME:
			size = asprintf(&buf,"Welcome to the Help Centre, What is your name?\r\n");
			break;
		case ASK_FOR_ROLE:
			size = asprintf(&buf,"Are you a TA or a Student (enter T or S)?\r\n");
			break;
		case INVALID_ROLE:
			size = asprintf(&buf,"Invalid role (enter T or S)\r\n");
			break;
		case PRESENT_TA_COMMANDS:	
			size = asprintf(&buf,"Valid commands for TA:\r\n\t\tstats\r\n\t\tnext\r\n\t\t(or use Ctrl-C to leave)\r\n");
			break;
		case FULL_QUEUE:
			buf = print_full_queue(stu_list);
			break;
		case ASK_FOR_COURSE:
			size = asprintf(&buf,"Valid course: CSC108, CSC148, CSC209\nWhich course are you asking about?\r\n");
			break;
		case ALREADY_IN_THE_QUEUE:
			size = asprintf(&buf, "You are already in the queue and cannot be added again for any course. Good-bye.\r\n");
			break;
		case INVALID_COURSE:
			size = asprintf(&buf, "This is not a valid course. Good-bye.\r\n");
			break;
		case PRESENT_STU_COMMANDS:
			size = asprintf(&buf, "You have been entered into the queue. While you wait, you can use the command stats to see which TAs are currently serving students\r\n");
			break;
		case CURRENTLY_SERVING:	
			buf = print_currently_serving(ta_list);
			break;
		case INCORRECT_SYNTAX:
			size = asprintf(&buf, "Incorrect syntax\r\n");
			break;
		case KICKED_OUT:
			size = asprintf(&buf,"We are disconnecting you from the server.\r\n");
			break;
		case TAKEN_BY_TA:
			size = asprintf(&buf,"Your turn to see the TA.\r\nWe are disconnecting you from the server now. Press Ctrl-C to close nc\r\n");
			break;
	}
	if(size == -1){
		perror("[hcq_server] asprintf\n");
        exit(1);
	}else if (write(user->sock_fd, buf, strlen(buf))!= strlen(buf)) { //can't write into it
		free(buf);
		release_user(user);         
   	}
	free(buf);
	return;
}

/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int n) {
	for(int i = 0; i < n-1 ;i++){
		if((buf[i] == '\r')&&(buf[i+1] == '\n')){
			return i+2;	
		}
	}
    return -1;
}


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
void commands_handler(User *user){
 	int result = 0;
	Student *helped_stu;
	switch (user->state){
		case WAITING_FOR_THE_NAME:	
			// Update user's name in the struct	
			user->username = Malloc(sizeof(char) * strlen(user->buffer.buf),"username"); 
			strcpy(user->username,user->buffer.buf);
			// Write next instruciton to user
			write_to(user,ASK_FOR_ROLE);
			// Update user's state
			user->state = WAITING_FOR_THE_ROLE;
			break;
		case WAITING_FOR_THE_ROLE:
			// Update user's role in the struct
			if(!strcmp(user->buffer.buf,"T")){
				user->role = IS_TA;
				add_ta(&ta_list,user->username);// Add new ta to the ta_list
				 write_to(user,PRESENT_TA_COMMANDS);// Present ta's commands
				// Update user's state
				user->state = TYPING_COMMANDS_AS_TA;
			}else if(!strcmp(user->buffer.buf,"S")){
				user->role = IS_STU;
				 write_to(user,ASK_FOR_COURSE);// Present ta's commands
				// Update user's state
				user->state = TYPING_COURSE;
			}else{// If user type in invalid role
				 write_to(user,INVALID_ROLE); // stay in current state
			}
			break;
		case TYPING_COURSE:
			// Add new student to stu_list
			result = add_student(&stu_list, user->username,user->buffer.buf,courses,
                        num_courses); 
        	if (result == 1) { // this student already in queue
				write_to(user,ALREADY_IN_THE_QUEUE);
				release_user(user);
        	} else if (result == 2) { // received a invalid course code
				write_to(user,INVALID_COURSE);	
				release_user(user);
        	}else{ // Student successfully added to the stu_list
				write_to(user,PRESENT_STU_COMMANDS);
				//	Update user's state
				user->state = TYPING_COMMANDS_AS_STU;
			}
			break;
		case TYPING_COMMANDS_AS_STU:
			if(!strcmp(user->buffer.buf,"stats")){
				write_to(user,CURRENTLY_SERVING);
			}else{
				write_to(user,INCORRECT_SYNTAX);
			}
			// stay in current state until being taken by some TA
			break;
		case TYPING_COMMANDS_AS_TA:
			if(!strcmp(user->buffer.buf,"stats")){
				write_to(user,FULL_QUEUE);
			}else if(!strcmp(user->buffer.buf,"next")){ // Take next student
				next_overall(user->username,&ta_list,&stu_list);
	
				// Find out the student who is currently being helped
				helped_stu = find_ta(ta_list,user->username)->current_student; 
				if(helped_stu == NULL){// this ta isn't helping anyone
					break;
				}
				// traverse user_list to find user fd
				User *target_user = user_list;
       			while (target_user!= NULL) {
					if((target_user->role == IS_STU)
						&&(!strcmp(target_user->username,helped_stu->name))){
						write_to(target_user,TAKEN_BY_TA);// Let the user know it will be disconnect 
						release_user(target_user);
					}
            		target_user = target_user->next;
        		}
			}else{
				 write_to(user,INCORRECT_SYNTAX);
			}
			// stay in current state
			break;
	}
	return;	
}


/*
 * Read information from given user. If read full line, implements user's commands.
 * Return this user's fd, if this user's fd is closed or user wrote things more than 30
 * characters.
 */
void read_from(User *user){
	int nbytes = 0;																	
	nbytes = read(user->sock_fd, user->buffer.after, user->buffer.room);
	// If client_fd has been closed
	if(nbytes == 0){
		release_user(user);
		return;
	}else if(nbytes == -1){
		perror("[hcq_server] read_from");
		exit(1);
	}
	
	// Update user's buffer
	user->buffer.inbuf += nbytes;
	
	int where;
	// the loop condition below calls find_network_newline
	// to determine if a full line has been read from the client.
	// Note: we use a loop here because a single read might result in
    // more than one full line.
	while((where = find_network_newline(user->buffer.buf, user->buffer.inbuf)) > 0) {
		// Truncate the "\r\n"
		user->buffer.buf[where-2] = '\0';
		// Implement user's command
		commands_handler(user);
		if(user->state == NOT_CONNECTED){
			return;
		}

		// update inbuf and remove the full line from the buffer.
		user->buffer.inbuf = user->buffer.inbuf - where;
		memmove(user->buffer.buf, user->buffer.buf+sizeof(char)*where, user->buffer.inbuf);
	}

	//update after and room, in preparation for the next read.
	user->buffer.after = user->buffer.buf+sizeof(char)*user->buffer.inbuf;
	user->buffer.room = BUFSIZE - user->buffer.inbuf;
	// If a user write something more than 30 characters, disconnect this user
	if(user->buffer.room == 0){
		write_to(user,KICKED_OUT);
		release_user(user);
	}
	return;
}


/* 
 * Add a user to the queue with given sock_fd.
 */
void add_user(int sock_fd){
	User * new_user = NULL;
	User *pre_user = user_list;
	if(user_list != NULL){// we have at least one user in the list so find one is released
        User *released_user = user_list;
        while (released_user!= NULL) {	// Loop throught the list all the way to the last user
			if(released_user->state == NOT_CONNECTED){
			   released_user->sock_fd = sock_fd;
			   released_user->state = WAITING_FOR_THE_NAME;
			   write_to(released_user,ASK_FOR_NAME);// Ask newly connect client for the name
			   return;
			}
			released_user = released_user->next;
			if(pre_user->next!=NULL){
				pre_user = pre_user->next;	
			}		
        }
	}
	// When there is currently no user in the list at all or we can't find any released user 
	// We create a new user	
	new_user = Malloc(sizeof(User),"User");
   	initialize_users(new_user);
	new_user->next = NULL;
	// Update new_user's sock_fd as well as state
	new_user->sock_fd = sock_fd;
	new_user->state = WAITING_FOR_THE_NAME;
	write_to(new_user,ASK_FOR_NAME);// Ask newly connect client for the name

	if (user_list == NULL) {
        // Set this new user as the head of list
		user_list = new_user;
    }else{
		// Add this new user to the tail of list
		pre_user->next = new_user;
	}
}


int main(void) {
	// Initialize course list
	num_courses = config_course_list(&courses,NULL);
	
	// Set up server socket
	struct sockaddr_in *self = init_server_addr(PORT);
    int listenfd = set_up_server_socket(self, MAX_BACKLOG);

	 // The client accept - message accept loop. First, we prepare to listen to multiple
    // file descriptors by initializing a set of file descriptors.
    int max_fd = listenfd;
    FD_ZERO(&all_fds);
    FD_SET(listenfd, &all_fds);

	while (1) {
		// select updates the fd_set it receives, so we always use a copy and retain the original.
        fd_set listen_fds = all_fds;
        int nready = select(max_fd + 1, &listen_fds, NULL, NULL, NULL);
        if (nready == -1) {
            perror("server: select");
            exit(1);
        }
		
		// Is it the original socket? Create a new connection ...
        if (FD_ISSET(listenfd, &listen_fds)) {
            int client_fd = accept_connection(listenfd);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            FD_SET(client_fd, &all_fds);
			add_user(client_fd); // Add new user
        }	
		
		// Next, check the clients.
        User *curr = user_list;
		while(curr!= NULL){
			if ((curr->sock_fd > -1) && (FD_ISSET(curr->sock_fd, &listen_fds))){
				read_from(curr);		
			}
			curr = curr->next; 
		}
	}
	// Should never get here.
    return 1;
}
