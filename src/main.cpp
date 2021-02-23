#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>
#include <limits>
#include <algorithm>

// chess pieces
#define HORSE  'J'
#define BISHOP 'S'
#define PAWN 'P'
#define EMPTY '-'

using namespace std;

class ChessBoard {
private:
    char *grid = nullptr;
    int size;
    int row_len;
    int pawn_cnt;

    // PDD specific
    int max_depth;

    void setAt(int row, int col, char value) {
        grid[row * row_len + col] = value;
    }

    class ChessPiece {
    private:
        int row;
        int col;
        char type;

    public:
        ChessPiece() = default;

        ChessPiece(int row, int col, char type) : row(row), col(col), type(type) {}

        int getRow() const {
            return row;
        }

        int getCol() const {
            return col;
        }

        char getType() const {
            return type;
        }

        void setRow(int row) {
            ChessPiece::row = row;
        }

        void setCol(int col) {
            ChessPiece::col = col;
        }

    };

    ChessPiece bishop;
    ChessPiece horse;


    void movePiece(ChessPiece &p, int row, int col) {
        if (at(row, col) == PAWN) pawn_cnt--;
        setAt(row, col, p.getType());
        setAt(p.getRow(), p.getCol(), EMPTY);
        p.setRow(row);
        p.setCol(col);
    }

public:

    // returned when accessing invalid position in chess board
    const static char INVALID_AT = '\0';

    ChessBoard(const string &filename) {
        ifstream ifs(filename);
        ifs >> row_len;
        ifs >> max_depth;
        size = row_len * row_len;
        pawn_cnt = 0;
        grid = new char[size];
        memset(grid, EMPTY, size);

        char c;
        char *head = grid;
        while (ifs.get(c) && head < grid + size) {
            if (c != '\n' && c != '\r') {
                int idx = int(head - grid);
                int row = int(idx / row_len);
                int col = idx % row_len;
                if (c == BISHOP) bishop = ChessPiece(row, col, BISHOP);
                if (c == HORSE) horse = ChessPiece(row, col, HORSE);
                if (c == PAWN) pawn_cnt++;
                *(head++) = c;
            }
        }
        ifs.close();
    };

    ChessBoard(const ChessBoard &oth) {
        grid = new char[oth.size];
        memcpy(grid, oth.grid, oth.size);
        size = oth.size;
        row_len = oth.row_len;
        bishop = oth.bishop;
        horse = oth.horse;
        pawn_cnt = oth.pawn_cnt;
        max_depth = oth.max_depth;
    };

    ~ChessBoard() {
        delete[] grid;
    }

    char at(int row, int col) const {
        if (row < 0 || col < 0 || row * row_len + col >= size) return INVALID_AT;
        return grid[row * row_len + col];
    };

    void moveBishop(int row, int col) {
        movePiece(bishop, row, col);
    }

    void moveHorse(int row, int col) {
        movePiece(horse, row, col);
    }

    int getPawnCnt() const {
        return pawn_cnt;
    }

    int getMaxDepth() const {
        return max_depth;
    }

    const ChessPiece &getBishop() const {
        return bishop;
    }

    const ChessPiece &getHorse() const {
        return horse;
    }

    int getRowLen() const {
        return row_len;
    }

    friend ostream &operator<<(ostream &os, const ChessBoard &g) {
        os << "Délka strany šachovnice: " << g.row_len << ", maximální hloubka: " << g.max_depth << endl;
        os << "Kůň na (" << g.horse.getRow() << "," << g.horse.getCol() << ")" << endl;
        os << "Střelec na (" << g.bishop.getRow() << "," << g.bishop.getCol() << ")" << endl;
        os << "Počet pěšáků " << g.pawn_cnt << endl;
        for (int i = 0; i < g.size; i++) {
            os << g.grid[i];
            if ((i + 1) % g.row_len) os << " | ";
            else os << endl;
        }
        return os << endl;
    }
};

class EvalPosition {
public:
    static int for_horse(const ChessBoard &g, int row, int col) {
        if (g.at(row, col) == PAWN) return 2;
        return 0;
    };

    static int for_bishop(const ChessBoard &g, int row, int col) {
        if (g.at(row, col) == PAWN) return 2;
        char c;

        // DIAG DOWN RIGHT
        for (int i = 1;; i++) {
            c = g.at(row + i, col + i);
            if (c == ChessBoard::INVALID_AT) break;
            if (c == PAWN) return 1;
        }

        // DIAG DOWN LEFT
        for (int i = 1;; i++) {
            c = g.at(row + i, col - i);
            if (c == ChessBoard::INVALID_AT) break;
            if (c == PAWN) return 1;
        }

        // DIAG UP RIGHT
        for (int i = 1;; i++) {
            c = g.at(row - i, col + i);
            if (c == ChessBoard::INVALID_AT) break;
            if (c == PAWN) return 1;
        }

        // DIAG UP LEFT
        for (int i = 1;; i++) {
            c = g.at(row - i, col - i);
            if (c == ChessBoard::INVALID_AT) break;
            if (c == PAWN) return 1;
        }

        return 0;
    }

};

