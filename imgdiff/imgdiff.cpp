#include <iostream>
#include <Windows.h>
#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")

DWORD HTMLColorToColor(wchar_t str[])
{
	DWORD dw;
	swscanf_s(str, L"%08X", &dw);
	return dw;
}

DWORD bgColor = 0xFFFFFFFF;
std::wstring pathOut = L"diff.png";

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	using namespace Gdiplus;
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

int wmain(int argc, wchar_t*argv[])
{
	if (argc < 3)
	{
		puts("命令行：imgdiff <图片文件1> <图片文件2> <输出图片=diff.png> <背景颜色(ARGB)=FFFFFFFF>");
		return 1;
	}
	if (argc >= 4)
		pathOut = argv[3];
	if (argc >= 5)
		bgColor = HTMLColorToColor(argv[4]);
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
		Bitmap bmp1(argv[1], FALSE), bmp2(argv[2], FALSE);//这玩意不能跟GdiplusShutdown放一个括号里
		if (bmp1.GetLastStatus() != Ok)
		{
			puts("加载文件1失败。");
		}
		else if(bmp2.GetLastStatus()!=Ok)
		{
			puts("加载文件2失败。");
		}
		else
		{
			if (bmp1.GetWidth() != bmp2.GetWidth() || bmp1.GetHeight() != bmp2.GetHeight())
				puts("两个图像的尺寸不一致，只会比较重叠的部分。");
			int w = min(bmp1.GetWidth(), bmp2.GetWidth()), h = min(bmp1.GetHeight(), bmp2.GetHeight());
			for (int i = 0; i < h; i++)
			{
				for (int j = 0; j < w; j++)
				{
					Color c1, c2;
					bmp1.GetPixel(j, i, &c1);
					bmp2.GetPixel(j, i, &c2);
					if (c1.GetValue() == c2.GetValue())
						bmp1.SetPixel(j, i, bgColor);
				}
			}
			CLSID pngCls;
			GetEncoderClsid(L"image/png", &pngCls);
			bmp1.Save(pathOut.c_str(), &pngCls);
		}
	}
	GdiplusShutdown(token);
	return 0;
}
