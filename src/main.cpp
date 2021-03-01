#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>
#include <limits>
#include <chrono>

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

    vector<string> move_log;

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

    void logMovePiece(int row, int col) {
        string msg = to_string(row) + "," + to_string(col);
        if (at(row, col) == PAWN) msg += " *";
        move_log.emplace_back(msg);
    }

    void movePiece(ChessPiece &p, int row, int col) {
        logMovePiece(row, col);
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
        move_log = oth.move_log;
    };

    ChessBoard &operator=(const ChessBoard &oth) {
        memcpy(grid, oth.grid, oth.size);
        size = oth.size;
        row_len = oth.row_len;
        bishop = oth.bishop;
        horse = oth.horse;
        pawn_cnt = oth.pawn_cnt;
        max_depth = oth.max_depth;
        move_log = oth.move_log;
        return *this;
    }

    ~ChessBoard() {
        delete[] grid;
    }

    char at(int row, int col) const {
        if (row < 0 || col < 0 || row >= row_len || col >= row_len) return INVALID_AT;
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

    const vector<string> &getMoveLog() const {
        return move_log;
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
        return os;
    }
};

class EvalPosition {
public:
    static int for_horse(const ChessBoard &g, int row, int col) {
        if (g.at(row, col) == PAWN) return 1;
        return 0;
    };

    static int for_bishop(const ChessBoard &g, int row, int col) {
        if (g.at(row, col) == PAWN) return 2;
        char c;

        // DIAG DOWN RIGHT
        for (int i = 1;; i++) {
            c = g.at(row + i, col + i);
            if (c == ChessBoard::INVALID_AT || c == HORSE) break;
            if (c == PAWN) return 1;
        }

        // DIAG DOWN LEFT
        for (int i = 1;; i++) {
            c = g.at(row + i, col - i);
            if (c == ChessBoard::INVALID_AT || c == HORSE) break;
            if (c == PAWN) return 1;
        }

        // DIAG UP RIGHT
        for (int i = 1;; i++) {
            c = g.at(row - i, col + i);
            if (c == ChessBoard::INVALID_AT || c == HORSE) break;
            if (c == PAWN) return 1;
        }

        // DIAG UP LEFT
        for (int i = 1;; i++) {
            c = g.at(row - i, col - i);
            if (c == ChessBoard::INVALID_AT || c == HORSE) break;
            if (c == PAWN) return 1;
        }

        return 0;
    }

};

class NextPossibleMoves {
private:
    // relative mapping for all possible horse movements
    // [ROW, COL]
    constexpr const static int horse_cand[8][2] = {
            {-2, -1},
            {-2, 1},

            {-1, 2},
            {1,  2},

            {2,  1},
            {2,  -1},

            {1,  -2},
            {-1, -2}
    };

public:
    struct NextMove {
        int row;
        int col;
        int cost;

        NextMove() = default;

        NextMove(int row, int col, int cost) : row(row), col(col), cost(cost) {}

        static bool add_bishop_if_possible(int row, int col, const ChessBoard &g, vector<NextMove> &moves) {
            char c = g.at(row, col);
            if (c == HORSE || c == ChessBoard::INVALID_AT) {
                return false;
            }
            if (c == PAWN) {
                moves.emplace_back(row, col, EvalPosition::for_bishop(g, row, col));
                return false;
            }
            if (c == EMPTY) {
                moves.emplace_back(row, col, EvalPosition::for_bishop(g, row, col));
                return true;
            }
            return false;
        }

        static bool add_horse_if_possible(int row, int col, const ChessBoard &g, vector<NextMove> &moves) {
            char c = g.at(row, col);
            if (c == EMPTY || c == PAWN) {
                moves.emplace_back(row, col, EvalPosition::for_horse(g, row, col));
                return true;
            }
            return false;
        }

        struct comparator {
            bool operator()(const NextMove &a, const NextMove &b) const {
                return a.cost > b.cost;
            }
        };

    };

    static vector<NextMove> for_horse(const ChessBoard &g) {
        int row = g.getHorse().getRow();
        int col = g.getHorse().getCol();
        vector<NextMove> moves = vector<NextMove>();
        moves.reserve(8);
        for (const auto &cand : NextPossibleMoves::horse_cand) {
            NextMove::add_horse_if_possible(cand[0] + row, cand[1] + col, g, moves);
        }

        sort(moves.begin(), moves.end(), NextPossibleMoves::NextMove::comparator());
        return moves;
    };

    static vector<NextMove> for_bishop(const ChessBoard &g) {
        vector<NextMove> moves = vector<NextMove>();
        moves.reserve(2 * g.getRowLen() - 2);
        int row = g.getBishop().getRow();
        int col = g.getBishop().getCol();

        // DIAG UP RIGHT
        for (int i = 1;; i++) {
            if (!NextMove::add_bishop_if_possible(row - i, col + i, g, moves)) break;
        }

        // DIAG UP LEFT
        for (int i = 1;; i++) {
            if (!NextMove::add_bishop_if_possible(row - i, col - i, g, moves)) break;
        }

        // DIAG DOWN RIGHT
        for (int i = 1;; i++) {
            if (!NextMove::add_bishop_if_possible(row + i, col + i, g, moves)) break;
        }

        // DIAG DOWN LEFT
        for (int i = 1;; i++) {
            if (!NextMove::add_bishop_if_possible(row + i, col - i, g, moves)) break;
        }

        sort(moves.begin(), moves.end(), NextPossibleMoves::NextMove::comparator());
        return moves;
    };

};

void bb_dfs(const ChessBoard &g, long depth, char play, long &best, ChessBoard &bestBoard, long &counter) {
    if (
            depth + g.getPawnCnt() >= best || // solution with lower cost already exists
            depth + g.getPawnCnt() > g.getMaxDepth() // max depth would be reached if each play would remove figure
            )
        return;
    if (g.getPawnCnt() == 0) {
        best = depth;
        bestBoard = ChessBoard(g);
        return;
    }
    counter++;

    if (play == HORSE) {
        for (const auto &m : NextPossibleMoves::for_horse(g)) {
            ChessBoard cpy = ChessBoard(g);
            cpy.moveHorse(m.row, m.col);
            bb_dfs(cpy, depth + 1, BISHOP, best, bestBoard, counter);
        }
    } else if (play == BISHOP) {
        for (const auto &m : NextPossibleMoves::for_bishop(g)) {
            ChessBoard cpy = ChessBoard(g);
            cpy.moveBishop(m.row, m.col);
            bb_dfs(cpy, depth + 1, HORSE, best, bestBoard, counter);
        }
    }
}

int main(int argc, char **argv) {

    for (int i = 1; i < argc; i++) {
        string filename = argv[i];
        long best = numeric_limits<long>::max();
        long counter = 0;

        ChessBoard board = ChessBoard(filename);
        cout << board << endl;
        auto start = chrono::high_resolution_clock::now();
        bb_dfs(board, 0, BISHOP, best, board, counter);
        auto stop = chrono::high_resolution_clock::now();

        cout << "Cena\tPočet volání\tČas [ms]" << endl;
        cout << best << "\t" << counter << "\t\t"
             << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << endl << endl;

        cout << "Tahy" << endl;
        for (const auto &move : board.getMoveLog()) {
            cout << move << endl;
        }

    }


    return 0;
}
