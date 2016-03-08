from bitarray import bitarray

# Helper functions


def algebraic_to_bitpos(algebraicpos):
    """
    :param algebraicpos: from "a1".."h8"
    :return: an integer from 0..63 = space in the array corresponding to the square represented by algebraicpos
    """

    assert(len(algebraicpos) == 2)
    assert(algebraicpos[1].isnumeric())

    file = algebraicpos[0].lower()
    rank = int(algebraicpos[1])

    assert('a' <= file <= 'h')
    assert(1 <= rank <= 8)

    retval = 8 * (rank-1)
    retval += (ord(file)-97)  # 97 is the ascii for "a"
    return retval


def bitpos_to_algebraic(bitpos):
    """
    :param bitpos: an integer from 0..63 which is the space in the array corresponding to the square
    :return: an algebraic description of the square from "a1".."h8"
    """
    assert (0 <= bitpos <= 63)

    file = chr(97 + (bitpos % 8))
    rank = (bitpos // 8) + 1
    return file + str(rank)




class ChessBoard:

    def __init__(self):
        self.white_castle_queen_side = True
        self.white_castle_king_side = True
        self.black_castle_queen_side = True
        self.black_castle_king_side = True
        self.white_to_move = True
        self.en_passant_target_square = -1
        self.halfmove_clock = 0
        self.fullmove_number = 1

        self.black_pawns_array = bitarray(64)
        self.black_knights_array = bitarray(64)
        self.black_bishops_array = bitarray(64)
        self.black_rooks_array = bitarray(64)
        self.black_queens_array = bitarray(64)
        self.black_king_array = bitarray(64)

        self.white_pawns_array = bitarray(64)
        self.white_knights_array = bitarray(64)
        self.white_bishops_array = bitarray(64)
        self.white_rooks_array = bitarray(64)
        self.white_queens_array = bitarray(64)
        self.white_king_array = bitarray(64)

        self.black_pawns_array.setall(False)
        self.black_knights_array.setall(False)
        self.black_bishops_array.setall(False)
        self.black_rooks_array.setall(False)
        self.black_queens_array.setall(False)
        self.black_king_array.setall(False)
        self.white_pawns_array.setall(False)
        self.white_knights_array.setall(False)
        self.white_bishops_array.setall(False)
        self.white_rooks_array.setall(False)
        self.white_queens_array.setall(False)
        self.white_king_array.setall(False)

    def initialize_start_position(self):
        self.black_pawns_array = bitarray("0000000000000000000000000000000000000000000000001111111100000000")
        self.black_knights_array = bitarray("0000000000000000000000000000000000000000000000000000000001000010")
        self.black_bishops_array = bitarray("0000000000000000000000000000000000000000000000000000000000100100")
        self.black_rooks_array = bitarray("0000000000000000000000000000000000000000000000000000000010000001")
        self.black_queens_array = bitarray("0000000000000000000000000000000000000000000000000000000000010000")
        self.black_king_array = bitarray("0000000000000000000000000000000000000000000000000000000000001000")

        self.white_pawns_array = bitarray("0000000011111111000000000000000000000000000000000000000000000000")
        self.white_rooks_array = bitarray("1000000100000000000000000000000000000000000000000000000000000000")
        self.white_knights_array = bitarray("0100001000000000000000000000000000000000000000000000000000000000")
        self.white_bishops_array = bitarray("0010010000000000000000000000000000000000000000000000000000000000")
        self.white_queens_array = bitarray("0001000000000000000000000000000000000000000000000000000000000000")
        self.white_king_array = bitarray("0000100000000000000000000000000000000000000000000000000000000000")

        self.white_castle_queen_side = True
        self.white_castle_king_side = True
        self.black_castle_queen_side = True
        self.black_castle_king_side = True
        self.white_to_move = True
        self.en_passant_target_square = -1
        self.halfmove_clock = 0
        self.fullmove_number = 1

    def convert_bitboard_to_array(self):
        board_array = [[" " for x in range(8)] for x in range(8)]
        for i in range(64):
            cur_square = " "
            if self.black_pawns_array[i]:
                cur_square = "p"
            elif self.black_knights_array[i]:
                cur_square = "n"
            elif self.black_bishops_array[i]:
                cur_square = "b"
            elif self.black_rooks_array[i]:
                cur_square = "r"
            elif self.black_queens_array[i]:
                cur_square = "q"
            elif self.black_king_array[i]:
                cur_square = "k"
            elif self.white_pawns_array[i]:
                cur_square = "P"
            elif self.white_knights_array[i]:
                cur_square = "N"
            elif self.white_bishops_array[i]:
                cur_square = "B"
            elif self.white_rooks_array[i]:
                cur_square = "R"
            elif self.white_queens_array[i]:
                cur_square = "Q"
            elif self.white_king_array[i]:
                cur_square = "K"

            if cur_square != " ":
                board_array[i // 8][i % 8] = cur_square

        return board_array

    def load_from_fen(self, fen):
        pass

    def convert_to_fen(self):

        retval = ""
        board_array = self.convert_bitboard_to_array()
        for file in range(7, -1, -1):
            num_blanks = 0
            for rank in range(0, 8, 1):
                if board_array[file][rank] == " ":
                    num_blanks += 1
                else:
                    if num_blanks > 0:
                        retval += str(num_blanks)
                        num_blanks = 0
                    retval += board_array[file][rank]

            if num_blanks > 0:
                retval += str(num_blanks)
            retval += "/"

        retval += " "

        if self.white_to_move:
            retval += "w"
        else:
            retval += "b"

        retval += " "

        castle_string = ""
        if self.white_castle_king_side:
            castle_string += "K"
        if self.white_castle_queen_side:
            castle_string += "Q"
        if self.black_castle_king_side:
            castle_string += "k"
        if self.black_castle_queen_side:
            castle_string += "q"
        if castle_string == "":
            castle_string = "-"
        retval += castle_string

        retval += " "

        if self.en_passant_target_square == -1:
            retval += "-"
        else:
            retval += bitpos_to_algebraic(self.en_passant_target_square)

        retval += " "

        retval += str(self.halfmove_clock) + " " + str(self.fullmove_number)

        return retval

