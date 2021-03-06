// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 CPPGETDATA_EXPORT
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// CPPGETDATA_API 函数视为自 DLL 导入，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <map>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <memory>
#ifdef CPPGETDATA_EXPORTS
#define CPPGETDATA_API __declspec(dllexport)
#else
#define CPPGETDATA_API __declspec(dllimport)
#endif
#define LL long long

const int MAXR = 1000;
const int CarNum = 10357;
const int PointNum = 27161;
using namespace std;

int tt[3];
//extern CPPGETDATA_API vector<MetaData> carData[10500];
// 此类导出自 CppGetData.dll

class Mypair {
public:
	int first, second;
};
class Coordinate {
public:
	int longitude, latitude;//所有的坐标点的值都乘以1000
};
class Position {
public:
	int longitude, latitude;
	vector<Mypair> cars;//first代表汽车id，second代表经过的时间
}point[PointNum + 5];//下标从0开始
class CarInfo {
public:
	vector<int> time, pos;//分别表示经过的时间和点的序列号，时间遵循从小到大
}carInfo[CarNum + 5];//下标从0开始
map<LL, int> pointMp;//根据点坐标查询点的ID，有需要可以用上
vector<int> showMap[PointNum];//这里面的数据是交给前端展示用的，以邻接表的方式存储，下标从0开始，下标为点ID
int vis[PointNum];
vector<int> timeInfo[10357 + 10];        //用于与showMap对应，记录下经过对应点的时间 
map<int, int> showMapFre[PointNum];//存储每条线经过次数，下标是点id，second是频率
struct Graph {
	vector<int> time, number;
}graph;                      //用于记录carCount中的折线图，time是时间分段信息(时间戳形式)，number是每一时间段对应车数量 
class CarFre {
public:
	int from, to, fre;//起点终点和频率
	bool operator < (const CarFre & e) const {
		return fre > e.fre;
	}
};
vector<CarFre> carFreQue;

string carCountString;
string tranAnswerOutString, tranAnswerInString;

class CPPGETDATA_API CCppGetData
{
public:
	CCppGetData(void) {
		roaded = 0;
	}
	//文件输出debug专用函数
	void debug(string str) {
		string FILENAME = address + "\\debug.txt";
		ofstream outstuf(FILENAME, ios::out);
		outstuf << str;
		outstuf.close();
	}

	//将结果数组转化成字符串
	string tranAnswerOut(vector<Mypair> & ans,int timeStart, int interval)
	{
		stringstream stream;
		stream << "[{\"relevanceOut\":[";
		int i;
		for (i = 0; i<ans.size(); i++)
		{
			stream << "[" << timeStart + i * interval << "," << ans[i].first << "],";
		}

		string str = stream.str();
		str[str.size() - 1] = ']';
		str += "}]";

		//ofstream outstuf("F:\\map.txt", ios::out);
		//outstuf << str;
		//outstuf.close();
		return str;
	}
	string tranAnswerIn(vector<Mypair> & ans, int timeStart, int interval)
	{
		stringstream stream;
		stream << "[{\"relevanceIn\":[";
		int i;
		for (i = 0; i<ans.size(); i++)
		{
			stream << "[" << timeStart + i * interval << "," << ans[i].second << "],";
		}

		string str = stream.str();
		str[str.size() - 1] = ']';
		str += "}]";

		//ofstream outstuf("F:\\map.txt", ios::out);
		//outstuf << str;
		//outstuf.close();
		return str;
	}
	//将要显示的showMap数组以string方式返回
	string traShowMap() {
		stringstream stream;
		stream << "[{\"car\":[";
		int i;
		for (i = 0; i<PointNum; i++)
		{
			int vis[PointNum + 5];
			memset(vis, 0, sizeof(vis));
			int e = showMap[i].size();
			for (int j = 0; j < e; j++) {
				if (vis[showMap[i][j]]) { continue; }//跳过重复的连线
				vis[showMap[i][j]] = 1;
				stream << "[" << point[i].longitude << "," << point[i].latitude << ","
					<< point[showMap[i][j]].longitude << "," << point[showMap[i][j]].latitude << "," << 1 << "],";
			}
		}
		string str = stream.str();
		str[str.size() - 1] = ']';
		str += "}]";

		//ofstream outstuf("F:\\map.txt", ios::out);
		//outstuf << str;
		//outstuf.close();
		return str;
	}

