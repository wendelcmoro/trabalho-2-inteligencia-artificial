#include <bits/stdc++.h>
#include "tabuleiro.h"
#include <unistd.h>

#define MAXSTR 512
#define UP 'u'
#define DOWN 'd'
#define LEFT 'l'
#define RIGHT 'r'
#define DEPTH_LIMIT 6

using namespace std;
using matrix = vector<vector<char>>;

char buf[MAXSTR];
char player;
char board[7+4][7+4];
bool global_wait = false;

struct movement {
    char type;
    pair<int, int> initial;
    vector<pair<int, int>> next;
};

struct game_action {
    matrix board;
    movement mov;
    int depth;
    bool expanded;
};

vector<vector<int>> values(100);
vector<pair<movement, int>> next_move;

// 
// Printers

// print current set board
void print_board() {
    cout << "Board: " << endl;
    for (int i = 2; i < 9; i++) {
        for (int j = 2; j < 9; j++) {
            cout << board[i][j];
        }
        cout << endl;
    }
    cout << endl;
}

// print a state in search
void print_game_action(game_action &game) {
    cout << "---  ACTION  ---" << endl;
    cout << "Board: " << endl;
    cout << "  2345678" << endl;
    for (int i = 2; i < 9; i++) {
        cout << i << " ";
        for (int j = 2; j < 9; j++) {
            cout << game.board[i][j];
        }
        cout << endl;
    }
    
    cout << "Expanded: " << game.expanded << endl;
    cout << "Depth: " << game.depth << endl;
    cout << "Start: (" << game.mov.initial.first << ", " << game.mov.initial.second << ")" << endl;
    cout << "Movements: ";
    for (auto pos : game.mov.next) {
        cout << " (" << pos.first << "," << pos.second << ") ";
    }
    cout << endl;
    cout << "--- END ACTION ---" << endl;
}

// print a movement
void print_movement(movement &moves) {
    cout << "start: " << moves.initial.first << ' ' << moves.initial.second << endl;
    for (unsigned int i = 0; i < moves.next.size(); i++) {
        cout << "goes to: " << moves.next[i].first << ' ' << moves.next[i].second << endl;
    }
}

// find fox position
pair<int, int> find_fox() {
    pair<int, int> startPos;    
    for (int i = 2; i < 9; i++) {
        for (int j = 2; j < 9; j++) {
            if (board[i][j] == 'r') {
                startPos = make_pair(i, j);
            }
        }
    }
    return startPos;
}

// find all goose positions
vector<pair<int, int>> find_geese() {
    vector<pair<int, int>> pos;
    for (int i = 2; i < 9; i++) {
        for (int j = 2; j < 9; j++) {
            if (board[i][j] == 'g') {
                pos.push_back(make_pair(i, j));
            }
        }
    }
    return pos;
}

// check if game ended
int check_end (char type) {
    int goose = 0;
    for (int i = 2; i < 9; i++) {
        for (int j = 2; j < 9; j++) {
            if (board[i][j] == 'g') {
                goose++;
            }
        }
    }

    pair<int, int> fox = find_fox();
    bool walled = (
        (board[fox.first-1][fox.second] == 'g' || board[fox.first-1][fox.second] == '#') &&
        (board[fox.first+1][fox.second] == 'g' || board[fox.first+1][fox.second] == '#') &&
        (board[fox.first][fox.second-1] == 'g' || board[fox.first][fox.second-1] == '#') &&
        (board[fox.first][fox.second+1] == 'g' || board[fox.first][fox.second+1] == '#')
    );

    if (goose < 4) {
        return type == 'r' ? 1 : -1;
    } else if (walled) {
        return type == 'r' ? -1 : 1;
    }
    return 0;
}

// Check if given position is free
bool is_free(int row, int col) {
    return board[row][col] != 'g' && board[row][col] != 'r' && board[row][col] != '#' && board[row][col] != ' ';
}

// analyze and create move
void analyze_move(int i, int j, int next_i, int next_j, vector<movement> &moves) {
    if (is_free(next_i, next_j)) {
        movement move;
        move.type = 'm';
        move.initial = make_pair(i, j);
        move.next = vector<pair<int, int>>();
        move.next.push_back(make_pair(next_i, next_j));

        moves.push_back(move);
    }
}

// given a start and end position, check if the fox can jump to the dir direction
bool fox_can_jump(int row, int col, char dir) {
    if (dir == DOWN) {
        return board[row + 2][col] == '-' && board[row + 1][col] == 'g';
    } else if (dir == UP) {
        return board[row - 2][col] == '-' && board[row - 1][col] == 'g';
    } else if (dir == RIGHT) {
        return board[row][col + 2] == '-' && board[row][col + 1] == 'g';
    } else if (dir == LEFT) {
        return board[row][col - 2] == '-' && board[row][col - 1] == 'g';
    }

    return false;
}

