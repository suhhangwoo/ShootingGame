#pragma comment (lib, "winmm.lib")
#include <windows.h>
#include <mmsystem.h>
#include <Digitalv.h>
#include <stdio.h>
#include "resource.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
HWND hWndMain;
LPCTSTR lpszClass = TEXT("ShootingGame");

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 516, 885, NULL,
		(HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	hWndMain = hWnd;

	while (GetMessage(&Message, NULL, 0, 0)) 
	{
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return (int)Message.wParam;
}

#define _USE_MATH_DEFINES
#include <math.h>

void PlayerMove(void);
void EnemyMove(void);
void BulletMove(void);
void BossMove(void);
void InitAimingBullet(float mx, float my, float ex, float ey, float speed, float& x, float& y, float& vx, float& vy);
void InitDirectedBullet(float ex, float ey, float& x, float& y,	float& vx, float& vy, float speed,float theta);
void MoveBullet(float& x, float& y, float vx, float vy);
void InitNWayBullets(float vx0, float vy0, float theta,	int n, float vx[], float vy[]);
void MoveLoopingBullet(float& x, float& y, float& vx, float& vy, float cx, float cy, float& r, float vr, float& theta, float omega);
BOOL CheckCollision(float x0, float y0, float x1, float y1, float mx0, float my0, float mx1, float my1);
void DrawHpBar(HDC hdc, int x, int y, HBITMAP hBit);
void TransBlt(HDC hdc, int x, int y, HBITMAP hbitmap, COLORREF clrMask);

struct BulletState
{
	float x = 0, y = 0;
	float vx = 0, vy = 0;
	float r = 5, theta = 0;
	BOOL isAble = FALSE;
};
struct CircleBulletState
{
	float x = 0, y = 0;
	float vx = 0, vy = 0;
	BOOL isAble = FALSE;
	BulletState childBullet[11];
};
struct EnemyState
{
	int x = 0, y = 0;
	int shootTime = 0;
	int kind = 0;
	int animeCount = 6;
	BOOL isAble = FALSE;
};
struct PlayerState
{
	int x = 252, y = 700;
	int delay = 0;
	int life = 5;
};
struct BossState
{
	int x = 252, y = -50;
	int life = 100;
	int state = 0;
	int animeCount = 11;
	int attackCount = 0;
};
struct Sound
{
	MCI_OPEN_PARMS openBgm;
	MCI_PLAY_PARMS playBgm;
	MCI_SEEK_PARMS SeekBgm;
	int dwID;
};

BulletState playerBullet[20];
CircleBulletState circleBullet[10];
BulletState enemyBullet[200];
EnemyState enemy[10];
Sound sound[6];
PlayerState player;
BossState boss;
HDC hMemDC;
HBITMAP hPlayer, hShot, hShotoval, hBackground, hBackBit, hOldBitmap;
HBITMAP hUI[5];
HBITMAP hEnemy[2];
HBITMAP hBoss, hHpBar;
HBITMAP hNum[10];
HBITMAP hFlame[6];
HBITMAP hBoom[11];
RECT crt, wrt;
enum { FIGHTER, BOARDER };
enum { MAIN, PLAY, GAMEOVER };
enum { BOOM = -1, WAIT, MOVE, ATTACK };
enum { ODD_NWAY, EVEN_NWAY, CIRCLE, IDLE };
enum { EXPLO_BOSS, EXPLO_ENEMY, EXPLO_PLAYER, SHOT_ENEMY, SHOT_PLAYER, HIT_BOSS };
const char* strArr[6] = { "explo_boss.wav", "explo_enemy.wav", "explo_player.wav", "shot_enemy.wav", "shot_player.wav", "hit_boss.wav" };
int score, backgroundY = 50, count = 0, gameState = MAIN;
int playerBullet_index = 0, enemy_index = 0, enemyBullet_index = 0, circleBullet_index = 0;
int attackType = IDLE, nwayCount = 0, nwayNum = 5;

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	HDC hdc, hMemDC2;
	PAINTSTRUCT ps;
	int h, scoreX = 350, temp = 10000000, oldScore = score;

	switch (iMessage) 
	{
	case WM_CREATE:
		hPlayer = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PLAYER));
		hShot = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SHOT));
		hShotoval = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_SHOTOVAL));
		hBackground = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BACKGROUND));
		hBoss = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BOSS));
		hHpBar = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_HPBAR));

		for (int i = 0; i < 2; i++)
			hEnemy[i] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_FIGHTER + i));
		for (int i = 0; i < 5; i++)
			hUI[i] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_LIFE + i));
		for (int i = 0; i < 10; i++)
			hNum[i] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_NUM0 + i));
		for (int i = 0; i < 6; i++)
			hFlame[i] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_FLAME1 + i));
		for (int i = 0; i < 11; i++)
			hBoom[i] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BOOM1 + i));

		for (int i = 0; i < 6; i++)
		{
			sound[i].openBgm.lpstrDeviceType = (LPCSTR)"waveaudio";
			sound[i].openBgm.lpstrElementName = (LPCSTR)strArr[i];
			mciSendCommand(0, MCI_OPEN, MCI_OPEN_ELEMENT | MCI_OPEN_TYPE, (DWORD)(LPVOID)&sound[i].openBgm);
			sound[i].dwID = sound[i].openBgm.wDeviceID;
		}
		PlaySound((LPCSTR)"bgm.wav", NULL, SND_ASYNC | SND_LOOP);

		hdc = GetDC(hWnd);
		hMemDC = CreateCompatibleDC(hdc);
		GetClientRect(hWnd, &crt);
		hBackBit = CreateCompatibleBitmap(hdc, crt.right, crt.bottom);
		(HBITMAP)SelectObject(hMemDC, hBackBit);
		PatBlt(hMemDC, 0, 0, crt.right, crt.bottom, WHITENESS);
		ReleaseDC(hWnd, hdc);

		SetRect(&wrt, crt.left - 30, crt.top - 30, crt.right + 30, crt.bottom + 30);

		srand(GetTickCount64());

		SetTimer(hWnd, 0, 20, NULL);
		return 0;
	case WM_TIMER:
		switch (wParam)
		{
		case 0:
			backgroundY -= 3;
			if (player.delay < 10)
				player.delay++;
			if (backgroundY < 0)
				backgroundY = 1500;

			if (gameState == PLAY)
			{
				if (boss.state != WAIT)
					BossMove();
				PlayerMove();
				EnemyMove();
				BulletMove();
			}			
			
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case 1:
			KillTimer(hWnd, 1);
			break;
		}
		return 0;
	case WM_LBUTTONDOWN:
		switch (gameState)
		{
		case MAIN:
			gameState = PLAY;
			break;
		case GAMEOVER:
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);

		// 배경 그림
		hMemDC2 = CreateCompatibleDC(hdc);
		SelectObject(hMemDC2, hBackground);
		h = min(1500 - backgroundY, crt.bottom);
		BitBlt(hMemDC, 0, 0, crt.right, h, hMemDC2, 0, backgroundY, SRCCOPY);
		if (h < crt.bottom)
		{
			BitBlt(hMemDC, 0, h, crt.right, crt.bottom - h, hMemDC2, 0, 0, SRCCOPY);
		}
		DeleteDC(hMemDC2);

		if (gameState == MAIN)
		{
			TransBlt(hMemDC, crt.right / 2, 600, hUI[2], RGB(255, 255, 255));
			TransBlt(hMemDC, crt.right / 2 , 200, hUI[3], RGB(255, 255, 255));
		}
		else if (gameState == GAMEOVER)
		{
			TransBlt(hMemDC, crt.right / 2, crt.bottom / 2, hUI[4], RGB(255, 255, 255));
		}
		else
		{
			// 플레이어 총알 그림
			for (int i = 0; i < 20; i++)
				if (playerBullet[i].isAble)
					TransBlt(hMemDC, playerBullet[i].x, playerBullet[i].y, hShot, RGB(255, 255, 255));

			// 적기 총알 그림
			for (int i = 0; i < 200; i++)
				if (enemyBullet[i].isAble)
					TransBlt(hMemDC, enemyBullet[i].x, enemyBullet[i].y, hShotoval, RGB(255, 255, 255));

			// 회전탄 총알 그림
			for (int i = 0; i < 10; i++)
			{
				for (int j = 0; j < 11; j++)
				{
					if (circleBullet[i].childBullet[j].isAble)
						TransBlt(hMemDC, circleBullet[i].childBullet[j].x, circleBullet[i].childBullet[j].y, hShotoval, RGB(255, 255, 255));
				}
			}

			// 플레이어 비행기 그림
			TransBlt(hMemDC, player.x, player.y, hPlayer, RGB(255, 255, 255));

			// 적기 그림
			for (int i = 0; i < 10; i++)
			{
				if (enemy[i].isAble)
				{
					if (enemy[i].kind == FIGHTER)
						TransBlt(hMemDC, enemy[i].x, enemy[i].y, hEnemy[0], RGB(255, 255, 255));
					else if (enemy[i].kind == BOARDER)
						TransBlt(hMemDC, enemy[i].x, enemy[i].y, hEnemy[1], RGB(255, 255, 255));
				}
				else if (enemy[i].animeCount < 6)
				{
					TransBlt(hMemDC, enemy[i].x, enemy[i].y, hFlame[enemy[i].animeCount], RGB(255, 255, 255));
				}
			}

			// 보스와 HP바 그림
			if (boss.state > 0)
			{
				TransBlt(hMemDC, boss.x, boss.y, hBoss, RGB(255, 255, 255));
				DrawHpBar(hMemDC, boss.x - 50, boss.y - 60, hHpBar);
			}
			else if (boss.animeCount < 11)
			{
				TransBlt(hMemDC, boss.x, boss.y, hBoom[boss.animeCount], RGB(255, 255, 255));
			}

			// HUD 그림
			TransBlt(hMemDC, crt.right / 2, crt.bottom - 25, hUI[1], RGB(255, 255, 255));

			// 남은 목숨 그림
			for (int i = 50; i <= 50 * player.life; i += 50)
				TransBlt(hMemDC, i, crt.bottom - 25, hUI[0], RGB(255, 255, 255));

			// 점수 그림
			for (int i = 0; i < 8; i++)
			{
				TransBlt(hMemDC, scoreX, crt.bottom - 25, hNum[oldScore / temp], RGB(255, 255, 255));
				oldScore %= temp;
				temp /= 10;
				scoreX += 15;
			}
		}
		
		// 완성된 그림 전송
		BitBlt(hdc, 0, 0, crt.right, crt.bottom, hMemDC, 0, 0, SRCCOPY);

		SelectObject(hMemDC, hOldBitmap);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		PlaySound(NULL, NULL, 0);
		for (int i = 0; i < 6; i++)
			if (sound[i].dwID)
				mciSendCommand(sound[i].dwID, MCI_CLOSE, 0, NULL);
		KillTimer(hWnd, 0);
		KillTimer(hWnd, 1);
		DeleteObject(hPlayer);
		DeleteObject(hShot);
		DeleteObject(hShotoval);
		DeleteObject(hBackground);
		DeleteObject(hBackBit);
		DeleteObject(hEnemy[0]);
		DeleteObject(hEnemy[1]);
		for (int i = 0; i < 5; i++)
			DeleteObject(hUI[i]);
		for (int i = 0; i < 10; i++)
			DeleteObject(hNum[i]);
		for (int i = 0; i < 6; i++)
			DeleteObject(hFlame[i]);
		for (int i = 0; i < 11; i++)
			DeleteObject(hBoom[i]);
		DeleteDC(hMemDC);
		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}

