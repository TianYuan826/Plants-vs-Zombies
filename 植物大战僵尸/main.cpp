/*
* 开发日志
* 1. 创建新项目（空项目模板）使用vs的任意版本
* 2. 导入素材
* 3. 实现最开始的游戏场景
* 4. 实现游戏顶部的工具栏
* 5. 实现工具栏中的植物卡牌
*/

#include <stdio.h>
#include <graphics.h> //easyx图形库的头文件，需要安装easyx图形库
#include <time.h>
#include "tools.h"
#include "vector2.h"
#include <math.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")
#pragma warning(disable: 4996)

#define WIN_WIDTH 900
#define WIN_HEIGHT 600
#define ZM_MAX 10

enum {WAN_DOU,XING_RI_KUI,ZHI_WU_COUNT};

IMAGE imgBg; //表示背景图片
IMAGE imgBar;
IMAGE imgCards[ZHI_WU_COUNT];
IMAGE* imgZhiWU[ZHI_WU_COUNT][20];

int cuX, cuY; //当前选中的植物，在移动的位置
int curZhiWu; // 0: 没有选中 1：选择了第一种植物

enum {GOING,WIN,FAIL};
int killCount; //已经杀掉的僵尸的个数
int zmCount; //已经出现的僵尸的个数
int gameStatus;

struct zhiwu {
	int type; //0: 没有选中 1：第一种植物
	int frameIndex; //序列帧的序号

	bool catched; //是否被僵尸捕获
	int deadTime; //死亡计数器

	int timer;
	int x, y;
	int shootTimer;
};
struct	zhiwu map[3][9]; //草坪

enum {SUNSHINE_DOWN,SUNSHINE_GROUND,SUNSHINE_COLLECT,SUNSHINE_PRODUCT};

struct sunshineBall {
	int x, y; //阳光球在飘落的过程中的坐标位置(x不变)
	int frameIndex; //当前显示的图片帧的序号
	int destY; //飘落的目标位置的y坐标
	bool used;	//是否在使用
	int timer; //定时器
	float xoff; //阳光收集过程中的偏移坐标x
	float yoff; //阳光收集过程中的偏移坐标y

	float t; //贝塞尔曲线的时间点 0...1
	vector2 p1, p2, p3, p4;
	vector2 pCur; //当前时刻阳光球产生过程的位置
	float speed;
	int status;
};
struct sunshineBall balls[10];
IMAGE imgSunshineBall[29];
int sunshine; //阳光值

struct zm { //僵尸
	int x,y;
	int frameIndex;
	bool used;
	int speed;
	int row;
	int blood;
	bool dead;
	bool eating; //正在吃植物
};
struct zm zms[10]; 
IMAGE imgZm[22];
IMAGE imgZmDead[20];
IMAGE imgZmEating[21];
IMAGE imgZmStand[11];

//子弹的数据类型
struct bullet {
	int x, y;
	int row;
	int used;
	int speed;
	bool blast;	//是否发射爆炸
	int frameIndex;

};
struct bullet bullets[30];
IMAGE imgBulletNormal;
IMAGE imgBullBlast[4];

bool fileExist(const char* name) {
	FILE* fp = fopen(name,"r");
	if (fp == NULL) {
		return false;
	}
	else {
		fclose(fp);
		return true;
	}

}

