#include <stdio.h> 
#include <memory.h>
#include "md5.h"
#include "generate.h"
#include "lutil.h"
  
int Check_CPU()
{
	union x
	{
		int a;
		char b;
	}c;
	c.a = 1;
	return(c.b == 1);
}

void Change_Endianness (long * src)
{
	int little_endian;
	little_endian = Check_CPU();
	/* there is no need to change endianness if the cpu is little endian mode */
	if(little_endian)
		return;

	unsigned char str[4],string[4];
	memcpy(str,src,4);
	int t,s;
	for(t=0,s=3;t<4;t++)
        {
		string[t]=str[s];
	        s--;
	}
	memcpy(src,string,4);
}

//__stdcall
int GenerateCrypt(char *szUser, 
							 char *szPassword, 
							 char *szChallenge64, 
                                                         long clientinfo,
                                                         long embkey,
							 char *szResult)
{
	unsigned char szDecoded[256];
	unsigned char szKey[256];
	unsigned char szAscii[256];

	unsigned int nDecodedLen;
	long challengetime = 0;
	int nMoveBits;
	long challengetime_new = 0;
	long a, b, c, d;
	unsigned int nKey;
	int nUser;
	unsigned int nEncoded;

	//Base64 ����
	nDecodedLen =  lutil_b64_pton(szChallenge64, szDecoded, 256);
	memcpy(&challengetime, szDecoded + 6, 4);

	Change_Endianness(&challengetime);
	
	//ȡ�����л�����
	challengetime |= ~embkey;

	//�õ�ѭ����λλ��
	nMoveBits = challengetime % 30;

	//���32λ��ѭ��λ��
	a = challengetime << ((32 - nMoveBits) % 32);
	b = challengetime >> (((unsigned int )nMoveBits) % 32);
	c = ~(0xffffffff << ((32 - nMoveBits) % 32));
	d = b & c;
	challengetime_new = a | d;

	//KEY-MD5
	nKey = KeyMD5Encode(szKey, (unsigned char*)szPassword, strlen((char*)szPassword), (unsigned char*)szDecoded, nDecodedLen);
	szKey[nKey] = 0;
	
	Change_Endianness(&clientinfo);
	Change_Endianness(&challengetime_new);

	nUser = strlen((char *)szUser);
	memcpy(szAscii, szUser, nUser);
	szAscii[nUser] = ' ';
	memcpy(szAscii+nUser+1, &challengetime_new,4);
	memcpy(szAscii+nUser+1+4,&clientinfo,4);
	memcpy(szAscii+nUser+1+4+4, szKey, nKey);
	//base64 ����
	nEncoded =  lutil_b64_ntop((unsigned char *)szAscii, nUser + 1 + 4 + 4 + nKey, szResult, 256);
	return nEncoded;
}
