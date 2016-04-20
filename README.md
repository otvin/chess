# "Bejola" chess

My attempt to build a relatively simple chess playing game in order to learn Python 3.  Python is not an ideal 
language for Chess.  As of v0.8, the program can compute perft(5) from the starting position in a little under 3.2 seconds
in an Ubuntu 15 vm running on a i7-4800MQ CPU @ 2.7GHz.  That's roughly 100k nodes per second.  For comparison, a good [C-based 
engine](http://home.hccnet.nl/h.g.muller/dwnldpage.html) could do it in under 0.05 seconds.  I gave the VM 4GB of RAM, and it uses
about 2GB of cache as Python objects aren't super optiized for size.  Right now, everything is single-threaded.  I have spent
most time until now making the game fast (relatively speaking), both optimizing the raw move generation as well as implementing
heuristics that allow the search tree to be heavily pruned. The program can already beat me, but I'm not very good at chess :).

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
   
   To run the cython version, after installing Colorama and xboard, you can ```cd cython``` and run ```python3 play_chess_cython.py```.  Alternatively, if you 
   would like to precompile the cython version, you can ```python3 setup_cython.py``` before running play_chess_cython.  
      



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
    * Added a special extra evaluation for KP vs. K or KP vs. KP end games to get kings in proper position to help pawns promote
* Use of an alternative GUI.  Our TA's wrote a GUI for our checkers game, and we just built the brains.  For this project, I added support for [Xboard](https://www.gnu.org/software/xboard/).

### "New" concepts already implemented

These are techniques that I had not implemented prior to this project.

* Using previous evaluation to drive move ordering.  If the opponent chooses what the engine felt was his best move, we prime our move ordering with what was thought to be our best response, then follow the above heuristic.
* Transposition Tables with Zobrist hashing.  These store both cached move lists, to save that computation, as well as the evaluation of the board and the depth at which that evaluation was computed, to save recomputing previous positions.
* Improved the move ordering so as to use the entire previous best line instead of just the previous move at the root.  This made a huge difference with the computer playing itself doing endgame problems with deep searching.
* Further improved the move ordering with the "Killer Heuristic" which uses previous "best moves" of sibling nodes when not playing the previous best line.  Huge search space reduction.

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

#### Performance Update: 4/7/2016

I read recently that most of the way to "speed up" a Chess engine is not to make the move generation faster, but to prune more from the search tree.  Adding the "Killer Heuristic"
really helped in that area.  This mechanism learns as the engine computes.  If a move is a good response to one move by a player, good chance it will be a good response to another
move by that player, since most possible moves don't really change the position much.  To understand the impact, I had the computer playing black, searching to a depth of 7 ply, and
responding to e2-e4.  It had taken 9.0MM nodes to make that computation - in my metric, a "node" is any node in the search tree, not just a leaf.  Adding the Killer Heuristic, it reduced 
the search space to 1.4MM nodes.  Huge.  This is the best position for the Killer Heuristic, because at other positions, there is already an expected best line that I had been
exploiting, and also there are previously computed nodes in the Transposition Table.  The number of nodes examined in a position varies from run to run because of the randomness of the 
hash table, which changes what hash collisions occur, but this type of change is very significant.

I also learned about a neat Cython tool.  Running ```cython chess_cython.pyx -a``` gave me an HTML view of the cython code, highlighting areas where the code had to drop from using 
fast C code to much slower Python code.  I found some minor issues where I had failed to type properly, but I also learned that every access to a tuple, list, or dictionary is 
slow.  That makes a ton of logical sense.  So using some basic C optimization techniques, like unrolling loops and such, I was able to convert a few critical paths to using much
more C.  As far as pure speed, my previous version of Perft in Cython had reduced time needed by 73% over pure Python.  My current version has reduced the time by 87% over pure Python.
I am sure I could find some more to squeeze but the Cython is getting less readable - much like performance-tuned C code.  With all the performance work the last two weeks,  I now can 
have the computer play itself in the Lasker-Reichhelm position, and white can force mate in 28 moves.  Takes 5 1/2 hours, with some moves taking 45-50 minutes to compute.  The 
first move from the starting position can be computed in under 4 seconds, whereas 2 weeks ago it couldn't even figure that one move out even overnight.

#### Performance Update: 4/10/2016

By taking some Python objects and converting them into C-style arrays, I was able to get a little over 2x speedup when doing the perft test.  Cython is much slower when dealing
with Python objects, either built-in or user-defined.  In order to use multi-processor support for Cython, which was on my list of things to consider, the parallelized blocks cannot
access Python objects at all.  Some code would be a trivial change.  Right now I have a class for generating move lists, and making that a set of functions that operate on a board
would be super simple.  The Transposition Table (huge hash table) is also an object, and I could convert that pretty easily as well.  The big hassle would be converting the 
chessboard so it's not an object.  Originally, in the recurisve move analysis, I would make a copy of the board and apply the move being analyzed to the copy, destroying the 
copy on completion.  Early on, I realized it's much faster to "unmake" the move than copy the object.  So during runtime there is only one copy of a board outstanding.  It just would
be a large amount of code to change.  I'm going to focus on some non-performance items for a while and contemplate whether these changes are worth it.

#### Performance Update :4/19/2016

So I bit the bullet and decided to write a C-based version to see what I could accomplish.  It was easier than starting from C, as I had the Python and Cython versions to copy from.  Some tricks
that had improved performance in Python/Cython, e.g. tracking piece locations for each piece, actually hurt C performance - the overhead of maintaining the structures exceeded the savings.  However,
being able to easily use arrays in lieu of lists is a big help.  I wrote two move generation routines, one that looked almost exactly like the Python version, and one that is more C-style - much more 
in-line, and harder to read.  However, the C-style one saved about 25%.  Comparing perft(6) times from the starting position, using the most current Python, Cython, and C versions, gives me the following:

```
    
    Python: 17:38.08
    
    Cython: 1:27.94
     
    C: 0:12.3
    
    FRC-perft:  0:00.28
    
```


As you can see, Cython executes in only 8.3% of the time as pure Python.  My C version needs 14.0% of the time as the Cython, or a mere 1.17% of the time that the pure Python needed.  But then,
just to show I'm humble, I downloaded frcperft 1.0 (FRC-perft 1.0, (c) 2008-2011 by AJ Siemelink) and it is 2 orders of magnitude faster than my C version.  I 
have a very long way to go to be competitive.

I've been digging into bitboard representation.  Up until now I've used a 120-character array (in Cython/C) or 120-character list (in pure Python) to represent the board.  That maps to 
a 12x10 array in concept, with the board being the center 8x8 squares.  The outer squares are marked as "off board" and make move calculation simpler, as we can generate slider moves as long as the
destination square is empty (and "off board" is not empty).  That is a perfectly fine board representation, but bitboards can be much faster.  With a bitboard, you have one 64-bit array for
each type of piece.  So for example to compute all White Pawn moves, you can take the 64 bit board representing White Pawns, and left shift it by 8 bits.  That would get you all the candidate
one square pawn moves.  Take a second bit board that has a 1 for every square that is occupied on the board.  If you take the first bit board and logically "and" it with the logical "not" of the 
 second board, you would find all the destination squares.  Then you can loop through those bits and create one move per bit.  So comparing the two methods:
 
Traditional move generation:

```


        for each square on the board {
            
            if (white pawn is on this square) {
        
                if (the square one rank ahead of the pawn is empty) {
            
                    create a move (start square, end square)
                }
            
            }
        }
```

Traditional apply move for this move:
```

    function apply_move (start, end) {
    
        board_array[end] = board_array[start]
    
        board_array[start] = empty
    }
```


Bitboard move generation:
```

    destination squares = (white pawn bit mask << 8) & (~squares occupied bitmask)
    
    for each bit in the mask {
    
        create move (destination_square - 8, destination_squre)  
```

Apply move for this move:
```

    function apply_move (start,end) {
    
        white_pawn_bitmask &= ~(a bitmask that has a 1 in the start square and 0's everywhere else)
    
        squares occupied bitmask &= ~(same bitmask that has 1 in start square and 0's everywhere else)
    
        white_pawn_bitmask &= (a bitmask that has a 1 in end square and 0's everywhere else)
    
        squares occupied bitmask &= (same bitmask that has 1 in end square and 0's everywhere else)
    
    }
```

in terms of computation - the Traditional move generation has to loop over 64 squares, and then for each square test
the type of piece that is there, then look one square ahead to see that it's empty, and create a move.  For a pedagogical example, assume you have 8 pawns
on the board and nothing else.  So count 64 operations for the loop, 64 tests to see if that square is a pawn, 8 tests to see if the square ahead is
empty, and 8 create move operations.  That's a total of 144 operations to create the moves.  For bitboards - One bitshift left operation, one and operation gets you a list
 of 8 bits for the moves.  Intel has an instruction to find the first non-zero bit in a sequence.  Call that instruction to find the first bit, create the move, then zero that bit out.
 Repeat that 8 times, so 24 operations there.  Total of 26 operations to create all the moves.  Yes, I know that a "create move" is a much more expensive operation than
 any of the others, so it's not going to be 1/6th as many instructions but you can see the bonus.
 
 The apply move for the bitmask as I wrote it is 10 operations, but the "not" can be removed by storing a bitmask that has 1's everywhere except the start square, as that's
 only 64 masks, which is cheap memory-wise.
 
 
 Note - Pawn moves, king moves, knight moves, and even double-pawn moves are easy this way.  Sliders are much harder, and I'm still trying to grasp those.  
 
 In the interim, in the /chess/c folder you can see my c code.



### What I'd like to do in the future

* Find some way of using Python's [multiprocessing module](https://docs.python.org/3.5/library/multiprocessing.html), (or more likely Cython's) just for kicks
* Opening book
* Endgame.  As of now, it looks ahead a certain fixed depth, which can't be more than 6-ply practically speaking.  It would fail miserably at any sort of non-trivial ending, although I recently beefed up KP vs. K / KP vs. KP end games.
* Allow computer to have a fixed amount of time per move instead of just a fixed depth, allowing it to go deeper in searches in certain positions
* Allow computer to "ponder" - think while the human is making their move
* Iterative Deepening.  I had this in previously but took out when I rewrote the search to use Transposition Tables for caching scores.
* Quiescence.  I had this in, but tests showed it was not effective, so I need to rethink what moves I consider during Quiescence.


I also want to clean up the comments so it's clear to others what I did and why.  Not that anyone else will read it, but mostly so that when I come back to it in the future, it will make sense.

   
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
