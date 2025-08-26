#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

#include "src/DualCube.h"
#include "src/getSysInfo.h"
#include "src/post.h"

#define VERSION "CLI.0.1.0"


// 隐藏控制台光标
void hideCursor() {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(hConsole, &cursorInfo);
}

// 显示控制台光标
void showCursor() {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = TRUE;
	SetConsoleCursorInfo(hConsole, &cursorInfo);
}

// 清除从指定位置到行尾的内容
void clearLine(int y) {
	COORD coord = {0, y};
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		DWORD written;
		FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ',
								  csbi.dwSize.X, coord, &written);
	}
}

// 渲染单个选项
void renderOption(int y, const char* text, BOOL selected) {
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), (COORD){.X = 0, .Y = y});
	clearLine(y);


	if (selected) {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (FOREGROUND_GREEN ));

		printf("> %s", text);
	} else {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED ));
		printf("  %s", text);
	}
}
// 显示带选项的菜单并返回用户选择
int showMenu(const char* title, const char** options, int optionCount) {
    int currentSelection = 0;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    COORD startPos = csbi.dwCursorPosition;

    // 打印标题
    printf("%s\n\n", title);

    // 初始渲染所有选项
    for (int i = 0; i < optionCount; i++) {
        renderOption(startPos.Y + 2 + i, options[i], i == currentSelection);
    }

    // 隐藏光标以获得更好的视觉效果
    hideCursor();

    // 设置控制台输入模式以读取键盘事件
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD originalMode;
    GetConsoleMode(hStdin, &originalMode);
    SetConsoleMode(hStdin, ENABLE_WINDOW_INPUT);

    // 处理用户输入
    BOOL selected = 0;
    INPUT_RECORD inputRecord;
    DWORD eventsRead = 0;

    while (!selected) {
        if (ReadConsoleInput(hStdin, &inputRecord, 1, &eventsRead)) {
            if (inputRecord.EventType == KEY_EVENT && inputRecord.Event.KeyEvent.bKeyDown) {
                WORD keyCode = inputRecord.Event.KeyEvent.wVirtualKeyCode;

                switch (keyCode) {
                    case VK_UP: {
                        // 保存当前选择状态
                        int prevSelection = currentSelection;

                        // 更新选择（循环）
                        currentSelection = (currentSelection - 1 + optionCount) % optionCount;

                        // 重新渲染前一个选项和当前选项
                        renderOption(startPos.Y + 2 + prevSelection, options[prevSelection], FALSE);
                        renderOption(startPos.Y + 2 + currentSelection, options[currentSelection], TRUE);
                        break;
                    }
                    case VK_DOWN: {
                        // 保存当前选择状态
                        int prevSelection = currentSelection;

                        // 更新选择（循环）
                        currentSelection = (currentSelection + 1) % optionCount;

                        // 重新渲染前一个选项和当前选项
                        renderOption(startPos.Y + 2 + prevSelection, options[prevSelection], FALSE);
                        renderOption(startPos.Y + 2 + currentSelection, options[currentSelection], TRUE);
                        break;
                    }
                    case VK_RETURN:
                        selected = true;
                        break;
                    default:
                        // 忽略其他按键
                        break;
                }
            }
        }
    }
    //清除其他选项
    for (int i = 0; i < optionCount; i++) {
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),(COORD){.X=0, .Y=startPos.Y + 2 + i});
        clearLine(startPos.Y + 2 + i);
    }
    renderOption(startPos.Y + 2, options[currentSelection], true);



    // 恢复控制台输入模式
    SetConsoleMode(hStdin, originalMode);

    // 恢复光标显示
    showCursor();
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED));

    return currentSelection;
}


void callback(traversalMsg msg) {

	static char str[256];
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	COORD startPos = csbi.dwCursorPosition;
	switch (msg.type) {
		case TRAVERSAL_MSG_STEP:

			int percentage = msg.data.step[0] / (msg.data.step[1]/ 100);
			clearLine(startPos.Y);
			int width = 50;
			int pos = (width * percentage) / 100;


			printf("\r[");
			for (int i = 0; i < width; ++i) {
				if (i < pos) {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
					printf("=");
				} else if (i == pos) {
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
					printf(">");
				} else {
					printf(" ");
				}
			}
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED));
			printf("] %d%% %d/%d", percentage,msg.data.step[0], msg.data.step[1]);
			fflush(stdout);
			break;
		case TRAVERSAL_MSG_INFO:
			clearLine(startPos.Y);
			printf("%s", msg.data.str);
			fflush(stdout);

			break;
			default:
	}



}

int main(int argc, char *argv[])
{
    SetConsoleOutputCP(65001); // 设置控制台输出编码为UTF-8


	SysInfo info = GetSysInfo();


	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_RED );
	printf("╔══════════════════════════════════════════════════╗\n");
	printf("║ DokiDoki Cube Test                               ║\n");
	printf("║                                                  ║\n");
	printf("║                                                  ║\n");
	printf("║                                                  ║\n");
	printf("║                                                  ║\n");
	printf("║                                                  ║\n");
	printf("╚══════════════════════════════════════════════════╝\n");

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	COORD coord = csbi.dwCursorPosition;

	coord.Y -= 5;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);

	printf("║ Machine ID: %s\n", info.machine_id);
	printf("║ Name      : %s\n", info.hostname);
	printf("║ OS        : %s\n", info.os_name);
	printf("║ CPU: %s\n", info.cpu_brand);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED));
	printf("\nCiao!\n");

	const char* test_names[] = {
		"Single Thread Full Rotate Test",
		"4 Thread Test",
		"Single Thread Semi Rotate Test",
		"Quit"
	};
	const char* confirmOptions[] ={"Yes", "No"};


	int choice = 0;
	do {
		choice = showMenu("\nChoose Item to Test:\n", test_names, sizeof(test_names) / sizeof(char*) );

		if (choice == 3)break;

		hideCursor();
		struct timespec start;
		clock_gettime(CLOCK_REALTIME, &start);

		printf("\nHello Cube\n");

		int nodes = 881798400;
		if (choice == 0) {
			traversalWithProgress(&callback);
		}else if (choice == 1) {
			traversalWithProgressAndParas(&callback, 4, 12);
		}else if (choice == 2) {
			traversalWithProgressAndParas(&callback, 1, 6);
			nodes = nodes / 24;
		}

		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		showCursor();
		long sec = now.tv_sec - start.tv_sec ;
		long nsec = now.tv_nsec - start.tv_nsec ;
		if (nsec < 0)
		{
			sec--;
			nsec += 1000000000;
		}
		printf("\n");
		printf("total time: %lums\n", sec * 1000 + nsec / 1000000);
		printf("Done\n");

		float score = nodes/ (sec*1000000000.0 + nsec)* 1000000.0;
		printf("Score: %.4f\n", score);

		int confirm = showMenu("\nSubmit your result?:\n", confirmOptions, 2);
		if (confirm == 0) {
			char **link = NULL;
			postmark(build_json(
				info.machine_id,
				info.hostname,
				info.os_name,
				info.cpu_brand,
				sec,
				nsec,
				VERSION
			),link);
		}

	}while (choice != 3);

	printf("\nGoobye~\n");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED));



 //
 //    printf("Press enter to exit...\n");
	// getchar();
	return 0;

}
