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

   This will play with computer as black and a search depth of 3 ply, which currently runs 10-30 seconds per move.  If you want to play with different settings, edit the play_game() statement in __main__ in chess.py.
