#include "stdafx.h"
#include "Recognition.h"

using namespace cv;
using namespace std;

Mat Source;/*源图像*/
Mat Hsv_Source;/*HSV图像*/
Mat Gass_Source;/*高斯图像*/
Mat Red_Channel;/*红色通道*/
Mat Blue_Channel;/*蓝色通道*/
Mat Barrier_Channel;/*障碍物通道*/
Mat Black_Limit;//黑色限制图像
Mat White_Limit;//白色限制图像
Mat Out_Put;//最终输出图像

VideoCapture *Capture_Camera;//摄像头

Rect Red_Zone;//红色矩形区域
Rect Blue_Zone;//蓝色矩形区域
Rect Barrier_Zone;//障碍物矩形区域

int Display_Switch;//图像显示开关
int Camera_Num;//摄像头编号
int Target_Width;//目标宽度
int Target_Height;//目标高度
int Target;//是否找到目标
int Black_X;
int Black_Y;
int White_X;
int White_Y;

int H_Range_Min[5];//H下阈值
int H_Range_Max[5];//H上阈值
int S_Range_Min[5];//S下阈值
int S_Range_Max[5];//S上阈值
int V_Range_Min[5];//V下阈值
int V_Range_Max[5];//V上阈值

string Display_Title;//结果显示标题

vector<vector<Point>> Contours; //连通域

void Recognition::Json_Get()
{
	Json::Reader Config_Reader;   
	Json::Value Config_Value;       
	ifstream IFS;
	IFS.open("config.json", ios::binary);
	if (Config_Reader.parse(IFS, Config_Value, false))
	{
		printf_s("SETUP: Json file reading...\n\n");
		H_Range_Min[BLACKBALL] = Config_Value["H_Min_Black"].asInt();
		S_Range_Min[BLACKBALL] = Config_Value["S_Min_Black"].asInt();
		V_Range_Min[BLACKBALL] = Config_Value["V_Min_Black"].asInt();
		H_Range_Max[BLACKBALL] = Config_Value["H_Max_Black"].asInt();
		S_Range_Max[BLACKBALL] = Config_Value["S_Max_Black"].asInt();
		V_Range_Max[BLACKBALL] = Config_Value["V_Max_Black"].asInt();
		H_Range_Min[BARRIER] = Config_Value["H_Min_Barrier"].asInt();
		S_Range_Min[BARRIER] = Config_Value["S_Min_Barrier"].asInt();
		V_Range_Min[BARRIER] = Config_Value["V_Min_Barrier"].asInt();
		H_Range_Max[BARRIER] = Config_Value["H_Max_Barrier"].asInt();
		S_Range_Max[BARRIER] = Config_Value["S_Max_Barrier"].asInt();
		V_Range_Max[BARRIER] = Config_Value["V_Max_Barrier"].asInt();
		H_Range_Min[BLUEZONE] = Config_Value["H_Min_Blue"].asInt();
		S_Range_Min[BLUEZONE] = Config_Value["S_Min_Blue"].asInt();
		V_Range_Min[BLUEZONE] = Config_Value["V_Min_Blue"].asInt();
		H_Range_Max[BLUEZONE] = Config_Value["H_Max_Blue"].asInt();
		S_Range_Max[BLUEZONE] = Config_Value["S_Max_Blue"].asInt();
		V_Range_Max[BLUEZONE] = Config_Value["V_Max_Blue"].asInt();
		H_Range_Min[REDZONE] = Config_Value["H_Min_Red"].asInt();
		S_Range_Min[REDZONE] = Config_Value["S_Min_Red"].asInt();
		V_Range_Min[REDZONE] = Config_Value["V_Min_Red"].asInt();
		H_Range_Max[REDZONE] = Config_Value["H_Max_Red"].asInt();
		S_Range_Max[REDZONE] = Config_Value["S_Max_Red"].asInt();
		V_Range_Max[REDZONE] = Config_Value["V_Max_Red"].asInt();
		H_Range_Min[WHITEBALL] = Config_Value["H_Min_White"].asInt();
		S_Range_Min[WHITEBALL] = Config_Value["S_Min_White"].asInt();
		V_Range_Min[WHITEBALL] = Config_Value["V_Min_White"].asInt();
		H_Range_Max[WHITEBALL] = Config_Value["H_Max_White"].asInt();
		S_Range_Max[WHITEBALL] = Config_Value["S_Max_White"].asInt();
		V_Range_Max[WHITEBALL] = Config_Value["V_Max_White"].asInt();
		printf_s("SETUP: Json file read success!\n");
	}
	else
	{
		printf_s("WARING: Json file read failed!\n");
		printf_s("CUSTOM: Press any key to continue...\n");
		char s; scanf_s(&s);
	}
	IFS.close();
}