// Get all possible jumps from a given position
void analyze_jumps(int initial_row, int initial_col, int row, int col, vector<pair<int, int>> &path, vector<movement> &moves) {
    // create movement
    if (initial_row != row || initial_col != col) {
        movement move;
        move.type = 's';
        move.initial = make_pair(initial_row, initial_col); 
        move.next = vector<pair<int, int>>(path);
        moves.push_back(move);
    }

    // next jump
    char directions[4] = {RIGHT, LEFT, UP, DOWN};
    vector<pair<int, int>> diff = {{0, 1}, {0, -1}, {-1, 0}, {1, 0}};
    for (int i = 0; i < 4; i++) {
        char dir = directions[i];
        int next_row = row + diff[i].first*2;
        int next_col = col + diff[i].second*2;

        if (fox_can_jump(row, col, dir)) {
            path.push_back(make_pair(next_row, next_col));
            board[row + diff[i].first][col + diff[i].second] = '-';

            analyze_jumps(initial_row, initial_col, next_row, next_col, path, moves);

            board[row + diff[i].first][col + diff[i].second] = 'g';
            path.pop_back();
        }
    }
}

// Get the list of all movements for a certain entity
vector<movement> get_moves(int row, int col) {
    char type = board[row][col];

    vector<movement> moves;

    analyze_move(row, col, row-1, col, moves);
    analyze_move(row, col, row+1, col, moves);
    analyze_move(row, col, row, col-1, moves);
    analyze_move(row, col, row, col+1, moves);

    vector<pair<int, int>> path = {{row, col}};
    if (type == 'r') {
        analyze_jumps(row, col, row, col, path, moves);
    }

    return moves;
}

// Save the current global board to a new variable
matrix board_to_matrix() {
    matrix conv(11);
    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < 11; j++) {
            conv[i].push_back(board[i][j]);
        }
    }

    return conv;
}

// Apply a movement in a board
matrix apply_moves (matrix table, movement &move) {
    if (move.next.size() == 0) {
        return table;
    }

    auto prev = move.initial;
    char who = table[prev.first][prev.second];

    if (move.type == 'm') {
        auto next = move.next[0];
        
        table[prev.first][prev.second] = '-';
        table[next.first][next.second] = who;
    } else {
        for (auto next : move.next) {
            auto goose = make_pair((prev.first + next.first)/2, (prev.second + next.second)/2);

            table[prev.first][prev.second] = '-';
            table[goose.first][goose.second] = '-';
            table[next.first][next.second] = 'r';

            prev = next;
        }
    }

    return table;
}

// Set a saved board to the current global board
void matrix_to_board(matrix &table) {
    for (int i = 2; i < 9; i++) {
        for (int j = 2; j < 9; j++) {
            board[i][j] = table[i][j];
        }
    }
}

int heuristic_fox (game_action &action) {
    auto [row, col] = find_fox();

    // Amount of goose
    int count_goose = 0;
    for (int i = 2; i < 9; i++) {
        for (int j = 2; j < 9; j++) {
            if (board[i][j] == 'g') {
                count_goose++;
            }
        }
    }
    return count_goose;
}

int heuristic_goose (game_action &action) {
    auto [row, col] = find_fox();

    // Amount of goose
    int count_goose = 0;
    for (int i = 2; i < 9; i++) {
        for (int j = 2; j < 9; j++) {
            if (board[i][j] == 'g') {
                count_goose++;
            }
        }
    }

    int blocked = 0;
    blocked += (int) (!is_free(row-1, col) && !fox_can_jump(row-1, col, UP));
    blocked += (int) (!is_free(row+1, col) && !fox_can_jump(row+1, col, DOWN));
    blocked += (int) (!is_free(row, col-1) && !fox_can_jump(row, col-1, LEFT));
    blocked += (int) (!is_free(row, col+1) && !fox_can_jump(row, col+1, RIGHT));

    int goose_points = count_goose*4;
    for (int i = 2; i < 9; i++) {
        for (int j = 2; j < 9; j++) {
            if (board[i][j] == 'g') {
                goose_points -= (int) (is_free(i-1, j));
                goose_points -= (int) (is_free(i+1, j));
                goose_points -= (int) (is_free(i, j-1));
                goose_points -= (int) (is_free(i, j+1));
            }
        }
    }

    return count_goose + blocked*4 + goose_points;
}

// Execute heuristic
int run_heuristic (game_action &action) {
    if (player == 'r') {
        return heuristic_fox(action);
    } 
    return heuristic_goose(action);
}

