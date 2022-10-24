<h1 align="center">
    OS Scheduler
</h1>

<h2>
    Description and Assumptions
</h2>
<p>
    A CPU scheduler determines an order for the execution of its scheduled processes. It
decides which process will run according to a certain data structure that keeps track
of the processes in the system and their status.
A process, upon creation, has one of the three states: Running, Ready, Blocked (doing
I/O, using other resources than CPU or waiting on unavailable resource).
A bad scheduler will make a very bad operating system, so our scheduler is optimized in terms of memory and time usage.
The project consists of two phases:
<ul>
    <li>
        <h5>phase 1:</h5>
        <p>
        Assume a Computer with 1-CPU and infinite memory. It is required to make a
        scheduler with its complementary components as sketched in the following diagrams.
        </p>
        <div>
            <img src="/diagram.png" title="diagram">
        </div>
    </li>
    <li>
        <h5>phase 2:</h5>
        <p>
        Memory allocation capabilities included using the buddy memory allocation system. It should allocate memory
        space for processes as they enter the system and free it as they leave so that it canbe re-used by later processes.<br>
        Same assumptions of phase one in addtion to:
        <ol>
        <li>
            The total memory size is 1024 bytes
        </li>
        <li>
            Each process size is less than or equal 1024 bytes.
        </li>
        <li>
            The memory allocated for a process as it enters the system is constant over the time it spends in the system.
        </li>
        <li>
            We slightly modified the the Process Generator (in code file process generator.c) to accept an extra process information which is memsize "size of the process".
        </li>
        </ol>
        </p>
    </li>
</ul>
</p>

<h2>
    How does the OS Scheduler work?
</h2>
<p>
<ul>
    <li>
        <h3>Process Generator (Simulation & IPC)</h3>
        <h6>Code File process generator.c</h6>
    </li>
    The process generator should do the following tasks...<br>
    • Read the input files (check the input/output section below).<br>
    • Ask the user for the chosen scheduling algorithm and its parameters, if there are any.<br>
    • Initiate and create the scheduler and clock processes.<br>
    • Create a data structure for processes and provide it with its parameters.<br>
    • Send the information to the scheduler at the appropriate time (when a process arrives), so that it will be put it in its turn.<br>
    • At the end, clear IPC resources.<br>
    <li>
        <h3>Part II: Clock (Simulation & IPC)</h3>
        <h6>Code File clk.c</h6>
    </li>
    The clock module is used to emulate an integer time clock.
    <li>
        <h3>Part III: Scheduler (OS Design & IPC)</h3>
        <h6>Code File scheduler.c</h6>
    </li>
    The scheduler is the core of your work, it should keep track of the processes and their
    states and it decides - based on the used algorithm - which process will run and for
    how long.<br>
    it manages the process uisng one of the following algorithms (depends on the user choice)<br>
    1. Non-preemptive Highest Priority First (HPF).<br>
    2. Shortest Remaining time Next (SRTN).<br>
    3. Round Robin (RR)<br><br>
    The scheduling algorithm only works on the processes in the ready queue. (Processes that have already arrived.)<br><br>
    The scheduler should be able to: <br>
    1. Start a new process. (Fork it and give it its parameters.)<br>
    2. Switch between two processes according to the scheduling algorithm. (Stop the old process and save its state and start/resume another one.)<br>
    3. Keep a process control block (PCB) for each process in the system. A PCB<br>
    should keep track of the state of a process; running/waiting, execution time, remaining time, waiting time, etc.
    4. Delete the data of a process when it gets notifies that it finished. When a process finishes it should notify the scheduler on termination, the scheduler does NOT terminate the process.<br><br>
    5. Report the following information <br>
    (a) CPU utilization.<br>
    (b) Average weighted turnaround time.<br>
    (c) Average waiting time.<br>
    (d) Standard deviation for average weighted turnaround time.<br><br>
    6. Generate two files: (check the input/output section below)<br>
    (a) Scheduler.log<br>
    (b) Scheduler.perf<br>
    (c) In addition to memory.log in phase two <br>
    <li>
    <h3>Part IV: Process (Simulation & IPC)</h3>
    <h6>Code File process.c</h6>
    Each process should act as if it is CPU-bound.<br>
    Again, when a process finishes it should notify the scheduler on termination, the scheduler does NOT terminate the process.
</li>
</ul>
</p>

<h2>
    How To Run
</h2>
<p>
You can use Makefile to build and run the project.<br>
To compile your project, use the command:<br>
make<br>
To run your project, use the command:<br>
make run
</p>
<ul>

</ul>

<h2>
    Limitations
</h2>
<ul>
    <li>Only three algorithms included "mentioned above" </li>
    <li> max number of processes is 15 to guarantee correct results
    </li>
</ul>

<h2>
    Authors
</h2>
<ul>
    <li>Bemoi Erian</li>
    <li>Mark Yasser</li>
    <li>Peter Atef</li>
    <li>Doaa Ashraf</li>
</ul>
