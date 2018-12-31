# DCS-help-centre-system
The Department of Computer Science has an undergraduate help centre where students can ask questions about their course work. Students swipe their T-cards to enter a software queue and wait their turn to see a TA.  I build an interactive system that could manage a similar help centre. 
First run help centre server.To connect to it, type the following at the shell: nc -C wolf.teach.cs.toronto.edu 53356. This command runs the netcat program to connect to the help centre server and the -C flag tells netcat to send network newlines.

Once you've connected, you'll be asked for your user name. Type a name and hit enter. Next you will be asked your role. If you indicate that you are a TA, then the server will tell you the valid commands for a TA which include stats and next. If you indicate that you are a student, then the server will ask you the course for which you want to ask a question and then tell you that you can use the command stats while you wait. Both TAs and students can leave the system by typing Control-C to close nc.

If you try out both roles, you will see that the results of calling stats are different depending on your role. Students see the list of TAs and who is currently being served. This is the result of the print_currently_serving function from hcq.c. TAs see the queue of students waiting which is the result of the function print_full_queue. Once a TA begins helping a student (by saying next), that student is sent a message to tell them to go see the TA and then is disconnected from the server. The nc window will remain open even though the connection has been terminated from the server. You can kill the nc process with Control-C.

Try connecting to the server. Open up at least one terminal where you connect as a TA and a few others where you connect as a student. 
Open yet another terminal window and connect. This time give a name that is longer than 30 characters. Notice that the server disconnects. 

