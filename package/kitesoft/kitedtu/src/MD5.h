/* 
* Copyright (c) 2008 - 2009, 盛星科技 
* All rights reserved. 
*  
* 文件名称：$HeadURL: https://codeserver/svn/KnitSystem/NewSystem/KnitSystem/MD5.h $ 
* 摘    要：
*  
* 当前版本：$Revision: 1861 $
* 最后修改：$Author: flyaqiao $
* 修改日期：$Date:: 2011-06-11 15:52:55#$
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