void PlayerMove(void)
{
	if ((GetAsyncKeyState(VK_LEFT) & 0x8000) && player.x - 20 > 0)
		player.x -= 5;
	if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) && player.x + 20 < crt.right)
		player.x += 5;
	if (GetAsyncKeyState(VK_SPACE) & 0x8000 && player.delay >= 10)
	{
		mciSendCommand(sound[SHOT_PLAYER].dwID, MCI_SEEK, MCI_SEEK_TO_START, (DWORD)(LPVOID)&sound[SHOT_PLAYER].SeekBgm);
		mciSendCommand(sound[SHOT_PLAYER].dwID, MCI_PLAY, MCI_NOTIFY, (DWORD)(LPVOID)&sound[SHOT_PLAYER].playBgm);
		playerBullet[playerBullet_index].isAble = TRUE;
		playerBullet[playerBullet_index].x = player.x;
		playerBullet[playerBullet_index].y = player.y - 10;
		playerBullet_index++;
		playerBullet_index %= 20;
		player.delay = 0;
	}
}

void EnemyMove(void)
{
	float temp_vx[5];
	float temp_vy[5];

	if (count == 0)
	{
		enemy[enemy_index].isAble = TRUE;
		enemy[enemy_index].shootTime = rand() % 20 + 10;
		enemy[enemy_index].x = rand() % 456 + 30;
		enemy[enemy_index].y = -30;
		enemy[enemy_index].kind = (rand() % 3) % 2;
		enemy_index++;
		enemy_index %= 10;
		count = rand() % 30 + 10;
	}

	for (int i = 0; i < 10; i++)
	{
		if (enemy[i].isAble)
		{
			enemy[i].y += 10;
			enemy[i].shootTime--;
			if (enemy[i].shootTime == 0)
			{
				mciSendCommand(sound[SHOT_ENEMY].dwID, MCI_SEEK, MCI_SEEK_TO_START, (DWORD)(LPVOID)&sound[SHOT_ENEMY].SeekBgm);
				mciSendCommand(sound[SHOT_ENEMY].dwID, MCI_PLAY, MCI_NOTIFY, (DWORD)(LPVOID)&sound[SHOT_ENEMY].playBgm);
				if (enemy[i].kind == FIGHTER)
				{
					enemyBullet[enemyBullet_index].isAble = TRUE;
					InitAimingBullet(player.x, player.y, enemy[i].x, enemy[i].y, 15, enemyBullet[enemyBullet_index].x,
						enemyBullet[enemyBullet_index].y, enemyBullet[enemyBullet_index].vx, enemyBullet[enemyBullet_index].vy);
					enemyBullet_index++;
					enemyBullet_index %= 200;
				}
				else if (enemy[i].kind == BOARDER)
				{
					InitAimingBullet(player.x, player.y, enemy[i].x, enemy[i].y, 15, enemyBullet[enemyBullet_index].x,
						enemyBullet[enemyBullet_index].y, enemyBullet[enemyBullet_index].vx, enemyBullet[enemyBullet_index].vy);
					InitNWayBullets(enemyBullet[enemyBullet_index].vx, enemyBullet[enemyBullet_index].vy, 5, 5,
						temp_vx, temp_vy);
					for (int j = 0; j < 5; j++)
					{
						enemyBullet[enemyBullet_index].x = enemy[i].x;
						enemyBullet[enemyBullet_index].y = enemy[i].y;
						enemyBullet[enemyBullet_index].vx = temp_vx[j];
						enemyBullet[enemyBullet_index].vy = temp_vy[j];
						enemyBullet[enemyBullet_index].isAble = TRUE;
						enemyBullet_index++;
						enemyBullet_index %= 200;
					}
				}
			}
		}
		else if (enemy[i].animeCount < 6)
		{
			enemy[i].animeCount++;
			enemy[i].y += 3;
		}
		if (enemy[i].y > wrt.bottom)
			enemy[i].isAble = FALSE;
	}

	if (score < 1000)
		count--;
	else if (boss.state == WAIT)
		boss.state = MOVE;
}

