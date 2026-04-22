# Tiny Shell Program
A Tiny shell which has internal commands, and can handle foreground and background external commands

Internal commands:
- kill <pid> (kills the program with that pid)
- jobs (lists out all current background processes)
- bye (closes all background processes and exits)

External commands:
- Program_name <args> (works as other shells)
- Program_name <args> & (creates a program in the background processes)

Program Key Points:
- Handles inter process communication to accurately keep track of background process and be able to signal to kill them
- Uses fork() to create child threads
