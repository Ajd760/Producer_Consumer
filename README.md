# Producer_Consumer
A producer-consumer exercise using Unix pthreads

This program simulates the producer consumer problem with multithreading using the Unix pthread API. 
           To run the program, compile with -lpthread library linked. 
           When running the program, it expects 3 arguments to be passed on the command line:
            1. Length of time in seconds for the main thread to sleep while the producer/consumer threads operate
            2. Number of Producer threads to create
            3. Number of Consumer threads to create

Assumptions: It is assumed that this program will be compiled in a Unix environment and it is dependent upon
               The header files listed below. It is also dependent upon my buffer.h header file which should be
               included in the same directory as this main.cpp, otherwise you need to point the compiler to it
             It is also assumed that the user will enter the proper parameters on the command line, otherwise 
               the program will notify that it did not receive the proper amount/type of parameters and abort.