void gameInit() {
	//加载背景图片
	//把字符集修改为“多字节字符集”
	loadimage(&imgBg, "res/bg.jpg");
	loadimage(&imgBar, "res/bar5.png");

	memset(imgZhiWU, 0, sizeof(imgZhiWU));
	memset(map, 0, sizeof(map));

	killCount = 0;
	zmCount = 0;
	gameStatus = GOING;


	//初始化植物卡牌
	char name[64];
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		//生成植物卡牌的文件名
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);
		loadimage(&imgCards[i], name);

		for (int j = 0; j < 20; j++) {
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i,j+1);
			// 先判断文件是否存在
			if (fileExist(name)) {
				imgZhiWU[i][j] = new IMAGE;
				loadimage(imgZhiWU[i][j], name);
			}
			else {
				break;
			}
		}

	}
	curZhiWu = 0;
	sunshine = 50;

	memset(balls, 0, sizeof(balls));
	for (int i = 0; i < 29; i++) {
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(&imgSunshineBall[i], name);
	}

	//配置随机种子
	srand(time(NULL));

	//创建游戏的图形窗口
	initgraph(WIN_WIDTH,WIN_HEIGHT,1);

	//设置字体
	LOGFONT f;
	gettextstyle(&f); //获取当前字体
	f.lfWeight = 15;
	f.lfHeight = 30;
	strcpy(f.lfFaceName, "Segoe UI Black");
	f.lfQuality = ANTIALIASED_QUALITY; //抗锯齿效果
	settextstyle(&f);
	setbkmode(TRANSPARENT); //设置字体背景模式为透明的
	setcolor(BLACK);

	//初始化僵尸数据
	memset(zms, 0, sizeof(zms));
	for (int i = 0; i < 22; i++) {
		sprintf_s(name, sizeof(name),"res/zm/%d.png", i + 1);
		loadimage(&imgZm[i], name);
	}

	//初始化子弹数据
	memset(bullets, 0, sizeof(bullets));
	loadimage(&imgBulletNormal, "res/bullets/bullet_normal.png");

	//初始化豌豆子弹的帧图片数组
	loadimage(&imgBullBlast[3], "res/bullets/bullet_blast.png");
	for (int i = 0; i < 3; i++) {
		float k = (i + 1) * 0.2;
		loadimage(&imgBullBlast[i], "res/bullets/bullet_blast.png",
			imgBullBlast[3].getwidth()*k, 
			imgBullBlast[3].getheight() * k,true);
	}

	//初始化僵尸死亡过程的图片帧
	for (int i = 0; i < 20; i++) {
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZmDead[i], name);
	}

	//初始化僵尸吃植物过程的图片帧
	for (int i = 0; i < 21; i++) {
		sprintf_s(name, sizeof(name), "res/zm_eat/%d.png", i + 1);
		loadimage(&imgZmEating[i], name);
	}

	for (int i = 0; i < 11; i++) {
		sprintf_s(name, sizeof(name), "res/zm_stand/%d.png", i + 1);
		loadimage(&imgZmStand[i], name);
	}
}

void drawSunshines() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++) {
		//if (balls[i].used || balls[i].xoff) {
		if(balls[i].used){
			IMAGE* img = &imgSunshineBall[balls[i].frameIndex];
			//putimagePNG(balls[i].x, balls[i].y, img);
			putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
		}
	}
	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshine);
	outtextxy(276, 67, scoreText); //输出阳光值分数

}

void drawCards() {
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		int x = 338 + i * 65;
		int y = 6;
		putimage(x, y, &imgCards[i]);

	}
}

void drawZhiWu() {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				//int x = 256 + j * 81;
				//int y = 179 + i * 102 + 14;
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				//putimagePNG(x, y, imgZhiWU[zhiWuType][index]);
				putimagePNG(map[i][j].x, map[i][j].y, imgZhiWU[zhiWuType][index]);
			}
		}
	}

	//渲染 拖动过程中的植物
	if (curZhiWu) {
		IMAGE* img = imgZhiWU[curZhiWu - 1][0];
		putimagePNG(cuX - img->getwidth() / 2, cuY - img->getheight() / 2, img);
	}
}

void drawZm() {
	int zmMax = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zmMax; i++) {
		if (zms[i].used) {
			//IMAGE* img = &imgZm[zms[i].frameIndex];
			IMAGE* img = NULL;
			if (zms[i].eating)img = imgZmEating;
			else if (zms[i].dead) img = imgZmDead;
			else img = imgZm;
			img += zms[i].frameIndex;
			putimagePNG(zms[i].x, zms[i].y - img->getheight(), img);


		}
	}
}

void drawBullet() {
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < bulletMax; i++) {
		if (bullets[i].used) {
			if (bullets[i].blast) {
				IMAGE* img = &imgBullBlast[bullets[i].frameIndex];
				putimagePNG(bullets[i].x, bullets[i].y, img);
			}
			else {
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
			}

		}
	}
}
void updateWindow() {
	BeginBatchDraw(); //开始缓冲
	putimage(-112, 0, &imgBg);  //显示背景图片
	putimagePNG(250, 0, &imgBar);

	drawCards();
	
	drawZhiWu();

	drawSunshines();

	drawZm();

	drawBullet();

	EndBatchDraw(); //结束双缓冲
}

