from bitarray import bitarray


# Helper functions

def algebraicToBit(strSpace):
    """
    :param strSpace: from a1..h8
    :return: an integer from 0..63 which is the space in the array corresponding to the string
    """


    assert(len(strSpace) == 2)
    assert(strSpace[1].isnumeric())

    cFile = strSpace[0].lower()
    iRank = int(strSpace[1])

    assert(cFile >= 'a' and cFile <= 'h')
    assert(iRank >=1 and iRank <=8)

    retval = 8 * (iRank-1)
    retval = retval + (ord(cFile)-97)
    return retval



class ChessBoard:

    def __init__(self):
        self.bWhiteCastleQueenSide = True
        self.bWhiteCastleKingSide = True
        self.bBlackCastleQueenSide = True
        self.bBlackCastleKingSide = True
        self.bWhiteToMove = True
        self.iEnPassantTargetSquare = -1


        self.arrBlackPawns = bitarray(64)
        self.arrBlackKnights = bitarray(64)
        self.arrBlackBishops = bitarray(64)
        self.arrBlackRooks = bitarray(64)
        self.arrBlackQueens = bitarray(64)
        self.arrBlackKing = bitarray(64)

        self.arrWhitePawns = bitarray(64)
        self.arrWhiteKnights = bitarray(64)
        self.arrWhiteBishops = bitarray(64)
        self.arrWhiteRooks = bitarray(64)
        self.arrWhiteQueens = bitarray(64)
        self.arrWhiteKing = bitarray(64)

        self.arrBlackPawns.setall(False)
        self.arrBlackKnights.setall(False)
        self.arrBlackBishops.setall(False)
        self.arrBlackRooks.setall(False)
        self.arrBlackQueens.setall(False)
        self.arrBlackKing.setall(False)
        self.arrWhitePawns.setall(False)
        self.arrWhiteKnights.setall(False)
        self.arrWhiteBishops.setall(False)
        self.arrWhiteRooks.setall(False)
        self.arrWhiteQueens.setall(False)
        self.arrWhiteKing.setall(False)

