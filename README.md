
## What is it <a name="introduction"></a>
The shell is a command language interpreter.
## What can it do now <a name="paragraph1"></a>
Aviable:
1. Execution of various commands
2. Redirection of i/o stream by using symbols '<' & '>' from/to file
3. Execution of complicated pipe commands using symbol '|'
4. Execution of programm in background mode by adding symbol '&' in the end
5. cd
6. Execution of complicated conveyer commands using lex "&&"
## How to compile and run <a name="paragraph2"></a>
1. Download
```
git clone https://github.com/DaurKas/shell
```
2. Compile
```
make all
```
3. Execute 
```
./bin/main
```
4. Entertain yourself, for example:
```
ls -l
pwd > out.txt
ls | grep .txt | sort
evince &
cd ~
cd dir1 && ls
sl
```
6. Finally 
```
quit
```
or
```
exit
```
* NOTE: You have to have cpplint installed in order to make programm
