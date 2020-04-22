# Cshell

## Functionalities

 The following functions have been written explicitly in C.

* **cd**  - Change directory

* **pwd** - Current Working directory

* **mkdir <dir>** - Make a folder (Alerts, if folder already exists)

* **rmdir <dir>** - Remove the folder (Alerts if no such file or folder exists)

* **ls** - List contents of pwd

* **ls -l** - List the contents in long listing format

* **exit** - Exit the shell ; also works for **z** char

## Execution

<pre style="background: rgb(238, 238, 238); border: 1px solid rgb(204, 204, 204); padding: 5px 10px;">make clean
make
./Cshell</pre>

`run.c` creates a child process to execute `shell.c` in a new terminal, suited for debuggging. To open it in current terminal, execute is as :

<pre style="background: rgb(238, 238, 238); border: 1px solid rgb(204, 204, 204); padding: 5px 10px;">make clean
make
./myshell</pre>