void BulletMove(void)
{
	for (int i = 0; i < 20; i++)
	{
		if (playerBullet[i].isAble)
		{
			playerBullet[i].y -= 10;
			if (playerBullet[i].y < -10)
				playerBullet[i].isAble = FALSE;
			for (int j = 0; j < 10; j++)
			{
				if (enemy[j].isAble)
				{
					if (CheckCollision(playerBullet[i].x - 5, playerBullet[i].y - 10, playerBullet[i].x + 5, playerBullet[i].y + 10,
						enemy[j].x - 10, enemy[j].y - 10, enemy[j].x + 10, enemy[j].y + 10))
					{
						if (enemy[j].kind == FIGHTER)
							score += 30;
						else if (enemy[j].kind == BOARDER)
							score += 50;
						mciSendCommand(sound[EXPLO_ENEMY].dwID, MCI_SEEK, MCI_SEEK_TO_START, (DWORD)(LPVOID)&sound[EXPLO_ENEMY].SeekBgm);
						mciSendCommand(sound[EXPLO_ENEMY].dwID, MCI_PLAY, MCI_NOTIFY, (DWORD)(LPVOID)&sound[EXPLO_ENEMY].playBgm);
						playerBullet[i].y += 10;
						playerBullet[i].isAble = FALSE;
						enemy[j].isAble = FALSE;
						enemy[j].animeCount = -1;
					}
				}
			}
		}
	}

	for (int i = 0; i < 10; i++)
	{
		if (circleBullet[i].isAble)
		{
			MoveBullet(circleBullet[i].x, circleBullet[i].y, circleBullet[i].vx, circleBullet[i].vy);
			for (int j = 0; j < 11; j++)
			{
				MoveLoopingBullet(circleBullet[i].childBullet[j].x, circleBullet[i].childBullet[j].y,
					circleBullet[i].childBullet[j].vx, circleBullet[i].childBullet[j].vy,
					circleBullet[i].x, circleBullet[i].y, circleBullet[i].childBullet[j].r, 3, circleBullet[i].childBullet[j].theta, 0.1);
				
				if (CheckCollision(circleBullet[i].childBullet[j].x - 5, circleBullet[i].childBullet[j].y - 5, 
					circleBullet[i].childBullet[j].x + 5, circleBullet[i].childBullet[j].y + 5,
					player.x - 10, player.y - 10, player.x + 10, player.y + 10))
				{
					if (player.life > 1)
						player.life--;
					/*else
						gameState = GAMEOVER;*/
					mciSendCommand(sound[EXPLO_PLAYER].dwID, MCI_SEEK, MCI_SEEK_TO_START, (DWORD)(LPVOID)&sound[EXPLO_PLAYER].SeekBgm);
					mciSendCommand(sound[EXPLO_PLAYER].dwID, MCI_PLAY, MCI_NOTIFY, (DWORD)(LPVOID)&sound[EXPLO_PLAYER].playBgm);
					circleBullet[i].childBullet[j].isAble = FALSE;
				}
			}

			if (circleBullet[i].y > wrt.bottom + 250)
			{
				circleBullet[i].isAble = FALSE;
				for (int j = 0; j < 11; j++)
				{
					circleBullet[i].childBullet[j].isAble = FALSE;
				}
			}
		}
	}

	for (int i = 0; i < 200; i++)
	{
		if (enemyBullet[i].isAble)
		{
			MoveBullet(enemyBullet[i].x, enemyBullet[i].y, enemyBullet[i].vx, enemyBullet[i].vy);

			if (enemyBullet[i].x < wrt.left || enemyBullet[i].x > wrt.right ||
				enemyBullet[i].y < wrt.top || enemyBullet[i].y > wrt.bottom)
				enemyBullet[i].isAble = FALSE;
			if (CheckCollision(enemyBullet[i].x - 5, enemyBullet[i].y - 5, enemyBullet[i].x + 5, enemyBullet[i].y + 5,
				player.x - 10, player.y - 10, player.x + 10, player.y + 10))
			{
				if (player.life > 1)
					player.life--;
				/*else
					gameState = GAMEOVER;*/
				mciSendCommand(sound[EXPLO_PLAYER].dwID, MCI_SEEK, MCI_SEEK_TO_START, (DWORD)(LPVOID)&sound[EXPLO_PLAYER].SeekBgm);
				mciSendCommand(sound[EXPLO_PLAYER].dwID, MCI_PLAY, MCI_NOTIFY, (DWORD)(LPVOID)&sound[EXPLO_PLAYER].playBgm);
				enemyBullet[i].isAble = FALSE;
			}
		}
	}
}

