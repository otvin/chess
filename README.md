# chess

My attempt to build a relatively simple chess playing game in order to learn Python 3.5.

# Requirements:
   Requires colorama 0.3.7. To obtain: 
   1. download the .tar.gz from: https://pypi.python.org/pypi/colorama
   2. tar xvf colorama-0.3.7.tar.gz
   3. python3.5 setup.py install
   
   Likely would work fine with later versions of colorama, I just have not tested beyond 0.3.7

# To run:
   1. Pull all files to a local directory
   2. python3.5 chess.py

   This will play with computer as black and a search depth of 3 ply, which currently runs about 3 seconds per move in my VM.
   
# runtime options:

```

usage: chess.py [-h] [-b] [-w] [--fen FEN] [--debug] [--depth DEPTH]

Play chess!

optional arguments:
  -h, --help     show this help message and exit
  -b, --black    computer plays black
  -w, --white    computer plays white
  --fen FEN      FEN for where game is to start
  --debug        print debug messages during play
  --depth DEPTH  Search depth in plies

```
