//指令格式
//尺寸 56
//颜色 RRGGBB
//单点上色：Y.X-0
//区域上色：Y1.X1-Y2.X2-0

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>
#include <Windows.h>
#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")

std::string AxisToPosCode(int n)
{
	if (n >= 9 && n < 26)
	{
		std::string s = "A";
		s[0] += n;
		return s;
	}
	return std::to_string(n + 1);
}

std::map<Gdiplus::ARGB, int>predefinedColorsIndex;
struct Stroke
{
	int x1, y1, x2, y2;
	Gdiplus::ARGB c;
	std::string ToString()
	{
		std::ostringstream oss;
		if (x1 == x2 && y1 == y2)
			oss << AxisToPosCode(y1) << "." << AxisToPosCode(x1) << "-";
		else
			oss << AxisToPosCode(y1) << "." << AxisToPosCode(x1) << "-" << AxisToPosCode(y2) << "." << AxisToPosCode(x2) << "-";
		if (predefinedColorsIndex.find(c) == predefinedColorsIndex.end())
			oss << "0";
		else
			oss << predefinedColorsIndex[c];
		return oss.str();
	}
};

Gdiplus::ARGB predefinedColors[16] = { 0xFFFFFFFF,
	0xFFE53A3F,0xFFFC910C,0xFFFCE003,0xFFF1F1CD,0xFFBFD40F,
	0xFF0EAC3C,0xFF76FBD3,0xFF0046AE,0xFF4F5298,0xFFBC209E,
	0xFFFAB1CD,0xFF6F462D,0xFF000000,0xFF808080,0xFFFFFFFF
};
std::map<Gdiplus::ARGB, std::vector<Stroke>>colorPalette;
Gdiplus::ARGB bgColor = 0xFFFFFFFF;
int maxCharPerLine = 20;

//一维装箱问题：https://blog.csdn.net/weixin_43867940/article/details/113835757
std::vector<std::string> MinLineCommands(std::vector<std::string>v)
{
	//元启发式算法（Greedy）
	//时间复杂度O(N^2)
	using namespace std;
	//字符串由长到短排序
	sort(v.begin(), v.end(), [](string a, string b)->bool{return a.length() > b.length(); });
	vector<string>buckets{""};
#define leftlength(i)(maxCharPerLine+1-buckets[i].length())
#define itemlength(i)(v[i].length()+1)
	for (int i = 0; i < v.size();)//后遍历字符串
	{
		int choose = -1;
		for (int j = 0; j < buckets.size(); j++)//先遍历容器
		{
			if (leftlength(j) >= itemlength(i) &&//这个容器可以容下当前字符串
				(choose == -1 || leftlength(j) < leftlength(choose)))//且该容器比之前选择的容器剩余空间小或者未选择容器
				choose = j;
		}
		if (choose == -1)//没有合适的容器
			buckets.push_back("");
		else//在选择的容器中存下当前字符串
			buckets[choose] += v[i++] + " ";
	}
	//去除结尾多余空格
	for (int i = 0; i < buckets.size();)
	{
		while (buckets[i].size()&&buckets[i].back() == ' ')
			buckets[i].pop_back();
		if (buckets[i].empty())
			buckets.erase(buckets.begin() + i);
		else
			i++;
	}
	return buckets;
}

std::string ColorToHTMLColor(Gdiplus::Color c)
{
	char fmt[7];
	sprintf_s(fmt, "%02X%02X%02X", c.GetR(), c.GetG(), c.GetB());
	return std::string(fmt);
}