void BossMove(void)
{
	float temp_vx[9];
	float temp_vy[9];

	if (boss.y < 100)
	{
		boss.y += 5;
	}
	else if (boss.state == MOVE)
	{
		boss.state = ATTACK;
		boss.attackCount = 0;
	}

	if (boss.state == ATTACK)
	{
		if (boss.attackCount == 0)
		{
			mciSendCommand(sound[SHOT_ENEMY].dwID, MCI_SEEK, MCI_SEEK_TO_START, (DWORD)(LPVOID)&sound[SHOT_ENEMY].SeekBgm);
			mciSendCommand(sound[SHOT_ENEMY].dwID, MCI_PLAY, MCI_NOTIFY, (DWORD)(LPVOID)&sound[SHOT_ENEMY].playBgm);

			if (attackType == IDLE)
			{
				nwayCount = 0;
				attackType = rand() % 3;
				if (attackType < CIRCLE)
					nwayNum = rand() % 3 + 5;
			}

			switch (attackType)
			{
			case ODD_NWAY:
				InitDirectedBullet(boss.x, boss.y, enemyBullet[enemyBullet_index].x, enemyBullet[enemyBullet_index].y,
					enemyBullet[enemyBullet_index].vx, enemyBullet[enemyBullet_index].vy, 10, 90);
				InitNWayBullets(enemyBullet[enemyBullet_index].vx, enemyBullet[enemyBullet_index].vy, 5, 9,
					temp_vx, temp_vy);
				for (int j = 0; j < 9; j++)
				{
					enemyBullet[enemyBullet_index].x = boss.x;
					enemyBullet[enemyBullet_index].y = boss.y;
					enemyBullet[enemyBullet_index].vx = temp_vx[j];
					enemyBullet[enemyBullet_index].vy = temp_vy[j];
					enemyBullet[enemyBullet_index].isAble = TRUE;
					enemyBullet_index++;
					enemyBullet_index %= 200;
				}
				if (nwayCount < nwayNum)
				{
					nwayCount++;
					attackType = EVEN_NWAY;
				}
				else
				{
					attackType = IDLE;
				}
				break;
			case EVEN_NWAY:
				InitDirectedBullet(boss.x, boss.y, enemyBullet[enemyBullet_index].x, enemyBullet[enemyBullet_index].y,
					enemyBullet[enemyBullet_index].vx, enemyBullet[enemyBullet_index].vy, 10, 90);
				InitNWayBullets(enemyBullet[enemyBullet_index].vx, enemyBullet[enemyBullet_index].vy, 5, 8,
					temp_vx, temp_vy);
				for (int j = 0; j < 8; j++)
				{
					enemyBullet[enemyBullet_index].x = boss.x;
					enemyBullet[enemyBullet_index].y = boss.y;
					enemyBullet[enemyBullet_index].vx = temp_vx[j];
					enemyBullet[enemyBullet_index].vy = temp_vy[j];
					enemyBullet[enemyBullet_index].isAble = TRUE;
					enemyBullet_index++;
					enemyBullet_index %= 200;
				}
				if (nwayCount < nwayNum)
				{
					nwayCount++;
					attackType = ODD_NWAY;
				}
				else
				{
					attackType = IDLE;
				}
				break;
			case CIRCLE:
				circleBullet[circleBullet_index].isAble = TRUE;
				InitAimingBullet(player.x, player.y, boss.x, boss.y, 10, circleBullet[circleBullet_index].x,
					circleBullet[circleBullet_index].y, circleBullet[circleBullet_index].vx, circleBullet[circleBullet_index].vy);

				for (int i = 0; i < 11; i++)
				{
					circleBullet[circleBullet_index].childBullet[i].isAble = TRUE;
					circleBullet[circleBullet_index].childBullet[i].r = 5;
					circleBullet[circleBullet_index].childBullet[i].theta = 0 + (36 * i);
				}

				circleBullet_index++;
				circleBullet_index %= 10;
				
				attackType = IDLE;
				break;
			}

			if (attackType < CIRCLE)
				boss.attackCount = 15;
			else
				boss.attackCount = 100;
		}
		boss.attackCount--;		
	}	

	for (int i = 0; i < 20; i++)
	{
		if (playerBullet[i].isAble && boss.state != BOOM)
		{
			if (CheckCollision(playerBullet[i].x - 5, playerBullet[i].y - 10, playerBullet[i].x + 5, playerBullet[i].y + 10,
				boss.x - 40, boss.y - 45, boss.x + 40, boss.y + 45))
			{
				if (boss.life > 10)
				{
					mciSendCommand(sound[HIT_BOSS].dwID, MCI_SEEK, MCI_SEEK_TO_START, (DWORD)(LPVOID)&sound[HIT_BOSS].SeekBgm);
					mciSendCommand(sound[HIT_BOSS].dwID, MCI_PLAY, MCI_NOTIFY, (DWORD)(LPVOID)&sound[HIT_BOSS].playBgm);
					boss.life -= 10;
				}
				else
				{
					mciSendCommand(sound[EXPLO_BOSS].dwID, MCI_SEEK, MCI_SEEK_TO_START, (DWORD)(LPVOID)&sound[EXPLO_BOSS].SeekBgm);
					mciSendCommand(sound[EXPLO_BOSS].dwID, MCI_PLAY, MCI_NOTIFY, (DWORD)(LPVOID)&sound[EXPLO_BOSS].playBgm);
					boss.animeCount = -1;
					boss.state = BOOM;
					score += 1000;
				}

				playerBullet[i].isAble = FALSE;
			}
		}
	}

	if (boss.animeCount < 11)
	{
		boss.animeCount++;
		boss.y += 3;
	}
}

