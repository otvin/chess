# chess

My attempt to build a relatively simple chess playing game in order to learn Python 3.5.

# Requirements:
   Requires colorama 0.3.7. To obtain: 
   1. download the .tar.gz from: https://pypi.python.org/pypi/colorama
   2. tar xvf colorama-0.3.7.tar.gz
   3. python3.5 setup.py install
   
   Likely would work fine with later versions of colorama, I just have not tested beyond 0.3.7

   Optional - can use xboard - can sudo apt-get xboard


# To run from terminal:
   1. Pull all files to a local directory
   2. python3.5 chess.py

   This will play with computer as black and a search depth of 3 ply, which currently runs about 3 seconds per move in my VM.

# To run via xboard:
   1. xboard -fcp "python3.5 -u chess.py"
   
# commands:

The game does not have a prompt (due to xboard integration).  You can see the below command list via help.  You can start from command line with --debug if you want debug output during an xboard game (e.g. xboard -fcp "python3.5 -u chess.py --debug")

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
     sd N          - set search depth to N plies.  N > 5 will be very slow right now.
     setboard FEN  - set current position to the FEN that is specified
     undo          - go back a half move (better: use 'remove' instead)
     xboard        - use xboard (GNU Chess) protocol

```