void collectSunshine(ExMessage* msg) {
	int count = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < count; i++) {
		int w = imgSunshineBall[0].getwidth();
		int h = imgSunshineBall[0].getheight();
		//int x = balls[i].x;
		//int y = balls[i].y;

		int x = balls[i].pCur.x;
		int y = balls[i].pCur.y;

		if (msg->x > x  && msg-> x< x + w && 
			msg->y > y && msg->y < y + h ){
			//balls[i].used = false;
			balls[i].status = SUNSHINE_COLLECT;
			//sunshine += 25;
			//mciSendString("play res/sunshine.mp3",0,0,0); //播放收集阳光的音频
			PlaySound("res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);
			//设置阳光球的偏移量
			//float destX = 262;
			//float destY = 0;
			//float angle = atan((y - destY) / (x - destX));
			//balls[i].xoff = 4 * cos(angle);
			//balls[i].yoff = 4 * sin(angle);
			balls[i].p1 = balls[i].pCur;
			balls[i].p4 = vector2(262, 0);
			balls[i].t = 0;
			float distance = dis(balls[i].p1 - balls[i].p4);
			float off = 8;
			balls[i].speed = 1.0 / (distance / off);
			break;
		}
	}

}

void userClick() {
	ExMessage msg;
	static int status = 0; //鼠标是否选中状态
	if (peekmessage(&msg)) { //如果有消息
		if (msg.message == WM_LBUTTONDOWN) {  //点击鼠标左键
			if (msg.x > 338 && msg.x < 338 + 65 * ZHI_WU_COUNT && msg.y < 96) {
				int index = (msg.x - 338) / 65;
				
				status = 1;
				curZhiWu = index + 1;
			}
			else {
				collectSunshine(&msg);
			}
		}
		else if (msg.message == WM_MOUSEMOVE&& status == 1) {  //鼠标移动
			cuX = msg.x;
			cuY = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP) {  //松开鼠标种下植物
			if (msg.x > 256 - 112 && msg.y > 179 && msg.y < 489) {
				
				int row = (msg.y - 179) / 102;
				int col = (msg.x - 256 + 112) / 81;
				if (map[row][col].type == 0) {
					map[row][col].type = curZhiWu;
					map[row][col].frameIndex = 0;
					map[row][col].shootTimer = 0;

					//int x = 256 + j * 81;
					//int y = 179 + i * 102 + 14;
					map[row][col].x = 256 - 112 + col * 81;
					map[row][col].y = 179 + row * 102 + 14;

				}
				
			}
			curZhiWu = 0;
			status = 0;
			
		}
	}
}
void createSunshine() {
	static int count = 0;
	static int fre = 400;
	count++;

	if (count >= fre) {
		fre = 200 + rand() % 200;
		count = 0;

		//从阳光池中取出一个可以使用的
		int ballMax = sizeof(balls) / sizeof(balls[0]);
		int i;
		for (i = 0; i < ballMax && balls[i].used; i++);
		if (i >= ballMax)return;
		balls[i].used = true;
		balls[i].frameIndex = 0;
		//balls[i].x = 260 + rand() % (900 - 260); //260...900
		//balls[i].y = 60;
		//balls[i].destY = 200 + (rand() % 4) * 90;
		balls[i].timer = 0;
		//balls[i].xoff = 0;
		//balls[i].yoff = 0;
		balls[i].t = 0;
		balls[i].p1 = vector2(260 + rand() % (900 - 260), 60);
		balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
		balls[i].status = SUNSHINE_DOWN;
		float distance = balls[i].p4.y - balls[i].p1.y;
		float off = 2;
		balls[i].speed = 1.0 / (distance / off);

	}

	//向日葵生产阳光
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == XING_RI_KUI + 1) {
				map[i][j].timer++;
				if (map[i][j].timer > 200) {
					map[i][j].timer = 0;
					int k;
					for (k = 0; k < ballMax && balls[k].used; k++);
					if (k >= ballMax) return;
					balls[k].used = true;
					balls[k].frameIndex = 0;
					balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
					int w = (100 + rand() % 50) * (rand() % 2 ? 1 : -1);
					balls[k].p4 = vector2(map[i][j].x + w,
						map[i][j].y + imgZhiWU[XING_RI_KUI][0]->getheight() -
						imgSunshineBall[0].getheight());
					balls[k].p2 = vector2(balls[k].p1.x + w * 0.3, balls[k].p1.y - 100);
					balls[k].p3 = vector2(balls[k].p1.x + w * 0.7, balls[k].p1.y - 100);
					balls[k].status = SUNSHINE_PRODUCT;
					balls[k].speed = 0.05;
					balls[k].t = 0;
				}

				
			}
		}
	}

}

void updateSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	static int count = 0;
	if (++count < 3) return;
	count = 0;
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			if (balls[i].status == SUNSHINE_COLLECT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t > 1) {
					sun->used = false;
					sunshine += 25;
				}
			}else if(balls[i].status == SUNSHINE_DOWN){
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1); //p1 + t*(p4 - p1)
				if (sun->t >= 1) {
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_GROUND) {
				balls[i].timer++;
				if (balls[i].timer > 100) {
					balls[i].used = false;
					balls[i].timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_PRODUCT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = calcBezierPoint(sun->t, sun->p1, sun->p2, sun->p3, sun->p4);
				if (sun->t > 1) {
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}

			/*if (balls[i].timer == 0) {
				balls[i].y += 2;
			}
		
			if (balls[i].y >= balls[i].destY) {
				balls[i].timer++;
				if (balls[i].timer > 100) {
					balls[i].used = false;
				}
			}*/
		}
		/*else if (balls[i].xoff) {
			float destX = 262;
			float destY = 0;
			float angle = atan((balls[i].y - destY) / (balls[i].x - destX));
			balls[i].xoff = 4 * cos(angle);
			balls[i].yoff = 4 * sin(angle);

			balls[i].x -= balls[i].xoff;
			balls[i].y -= balls[i].yoff;
			if (balls[i].x < 262 || balls[i].y < 0) {
				balls[i].xoff = 0;
				balls[i].yoff = 0;
				sunshine += 25;
			}
		}*/
	}
}

void createZM() {
	if (zmCount > ZM_MAX) {
		return;
	}

	static int count = 0;
	static int freZM = 200;
	count++;
	if (count > freZM) {
		freZM = 300 + rand() % 200;
		count = 0;
		int zmMax = sizeof(zms) / sizeof(zms[0]);
		int i;
		for (i = 0; i < zmMax && zms[i].used; i++);
		if (i < zmMax) {
			memset(&zms[i], 0, sizeof(zms[i]));
			zms[i].used = true;
			zms[i].x = WIN_WIDTH;
			zms[i].row = rand() % 3; //0..2
			zms[i].y = 172 + (1 + zms[i].row) * 100;
			zms[i].speed = 2;
			zms[i].blood = 100;
			zms[i].dead=false;
			zmCount++;
		}
	}
	
}

void updateZM() {
	int zmMax = sizeof(zms) / sizeof(zms[0]);
	static int count = 0;
	count++;
	if (count > 4) {
		count = 0;
		//更新僵尸的位置
		for (int i = 0; i < zmMax; i++) {
			if (zms[i].used) {
				zms[i].x -= zms[i].speed;
				if (zms[i].x < 56) {
					//printf("GAME OVER\n");
					//MessageBox(NULL, "over", "over", 0); //待优化
					//exit(0); //待优化
					gameStatus = FAIL;
				}
			}
		}
	}

	//僵尸行走的动作
	static int count2 = 0;
	count2++;
	if (count2 > 8) {
		count2 = 0;
		for (int i = 0; i < zmMax; i++) {
			if (zms[i].used) {

				if (zms[i].dead) {
					zms[i].frameIndex++;
					if (zms[i].frameIndex >= 20) {
						zms[i].used = false;
						killCount++;
						if (killCount == ZM_MAX) {
							gameStatus = WIN;
						}
						
					}
				}
				else if (zms[i].eating) {
					zms[i].frameIndex = (zms[i].frameIndex + 1) % 21;
				}
				else {
					zms[i].frameIndex = (zms[i].frameIndex + 1) % 22;
				}
			}
		}
	}
	
	
}