	string traShowPoint() {
		carCountNum = 0;
		stringstream stream;
		stream << "[{\"point\":[";
		int i;
		for (i = 0; i<PointNum; i++)
		{
			if (showMap[i].size()>0) {
				stream << "[" << point[i].longitude << "," << point[i].latitude << "],";
			}
			++carCountNum;
		}
		string str = stream.str();
		str[str.size() - 1] = ']';
		str += "}]";

		//ofstream outstuf("F:\\point.txt", ios::out);
		//outstuf << str;
		//outstuf.close();
		return str;
	}

	//F3把图表的数组表示转化成字符串格式
	string getGraph() {
		stringstream stream;
		stream << "[{\"point\":[";
		for (int i = 0; i < graph.time.size(); ++i)
		{
			stream << "[" << graph.time[i] << "," << graph.number[i] << "],";
		}
		string str = stream.str();
		str[str.size() - 1] = ']';
		str += "}]";
		return str;
	}

	//从文件中加载数据
	int LoadData() {//address是加载文件data.txt的绝对目录，此函数完成加载数据以及将数据整理后存入全局变量中
		if (roaded == 1)//防止重复加载，会gg的
			return 0;
		roaded = 1;

		string FILENAME = address + "\\data.txt";
		FILE * f = fopen(FILENAME.c_str(), "rb");
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);
		char* s = new char[fsize + 1];
		fread(s, fsize, 1, f);
		fclose(f);
		s[fsize] = 0;
		string ss = s;
		delete[] s;
		istringstream instuf(ss);

		//strcpy_s(buffer, address);
		//strcat_s(buffer, "\\data.txt");
		//buffer = "F:\\data.txt";
		//ifstream instuf(buffer.c_str(), ios::in);

		//instuf.sync_with_stdio(false);
		//instuf.tie(0);
		//if (!instuf)
		//{
		//	
		//	return 0;//文件不存在
		//}
		int pn, ind, n, time, tmp;
		LL x;
		instuf >> pn;

		for (int i = 0; i < pn; i++) {
			instuf >> x;
			pointMp[x] = i;//后面优化必须用上
			point[i].longitude = x / 100000;
			point[i].latitude = x % 100000;
		}

		instuf >> tmp;
		int lastInd;
		for (int i = 0; i < 10357; i++) {
			instuf >> x >> n;
			for (int j = 0; j < n; j++) {
				instuf >> time >> ind;
				carInfo[i].time.push_back(time);
				carInfo[i].pos.push_back(ind);
				//point[ind].cars.push_back({ time,ind });
				//if (j) {
				//	showMapFre[lastInd][ind]++;//记录线的频率
				//}
				//lastInd = ind;
			}
		}

