#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>
#include <limits>
#include <chrono>
#include <algorithm>
#include <omp.h>
#include "mpi.h"

// chess pieces
#define HORSE  'J'
#define BISHOP 'S'
#define PAWN 'P'
#define EMPTY '-'

// MPI message TAGs
#define TAG_DONE = 0; // work is done, #define result
#define TAG_WORK = 1; // work to be done, #d staring state
#define TAG_FINISHED = 2; // there is no more work to be #define
#define TAG_UPDATE = 3; // update on the best solution found by slave on it's instance


// relative mapping for all possible horse movements
// [ROW, COL]
const int HORSE_CAND[8][2] = {
        {-2, -1},
        {-2, 1},

        {-1, 2},
        {1,  2},

        {2,  1},
        {2,  -1},

        {1,  -2},
        {-1, -2}
};

#define EPOCH_CNT 3

using namespace std;

class ChessBoard {
private:
    char *grid = nullptr;
    int size;
    int row_len;
    int pawn_cnt;
    int min_depth;

    // PDP hint heuristic
    int max_depth;


    void setAt(int row, int col, char value) {
        grid[row * row_len + col] = value;
    }

    class ChessMove {
    private:
        int row;
        int col;
        bool tookPawn;
    public:
        ChessMove(int row, int col, bool tookPawn) : row(row), col(col), tookPawn(tookPawn) {}


        friend ostream &operator<<(ostream &os, const ChessMove &m) {
            os << m.row << "," << m.col;
            if (m.tookPawn) os << " *";
            return os;
        }

    };

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
    vector<ChessMove> move_log;

    void logMovePiece(int row, int col) {
        bool tookPawn = at(row, col) == PAWN;
        move_log.emplace_back(ChessMove(row, col, tookPawn));
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
        min_depth = pawn_cnt;
    };