Recognition::Recognition(int camera_num, int display_sw)
{
	Display_Switch = display_sw;
	Camera_Num = camera_num;
	Capture_Camera = new VideoCapture(camera_num);
	Capture_Camera->read(Source);
	printf_s("SETUP: ");
	if (Capture_Camera->isOpened() == true)
	{
		printf_s("#");printf_s("%d", Camera_Num);printf_s(" camera is opened!\n");
	}
	else
	{
		printf_s("#");printf_s("%d", Camera_Num);printf_s(" camera is closed!\n");
		printf_s("WARNING: Press ant key to continue...\n");
		waitKey(0);
		scanf_s(&d);
	}
	for (int i = 0;i < 10;i++)
	{
		Capture_Camera->read(Source);
		waitKey(33);
	}
	Json_Get();
	waitKey(33);
}

Recognition::Recognition()
{
	Display_Switch = DISPLAY_OFF;
	Camera_Num = 0;
	Capture_Camera = new VideoCapture(0);
	Display_Title = "#0摄像头";
	Capture_Camera->read(Source);
	printf_s("SETUP: ");
	if (Capture_Camera->isOpened() == true)
	{
		printf_s("#0 camera is opened!\n");
	}
	else
	{
		printf_s("#0 camera is closed!\n");
		printf_s("Press ant key to continue...\n");
		char d;scanf_s("%s", &d);
	}
	for (int i = 0;i < 10;i++)
	{
		Capture_Camera->read(Source);
		waitKey(33);
	}
	waitKey(33);
}

void Recognition::Channel_Separation()
{
	//获取阈值颜色
	Scalar Black_Range_0(H_Range_Min[BLACKBALL], S_Range_Min[BLACKBALL], V_Range_Min[BLACKBALL]);
	Scalar Black_Range_1(H_Range_Max[BLACKBALL], S_Range_Max[BLACKBALL], V_Range_Max[BLACKBALL]);
	Scalar White_Range_0(H_Range_Min[WHITEBALL], S_Range_Min[WHITEBALL], V_Range_Min[WHITEBALL]);
	Scalar White_Range_1(H_Range_Max[WHITEBALL], V_Range_Max[WHITEBALL], V_Range_Max[WHITEBALL]);
	Scalar Blue_Range_Min(H_Range_Min[BLUEZONE], S_Range_Min[BLUEZONE], V_Range_Min[BLUEZONE]);
	Scalar Blue_Range_Max(H_Range_Max[BLUEZONE], S_Range_Max[BLUEZONE], V_Range_Max[BLUEZONE]);
	Scalar Red_Range_Min(H_Range_Min[REDZONE], S_Range_Min[REDZONE], V_Range_Min[REDZONE]);
	Scalar Red_Range_Max(H_Range_Max[REDZONE], S_Range_Max[REDZONE], V_Range_Max[REDZONE]);
	Scalar Barrier_Range_Min(H_Range_Min[BARRIER], S_Range_Min[BARRIER], V_Range_Min[BARRIER]);
	Scalar Barrier_Range_Max(H_Range_Max[BARRIER], S_Range_Max[BARRIER], V_Range_Max[BARRIER]);
	//通道分离
	inRange(Hsv_Source, Red_Range_Min, Red_Range_Max, Red_Channel);
	inRange(Hsv_Source, Blue_Range_Min, Blue_Range_Max, Blue_Channel);
	inRange(Hsv_Source, Black_Range_0, Black_Range_1, Black_Limit);
	inRange(Hsv_Source, White_Range_0, White_Range_1, White_Limit);
	inRange(Hsv_Source, Barrier_Range_Min, Barrier_Range_Max, Barrier_Channel);
	/*imshow("黑色通道", Black_Limit);
	imshow("白色通道", White_Limit);
	imshow("红色通道", Red_Channel);
	imshow("蓝色通道", Blue_Channel);
	imshow("障碍通道", Barrier_Channel);*/
}

double Recognition::Find_Black_Ball()
{
	
	findContours(Black_Limit, Contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE); //寻找连通域
	double Max_Area = 0;//最大连通域面积
	vector<cv::Point> Max_Contour;
	for (size_t i = 0; i < Contours.size(); i++)//寻找最大连通域
	{
		double area = contourArea(Contours[i]);
		if (area > Max_Area)
		{
			Max_Area = area;
			Max_Contour = Contours[i];
		}
	}
	Rect Max_Rect = boundingRect(Max_Contour);//最大连通域矩形

	if (Max_Area < BALL_SQUARE_MIN || Max_Area > BALL_SQUARE_MAX)
	{
		Black_X = -1;
		Black_Y = -1;
		return -1;
	}
	else
	{
		Black_X = Max_Rect.x;
		Black_Y = Max_Rect.y;
		return Max_Area;
	}
}

double Recognition::Find_White_Ball()
{
	findContours(White_Limit, Contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE); //寻找连通域
	double Max_Area = 0;//最大连通域面积
	vector<cv::Point> Max_Contour;
	for (size_t i = 0; i < Contours.size(); i++)//寻找最大连通域
	{
		double area = contourArea(Contours[i]);
		if (area > Max_Area)
		{
			Max_Area = area;
			Max_Contour = Contours[i];
		}
	}
	Rect Max_Rect = boundingRect(Max_Contour);//最大连通域矩形
	if (Max_Area < BALL_SQUARE_MIN || Max_Area > BALL_SQUARE_MAX)
	{
		White_X = -1;
		White_Y = -1;
		return -1;
	}
	else
	{
		White_X = Max_Rect.x;
		White_Y = Max_Rect.y;
		return Max_Area;
	}
}

