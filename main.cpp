#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include<vector>
#include "poemblock.h"
#include <windows.h>
#include <locale>
#include <ctime>
#include <chrono>
#include <thread>
#include <atomic>
#include<future>

using namespace std;

atomic<bool> input_received(false);
string user_input;

auto input_thread_func = [&]() {
    getline(cin, user_input);
    input_received = true;
    };


int main() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    setlocale(LC_ALL, "chs");
    srand((unsigned)time(NULL));
    vector<poem> poems = loadpoems("poems.txt");
LOGIN:
    load_users();
    while (true) {
        cout << "== 欢迎 ==\n";
        cout << "1. 注册\n2. 登录\n3. 退出\n请选择：";
        int c; cin >> c;
        if (c == 1) {
            register_user();
        }
        else if (c == 2) {
            if (login_user()) break; 
        }
        else {
            return 0;
        }
    }

    while (true) {
        cout << "\n== 主菜单 ==\n";
        cout << "当前用户：" << current_user << "\n";
        cout << "1. 账户管理\n";
        cout << "2. 查看排名\n";
        cout << "3. 开始游戏\n";
        cout << "4. 退出\n";
        cout << "请选择：";
        int c; cin >> c;
        if (c == 1) {
            cout << "\n-- 账户管理 --\n";
            cout << "1. 修改用户名\n";
            cout << "2. 修改密码\n";
            cout << "3. 注销并删除账户\n";
            cout << "4. 返回\n";
            cout << "请选择：";
            int m; cin >> m;
            if (m == 1) change_username();
            else if (m == 2) change_password();
            else if (m == 3) {
                logout_user();
                goto LOGIN;
            }
        }
        else if (c == 2) {
            cout << "查看排名功能尚未实现。\n";
        }
        else if (c == 3) {
            break; 
        }
        else {
            return 0;
        }
    }

    wcout << L"欢迎来到活字排诗游戏！" << endl;
    cout << "请选择游戏模式：\n1. 单人模式\n2. 多人模式\n输入模式编号：";
    int mode;
    wcin >> mode;
    wcin.ignore();
    if (mode == 1) {
        cout << "当前为单人模式" << endl;
        wcout << L"请选择诗文：\n1. 随机选择\n2. 指定编号" << endl;
    int select_mode = 1;
    wcin >> select_mode;
    cin.ignore();

    int poem_index = 0;
    if (select_mode == 2) {
        wcout << L"可选诗文列表：" << endl;
        for (size_t i = 0; i < poems.size(); ++i) {
            wcout << i << L". " << poems[i].title << endl;
        }

        wcout << L"请输入诗文编号：";
        wcin >> poem_index;
        cin.ignore();
        if (poem_index < 0 || poem_index >= poems.size()) {
            wcout << L"编号无效，将使用默认第一首。" << endl;
            poem_index = 0;
        }
    }
    else {
        poem_index = rand() % poems.size();
        wcout << L"已随机选择《" << poems[poem_index].title << L"》" << endl;
    }

    if (poems.empty()) {
        wcout << L"没有找到有效的诗文数据。" << endl;
        return 0;
    }

    int boardsize = 6;
    wcout << L"请选择活字版边长（6 ~ 9）：";
    wcin >> boardsize;
    cin.ignore();

    if (boardsize < 6 || boardsize > 9) {
        wcout << L"输入无效，使用默认边长 6。" << endl;
        boardsize = 6;
    }

    int minchars = boardsize * boardsize;
    if (poems[poem_index].lines.size() * 6 < minchars) {
        wcout << L"选中的诗文字数不足以填满 " << boardsize << L"x" << boardsize << L" 活字版，自动调整为边长 6。" << endl;
        boardsize = 6;
    }

    wcout << L"规则：" << endl;
    wcout << L"1. 交换两行：row a b (最左端为第0列，最上段同理）" << endl;
    wcout << L"2. 交换两列：col a b" << endl;
    wcout << L"3. 十字交换：cross x1 y1 x2 y2" << endl;
    wcout << L"4. 查看活字版：display" << endl;
    wcout << L"5. 放弃游戏：quit" << endl;
    wcout << L"请尽快复原原文，加油！" << endl;

    int difficulty = 1;
    wcout << L"请选择难度等级（1-简单，2-中等，3-困难）: ";
    wcin >> difficulty;
    cin.ignore();

    int time_limit = 0;
    wcout << L"是否启用限时功能？（0-否，1-是）: ";
    int use_timer = 0;
    wcin >> use_timer;
    if (use_timer == 1) {
        wcout << L"请输入限时秒数（例如180表示3分钟）: ";
        wcin >> time_limit;
    }
    cin.ignore();

    int shuffle_count;
    switch (difficulty) {
    case 1: shuffle_count = 5; break;
    case 2: shuffle_count = 10; break;
    case 3: shuffle_count = 15; break;
    default: shuffle_count = 5; break;
    }
    movabletypeboard board(poems[poem_index], boardsize);
    board.shuffle(shuffle_count);

    atomic<bool> timeout(false);
    future<void> timer_future;

    if (time_limit > 0) {
        timer_future = async(launch::async, [&]() {
            this_thread::sleep_for(chrono::seconds(time_limit));
            timeout = true;
            });
    }

    auto start_time = chrono::steady_clock::now();
    board.display();

    while (true) {
        if (timeout) {
            wcout << L"\n时间已到！游戏失败。" << endl;
            wcout << L"正确答案如下：" << endl;
            vector<movabletypeboard::move> steps = board.getshufflemoves();
            for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
                if (it->type == "row") {
                    wcout << L"交换第 " << it->a << L" 行 和 第 " << it->b << L" 行" << endl;
                }
                else if (it->type == "col") {
                    wcout << L"交换第 " << it->a << L" 列 和 第 " << it->b << L" 列" << endl;
                }
                else if (it->type == "cross") {
                    wcout << L"交换中心点 (" << it->a << L"," << it->b << L") 和 (" << it->c << L"," << it->d << L") 的十字" << endl;
                }
            }
            board.reset();
            board.display();
            auto end_time = chrono::steady_clock::now();
            auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time).count();
            wcout << L"总用时：" << duration << L" 秒。" << endl;
            break;
        }

        wcout << L"请输入指令：";
        input_received = false;
        user_input.clear();
        thread input_thread(input_thread_func);

        // 等待输入或超时
        for (int i = 0; i < 100; ++i) {
            if (input_received) break;
            this_thread::sleep_for(chrono::milliseconds(100));
            if (timeout) {
                if (input_thread.joinable()) {
                    input_thread.detach();
                }
                wcout << L"\n时间已到！游戏失败。" << endl;
                vector<movabletypeboard::move> steps = board.getshufflemoves();
                for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
                    if (it->type == "row") {
                        wcout << L"交换第 " << it->a << L" 行 和 第 " << it->b << L" 行" << endl;
                    }
                    else if (it->type == "col") {
                        wcout << L"交换第 " << it->a << L" 列 和 第 " << it->b << L" 列" << endl;
                    }
                    else if (it->type == "cross") {
                        wcout << L"交换中心点 (" << it->a << L"," << it->b << L") 和 (" << it->c << L"," << it->d << L") 的十字" << endl;
                    }
                }
                board.reset();
                board.display();
                auto end_time = chrono::steady_clock::now();
                auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time).count();
                wcout << L"用时：" << duration << L" 秒。" << endl;
                return 0;
            }
        }

        input_thread.join();

        string cmd = user_input;

        if (cmd == "quit") {
            wcout << L"你选择了放弃，正确答案如下：" << endl;
            vector<movabletypeboard::move> steps = board.getshufflemoves();
            for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
                if (it->type == "row") {
                    wcout << L"交换第 " << it->a << L" 行 和 第 " << it->b << L" 行" << endl;
                }
                else if (it->type == "col") {
                    wcout << L"交换第 " << it->a << L" 列 和 第 " << it->b << L" 列" << endl;
                }
                else if (it->type == "cross") {
                    wcout << L"交换中心点 (" << it->a << L"," << it->b << L") 和 (" << it->c << L"," << it->d << L") 的十字" << endl;
                }
            }
            wcout << L"正确的活字版如下：" << endl;
            board.reset();
            board.display();
            auto end_time = chrono::steady_clock::now();
            auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time).count();
            wcout << L"用时：" << duration << L" 秒。" << endl;
            break;
        }
        else if (cmd == "display") {
            board.display();
        }
        else {
            if (cmd.find("row") == 0) {
                int a, b;
                if (sscanf(cmd.c_str(), "row %d %d", &a, &b) == 2) {
                    if (board.swaprows(a, b)) {
                        wcout << L"成功交换第 " << a << L" 行和第 " << b << L" 行！" << endl;
                        board.display();
                    }
                    else {
                        wcout << L"行交换失败，请检查输入。" << endl;
                    }
                }
                else {
                    wcout << L"指令格式错误，应为：row a b" << endl;
                }
            }
            else if (cmd.find("col") == 0) {
                int a, b;
                if (sscanf(cmd.c_str(), "col %d %d", &a, &b) == 2) {
                    if (board.swapcols(a, b)) {
                        wcout << L"成功交换第 " << a << L" 列和第 " << b << L" 列！" << endl;
                        board.display();
                    }
                    else {
                        wcout << L"列交换失败，请检查输入。" << endl;
                    }
                }
                else {
                    wcout << L"指令格式错误，应为：col a b" << endl;
                }
            }
            else if (cmd.find("cross") == 0) {
                int x1, y1, x2, y2;
                if (sscanf(cmd.c_str(), "cross %d %d %d %d", &x1, &y1, &x2, &y2) == 4) {
                    if (board.swapcross(x1, y1, x2, y2)) {
                        wcout << L"成功进行十字交换！" << endl;
                        board.display();
                    }
                    else {
                        wcout << L"十字交换失败，请检查中心点是否正确。" << endl;
                    }
                }
                else {
                    wcout << L"指令格式错误，应为：cross x1 y1 x2 y2" << endl;
                }
            }
            else {
                wcout << L"无效指令，请重新输入。" << endl;
            }
        }

        if (board.issolved()) {
            wcout << L"恭喜你，成功复原整首诗了！" << endl;
            auto end_time = chrono::steady_clock::now();
            auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time).count();
            wcout << L"用时：" << duration << L" 秒。" << endl;
            break;
        }
    }

    }

    else if (mode == 2) {
        cout << "当前为多人模式，总限时为20分钟" << endl;
        vector<int> sizes = { 6, 7, 8, 9 };
        int total_limit = 1200;  
        int used_time = 0;
        int total_completed = 0;
        auto total_start = chrono::steady_clock::now(); 

        for (int player = 0; player < 4; ++player) {
            auto now = chrono::steady_clock::now();
            int elapsed = chrono::duration_cast<chrono::seconds>(now - total_start).count();
            bool player_quit = false;
            auto player_start = chrono::steady_clock::now();
            if (elapsed >= total_limit) {
                wcout << L"\n总时间已用完，游戏结束！" << endl;
                break;
            }

            input_received = false;
            user_input.clear();
            cout << "\n===== 玩家 " << (player + 1)
                << " 开始游戏（" << sizes[player] << "x"
                << sizes[player] << "） =====\n";
            wcout << L"剩余时间：" << (total_limit - elapsed) << L"秒" << endl;

            auto start = chrono::steady_clock::now();
            wcout << L"请选择诗文：\n1. 随机选择\n2. 指定编号" << endl;
            int select_mode = 1;
            wcin >> select_mode;
            wcin.ignore();

            int poem_index = 0;
            if (select_mode == 2) {
                wcout << L"可选诗文列表：" << endl;
                for (size_t j = 0; j < poems.size(); ++j) {
                    wcout << j << L". " << poems[j].title << endl;
                }

                wcout << L"请输入诗文编号：";
                wcin >> poem_index;
                wcin.ignore();
                if (poem_index < 0 || poem_index >= poems.size()) {
                    wcout << L"编号无效，将使用默认第一首。" << endl;
                    poem_index = 0;
                }
            }
            else {
                poem_index = rand() % poems.size();
                wcout << L"已随机选择《" << poems[poem_index].title << L"》" << endl;
            }

            if (poems.empty()) {
                wcout << L"没有找到有效的诗文数据。" << endl;
                return 0;
            }

            int boardsize = sizes[player];
            int minchars = boardsize * boardsize;

            wcout << L"规则：" << endl;
            wcout << L"1. 交换两行：row a b (最左端为第0列，最上段同理）" << endl;
            wcout << L"2. 交换两列：col a b" << endl;
            wcout << L"3. 十字交换：cross x1 y1 x2 y2" << endl;
            wcout << L"4. 查看活字版：display" << endl;
            wcout << L"5. 放弃游戏：quit" << endl;
            wcout << L"请尽快复原原文，加油！" << endl;

            int difficulty = 1;
            wcout << L"请选择难度等级（1-简单，2-中等，3-困难）: ";
            wcin >> difficulty;
            wcin.ignore(1000, L'\n');

            int shuffle_count;
            switch (difficulty) {
            case 1: shuffle_count = 5; break;
            case 2: shuffle_count = 10; break;
            case 3: shuffle_count = 15; break;
            default: shuffle_count = 5; break;
            }
            movabletypeboard board(poems[poem_index], boardsize);
            board.shuffle(shuffle_count);

            auto start_time = chrono::steady_clock::now();
            board.display();
            bool round_finished = false;

            while (!round_finished) {
                now = chrono::steady_clock::now();
                elapsed = chrono::duration_cast<chrono::seconds>(now - total_start).count();
                if (elapsed >= total_limit) {
                    wcout << L"\n总时间已用完，游戏结束！" << endl;
                    break;
                }

                wcout << L"请输入指令：";
                input_received = false;
                user_input.clear();
                thread input_thread(input_thread_func);

                for (int j = 0; j < 100; ++j) {
                    if (input_received) break;
                    this_thread::sleep_for(chrono::milliseconds(100));

                    now = chrono::steady_clock::now();
                    elapsed = chrono::duration_cast<chrono::seconds>(now - total_start).count();
                    if (elapsed >= total_limit) break;
                }

                if (input_thread.joinable()) {
                    input_thread.detach();
                }

                string cmd = user_input;

                if (cmd == "quit") {
                    player_quit = true;
                    wcout << L"你选择了放弃，正确答案如下：" << endl;
                    vector<movabletypeboard::move> steps = board.getshufflemoves();
                    for (auto it = steps.rbegin(); it != steps.rend(); ++it) {
                        if (it->type == "row") {
                            wcout << L"交换第 " << it->a << L" 行 和 第 " << it->b << L" 行" << endl;
                        }
                        else if (it->type == "col") {
                            wcout << L"交换第 " << it->a << L" 列 和 第 " << it->b << L" 列" << endl;
                        }
                        else if (it->type == "cross") {
                            wcout << L"交换中心点 (" << it->a << L"," << it->b << L") 和 ("
                                << it->c << L"," << it->d << L") 的十字" << endl;
                        }
                    }
                    board.reset();
                    board.display();
                    round_finished = true;
                }
                else if (cmd == "display") {
                    board.display();
                }
                else if (cmd.find("row") == 0) {
                    int a, b;
                    if (sscanf(cmd.c_str(), "row %d %d", &a, &b) == 2) {
                        if (board.swaprows(a, b)) {
                            wcout << L"成功交换第 " << a << L" 行和第 " << b << L" 行！" << endl;
                            board.display();
                        }
                        else {
                            wcout << L"行交换失败，请检查输入。" << endl;
                        }
                    }
                    else {
                        wcout << L"指令格式错误，应为：row a b" << endl;
                    }
                }
                else if (cmd.find("col") == 0) {
                    int a, b;
                    if (sscanf(cmd.c_str(), "col %d %d", &a, &b) == 2) {
                        if (board.swapcols(a, b)) {
                            wcout << L"成功交换第 " << a << L" 列和第 " << b << L" 列！" << endl;
                            board.display();
                        }
                        else {
                            wcout << L"列交换失败，请检查输入。" << endl;
                        }
                    }
                    else {
                        wcout << L"指令格式错误，应为：col a b" << endl;
                    }
                }
                else if (cmd.find("cross") == 0) {
                    int x1, y1, x2, y2;
                    if (sscanf(cmd.c_str(), "cross %d %d %d %d", &x1, &y1, &x2, &y2) == 4) {
                        if (board.swapcross(x1, y1, x2, y2)) {
                            wcout << L"成功进行十字交换！" << endl;
                            board.display();
                        }
                        else {
                            wcout << L"十字交换失败，请检查中心点是否正确。" << endl;
                        }
                    }
                    else {
                        wcout << L"指令格式错误，应为：cross x1 y1 x2 y2" << endl;
                    }
                }
                else {
                    wcout << L"无效指令，请重新输入。" << endl;
                }

                if (!player_quit && board.issolved()) {
                    auto end = chrono::steady_clock::now();
                    auto duration = chrono::duration_cast<chrono::seconds>(end - player_start).count();
                    wcout << L"恭喜玩家 " << (player + 1) << L" 复原成功！用时 " << duration << L" 秒。" << endl;
                    total_completed++; 
                    round_finished = true;

                    used_time += duration; 
                }
            }

            auto end_round = chrono::steady_clock::now();
            used_time = chrono::duration_cast<chrono::seconds>(end_round - total_start).count();
        }

    wcout << L"\n==== 所有玩家已完成 ====" << endl;
    wcout << L"总共完成题数：" << total_completed << L" / 4" << endl;
    wcout << L"总用时：" << used_time << L" 秒。" << endl;
    }


    return 0;
}
