
# Lab work <mark>NUMBER 4</mark>: <mark>MyShell</mark>
Authors (team):<br>
Yurii Zinchuk[https://github.com/yurii-zinchuk],<br>
Liliana Hotsko[https://github.com/NaniiiGock],<br>
Anna Polova[https://github.com/Annnnya]

## Prerequisites
G++, CMAKE, BOOST, readline. Also, you may find dependencies in `./dependencies` folder.<br>

To get `readline` lib on Ubuntu, use:
```bash
sudo apt-get install libreadline-dev
```

### Compilation

To compile the project, use either 

```bash
$ ./compile.sh
```

or utilize any IDE's build tool using CMake.


### Usage

The program `myshell` is used from cmd. It acts like a shel (command interpreter). It takes as its input various types of arguments: own scripts marked with `.msh`, inbuilt programs which it executes in current process, outher programs (executables) wich it executes in a child process. `myshell`'s behaviour is based on the arguments passed to it.

To use `myshell`, first build the project as described in `Compilation` part, then:
```bash
$ <path-to-executable>/myshell
```
, where<br>
<path-to-executable> is path to the directory where `myshell` executable is located.<br>

This will open the shell itself and you will be able to execute this shell's own commands directly form the cmd prompt:
```bash
$ mecho Hello, world!
```
Another way is to pass `.msh` scripts to `myshell` from outside the interpreter (for instance, being in bash interpreter at that moment):
```bash
$ ./myshell script.msh
```
This will run `sript.msh` without opning `myshell` as an interpreter, just use it to run the script and finish.

Also, `myshell` supports all sorts of operations listed in the assignment, for example manipulating enviorment variables and substitiuting them:
```bash
$ mexport VAR hello
$ mecho $VAR
hello
$ 
```
For more detailed and complete usage guide, please refer to the assignment, as we followed its instructions.


# Additional tasks
<mark> task with " ' ' "<mark>


