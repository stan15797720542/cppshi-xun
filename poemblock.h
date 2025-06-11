#ifndef POEMBLOCK_H
#define POEMBLOCK_H
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <set>
#include <chrono>
#include <random> 

#include <windows.h>
#include <graphics.h>

using namespace std;

// --- 全局常量 ---
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// --- 界面状态枚举  ---
enum class AppState {
    LOGIN,
    REGISTER,
    MAIN_MENU,
    ACCOUNT_MANAGE,
    RANKING,
    POEM_LIB_MENU,
    VIEW_POEMS,
    ADD_POEM,
    DELETE_POEM,
    GAME_MODE_SELECT,
    SINGLE_PLAYER_SETUP,
    ADVENTURE_MENU,
    ADVENTURE_CHAPTER_VIEW,
    MULTIPLAYER_SETUP,
    MULTIPLAYER_SUMMARY,
    EXIT
};


struct Button {
    int x, y, width, height;
    wstring text;
    COLORREF color;
    bool isVisible = true;
    int fontSize = 0; 
};

struct User {
    string password;
    map<int, int> best_single = { {1, 0}, {2, 0}, {3, 0} };
    map<int, int> best_multi = { {1, 0}, {2, 0}, {3, 0} };
};

struct adventureprogress {
    int libai_level = 0, dufu_level = 0, sushi_level = 0, zhuzibaijia_level = 0;
    int libai_score = 0, dufu_score = 0, sushi_score = 0, zhuzibaijia_score = 0;
    map<string, vector<int>> best_scores;
};

struct poem {
    wstring title;
    vector<wstring> lines;
    string original_content;
    poem(const wstring& t = L"", const vector<wstring>& l = {}, const string& c = "");
};

struct characterblock {
    wchar_t character;
    characterblock(wchar_t c = L' ');
};

// --- 活字板类  ---
class movabletypeboard {
public:
    struct move {
        string type; int a, b, c, d;
        move(const string& t, int a, int b, int c = 0, int d = 0);
    };

    movabletypeboard(const poem& p, int boardsize);
    void shuffle(int count);
    bool swaprows(int r1, int r2);
    bool swapcols(int c1, int c2);
	bool swapcross(int r1, int c1, int r2, int c2);
    bool issolved();
    void reset();
    void display(int grid_x, int grid_y, int cell_size, const vector<pair<int, int>>& selections);
    void displayOriginal(int grid_x, int grid_y, int cell_size);
    int getsize() const { return size; }
    vector<move> get_shuffle_moves() const { return shuffle_moves; }

private:
    int size;
    vector<vector<characterblock>> grid;
    vector<characterblock> originalorder;
    vector<move> shuffle_moves;
    bool isvalidcrosscenter(int x, int y);
};

// --- 全局变量声明 ---
extern map<wstring, User> users;
extern map<wstring, adventureprogress> user_progress;
extern wstring current_user;
extern AppState currentState;

// 工具函数
wstring s2ws(const string& str);
string ws2s(const wstring& wstr);

// 数据 I/O
void load_users();
void save_users();
void load_progress();
void save_progress();
vector<poem> loadpoems_gui(const string& filename);
void add_poem_to_file_w(const string& filename, const wstring& title, const wstring& content);
void delete_poem_from_file_w(const string& filename, const wstring& title_to_delete);
void update_ranking_single(const wstring& username, int difficulty, long long time_used);
int get_shuffle_count_by_difficulty(int difficulty);
int get_shuffle_count(int level);
int calculate_score(int steps, long long time_used, int shuffle_count, int board_size);
int get_unlock_requirement(const string& chapter, int current_poems_count);
bool is_chapter_unlocked(const string& chapter, const adventureprogress& prog);

// UI 核心
void drawButton(const Button& btn);
bool isMouseOnButton(const MOUSEMSG& msg, const Button& btn);

// 各界面运行函数
void runLoginScreen();
void runRegisterScreen();
void runMainMenu();
void runAccountManageScreen();
void runRankingScreen();
void runPoemLibMenu();
void runViewPoemsScreen(const string& filename);
void runAddPoemScreen(const string& filename);
void runDeletePoemScreen(const string& filename);
void runChapterSelectScreen(AppState intended_action);
void runGameModeSelectScreen();
void runSinglePlayerSetup();
void runAdventureMenuScreen();
void runAdventureChapterScreen(const string& chapter_id, const wstring& chapter_name, int board_size);

// 游戏主循环函数
void playSinglePlayerGame(const poem& p, int difficulty, int boardsize, bool timer_enabled, int time_limit_seconds);
void playAdventureGame(const poem& p, const string& chapter_id, int level, int boardsize);
void update_ranking_multi(const wstring& username, int difficulty, int time_used);
void playSinglePlayerGame(const poem& p, int difficulty, int boardsize, bool timer_enabled, int time_limit_seconds);
// 多人游戏界面
void runMultiplayerSetup();
void runMultiplayerSummaryScreen(int rounds_completed, long long total_time_used);

// 多人游戏核心流程
void playMultiplayerGame(int difficulty);
bool playSingleMultiplayerRound(int round_num, int board_size, chrono::steady_clock::time_point total_start_time, int difficulty);
#endif 