class NextPossibleMoves {
public:
    struct NextMove {
        int row;
        int col;
        int cost;

        NextMove() = default;

        NextMove(int row, int col, int cost) : row(row), col(col), cost(cost) {}

        struct comparator {
            bool operator()(const NextMove &a, const NextMove &b) const {
                return a.cost > b.cost;
            }
        };
    };

    static vector<NextMove> for_horse(const ChessBoard &g) {
        int row = g.getHorse().getRow();
        int col = g.getHorse().getCol();

        int cand[8][2] = {
                {row + 2, col + 1},
                {row + 2, col - 1},
                {row - 2, col + 1},
                {row - 2, col - 1},

                {row + 1, col + 2},
                {row + 1, col - 2},
                {row - 1, col + 2},
                {row - 1, col - 2},
        };

        char c;
        int nrow, ncol;
        vector<NextMove> moves = vector<NextMove>();
        moves.reserve(8);
        for (const auto &i : cand) {
            nrow = i[0];
            ncol = i[1];
            c = g.at(nrow, ncol);
            if (c == EMPTY || c == PAWN) {
                moves.emplace_back(nrow, ncol, EvalPosition::for_horse(g, nrow, ncol));
            }
        }

        sort(moves.begin(), moves.end(), NextPossibleMoves::NextMove::comparator());
        return moves;
    };

    static vector<NextMove> for_bishop(const ChessBoard &g) {
        vector<NextMove> moves = vector<NextMove>();
        moves.reserve(2 * g.getRowLen() - 2);
        int row = g.getHorse().getRow();
        int col = g.getHorse().getCol();
        int nrow, ncol;
        char c;

        // DIAG DOWN RIGHT
        for (int i = 1;; i++) {
            nrow = row - i;
            ncol = col + 1;
            c = g.at(nrow, ncol);
            if (c == HORSE || c == ChessBoard::INVALID_AT) break;
            if (c == EMPTY || c == PAWN) {
                moves.emplace_back(nrow, ncol, EvalPosition::for_horse(g, nrow, ncol));
            }
        }

        // DIAG DOWN LEFT
        for (int i = 1;; i++) {
            nrow = row - i;
            ncol = col - 1;
            c = g.at(nrow, ncol);
            if (c == HORSE || c == ChessBoard::INVALID_AT) break;
            if (c == EMPTY || c == PAWN) {
                moves.emplace_back(nrow, ncol, EvalPosition::for_horse(g, nrow, ncol));
            }
        }

        // DIAG UP RIGHT
        for (int i = 1;; i++) {
            nrow = row + i;
            ncol = col + 1;
            c = g.at(nrow, ncol);
            if (c == HORSE || c == ChessBoard::INVALID_AT) break;
            if (c == EMPTY || c == PAWN) {
                moves.emplace_back(nrow, ncol, EvalPosition::for_horse(g, nrow, ncol));
            }
        }

        // DIAG UP LEFT
        for (int i = 1;; i++) {
            nrow = row + i;
            ncol = col - 1;
            c = g.at(nrow, ncol);
            if (c == HORSE || c == ChessBoard::INVALID_AT) break;
            if (c == EMPTY || c == PAWN) {
                moves.emplace_back(nrow, ncol, EvalPosition::for_horse(g, nrow, ncol));
            }
        }

        sort(moves.begin(), moves.end(), NextPossibleMoves::NextMove::comparator());
        return moves;
    };
};

void bb_dfs(const ChessBoard &g, long depth, char play, long &best, long &counter) {
    counter++;
    if (depth >= best || depth > g.getMaxDepth()) return;
    if (g.getPawnCnt() == 0) {
        best = depth;
        return;
    }

    if (play == HORSE) {
        for (const auto &m : NextPossibleMoves::for_horse(g)) {
            ChessBoard cpy = ChessBoard(g);
            cpy.moveHorse(m.row, m.col);
            bb_dfs(cpy, depth + 1, BISHOP, best, counter);
        }
    } else if (play == BISHOP) {
        for (const auto &m : NextPossibleMoves::for_bishop(g)) {
            ChessBoard cpy = ChessBoard(g);
            cpy.moveBishop(m.row, m.col);
            bb_dfs(cpy, depth + 1, HORSE, best, counter);
        }
    }
}

int main(int argc, char **argv) {

    for (int i = 1; i < argc; i++) {
        string filename = argv[i];
        long best = numeric_limits<long>::max();
        long counter = 0;

        ChessBoard board = ChessBoard(filename);
        cout << board;
        bb_dfs(board, 0, BISHOP, best, counter);

        cout << "Cena: " << best << endl;
        cout << "Počet volání: " << counter << endl;
    }


    return 0;
}
