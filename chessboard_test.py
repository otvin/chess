import chessboard

def test_algebraic_to_bitpos_converters():
    for file in range(97, 105, 1):
        for rank in range (1, 9, 1):
            algebraicpos = chr(file) + str(rank)
            converted_algebraicpos = chessboard.bitpos_to_algebraic(chessboard.algebraic_to_bitpos(algebraicpos))
            if converted_algebraicpos != algebraicpos:
                print("Failure: ", algebraicpos, " converted to ", converted_algebraicpos)

