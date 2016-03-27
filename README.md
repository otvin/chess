# "Bejola" chess

My attempt to build a relatively simple chess playing game in order to learn Python 3.5.  Python is not an ideal 
language for Chess.  As of v0.5.3, the program can compute perft(5) from the starting position in about 48 seconds
in an Ubuntu 15 vm running on a i7-4800MQ CPU @ 2.7GHz.  That's roughly 100k nodes per second.  For comparison, 
a C-based engine could easily do 10-30x faster if not more.  I gave the VM 4GB of RAM, but it could use more and build
a bigger cache.  Right now, everything is single-threaded, but I may make it multi-threaded for grins.  I have spent
most time until now making the move generation fast (relatively speaking).  Next, I want to do some code cleanup, 
make the testing tools easier to use, and then move to more 'brains.'  The program can already beat me, but I'm not
very good at chess :).

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
* Minimax search with Alpha Beta pruning
* Move ordering heuristic - in this case moves that capture are searched first, in "MVV-LVA" (Most valuable victim minus least valuable attacker), then moves that check, then other moves.
* Static evaluation function - [shamelessly stolen from here.](https://chessprogramming.wikispaces.com/Simplified+evaluation+function)
* Use of an alternative GUI.  Our TA's wrote a GUI for our checkers game, and we just built the brains.  For this project, I added support for [Xboard](https://www.gnu.org/software/xboard/).

### "New" concepts already implemented

These are techniques that I had not implemented prior to this project.

* Using previous evaluation to drive move ordering.  If the opponent chooses what the engine felt was his best move, we prime our move ordering with what was thought to be our best response, then follow the above heuristic.
* Iterative Deepening.  The engine starts at a 2-ply search, then increments by 2 additional plies using the best move from earlier search to prime the next search.
* Transposition Tables with Zobrist hashing.  As of now, these are only used to store move lists, since my focus has been on fast move generation.

### What I'd like to do in the future

* Use Transposition Tables to store board evaluation and improve alpha-beta search
* Find some way of using Python's [multiprocessing module](https://docs.python.org/3.5/library/multiprocessing.html), just for kicks
* Quiescence searches - another technique that would have avoided the bug that bit me in Checkers.  Yes, I still wonder what "could have been."
* Opening book
* Endgame.  As of now, it looks ahead a certain fixed depth, which can't be more than 6-ply practically speaking.  It would fail miserably at any sort of non-trivial ending.
* Allow computer to have a fixed amount of time per move instead of just a fixed depth, allowing it to go deeper in searches in certain positions
* Allow computer to "ponder" - think while the human is making their move

I also want to clean up the comments so it's clear to others what I did and why.

# Requirements:
   I added XBoard support because my kids demanded it.  Prior to that I had put in a little ASCII board.  For that to render, colorama 0.3.7 is required. To obtain, download the .tar.gz from: [https://pypi.python.org/pypi/colorama](https://pypi.python.org/pypi/colorama),
   do a ```tar xvf colorama-0.3.7.tar.gz``` followed by a ```python3.5 setup.py install``` from your terminal.
   
   If you want xboard instead, you can do a ```sudo apt-get xboard```, at least on Ubuntu.
   
   Easiest way to run the game is ```git clone``` to bring the code to your box, and then ```python3.5 chess.py```.  If you are running
   Xboard, the command is ```xboard -fcp "python3.5 -u chess.py"```.
      
   
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