void InitAimingBullet(
	float mx, float my,   // 메인 캐릭터의 위치
	float ex, float ey,   // 적의 좌표
	float speed,          // 탄의 속도
	float& x, float& y,   // 탄의 좌표
	float& vx, float& vy  // 탄의 속도 벡터
) 
{
	// 탄의 좌표를 설정하기
	x = ex;
	y = ey;

	// 목표까지의 거리 d를 구하기
	float d = sqrt((mx - ex) * (mx - ex) + (my - ey) * (my - ey));

	// 속도가 일정한 값(speed)이 되도록 속도벡터를 구하기
	// 목표까지의 거리 d가 0일 때는 아랫쪽으로 발사함
	if (d) 
	{
		vx = (mx - ex) / d * speed;
		vy = (my - ey) / d * speed;
	}
	else 
	{
		vx = 0;
		vy = speed;
	}
}

void InitDirectedBullet(
	float ex, float ey,    // 적의 좌표
	float& x, float& y,    // 탄의 좌표
	float& vx, float& vy,  // 탄의 속도벡터
	float speed,           // 탄의 속도
	float theta            // 발사각도
) 
{
	// 탄의 좌표를 설정하기
	x = ex; y = ey;

	// speed의 속도로 각도 theta 방향으로 날아가는 탄의 속도벡터(vx,vy)를 구하기
	// M_PI는 원주율
	vx = cos(M_PI / 180 * theta) * speed;
	vy = sin(M_PI / 180 * theta) * speed;
}

