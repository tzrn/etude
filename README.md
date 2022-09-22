# Etude
**A really small interpreter written in c.**


It isn't great and a text-based casino game is the best it can do for now.

The initial purpuse was to make somthing like a shell and get to know c better.

It's all in one file. To compile it just run `gcc etude.c -o etude`.

**Usage**
+ `./etude some_file_with_code.et`

# Quick reference
The program in etude is a series of commands. One command a string.

Only 4 first charecter of a command matter (prin and printf is the same command), unless command is shorter than 4 charecters (e.g. int)

**Creating variables**
+ There are 4 variable types - **int, real, char, str**
+ To create a variable you write it's type followed by its name and it's value (e.g. `int a 0` `str s text`)
+ You cannot leave out the value
+ command `list` lists all variables and their values

**Doing arythmetics**
+ There are 4 operations **sum, sub, prod, div**
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
+ Command **doif** takes a condition and a command. It executes command only if the statement true.
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

**Some more commands**
+ **clear** to clear the terminal emulation screen
+ **wait** to wait for specified amount of seconds
+ **swap** to swap variables value (e.g. `swap str "lamp"` `swap num $num2`)
+ **rand** `rand x y` to print random number from x to y and `rand x y v` to write that number to v

There are also couple of examples in the examples folder.