void shoot() {
	int lines[3] = {0};
	static int count = 0;
	if (++count < 2) return;
	count = 0;
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int dangerX = WIN_WIDTH - imgZm[0].getwidth();
	for (int i = 0; i < zmCount; i++) {
		if (zms[i].used && zms[i].x < dangerX) {
			lines[zms[i].row] = 1;
		}
	}

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == WAN_DOU + 1 && lines[i]) {
				/*static int count = 0;
				count++;
				if (count > 40) {
					count = 0;*/

					map[i][j].shootTimer++;
					if (map[i][j].shootTimer > 20) {
						map[i][j].shootTimer = 0;
					

					int k;
					for (k = 0; k < bulletMax && bullets[k].used; k++);

					if (k < bulletMax) {
						bullets[k].used = true;
						bullets[k].speed = 4;
						bullets[k].row = i;
						bullets[k].blast = false;
						bullets[k].frameIndex = 0;
						int zwX = 256 - 112 + j * 81;
						int zwY = 179 + i * 102 + 14;
						bullets[k].x = zwX + imgZhiWU[map[i][j].type - 1][0]->getwidth() - 10;
						bullets[k].y = zwY + 5;
					}
				}
				}
				
		}
	}
}

void updateBullets() {
	int bulletCount = sizeof(bullets) / sizeof(bullets[0]);
	static int count = 0;
	if (++count < 2) return;
	count = 0;
	for (int i = 0; i < bulletCount; i++) {
		if (bullets[i].used) {
			bullets[i].x += bullets[i].speed;
			if (bullets[i].x > WIN_WIDTH) {
				bullets[i].used = false;
			}

			//实现子弹的碰撞检测
			if (bullets[i].blast) {
				bullets[i].frameIndex++;
				if (bullets[i].frameIndex > 4) {
					bullets[i].used = false;
				}
			}
		}
		

	}
}

void createZhiWu() {
	static int count = 0;
	count++;
	if (count > 2) {
		count = 0;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 9; j++) {
				if (map[i][j].type > 0) {
					map[i][j].frameIndex++;
					int zhiWuType = map[i][j].type - 1;
					int index = map[i][j].frameIndex;
					if (imgZhiWU[zhiWuType][index] == NULL) {
						map[i][j].frameIndex = 0;
					}
				}

			}
		}
	}
	
}

void checkBulletToZm() {
	int zCount = sizeof(zms) / sizeof(zms[0]);
	int bCount = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < bCount; i++) {
		if (bullets[i].used == false || bullets[i].blast) continue;
		for (int j = 0; j < zCount; j++) {
			if (zms[j].used == false) continue;
			int x1 = zms[j].x + 80;
			int x2 = zms[j].x + 110;
			int x = bullets[i].x;
			if (zms[j].dead == false && bullets[i].row == zms[j].row && x > x1 && x < x2) {
				zms[j].blood -= 20;

				bullets[i].blast = true;
				bullets[i].speed = 0;

				if (zms[j].blood <= 0) {
					zms[j].dead = true;
					zms[j].frameIndex = 0;
					zms[j].speed = 0;
				}
				break;

			}
		}
	}
}

void checkZmToZhiWu() {
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zCount; i++) {
		if (zms[i].dead) continue;
		int row = zms[i].row;
		for (int j = 0; j < 9; j++) {
			if (map[row][j].type == 0) continue;
			//		x1    x2
			//      [      ]
			//			[       ]
			//          x3
			int zhiWuX = 256 - 112 + j * 81;
			int x1 = zhiWuX + 10;
			int x2 = zhiWuX + 60;
			int x3 = zms[i].x + 80;
			if (x3 > x1 && x3 < x2) {
				if (map[row][j].catched) {
					//zms[i].frameIndex++;
					//if (zms[i].frameIndex > 100) {
					map[row][j].deadTime++;
					if(map[row][j].deadTime > 100){
						map[row][j].deadTime = 0;
						map[row][j].type = 0;
						zms[i].eating = false;
						zms[i].frameIndex = 0;
						zms[i].speed = 1;
					}
				}
				else {
					map[row][j].deadTime = 0;
					map[row][j].catched = true;
					zms[i].speed = 0;
					zms[i].eating = true;
					zms[i].frameIndex = 0;
				}
			}
		}
	}
}

void collisionCheck() {
	checkBulletToZm(); //豌豆子弹对僵尸的碰撞检测
	checkZmToZhiWu(); //僵尸对植物的碰撞检测
}