std::vector<std::string> GenerateCommands(Gdiplus::Bitmap&bmp)
{
	using namespace Gdiplus;
	UINT w = bmp.GetWidth(), h = bmp.GetHeight();
	std::vector<std::vector<bool>>covered(h, std::vector<bool>(w, false));
	for (UINT i = 0; i < h; i++)
	{
		for (UINT j = 0; j < w; j++)
		{
			Color c,cNext;
			bmp.GetPixel(j, i, &c);
			ARGB argb = c.GetValue();
			Stroke st{ j,i,j,i,argb };
			if (argb == bgColor)
			{
				covered[i][j] = true;
			}
			else if(!covered[i][j])
			{
				//向右遍历
				while (st.x2+1<w)
				{
					bmp.GetPixel(st.x2 + 1, st.y2, &cNext);
					if (argb == cNext.GetValue())
					{
						st.x2++;
						covered[st.y2][st.x2] = true;
					}
					else
					{
						break;
					}
				}
				//向下遍历
				bool success = true;
				while (st.y2 + 1 < h)
				{
					for (int k = st.x1; k <= st.x2; k++)
					{
						bmp.GetPixel(k, st.y2 + 1, &cNext);
						if (argb != cNext.GetValue())
						{
							success = false;
							break;
						}
					}
					if (success)
					{
						st.y2++;
						for (int k = st.x1; k <= st.x2; k++)
							covered[st.y2][k] = true;
					}
					else
					{
						break;
					}
				}
				colorPalette[argb].push_back(st);
			}
		}
	}
	std::vector<std::string>v, predefinedColorCommands, customColorCommands;
	std::ostringstream oss;
	oss << "尺寸 " << bmp.GetWidth();
	v.push_back(oss.str());
	for (auto p : colorPalette)
	{
		if (predefinedColorsIndex.find(p.first) == predefinedColorsIndex.end())
		{
			oss.str("");
			oss << "颜色 " << ColorToHTMLColor(p.first);
			v.push_back(oss.str());
			for (auto k : p.second)
				customColorCommands.push_back(k.ToString());
			customColorCommands = MinLineCommands(customColorCommands);
			v.insert(v.end(), customColorCommands.begin(), customColorCommands.end());
			customColorCommands.clear();
		}
		else
		{
			for (auto k : p.second)
				predefinedColorCommands.push_back(k.ToString());
		}
	}
	predefinedColorCommands = MinLineCommands(predefinedColorCommands);
	v.insert(v.end(), predefinedColorCommands.begin(), predefinedColorCommands.end());
	return v;
}

DWORD HTMLColorToColor(wchar_t str[])
{
	DWORD dw;
	swscanf_s(str, L"%08X", &dw);
	return dw;
}

int wmain(int argc, wchar_t*argv[])
{
	TCHAR srcFile[MAX_PATH] = TEXT("");
	bool outputToFile = false;
	if (argc < 2)
	{
		puts("命令行：pix2cmd <图片文件> <背景颜色(ARGB)=FFFFFFFF> <每行最大字数=20>");
		if (wcschr(argv[0], ':') == NULL)
			return 1;
		outputToFile = true;
		OPENFILENAME ofn = { sizeof(OPENFILENAME),NULL,NULL,TEXT("图片文件(GDI+)\0*.bmp;*.jpg;*.jpeg;*.png;*.gif;*.tif;*.tiff\0所有文件\0*\0\0"),
		NULL,NULL,0,srcFile,ARRAYSIZE(srcFile),NULL,NULL,NULL,TEXT("选择要转换的图片"),OFN_FILEMUSTEXIST ,0,0,NULL,NULL,NULL,NULL };
		if (!GetOpenFileName(&ofn))
			return 2;
	}
	else
		lstrcpy(srcFile, argv[1]);
	if (argc >= 3)
		bgColor = HTMLColorToColor(argv[2]);
	if (argc >= 4)
		maxCharPerLine = _wtoi(argv[3]);
	for (int i = 1; i < ARRAYSIZE(predefinedColors); i++)
		predefinedColorsIndex.insert(std::make_pair(predefinedColors[i], i));
	using namespace std;
	using namespace Gdiplus;
	ULONG_PTR token;
	GdiplusStartupInput gdipInput;
	if (GdiplusStartup(&token, &gdipInput, NULL) != Ok)
	{
		puts("Gdiplus 加载失败。");
		return -2;
	}
	{
		//加载图像
		Bitmap bmp(srcFile, FALSE);//这玩意不能跟GdiplusShutdown放一个括号里
		if (bmp.GetLastStatus() != Ok)
		{
			puts("加载文件失败。");
		}
		else
		{
			//生成指令
			if (outputToFile)
			{
				cout << "宽：" << bmp.GetWidth() << " 高：" << bmp.GetHeight() << endl << "正在转换……" << endl;
				//生成指令
				auto v = GenerateCommands(bmp);
				cout << "一共" << v.size() << "行指令。" << endl;
				lstrcat(srcFile, TEXT("_pix2cmd.txt"));
				ofstream f(srcFile);
				for (auto s : v)
					f << s << endl;
				ShellExecute(NULL, TEXT("open"), srcFile, NULL, NULL, SW_SHOW);
			}
			else
			{
				for (auto s : GenerateCommands(bmp))
					cout << s << endl;
			}
		}
	}
	GdiplusShutdown(token);
	return 0;
}