    ChessBoard(const ChessBoard &oth) {
        grid = new char[oth.size];
        memcpy(grid, oth.grid, oth.size);
        size = oth.size;
        row_len = oth.row_len;
        bishop = oth.bishop;
        horse = oth.horse;
        pawn_cnt = oth.pawn_cnt;
        min_depth = oth.min_depth;
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
        min_depth = oth.min_depth;
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

    const vector<ChessMove> &getMoveLog() const {
        return move_log;
    }

    friend ostream &operator<<(ostream &os, const ChessBoard &g) {
        os << "Délka strany šachovnice: " << g.row_len << endl;
        os << "minimální hloubka: " << g.min_depth << ", maximální hloubka: " << g.max_depth << endl;
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

    int getMinDepth() const {
        return min_depth;
    }
};

class EvalPosition {
public:
    static int for_horse(const ChessBoard &g, int row, int col) {
        // take pawn
        if (g.at(row, col) == PAWN) return 3;

        // take pawn next move
        for (const auto &cand : HORSE_CAND) {
            if (g.at(row + cand[0], col + cand[1]) == PAWN)
                return 2;
        }

        // one square away from pawn
        if (
                g.at(row + 1, col + 1) == PAWN ||
                g.at(row + 1, col - 1) == PAWN ||
                g.at(row + 1, col) == PAWN ||
                g.at(row - 1, col - 1) == PAWN ||
                g.at(row - 1, col + 1) == PAWN ||
                g.at(row - 1, col) == PAWN ||
                g.at(row, col + 1) == PAWN ||
                g.at(row, col - 1) == PAWN
                )
            return 1;

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
        for (const auto &cand : HORSE_CAND) {
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


// return true if there is better board available
bool betterBoardExists(long depth, long best, ChessBoard *g) {
    return
            depth + g->getPawnCnt() >= best || // solution with lower cost already exists
            depth + g->getPawnCnt() > g->getMaxDepth() || // max depth would be reached if each play would remove figure
            best == g->getMinDepth(); // optimum was reached
}

void bb_dfs_seq(ChessBoard *g, long depth, char play, long &best, ChessBoard *bestBoard, long &counter) {
    if (!betterBoardExists(depth, best, g)) {
        if (g->getPawnCnt() == 0) {
#pragma omp critical
            {
                if (!betterBoardExists(depth, best, g)) {
                    best = depth;
                    *bestBoard = *g;
                }
            }
        } else if (play == HORSE) {
            for (const auto &m : NextPossibleMoves::for_horse(*g)) {
                ChessBoard *cpy = new ChessBoard(*g);
                cpy->moveHorse(m.row, m.col);
                bb_dfs_seq(cpy, depth + 1, BISHOP, best, bestBoard, counter);
            }
        } else if (play == BISHOP) {
            for (const auto &m : NextPossibleMoves::for_bishop(*g)) {
                ChessBoard *cpy = new ChessBoard(*g);
                cpy->moveBishop(m.row, m.col);
                bb_dfs_seq(cpy, depth + 1, HORSE, best, bestBoard, counter);
            }
        }
    }
    delete g;
#pragma omp atomic update
    counter++;
}

struct Instance {
    ChessBoard *board;
    int depth;
    char play;

    void freeMem() const {
        delete board;
    }

    Instance(ChessBoard *board, int depth, char play) : board(board), depth(depth), play(play) {}
};

vector<Instance> generateInstances(ChessBoard *initBoard, int initDepth, char initPlay) {
    vector<Instance> instances = vector<Instance>();
    instances.emplace_back(initBoard, initDepth, initPlay);

    for (int i = 0; i < EPOCH_CNT; i++) {
        vector<Instance> instancesNext = vector<Instance>();
        for (const auto &ins : instances) {
            if (ins.play == HORSE) {
                for (const auto &m : NextPossibleMoves::for_horse(*ins.board)) {
                    ChessBoard *cpy = new ChessBoard(*ins.board);
                    cpy->moveHorse(m.row, m.col);
                    instancesNext.emplace_back(cpy, ins.depth + 1, BISHOP);
                }
            } else if (ins.play == BISHOP) {
                for (const auto &m : NextPossibleMoves::for_bishop(*ins.board)) {
                    ChessBoard *cpy = new ChessBoard(*ins.board);
                    cpy->moveBishop(m.row, m.col);
                    instancesNext.emplace_back(cpy, ins.depth + 1, HORSE);
                }
            }
            ins.freeMem();
        }
        instances = instancesNext;
    }
    cout << "Vygenerováno " << instances.size() << " instancí pro počet epoch " << EPOCH_CNT << "." << endl << endl;
    return instances;
}

void bb_dfs_data_par(ChessBoard *g, long &best, ChessBoard *bestBoard, long &counter) {
    vector<Instance> instances = generateInstances(g, 0, BISHOP);
#pragma omp parallel for shared(best, bestBoard, counter, instances) schedule(dynamic) default(none)
    for (unsigned long i = 0; i < instances.size(); i++) {
        bb_dfs_seq(instances[i].board, instances[i].depth, instances[i].play, best, bestBoard, counter);
    }
}

int main(int argc, char **argv) {

    int my_rank;
    int p;

    /* start up MPI */
    MPI_Init(&argc, &argv);

    /* find out process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    /* find out number of processes */
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    cout << my_rank << "/" << p << endl;

    if (p == 0) { // master process
        string filename = argv[0];
        long best = numeric_limits<long>::max();
        ChessBoard bestBoard = ChessBoard(filename);
        long counter = 0;

        cout << bestBoard << endl;
        auto start = chrono::high_resolution_clock::now();
        bb_dfs_data_par(new ChessBoard(filename), best, &bestBoard, counter);
        auto stop = chrono::high_resolution_clock::now();

        cout << "Cena\tPočet volání\tČas [ms]" << endl;
        cout << best << "\t" << counter << "\t\t"
             << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << endl << endl;

        cout << "Tahy" << endl;
        for (const auto &move : bestBoard.getMoveLog()) {
            cout << move << endl;
        }

    } else { // slave process

    }

    MPI_Finalize();
    return 0;
}