void MoveBullet(
	float& x, float& y,  // 탄의 좌표
	float vx, float vy   // 탄의 속도
) 
{
	// 탄의 좌표(x,y)에 속도를 더해줌
	x += vx;
	y += vy;
}

void InitNWayBullets(
	float vx0, float vy0,    // 중심이 되는 탄의 속도
	float theta,             // 탄과 탄사이의 각도
	int n,                   // 탄의 갯수
	float vx[], float vy[]   // n-way탄의 속도
) 
{
	// 탄과 탄 사이의 각도를 라디안으로 변환하기
	float rad_step = M_PI / 180 * theta;

	// 가장자리의 탄과 중심 부분의 탄의 각도를 계산하기
	float rad = n % 2 ? -n / 2 * rad_step : (-n / 2 + 0.5) * rad_step;

	// n개의 탄의 속도를 계산하기
	for (int i = 0; i < n; i++, rad += rad_step) 
	{
		// (vx[i], vy[i])를 구하기
		// 속도 벡터 (vx0, vy0)를 rad만큼 회전시키기
		float c = cos(rad), s = sin(rad);
		vx[i] = vx0 * c - vy0 * s;
		vy[i] = vx0 * s + vy0 * c;
	}
}

void MoveLoopingBullet(
	float& x, float& y,    // 탄의 좌표
	float& vx, float& vy,  // 탄의 속도
	float cx, float cy,    // 회전중심의 좌표
	float& r,              // 반경
	float vr,              // 반경의 변화
	float& theta,          // 각도(라디안)
	float omega            // 한 번 이동할 때 변화하는 각도(라디안)
) 
{
	// 각도를 변화시키기
	theta += omega;

	// 반경을 변화시키기
	r += vr;

	// 위치를 계산하기
	x = cx + r * cos(theta);
	y = cy + r * sin(theta);

	// 탄의 속도(필요할 때만)
	vx = -r * omega * sin(theta);
	vy = r * omega * cos(theta);
}