void updateGame() {
	createZhiWu(); //创建植物

	createSunshine(); //创建阳光
	updateSunshine(); //更新阳光的状态

	createZM(); //创建僵尸
	updateZM(); //更新僵尸的状态

	shoot(); //发射豌豆子弹
	updateBullets(); //更新豌豆子弹

	collisionCheck(); //实现豌豆子弹和僵尸的碰撞检测

}

void startUI() {
	IMAGE imgBg, imgMenu1, imgMenu2;
	loadimage(&imgBg, "res/menu.png");
	loadimage(&imgMenu1, "res/menu1.png");
	loadimage(&imgMenu2, "res/menu2.png");

	bool flag = true;
	while (1) {
		BeginBatchDraw();
		putimage(0, 0, &imgBg);
		putimagePNG(474, 75, flag ? &imgMenu2 : &imgMenu1);

		ExMessage msg;
		if (peekmessage(&msg)) {
			if (msg.message == WM_LBUTTONDOWN &&
				msg.x > 474 && msg.x < 474 + 300 &&
				msg.y>75 && msg.y < 75 + 140) {
				flag = false;
			}
			else if (msg.message == WM_LBUTTONUP && flag == false) {
				EndBatchDraw();
				break;
			}
		}
		EndBatchDraw();
	}
}

void viewScence() {
	int xMin = WIN_WIDTH - imgBg.getwidth(); //900-1400
	vector2 points[9] = {
			{550,80},{530,160},{630,170},{530,200},{515,270},
			{565,370},{605,340},{705,280},{690,340} };

	int index[9];
	for (int i = 0; i < 9; i++) {
		index[i] = rand() % 11;
	}
	int count = 0;
	for (int x = 0; x >= xMin; x -= 2) {
		BeginBatchDraw();
		putimage(x, 0, &imgBg);
		count++;
		for (int i = 0; i < 9; i++) {
			putimagePNG(points[i].x - xMin + x, points[i].y, &imgZmStand[index[i]]);
			if (count >= 10) {
				index[i] = (index[i] + 1) % 11;
			}
		}
		if (count >= 10) count = 0;
		EndBatchDraw();
		Sleep(5);
	}
		
		//停留1s左右
		for (int i = 0; i < 100; i++) {
			BeginBatchDraw();
			putimage(xMin, 0, &imgBg);
			for (int i = 0; i < 9; i++) {
				putimagePNG(points[i].x , points[i].y, &imgZmStand[index[i]]);
					index[i] = (index[i] + 1) % 11;
			}
			EndBatchDraw();
			Sleep(30);
		}

		for (int x = xMin; x <= -112; x += 2) {
			BeginBatchDraw();
			putimage(x, 0, &imgBg);
			count++;
			for (int i = 0; i < 9; i++) {
				putimagePNG(points[i].x - xMin + x, points[i].y, &imgZmStand[index[i]]);
				if (count >= 10) {
					index[i] = (index[i] + 1) % 11;
				}
			}
			if (count >= 10) count = 0;
			EndBatchDraw();
			Sleep(3);
		}


}

void barsDown() {
	int height = imgBar.getheight();
	for (int y = -height; y <= 0; y++) {
		BeginBatchDraw();

		putimage(-112, 0, &imgBg);
		putimagePNG(250, y, &imgBar);

		for (int i = 0; i < ZHI_WU_COUNT; i++) {
			int x = 338 + i * 65;
			putimage(x, y + 6, &imgCards[i]);

		}
		EndBatchDraw();
		Sleep(10);
	}
}

bool checkOver() {
	int ret = false;
	if (gameStatus == WIN) {
		Sleep(1000);
		mciSendString("play res/win.mp3", 0, 0, 0);
		loadimage(0, "res/win2.png");
		ret = true;
		
	}
	else if (gameStatus == FAIL) {
		Sleep(1000);
		mciSendString("play res/lose.mp3", 0, 0, 0);
		loadimage(0, "res/fail2.png");
		ret = true;
		
	}
	return ret;
}

int main(void)
{
	gameInit();
	startUI();

	viewScence();

	barsDown();

	int timer = 0;
	bool flag = true;
	while (1) {
		userClick();
		timer += getDelay();
		if (timer > 10) {
			flag = true;
			timer = 0;
		}
		if (flag) {
			updateWindow();
			updateGame();
			flag = false;
			if (checkOver()) break;
		}
		

	
	}
	

	system("pause");
	return 0;
}