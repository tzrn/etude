# Etude
**A really small interpreter written in c.**


It isn't great and a text-based casino game is the best it can do for now.

The initial purpuse was to make somthing like a shell and get to know c better.

It's all in one file. To compile it just run `gcc etude.c -o etude`.

**Usage**
+ `./etude some_file_with_code.et`

# Quick reference
The program in etude is a series of commands. One command a string.
+ The execution starts from %main routine
```
@a comment. starts with '@'.
%main
print "Hello world!" `
ret
```

Only 4 first charecter of a command matter (prin and printf is the same command), unless command is shorter than 4 charecters (e.g. int)

**Creating variables**
+ There are 4 variable types - **int, real, char, str**
+ To create a variable you write it's type followed by its name and it's value (e.g. `int a 0` `str s text`)
+ there is also an **iarr** type (integer array). To create it you write iarr and number of elements: `iarr 10`
+ you can access array elements like this: `print /arr/0`, `swap /arr/$a /arr2/4`
+ You cannot leave out the value
+ command `list` lists all variables and their values

**Doing arythmetics**
+ There are 5 operations **sum, sub, prod, div, mod**
+ To use one of them you write the name of the operation followed by var name followed by secont argument (e.g. `div a 2`)
+ You can only do arythmetics operation by changing variable values with this command
+ second argument can be a variable but you need **$** sign for it (e.g. `sum a $b`)
+ you can concatanate string by summing them
+ variable and second value must be same type (if a is real then you've got to do `prod a 3.0`)

**Data input and output**
+ to input data you use command scan + var name (e.g. `scan a`)
+ to output data use command **prin** (or print, or printf etc.)
+ if you print arguments without double quotes it will ignore the spaces
+ to include spaces you need to put text in double quotes (e.g. `print "-> -_- <-"`)
+ to insert a variable in text use $ if var doesnt exist it'll just output it as it is (e.g. `print "I have " $i " apples and 5" $`)
+ to add a new string use ` charecter 

**Control flow**
+ **doif - else - fin** (or just doif - fin) - conditions, can be nested.
+ you can goto between conditions but do not goto between subroutines. ever.
Doif example:
```
doif $a > 10
  printf "more than 10" `
  doif $a < 20
    printf "but less than 20" `
  fin
else
  printf "less or quel to 10" `
fin
```
+ Use $ to compare variables
+ Comparation types - =,<,> (e.g. `doif $a > 10 print big`)
+ use exclamaition point with a word (up to 16 bytes) to create a **label**
+ Command **goto** goes to **label** and continues execution from there
```
int a 1
!loop
print $a `
sum a 1
doif $a < 11 goto loop
```
+ use % to create a **subroutine** and **ret** to return from it
+ use **gosu** with subroutine's name wihtout % to run it
+ subroutines have local variable scope (cant see global, deleted after ret)
+ subroutines can be used as function - use cat pass arguments and return values
+ do define whar args subroutine takes put them after sobroutine definition (e.g. `%mod x y`). They will be converted to vars.
+ to pass args give write them after gosub. To write returen value to a variable write > var after that.
+ to return value put it after ret (you can return vars using $)
+ You should put them in the end of program. Put **end** after your main program or it will start to execute all the subroutines.

Here's an example
```
int n 0 
gosu avarage 250 600 > n 
print $n `
end

%avarage a b 
sum a $b
div a 2 
ret $a
```

**Some more commands**
+ **clear** to clear the terminal emulation screen
+ **wait** to wait for specified amount of seconds
+ **swap** to swap variables value (e.g. `swap str "lamp"` `swap num $num2`)
+ **rand** `rand x y` to print random number from x to y and `rand x y v` to write that number to v
+ **glob** to go to the global variable scope
+ **loc** go back to local variable scope

There are also couple of examples in the examples folder.