// Compute minimax value to given state
int get_minimax_value (game_action &action) {
    bool is_min = false;
    int depth = action.depth+1;

    if (action.depth % 2 == 0) {
        is_min = true;
    }

    int ret_value = is_min ? INT_MAX : INT_MIN;
    if (is_min) {
        for (int val : values[depth]) {
            if (val < ret_value) {
                ret_value = val;
            }
        }
    } else {
        for (int val : values[depth]) {
            if (val > ret_value) {
                ret_value = val;
            }
        }
    }

    values[depth].clear();

    return ret_value;
}

// algoritm minimax
movement minimax () {
    bool start_fox = player == 'r';

    // value
    stack<game_action> boards;
    game_action action;
    action.board = board_to_matrix();
    action.depth = 0;
    action.expanded = false;
    boards.push(action);

    // search states with dfs
    while (!boards.empty()) {
        // get a board
        action = boards.top();
        boards.pop();

        char actual_player;
        if ((action.depth % 2 == 0 && start_fox) || (action.depth % 2 == 1 && !start_fox)) {
            actual_player = 'r';
        } else {
            actual_player = 'g';
        }

        if (global_wait) {
            sleep(1);
        }

        if (action.depth == DEPTH_LIMIT) {
            // if reached depth limit, run heuristic
            int value = run_heuristic(action);
            values[action.depth].push_back(value);

        } else if (action.expanded) {
            // if was expanded before, get minimax value to this node
            int value = get_minimax_value(action);
            values[action.depth].push_back(value);

            // if depth is 1, save movement
            if (action.depth == 1) {
                next_move.push_back(make_pair(action.mov, value));
            }

        } else {

            // set expanded
            action.expanded = true;
            boards.push(action);

            matrix_to_board(action.board);

            // get all entity positions
            vector<pair<int, int>> positions;
            if (actual_player == 'r') {
                positions = vector<pair<int, int>>();
                positions.push_back(find_fox());
            } else {
                positions = find_geese();
            }

            // for each position
            for (pair<int, int> pos : positions) {

                // for each movement
                vector<movement> moves = get_moves(pos.first, pos.second);
                for (movement mov : moves) {

                    // create new possible action
                    game_action new_action;
                    new_action.board = apply_moves(board_to_matrix(), mov);
                    new_action.mov = mov;
                    new_action.depth = action.depth + 1;
                    new_action.expanded = false;
                    boards.push(new_action);
                }
            }
        }
    }

    // get next movement
    movement ret;
    for (auto next : next_move) {
        if (next.second == values[0][0]) {
            ret = next.first;
        }
    }
    return ret;
}

// throw the board to a 2D char array instead of an array of char
// remove all other characters that are not part of the game board
void adapt_board() {
    int start = 0;
    for (start = 0; start < MAXSTR; start++) {
        if (buf[start] == '#') {
            break;
        }
    }

    for (int i = 1; i < 10; i++) {
        for (int j = 1; j < 10; j++) {
            board[i][j] = buf[(i-1)*10 + (j-1) + start];
        }
    } 
}

// Convert choose movement to char *
void movement_to_str(movement move, char *buf) {
    auto [row, col] = move.initial;
    
    stringstream ss;
    ss << "" << player << ' ' << move.type << ' ';
    
    // Fix later
    if (move.type == 's') {
        ss << move.next.size() << ' ';
    }
    ss << (row-1) << ' ' << (col-1) << ' ';

    // Fix later
    size_t i = 0; 
    if (move.type == 's') {
        i++;
    }

    for (; i < move.next.size(); i++) {
        auto [a, b] = move.next[i];
        ss << (a-1) << ' ' << (b-1);
        if (i+1 != move.next.size()) {
            ss << ' ';
        }
    }

    string output = ss.str();

    cout << "Move chosen: " << output << endl;

    memset(buf, '\0', MAXSTR);
    memcpy(buf, output.c_str(), output.size());
}

// 
// main
int main(int argc, char **argv) {
    memset(&board, '#', 11*11);

    tabuleiro_conecta(argc, argv);

    player = *argv[1];
    cout << "Playing with " << (player == 'r' ? "fox" : "goose") << endl;

    while (1) {
        // Receive board
        tabuleiro_recebe(buf);
        
        cout << "Board received" << endl;

        // Convert *char to char[][]
        adapt_board();

        if (check_end(player) != 0) {
            cout << "Game ended" << endl;
            exit(0);
        }

        // Run minimax
        movement mov = minimax();
        
        for (int i = 0; i < 100; i++) {
            values[i].clear();
        }
        next_move.clear();

        // Fix later, bad sintax
        movement_to_str(mov, buf);

        // Send movement
        tabuleiro_envia(buf);
    }
}