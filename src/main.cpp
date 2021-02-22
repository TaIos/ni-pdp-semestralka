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
#define EMPTY 'X'

using namespace std;

class ChessBoard {
private:
    char *grid = nullptr;
    int size;
    int row_len;
    int pawn_cnt;

    // PDD specific
    int max_depth;

    void set_at(int row, int col, char value) {
        grid[row * row_len + col] = value;
    }

    class ChessPiece {
    private:
        int row;
        int col;
        char type;
        ChessBoard *g;

    public:
        ChessPiece() = default;

        ChessPiece(int row, int col, const char type, ChessBoard *g) : row(row), col(col), type(type), g(g) {}

        void move(int row, int col) {
            if (g->at(row, col) == PAWN) g->pawn_cnt--;
            g->set_at(row, col, type);
            g->set_at(this->row, this->col, EMPTY);
            this->row = row;
            this->col = col;
        }

        int getRow() const {
            return row;
        }

        int getCol() const {
            return col;
        }

    };

    ChessPiece bishop;
    ChessPiece horse;

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
                if (c == BISHOP) bishop = ChessPiece(row, col, BISHOP, this);
                if (c == HORSE) horse = ChessPiece(row, col, HORSE, this);
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
        bishop = ChessPiece(oth.bishop.getRow(), oth.bishop.getCol(), BISHOP, this);
        horse = ChessPiece(oth.horse.getRow(), oth.horse.getCol(), HORSE, this);
    };

    ~ChessBoard() {
        delete[] grid;
    }

    char at(int row, int col) const {
        if (row < 0 || col < 0 || row * row_len + col >= size) return INVALID_AT;
        return grid[row * row_len + col];
    };

    int getPawnCnt() const {
        return pawn_cnt;
    }

    int getMaxDepth() const {
        return max_depth;
    }

    ChessPiece &getBishop() {
        return bishop;
    }

    ChessPiece &getHorse() {
        return horse;
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
    static vector<pair<int, int>> for_horse(const ChessBoard &g) {
        g.getPawnCnt();
        return vector<pair<int, int>>();
    };

    static vector<pair<int, int>> for_bishop(const ChessBoard &g) {
        g.getPawnCnt();
        return vector<pair<int, int>>();
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
        for (auto p : NextPossibleMoves::for_horse(g)) {
            ChessBoard cpy = ChessBoard(g);
            cpy.getHorse().move(p.first, p.second);
            bb_dfs(cpy, depth + 1, BISHOP, best, counter);
        }
    } else if (play == BISHOP) {
        for (auto p : NextPossibleMoves::for_bishop(g)) {
            ChessBoard cpy = ChessBoard(g);
            cpy.getBishop().move(p.first, p.second);
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