double Recognition::Find_Zone(int channel)
{
	if (channel == REDZONE)
	{
		findContours(Red_Channel, Contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE); //寻找连通域
	}
	else if (channel == BARRIER)
	{
		findContours(Barrier_Channel, Contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE); //寻找连通域
	}
	else
	{
		findContours(Blue_Channel, Contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE); //寻找连通域
	}
	double Max_Area = 0;//最大连通域面积
	vector<cv::Point> Max_Contour;
	for (size_t i = 0; i < Contours.size(); i++)//寻找最大连通域
	{
		double area = contourArea(Contours[i]);
		if (area > Max_Area)
		{
			Max_Area = area;
			Max_Contour = Contours[i];
		}
	}
	Rect Max_Rect = boundingRect(Max_Contour);//最大连通域矩形
	if (Max_Area < 625 || Max_Rect.x < 50 || Max_Rect.x>590 || Max_Rect.y < 50 || Max_Rect.y>430)
	{
		X[channel] = -1;
		Y[channel] = -1;
		Square[channel] = -1;
		return TARGET_NOT_FOUND;
	}
	else
	{
		X[channel] = Max_Rect.x;
		Y[channel] = Max_Rect.y;
		Square[channel] = Max_Area;
		return TARGET_FOUND;
	}
}

void Recognition::Display_Result()
{
	stringstream SS;
	string str_tmp;
	SS << Camera_Num;
	SS >> str_tmp;
	Display_Title = "#" + str_tmp + "摄像头";
	Out_Put = Source.clone();
	if (Square[REDZONE] != -1)
	{
		Red_Zone.width = sqrt(Square[REDZONE]) * 0.8;
		Red_Zone.height = sqrt(Square[REDZONE]) / 0.8;
		Red_Zone.x = X[REDZONE];
		Red_Zone.y = Y[REDZONE];
		rectangle(Out_Put, Red_Zone, Scalar(0, 0, 255), 2, 8, 0);
	}
	if (Square[BLUEZONE] != -1)
	{
		Blue_Zone.width = sqrt(Square[BLUEZONE]) * 0.8;
		Blue_Zone.height = sqrt(Square[BLUEZONE]) / 0.8;
		Blue_Zone.x = X[BLUEZONE];
		Blue_Zone.y = Y[BLUEZONE];
		rectangle(Out_Put, Blue_Zone, Scalar(255, 0, 0), 2, 8, 0);
	}
	if (Square[BARRIER] != -1)
	{
		Barrier_Zone.width = sqrt(Square[BARRIER]) * 0.8;
		Barrier_Zone.height = sqrt(Square[BARRIER]) / 0.8;
		Barrier_Zone.x = X[BARRIER];
		Barrier_Zone.y = Y[BARRIER];
		rectangle(Out_Put, Barrier_Zone, Scalar(0, 100, 255), 2, 8, 0);
	}
	if (Square[BALL] != -1)
	{
		int circle_x = X[BALL] - sqrt(Square[BALL]) / 2;
		int circle_y = Y[BALL] - sqrt(Square[BALL]) / 2;
		int circle_r = sqrt(Square[BALL]) / 2;
		circle(Out_Put, Point(circle_x, circle_y), circle_r, Scalar(0, 0, 0), 2, 8, 0);
	}
	imshow(Display_Title, Out_Put);
	
}

int Recognition::Find_Ball()
{
	int square_temp1 = Find_Black_Ball();
	int x_temp = X[BALL];
	int y_temp = Y[BALL];
	int square_temp2 = Find_White_Ball();
	if (square_temp1 == -1 && square_temp2 == -1)
	{
		Square[BALL] = -1;
		return TARGET_NOT_FOUND;
	}
	else if (square_temp1 > square_temp2)
	{
		X[BALL] = x_temp;
		Y[BALL] = y_temp;
		Square[BALL] = square_temp1;
		return TARGET_FOUND;
	}
	else
	{
		Square[BALL] = square_temp2;
		return TARGET_FOUND;
	}
}

int Recognition::Recognize()
{
	Capture_Camera->read(Source);//读取摄像头
	GaussianBlur(Source, Gass_Source, Size(15, 15), 4, 4);//高斯模糊
	cvtColor(Gass_Source, Hsv_Source, COLOR_BGR2HSV);//RGB转HSV
	Channel_Separation();
	Find_Ball();//找球
	Find_Zone(BLUEZONE);//寻找蓝色区域
	Find_Zone(REDZONE);//寻找红色区域
	Find_Zone(BARRIER);//寻找障碍物
	if (Display_Switch == DISPLAY_ON)//显示图像
	{
		Display_Result();
	}
	return 0;
}


