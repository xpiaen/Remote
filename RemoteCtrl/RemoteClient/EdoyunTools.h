#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>

class CEdoyunTools
{
public:
    static void Dump(BYTE* pData, size_t nSize) {//将字节流转化为16进制字符串并输出到调试窗口
        std::string strOut;
        for (size_t i = 0; i < nSize; i++) {
            char buf[8] = "";
            if (i > 0 && (i % 16 == 0))strOut += "\n";
            snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
            strOut += buf;
        }
        strOut += '\n';
        OutputDebugStringA(strOut.c_str());
    }
    static int Bytes2Image(CImage& image, const std::string& strBuffer) {//将字节流转换为图片
		BYTE* pData = (BYTE*)strBuffer.c_str();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);//为图片数据申请内存
		if (hMem == NULL) {
			TRACE("内存不足！");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (hRet == S_OK) {
			ULONG length = 0;
			pStream->Write(pData, strBuffer.size(), &length);
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			if ((HBITMAP)image != NULL)image.Destroy();//释放原来的位图
			image.Load(pStream);
		}
		return hRet;
    }
};

