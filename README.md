# "Bejola" chess

My attempt to build a relatively simple chess playing game in order to learn Python 3.  Python is not an ideal 
language for Chess.  As of v0.5.3, the program can compute perft(5) from the starting position in about 48 seconds
in an Ubuntu 15 vm running on a i7-4800MQ CPU @ 2.7GHz.  That's roughly 100k nodes per second.  For comparison, 
a C-based engine could easily do 10-30x faster if not more.  I gave the VM 4GB of RAM, but it could use more and build
a bigger cache.  Right now, everything is single-threaded, but I may make it multi-threaded for grins.  I have spent
most time until now making the move generation fast (relatively speaking).  Next, I want to do some code cleanup, 
make the testing tools easier to use, and then move to more 'brains.'  The program can already beat me, but I'm not
very good at chess :).

# Requirements:
   I added XBoard support because my kids demanded it.  Prior to that I had put in a little ASCII board.  For that to render, colorama 0.3.7 is required. To obtain, download the .tar.gz from: [https://pypi.python.org/pypi/colorama](https://pypi.python.org/pypi/colorama),
   do a ```tar xvf colorama-0.3.7.tar.gz``` followed by a ```python3.5 setup.py install``` from your terminal.
   
   If you want to use xboard, you can do a ```sudo apt-get xboard```, at least on Ubuntu.
   
   Easiest way to run the game is ```git clone``` to bring the code to your box, and then ```python3 chess.py```.  If you are running
   Xboard, the command is ```xboard -fcp "python3 -u chess.py"```.  Note, `python3.5` works as well.  On my box, python3 is version 3.4.3+.
   
   To validate the move generation engine, you can run several perft tests via ```python3 perft_test.py```.
   
   I have built a quick EPD position tester, currently with the Bratko-Kopec test built-in.  To execute, run ```python3 epd_tests.py``` - note I have it
   set to a pretty shallow depth (5 ply) and it only gets 12.5% correct at that depth.  I have not really begun tuning the evaluation function.  It 
   currently uses the pure python version, but will move it to use Cython shortly.
      



### Why am I doing this?

Back in 1993, I took CS 352 (Data Structures) with [Randy Pausch](https://en.wikipedia.org/Randy_Pausch), and our 
 final class project was a checkers game.  Mr. Pausch left the assignment very open-ended, and said that the winner
 of the class checkers tournament would get an A for the final without having to take it.  We lost, because I 
 overtweaked the move generation logic.  However, I can still hear Randy's voice the day of the tournament, when 
 he listed concepts that we had learned on our own,like Minimax search, that he never mentioned
 in class.  Before Google, we had to go to the library to read old papers to learn about things like that.  This class,
 more than any other, got me interested in CS as a career.  However, due to time there were numerous ideas I never
 got to try.  Iterative Deepening, for example would have avoided the bug that cost us the automatic A.
   
In the Summer of 1993, I talked with a friend about writing a chess program, but never got around to it.  Over the last
20 years I have occasionally researched it, but recently decided it's time.  I chose Python as I haven't really used
the language since 1.x in the mid-1990s, and wanted to come up to speed on something "new," while choosing a language
that allows me to focus more on the techniques used in modern chess programs while giving me lots of tools that keep 
 things simple.  I give up raw performance, and the hours that can be spent debugging C programs (most Chess programs
 are written in C for performance reasons). This game plays legal chess, but does not realize
that certain positions (e.g. KB vs K) are stalemates.

### "Old" concepts I implemented

Much of the core is re-implementing what I had already done years ago, just in a different language with different
(albeit more complex) game rules.  

* Legal move generation, validated with perft tests from multiple positions
* Minimax search with Alpha Beta pruning (Implemented now as "Negamax," same math, less code.)
* Move ordering heuristic - in this case moves that capture are searched first, in "MVV-LVA" (Most valuable victim minus least valuable attacker), then moves that check, then other moves.
* Static evaluation function - [shamelessly stolen from here.](https://chessprogramming.wikispaces.com/Simplified+evaluation+function)
* Use of an alternative GUI.  Our TA's wrote a GUI for our checkers game, and we just built the brains.  For this project, I added support for [Xboard](https://www.gnu.org/software/xboard/).

### "New" concepts already implemented

These are techniques that I had not implemented prior to this project.

* Using previous evaluation to drive move ordering.  If the opponent chooses what the engine felt was his best move, we prime our move ordering with what was thought to be our best response, then follow the above heuristic.
* Transposition Tables with Zobrist hashing.  These store both cached move lists, to save that computation, as well as the evaluation of the board and the depth at which that evaluation was computed, to save recomputing previous positions.
* Improved the move ordering so as to use the entire previous best line instead of just the previous move at the root.  This made a huge difference with the computer playing itself doing endgame problems with deep searching.

### Cython vs. Python?

After a few weeks of working on this, it became clear that while Python makes coding easy, the performance is very poor relative to C.  In researching endgames, I came across the
[Lasker-Reichhelm Position](https://chessprogramming.wikispaces.com/Lasker-Reichhelm+Position).  Per [Wikipedia](https://en.wikipedia.org/wiki/Corresponding_squares#Lasker-Reichhelm_position),
White arrives in a winning position in ply 17.  However, it would take until ply 25 for a brute-force search with simplistic static evaluation to see the possibility of the capture of the
black f pawn.  The promotion comes in ply 37, but once the capture is made, it's a much lesser 12-ply search to promote.  While researching endgame heuristics and endgame table bases,
 I saw an off-hand comment that it can take a modern engine a couple minutes max to be able to solve that position.  I turned my Python engine on it and let it run overnight.  
 Several hours later, I saw it had completed the search to ply 21 in 7 hrs 6 minutes.  In this position, iterative deepening is unhelpful, so I could turn that off and it would have taken
 about 75% of that.  1.2BB positions were examined in those 7 hours, or about 47,600 per second.  Very slow.  Either my engine was doomed to have an endgame about as good as Sargon IV did on my Commodore 64 back in 1987, or I needed to find a speedup.  I thought about writing a move generator in C, 
 just to see if I could, but I did develop an allergy to `malloc()` and `free()` back in college.

So I have spent some time working with Cython. [Cython](http://www.cython.org) compiles Python code into C, where it then runs much faster.  Python has long supported C-language extensions, so you could write core number-crunching
or computationally-intense code in C, and wrap it in Python.  In the early 1990s, [Alice](http://www.alice.org) was built this way.  That was back when Alice was at UVA not CMU.  C code is much harder to write than Python, obviously.  Cython's goal appears to be to allow you to build your C extension in a language very close to Python, and then you can easily call into 
those extensions from your Python code.  Of course, you can then make your app one big C extension, and put a 2-line Python wrapper around it.  That is what I did with the included
chess_cython.pyx.  Just taking the same code from chess.py and running it through Cython gave a 23% improvement in perft() tests.  While Python is duck-typed, Cython allows for strict typing,
which can then boost performance.  Spending an hour going through the code and adding types, making no other changes, brings the total improvement to 27% over pure Python.  I want
to see how far I can get the performance improvement.  Specifically, instead of encoding moves as a 7-item Python list, I could try a 7-integer array, which should be much faster.

If you want to try the Cython out, do a `sudo apt-get cython3 install` and then invoke the engine by `python3 play_chess_cython.py`.  You can also compare the two side-by-side
in a perft() test. `python3 perft_test.py` will run the pure Python, and `python3 perft_cython.py` will run the Cython version.  As a note, the way I have Cython set up, it compiles
on the fly and then executes.  This means there is a delay between startup and execution.  Cython does support pre-compiling to give the instant start-up time, but I don't need it in 
development.

#### Cython Update: 3/29/2016

Spent the last 2 days on the Cython version and have some interesting results to share.  First, I will say that I don't miss the test/fix/compile cycle.  That few seconds for each 
change is a bit annoying.  However, the changes were straightforward.  I only got one seg fault, and it was trying to raise an exception from a function that returned an int instead
of a Python object.  I tried the method from the documentation e.g. `cdef int myfunc(int myarg) except -1:` and returning -1 raised a seg fault.  I just handled another way.  The 
time consuming part was rewriting a data structure (the chessmove) to something that would perform better.  In the Pure Python version, a move is a list with 7 elements.  In the Cython version,
a move is a 8-byte long long.  The code is faster, but much less readable, with much bit shifting and masking.  That's not unusual when tuning for performance.  

I did 4 successive iterations of work with the Cython version.  I ran my perft_series() on each iteration, and compared with the pure python version.  As I said before, running my
Python code through Cython with no changes got me a 27% improvement.  Going through and quickly adding types to return values of frequently called functions, arguments of those
functions, and using cdef to define the types of variables in those functions, and moving the functions to "cdef" style, cut 44% off of the unedited version.  Changing the data 
structure of my Move from a list into the long long, to eliminate the overhead of creating the Python objects, got me a 24% reduction over the previous version.  Finally, going to 
the Transposition Table (class ChessPositionCache) cut 12% over the prior version.  The good news: if I look at the final version compared to the Pure Python, I've cut a bit under 73% off of the execution time.
  The bad news: That's a 3-4x speedup, so a C-based algorithm is still somewhere between 3-10x faster, likely more.  I will see how it does on the Lasker-Reichhelm position tonight.

#### Performance Update: 4/3/2016

After rewriting the search so that it can use the Transposition Tables to store score information, not just move generation, both the Python and Cython versions significantly improved
in endgame positions.  The Cython version was able to look ahead 28 plies to see that it could capture the pawn, taking 77.7 seconds.  Because of the caching, while 4.38MM nodes
were examined in that time (not much faster, measured by "nodes per second," than the previous version), in 143k cases, it had previously evaluated that exact position and it terminated the search.  In an
additional 1.8MM cases, it had seen enough information about the position in previous transpositions that it could significantly narrow the search window, leading in theory to
more aggressive pruning.  So previously, it took evaluation of 1.2BB positions to get to ply 21.  With caching enhancing the pruning, there were nearly 3 orders of magnitude fewer
nodes searched while going 7 plies deeper.  

I am currently maintaining both the pure Python and the Cython versions, which is a bit tedious.  Python is easier for development but Cython gives better performance.  



### What I'd like to do in the future

* Find some way of using Python's [multiprocessing module](https://docs.python.org/3.5/library/multiprocessing.html), just for kicks
* Opening book
* Endgame.  As of now, it looks ahead a certain fixed depth, which can't be more than 6-ply practically speaking.  It would fail miserably at any sort of non-trivial ending.
* Allow computer to have a fixed amount of time per move instead of just a fixed depth, allowing it to go deeper in searches in certain positions
* Allow computer to "ponder" - think while the human is making their move
* Iterative Deepening.  I had this in previously but took out when I rewrote the search to use Transposition Tables for caching scores.
* Quiescence.  I had this in, but tests showed it was not effective, so I need to rethink what moves I consider during Quiescence.


I also want to clean up the comments so it's clear to others what I did and why.

   
# Commands:

The game does not have a prompt (due to xboard integration).  If you run it from the command line, it starts assuming human plays white, computer
plays black, and a search depth of 4 ply.  So, it's waiting for you to make a move.  You can see the below command list via help.  If you use the 
debug command or run with the --debug flag, it will dump a ton of "only useful to me" information to screen.  If you use debug mode with Xboard,
(e.g. ```xboard -fcp "python3.5 -u chess.py --debug"```) it will dump to a file instead.

From the game's "help" command:

```

Sample move syntax:
     e2e4  - regular move
     a7a8q - promotion
     e1g1  - castle

Other commands:

     both          - computer plays both sides - cannot break until end of game
     debug         - enable debugging output / chessdebug.txt log file
     draw          - request draw due to 50 move rule
     force         - human plays both white and black
     go            - computer takes over for color currently on move
                   - NOTE: engine will pause between moves if you make computer play both sides
     help          - this list
     history       - print the game's move history
     new           - begin new game, computer black
     nopost        - disable POST
     ping TEXT     - reply with 'pong TEXT'
     post          - see details on Bejola's thinking
                   - format: PLY SCORE TIME NODES MOVE_TREE
                   - where TIME is in centiseconds, and NODES is nodes searched. SCORE < 0 favors black
     print         - print the board to the terminal
     printpos      - print a list of pieces and their current positions
     quit          - exit game
     remove        - go back a full move
     resign        - resign your position
     sd N          - set search depth to N plies.  N > 6 will be very slow.
     setboard FEN  - set current position to the FEN that is specified
     undo          - go back a half move (better: use 'remove' instead)
     xboard        - use xboard (GNU Chess) protocol
                   - this command is automatically sent by xboard. Should only
                   - be used interactively if you want to debug xboard issues.



```