BOOL CheckCollision
(
	float x0, float y0,   //탄의 좌상좌표
	float x1, float y1,   //탄의 우하좌표
	float mx0, float my0, //메인 캐릭터의 좌상좌표
	float mx1, float my1  //메인 캐릭터의 우하좌표
)
{
	return (!(x1 <= mx0 || mx1 <= x0 || y1 <= my0 || my1 <= y0) || (mx0 < x1 && x0 < mx1 && my0 < y1 && y0 < my1));
}

void DrawHpBar(HDC hdc, int x, int y, HBITMAP hBit)
{
	HDC MemDC;
	HBITMAP OldBitmap;
	BITMAP bit;
	int by;

	MemDC = CreateCompatibleDC(hdc);
	OldBitmap = (HBITMAP)SelectObject(MemDC, hBit);

	GetObject(hBit, sizeof(BITMAP), &bit);
	by = bit.bmHeight;

	BitBlt(hdc, x, y, boss.life, by, MemDC, 0, 0, SRCCOPY);

	SelectObject(MemDC, OldBitmap);
	DeleteDC(MemDC);
}

void TransBlt(HDC hdc, int x, int y, HBITMAP hbitmap, COLORREF clrMask)
{
	BITMAP bm;
	COLORREF cColor;
	HBITMAP bmAndBack, bmAndObject, bmAndMem, bmSave;
	HBITMAP bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
	HDC		hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
	POINT	ptSize;

	hdcTemp = CreateCompatibleDC(hdc);
	SelectObject(hdcTemp, hbitmap);
	GetObject(hbitmap, sizeof(BITMAP), (LPSTR)&bm);
	ptSize.x = bm.bmWidth;
	ptSize.y = bm.bmHeight;
	DPtoLP(hdcTemp, &ptSize, 1);

	hdcBack = CreateCompatibleDC(hdc);
	hdcObject = CreateCompatibleDC(hdc);
	hdcMem = CreateCompatibleDC(hdc);
	hdcSave = CreateCompatibleDC(hdc);

	bmAndBack = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);
	bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);
	bmAndMem = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
	bmSave = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);

	bmBackOld = (HBITMAP)SelectObject(hdcBack, bmAndBack);
	bmObjectOld = (HBITMAP)SelectObject(hdcObject, bmAndObject);
	bmMemOld = (HBITMAP)SelectObject(hdcMem, bmAndMem);
	bmSaveOld = (HBITMAP)SelectObject(hdcSave, bmSave);

	SetMapMode(hdcTemp, GetMapMode(hdc));

	BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

	cColor = SetBkColor(hdcTemp, clrMask);

	BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

	SetBkColor(hdcTemp, cColor);

	BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, NOTSRCCOPY);
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, x - (ptSize.x / 2), y - (ptSize.y / 2), SRCCOPY);
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);
	BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);
	BitBlt(hdc, x - (ptSize.x / 2), y - (ptSize.y / 2), ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY);
	BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);

	DeleteObject(SelectObject(hdcBack, bmBackOld));
	DeleteObject(SelectObject(hdcObject, bmObjectOld));
	DeleteObject(SelectObject(hdcMem, bmMemOld));
	DeleteObject(SelectObject(hdcSave, bmSaveOld));

	DeleteDC(hdcMem);
	DeleteDC(hdcBack);
	DeleteDC(hdcObject);
	DeleteDC(hdcSave);
	DeleteDC(hdcTemp);
}