//指令格式
//尺寸 56
//颜色 RRGGBB
//单点上色：Y.X-0
//区域上色：Y1.X1-Y2.X2-0

#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <Windows.h>
#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")

struct Stroke
{
	int x1, y1, x2, y2;
	Gdiplus::ARGB c;
	std::string ToString()
	{
		std::ostringstream oss;
		if (x1 == x2 && y1 == y2)
			oss << y1 << "." << x1 << "-0";
		else
			oss << y1 << "." << x1 << "-" << y2 << "." << x2 << "-0";
		return oss.str();
	}
};
std::map<Gdiplus::ARGB, std::vector<Stroke>>colorPalette;
Gdiplus::ARGB bgColor = 0xFFFFFFFF;
int instructionsPerLine = 1;

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
	std::vector<std::string>v;
	std::ostringstream oss;
	oss << "尺寸 " << bmp.GetWidth();
	v.push_back(oss.str());
	for (auto p : colorPalette)
	{
		oss.str("");
		oss << "颜色 " << ColorToHTMLColor(p.first);
		v.push_back(oss.str());
		for (auto k : p.second)
			v.push_back(k.ToString());
	}
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
	if (argc < 2)
	{
		puts("命令行：pix2cmd <图片文件> <背景颜色(RGBA)=FFFFFFFF> <每行指令数=1>");
		return 1;
	}
	if (argc >= 3)
		bgColor = HTMLColorToColor(argv[2]);
	if (argc >= 4)
		instructionsPerLine = _wtoi(argv[3]);
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
		Bitmap bmp(argv[1], FALSE);//这玩意不能跟GdiplusShutdown放一个括号里
		if (bmp.GetLastStatus() != Ok)
		{
			puts("加载文件失败。");
		}
		else
		{
			cout << "宽：" << bmp.GetWidth() << " 高：" << bmp.GetHeight() << endl;
			//生成指令
			auto v = GenerateCommands(bmp);
			cout << "一共" << v.size() << "条指令。" << endl;
			int endlCounter = 0;
			for (int i = 0; i < v.size(); i++)
			{
				if (v[i][0] >= '0'&&v[i][0] <= '9')
				{
					if (endlCounter == instructionsPerLine)
						endlCounter = 0;
					if (endlCounter == 0)
						cout << endl;
					else
						cout << " ";
					endlCounter++;
				}
				else
				{
					cout << endl;
					endlCounter = 0;
				}
				cout << v[i];
			}
		}
	}
	GdiplusShutdown(token);
	return 0;
}
