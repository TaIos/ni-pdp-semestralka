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

/**
 * MPI message TAGs
 */
enum MessageTag {
    DONE = 0, // work is done, #define result
    WORK = 1, // work to be done, #d staring state
    FINISHED = 2, // there is no more work to be #define
    UPDATE = 3 // update on the bestPathLen solution found by slave on it's instance
};


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

void ensureBufferSize(char **buf, int current, int needed) {
    if (current >= needed) return;
    delete[] *buf;
    *buf = new char[needed];
}

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

    ChessBoard() = default;

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

    char *serialize(char *buffer, int bufLen, int &outLen) {
        outLen = 10;
        return new char[10];
    }

    static ChessBoard deserialize(char *buf, int bufLen) {
        return ChessBoard();
    }

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

struct Instance {
    ChessBoard board;
    int depth;
    char play;

    Instance(const ChessBoard &board, int depth, char play) : board(board), depth(depth), play(play) {}

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
bool betterBoardExists(Instance *ins, long bestPathLen) {
    return
            ins->depth + ins->board.getPawnCnt() >= bestPathLen || // solution with lower cost already exists
            ins->depth + ins->board.getPawnCnt() > ins->board.getMaxDepth() ||
            // max depth would be reached if each play would remove figure
            bestPathLen == ins->board.getMinDepth(); // optimum was reached
}

void bb_dfs_seq(Instance *ins, ChessBoard &bestBoard, long &bestPathLen, long &counter) {
    if (!betterBoardExists(ins, bestPathLen)) {
        if (ins->board.getPawnCnt() == 0) {
#pragma omp critical
            {
                if (!betterBoardExists(ins, bestPathLen)) {
                    bestPathLen = ins->depth;
                    bestBoard = ins->board;
                }
            }
        } else if (ins->play == HORSE) {
            for (const auto &m : NextPossibleMoves::for_horse(ins->board)) {
                ChessBoard cpy(ins->board);
                cpy.moveHorse(m.row, m.col);
                bb_dfs_seq(new Instance(cpy, ins->depth + 1, BISHOP), bestBoard, bestPathLen, counter);
            }
        } else if (ins->play == BISHOP) {
            for (const auto &m : NextPossibleMoves::for_bishop(ins->board)) {
                ChessBoard cpy(ins->board);
                cpy.moveBishop(m.row, m.col);
                bb_dfs_seq(new Instance(cpy, ins->depth + 1, HORSE), bestBoard, bestPathLen, counter);
            }
        }
    }
    delete ins;
#pragma omp atomic update
    counter++;
}

vector<Instance *> generateInstances(const ChessBoard &initBoard, int initDepth, char initPlay) {
    vector<Instance *> instances = vector<Instance *>();
    instances.emplace_back(new Instance(initBoard, initDepth, initPlay));

    for (int i = 0; i < EPOCH_CNT; i++) {
        vector<Instance *> instancesNext = vector<Instance *>();
        for (const auto &ins : instances) {
            if (ins->play == HORSE) {
                for (const auto &m : NextPossibleMoves::for_horse(ins->board)) {
                    ChessBoard cpy(ins->board);
                    cpy.moveHorse(m.row, m.col);
                    instancesNext.emplace_back(new Instance(cpy, ins->depth + 1, BISHOP));
                }
            } else if (ins->play == BISHOP) {
                for (const auto &m : NextPossibleMoves::for_bishop(ins->board)) {
                    ChessBoard cpy(ins->board);
                    cpy.moveBishop(m.row, m.col);
                    instancesNext.emplace_back(new Instance(cpy, ins->depth + 1, HORSE));
                }
            }
        }
        instances = instancesNext;
    }
    cout << "Vygenerováno " << instances.size() << " instancí pro počet epoch " << EPOCH_CNT << "." << endl << endl;
    return instances;
}

ChessBoard bb_dfs_data_par(const ChessBoard &startBoard, long &bestPathLen, long &counter) {
    vector<Instance *> instances = generateInstances(startBoard, 0, BISHOP);
    ChessBoard bestBoard(startBoard);
#pragma omp parallel for shared(instances, bestBoard, bestPathLen, counter) schedule(dynamic) default(none)
    for (unsigned long i = 0; i < instances.size(); i++) {
        bb_dfs_seq(instances[i], bestBoard, bestPathLen, counter);
    }
    return bestBoard;
}

int main(int argc, char **argv) {

    int bufLen = 1000;
    char *buf = new char[bufLen];
    int my_rank;
    int p;

    /* start up MPI */
    MPI_Init(&argc, &argv);

    /* find out process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    /* find out number of processes */
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    if (p == 0) { // master process
        long bestPathLenGlobal = numeric_limits<long>::max();
        ChessBoard initBoard = ChessBoard(argv[0]);
        vector<Instance *> instances = generateInstances(initBoard, 0, HORSE);
        vector<Instance *>::iterator head = instances.begin();

        // send initial work to each slave
        for (int i = 1; i < p && head != instances.end(); i++, head++) {
//            MPI_Pack(&a, 1, MPI_INT, buffer, LENGTH, &position, MPI_COMM_WORLD);
        }

        /*
        long counter = 0;

        cout << startingBoard << endl;
        auto start = chrono::high_resolution_clock::now();
        bb_dfs_data_par(new ChessBoard(filename), bestPathLenGlobal, &startingBoard, counter);
        auto stop = chrono::high_resolution_clock::now();

        cout << "Cena\tPočet volání\tČas [ms]" << endl;
        cout << bestPathLenGlobal << "\t" << counter << "\t\t"
             << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << endl << endl;

        cout << "Tahy" << endl;
        for (const auto &move : startingBoard.getMoveLog()) {
            cout << move << endl;
        }
         */

    } else { // slave process
        int flag;
        MPI_Status status;
        long bestPathLenSlave = numeric_limits<long>::max();
        long counterSlave = 0;

        while (true) {
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                if (status.MPI_TAG == MessageTag::WORK) {
                    // receive & deserialize message
                    int msgLen;
                    MPI_Get_count(&status, MPI_CHAR, &msgLen);
                    ensureBufferSize(&buf, bufLen, msgLen);
                    MPI_Recv(&buf[0], msgLen, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD,
                             MPI_STATUS_IGNORE);
                    ChessBoard receivedBoard = ChessBoard::deserialize(buf, msgLen);

                    // run
                    ChessBoard bestBoard = bb_dfs_data_par(receivedBoard, bestPathLenSlave, counterSlave);

                    // send result
                    bestBoard.serialize(buf, bufLen, msgLen);
                    MPI_Send(buf, msgLen, MPI_CHAR, 0, MessageTag::DONE, MPI_COMM_WORLD);
                } else if (status.MPI_TAG == MessageTag::FINISHED) {
                    break;
                }
            }
        }
    }

    delete[]buf;
    MPI_Finalize();
    return 0;
}
