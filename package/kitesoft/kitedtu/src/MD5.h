/* 
* Copyright (c) 2008 - 2009, ʢ�ǿƼ� 
* All rights reserved. 
*  
* �ļ����ƣ�$HeadURL: https://codeserver/svn/KnitSystem/NewSystem/KnitSystem/MD5.h $ 
* ժ    Ҫ��
*  
* ��ǰ�汾��$Revision: 1861 $
* ����޸ģ�$Author: flyaqiao $
* �޸����ڣ�$Date:: 2011-06-11 15:52:55#$
*
*/ 
#ifndef _MD5_H_
#define _MD5_H_

//int MD5String(const char* inStr, char* outStr, int outLen = 34);
//int MD5String(const TCHAR* inStr, TCHAR* outStr, int outLen = 34);
int MD5Calc(unsigned char * inBuf, int inLen, unsigned int out[4]);

//BOOL MD5Check(char *md5string, char* string);
//BOOL MD5Check(LPCTSTR md5string, LPCTSTR string);
int MD5Check(unsigned char * inBuf, int inLen, unsigned int md5[4]);

#endif //_MD5_H_