		//freStat();//F7需要
		return CarNum;
	}

	int timeConversion(int day, int hour, int minu, int sec) {
		return (day - 2) * 86400 + hour * 3600 + minu * 60 + sec;
	}

	//查找时间
	//若返回false，则表明[start,end]时间段不包含在里面
	//若返回true，则表明包含，并且res[1]对应start的index，res[2]对应end的index
	bool time_find(vector<int> a, int start, int end, int res[])
	{
		if (a.size() == 0)
		{
			return false;
		}
		//时间间隔为0
		if (start >= end)	return false;

		int low = 0;
		int high = a.size() - 1;

		while (a[low] < start)	low++;
		res[1] = low;

		while (a[high] > end)	high--;
		res[2] = high;

		return true;

	}

	//判断一个点是否在某个矩形框之内
	//posa为左上角 posb为右下角
	bool isInside(int lon, int lat, Coordinate posa, Coordinate posb)
	{

		//cout << "isInside" << endl;
		//cout << "lat= " << lat << "  lon= " << lon << endl;

		//处理为整数
		int ax = posa.longitude;
		int bx = posb.longitude;
		int ay = posa.latitude;
		int by = posb.latitude;

		//cout << "ax = " << ax << endl;
		//cout << "bx = " << bx << endl;
		//cout << "ay = " << ay << endl;
		//cout << "by = " << by << endl;

		if ((lon >= ax) && (lon <= bx) && (lat <= ay) && (lat >= by))
		{
			//cout << "在里面" << endl;
			return true;
		}


		else return false;

	}

	//区域关联1
	//interval为时间间隔
	void relevance(Coordinate posa_1, Coordinate posb_1, Coordinate posa_2, Coordinate posb_2,
		int timeStart, int timeEnd, int interval)
	{

		//读取记录
		ifstream fin;
		fin.open("D:\\2.txt", ios::in);
		double temp1, temp2;
		fin >> temp1 >> temp2;
		int xxx = (int)(temp1 * 1000);
		//debug(std::to_string(xxx));
		posa_1.longitude = xxx;
		//debug(std::to_string(posa_1.longitude));
		posa_1.latitude = (int)(temp2 * 1000);

		fin >> temp1 >> temp2;
		posb_1.longitude = (int)(temp1 * 1000);
		posb_1.latitude = (int)(temp2 * 1000);

		fin >> temp1 >> temp2;
		posa_2.longitude = (int)(temp1 * 1000);
		posa_2.latitude = (int)(temp2 * 1000);

		fin >> temp1 >> temp2;
		posb_2.longitude = (int)(temp1 * 1000);
		posb_2.latitude = (int)(temp2 * 1000);

		//结果数据
		vector<Mypair> ans;
		//debug(std::to_string(posa_1.longitude));
		//debug(std::to_string(posa_1.latitude));
		//debug(std::to_string(posb_1.longitude));
		//debug(std::to_string(posb_1.latitude));
		//debug(std::to_string(posa_2.longitude));
		//debug(std::to_string(posa_2.latitude));
		//debug(std::to_string(posb_1.longitude));
		//debug(std::to_string(posb_2.latitude));
		//两个不合理情况
		if (timeStart > timeEnd)	return ;
		if (interval <= 0)	return ;

		//求出总的分的时间段
		int cnt = (timeEnd - timeStart) / interval;

		//一开始每个小的时间段的进出数目都赋值为0
		for (int i = 0; i < cnt; i++)
		{
			Mypair tempMypair;
			tempMypair.first = 0;		//这里的first和second
			tempMypair.second = 0;
			ans.push_back(tempMypair);
		}


		//遍历所有车辆信息
		for (int car = 0; car < CarNum; car++)
		{
			if (carInfo[car].time.size() == 0)	continue;
			int low = 0;
			int high = carInfo[car].time.size() - 1;
			if (carInfo[car].time[0] >= timeEnd || carInfo[car].time[high] <= timeStart)
			{
				//和要求的时间段没有交集
				continue;
			}

			//寻找交集的时间段
			int interStartTime = max(carInfo[car].time[low], timeStart);
			int interEndTime = min(carInfo[car].time[high], timeEnd);

			int t[3];
			//判断往来
			bool one = false;
			int index_one = 0;
			bool two = false;
			int index_two = 0;

			if (time_find(carInfo[car].time, interStartTime, interEndTime, t))
			{
				for (int i = t[1]; i <= t[2]; i++)
				{
					//找到位置信息
					int index = carInfo[car].pos[i];
					if (isInside(point[index].longitude, point[index].latitude, posa_1, posb_1))
					{
						//如果该点在第一个矩形框之内
						one = true;
						index_one = i;
					}
					else if (isInside(point[index].longitude, point[index].latitude, posa_2, posb_2))
					{
						//如果该点在第二个矩形框之内
						two = true;
						index_two = i;
					}

					//存在一进一出的情况
					if (one && two)
					{
						if (index_one < index_two)
						{//从矩形一出来，进入矩形二

						 //计算时间段
							int t1 = (carInfo[car].time[index_one] - timeStart) / interval;
							ans[t1].first++;
						}
						else if (index_one > index_two)
						{//从矩形二出来，进入矩形一

						 //计算时间段
							int t2 = (carInfo[car].time[index_two] - timeStart) / interval;
							ans[t2].second++;
						}

						one = false;
						two = false;
					}

				}
			}

		}
		tranAnswerOutString = tranAnswerOut(ans, timeStart, interval);
		tranAnswerInString = tranAnswerIn(ans, timeStart, interval);
		//debug(temp);
		return;
	}
	//区域关联2
	//interval为时间间隔
	void inOut(Coordinate posa, Coordinate posb, int timeStart, int timeEnd, int interval)
	{

		//结果数据
		vector<Mypair> ans;
		int getone = 0;

		//两个不合理情况
		if (timeStart > timeEnd)	return ;
		if (interval <= 0)	return ;

		//求出总的分的时间段
		int cnt = (timeEnd - timeStart) / interval;

		//一开始每个小的时间段的进出数目都赋值为0
		for (int i = 0; i < cnt; i++)
		{
			Mypair tempMypair;
			tempMypair.first = 0;
			tempMypair.second = 0;
			ans.push_back(tempMypair);
		}
		cout << "段数为：" << cnt << endl;

		//遍历所有车辆信息
		for (int car = 0; car < CarNum; car++)
		{
			if (carInfo[car].time.size() == 0)	continue;
			int low = 0;
			int high = carInfo[car].time.size() - 1;
			if (carInfo[car].time[0] >= timeEnd || carInfo[car].time[high] <= timeStart)
			{
				//和要求的时间段没有交集
				continue;
			}

			//寻找交集的时间段
			int interStartTime = max(carInfo[car].time[low], timeStart);
			int interEndTime = min(carInfo[car].time[high], timeEnd);

			int t[3];
			//判断出入
			bool out = false;
			bool in = false;
			int index_out = 0;
			int index_in = 0;

			if (time_find(carInfo[car].time, interStartTime, interEndTime, t))
			{
				for (int i = t[1]; i <= t[2]; i++)
				{
					//找到位置信息
					int index = carInfo[car].pos[i];
					if (isInside(point[index].longitude, point[index].latitude, posa, posb))
					{
						in = true;
						index_in = i;
						//cout << "indext_in= " << i << endl;
					}
					else
					{
						out = true;
						index_out = i;
						//cout << "index_out= " << i << endl;
					}

					if (in && out)
					{
						//cout << "get one " << endl;
						getone++;
						if (index_in < index_out)
						{//从矩形中出来

						 //计算时间段
							int t1 = (carInfo[car].time[index_in] - timeStart) / interval;
							//cout << "t1= " << t1 << endl;
							ans[t1].first++;
						}
						else if (index_in > index_out)
						{//进入到矩形中
						 //计算时间段
							int t2 = (carInfo[car].time[index_out] - timeStart) / interval;
							//cout << "t2= " << t2 << endl;
							ans[t2].second++;
						}

						in = false;
						out = false;

					}


				}
			}

		}

		//cout << "getone= " << getone << endl;
		//cout << "计算结束" << endl;
		tranAnswerOutString = tranAnswerOut(ans, timeStart, interval);
		tranAnswerInString = tranAnswerIn(ans, timeStart, interval);
		return;
	}
	string getTranAnswerOutString() {
		return tranAnswerOutString;
	}
	string getTranAnswerInString() {
		return tranAnswerInString;
	}

	//展示id为从start到end的汽车轨迹
	void showTrack(int start, int end) {           //查找start到end号和其之间的所有车
		start--, end--;//我们的carID从0开始
		for (int i = 0; i < PointNum; ++i)             //先清空数据  
			showMap[i].clear();

		if (start < 0 || end >= 10357) {                       //判断用户输入的车辆范围是否合乎标准 
			cout << "请输入合理的车辆编号信息" << endl;
			return;
		}

		for (int i = start; i <= end; ++i)
		{
			if (carInfo[i].pos.size() == 0) continue;
			int tmp = carInfo[i].pos[0];
			for (int j = 1; j < carInfo[i].pos.size(); ++j)
			{
				showMap[min(tmp, carInfo[i].pos[j])].push_back(max(tmp, carInfo[i].pos[j]));
				tmp = carInfo[i].pos[j];
			}
		}
		return;
	}

	//结果存储到showMap中，邻接表的方式存储，posa为左上角点，posb为右下角点
	string carCount(Coordinate posa, Coordinate posb, int timeStart, int timeEnd) {
		memset(vis, 0, sizeof(vis));

		//读取记录
		ifstream fin;
		fin.open("D:\\3.txt");
		double temp1, temp2;
		fin >> temp1 >> temp2;
		posa.longitude = (int)(temp1 * 1000);
		posa.latitude = (int)(temp2 * 1000);

		fin >> temp1 >> temp2;
		posb.longitude = (int)(temp1 * 1000);
		posb.latitude = (int)(temp2 * 1000);

		for (int i = 0; i < 10358; ++i)             //先清空数据  
		{
			showMap[i].clear();
			timeInfo[i].clear();
		}

		graph.number.clear();
		graph.time.clear();

		for (int i = 0; i < 10358; ++i) {           //得到showMap 
			for (int j = 0; j < carInfo[i].pos.size(); ++j) {

				if (carInfo[i].time[j] > timeEnd) break;           //时间顺序排列，如果i车经过点pos[j]的时间time[j]大于timeEnd,那么其后所有点的时间都大于timeEnd 

				if (carInfo[i].time[j] >= timeStart && carInfo[i].time[j] <= timeEnd) {               //i车经过点pos[j]的时间time[j]  (先判断时间)

					int pointID = carInfo[i].pos[j];            //i车经过的第j个点的序号 

					if (point[pointID].longitude >= posa.longitude && point[pointID].longitude <= posb.longitude && point[pointID].latitude <= posa.latitude && point[pointID].latitude >= posb.latitude)
					{
						/*
						判断该点是否在矩形内
						*/
						vis[pointID] ++;
						showMap[i].push_back(carInfo[i].pos[j]);
						timeInfo[i].push_back(carInfo[i].time[j]);

					}
				}
			}
		}

		int tempTime = timeStart;

		while (tempTime < timeEnd) {
			graph.time.push_back(tempTime);
			tempTime += 3600;                                //每隔半小时分一次段（此处可优化，或可设置为用户自行设置） 
		}

		graph.time.push_back(timeEnd);

		for (int i = 0; i < graph.time.size(); ++i)           //时间段， 统计每一个时间点的数目 
		{
			int singleNumber = 0;
			for (int j = 0; j < 10358; ++j) {                 //车总数 

				for (int k = 0; k < showMap[j].size(); ++k) {

					if (timeInfo[j][k] >= graph.time[i] - 600 && timeInfo[j][k] <= graph.time[i] + 600) {          //统计时间点前后5分钟之内的数量 
						singleNumber++;
						break;
					}
				}
			}
			graph.number.push_back(singleNumber);
		}
		stringstream stream;
		stream << "[{\"carCount\":[";
		for (int i = 0; i < PointNum; i++) {
			if (vis[i]) {
				stream << "[" << point[i].longitude << "," << point[i].latitude << "," << vis[i] << "],";
			}
		}
		carCountString = stream.str();
		carCountString[carCountString.size() - 1] = ']';
		carCountString += "}]";
		return getGraph();
	}
	string getCarCountString() {
		return carCountString;
	}

	//获取程序当前目录
	void getPath(char * str) {
		address = str;
	}

	//实现拓展功能F4,r值为5的倍数，最小为10，最大值为1000，边上点，往左上角区域归并（即lo）
	void carDensity(Coordinate posa, Coordinate posb, int timeStart, int timeEnd, int r) {
		string FILENAME = address + "\\chat.txt";
		ofstream outstuf(FILENAME, ios::out);
		int lo = posb.longitude - posa.longitude;
		int la = posa.latitude - posb.latitude;
		outstuf << lo / r << " " << la / r << "\n";
		for (int i = 0; i < lo / r; i++) {
			for (int j = 0; j < la / r; j++) {
				carDenString[i][j] = carCount(Coordinate({ posa.longitude + i * r , posa.latitude - j * r }),
					Coordinate({ posa.longitude + i * r + r , posa.latitude - j * r - r }), timeStart, timeEnd);
				outstuf << carDenString[i][j] << "\n";
			}
		}
		outstuf.close();
	}
	string getCarDensity(int i, int j) {
		return carDenString[i][j];
	}

	//F7实现
	string topFreCar(int k) {
		stringstream stream;
		stream << "[{\"fre\":[";
		int i;
		for (i = 0; i<k; i++)
		{
			stream << "[" << carFreQue[i].from << "," << carFreQue[i].to << ","
				<< carFreQue[i].fre << "],";
		}
		string str = stream.str();
		str[str.size() - 1] = ']';
		str += "}]";

		return str;
	}

	//统计各个线的频率并且排序好
	void freStat() {
		for (int i = 0; i < PointNum; i++) {
			map<int, int>::iterator iter;
			iter = showMapFre[i].begin();
			while (iter != showMapFre[i].end()) {
				insertTopFreCar(i, iter->first, iter->second);
				iter++;
			}
		}
	}

	//将每条线的频率二分插入，从大到小存储到向量carFreQue中
	void insertTopFreCar(int from, int to, int fre) {
		CarFre tmp = CarFre({ from,to,fre });
		carFreQue.insert(lower_bound(carFreQue.begin(), carFreQue.end(), tmp), tmp);//二分插入
	}

	int roaded;//标记是否加载过文件，防止重复加载
	string buffer;
	CCppGetData * tmp;
	string address;
	int carCountNum;
	string carDenString[MAXR][MAXR];
};

extern CPPGETDATA_API int nCppGetData;

CPPGETDATA_API int fnCppGetData(void);
