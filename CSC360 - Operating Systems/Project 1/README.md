# Objective: Create a Process Manager

## Background Execution of Programs
1. Start a Program in the Background (**bg foo**)
   * PMan starts a program, **foo**, when command **bg foo** is inputted
2. Display a list of programs running in the background (**bglist**)
   * PMan displays **process ID**, **filename**, and **total number** of running programs in the background after inputting **bglist**
   * Example: <br> **123**: **/home/user/a1/foo** <br>
              **456**: **/home/user/a1/foo** <br>
              **Total background jobs**: 2 <br>
3. Terminate, Temporarily Stop, and Resume Programs (**bgkill pid**, **bgstop pid**, **bgstart pid**)
   * PMan will send the **TERM** signal to the program with the process id (**pid**) to terminate/kill the program after receiving the **bgkill pid** command <br>
     PMan needs to notify users when a program is **terminated**
   * PMan will send the **STOP** signal to the program with the **pid** to stop the program temporarily after receiving the **bgstop pid** command
   * PMan will send the **CONT** signal to the program with the **pid** to resume the program after receiving the **bgstart pid** command

## Status of Process
* Display the following information of an existing process (**pstat pid**) <br>
1. **comm**: The filename of the executable <br>
2. **state**: <br> **R** (Running) <br> **S** (Sleepingin an interruptible wait) <br> **D** (Waiting in uninterruptible disk sleep) <br>
**Z** (Zombie) <br> **T** (Stopped on a signal) <br>
3. **utime**: The amount of time the process has been scheduled in user mode (in clock ticks). This includes guest time <br>
4. **stime**: The amount of time the process has been scheduled in kernel mode (in clock ticks) <br>
5. **rss**: The number of pages the process has in actual memory (also known as resident set size) <br>
6. **voluntary ctxt switches**: The number of voluntary context switches <br>
7. **nonvoluntary ctxt switches**: The number of involuntary context switches <br>

## Error Handling
* If user inputs an **invalid command**, PMan needs to return an error message without interrupting any processes
* If **pid** does not exist, PMan returns an error message to notify the user about the invalid pid
