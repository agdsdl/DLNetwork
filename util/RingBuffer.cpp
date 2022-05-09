#include "RingBuffer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>

using namespace DLNetwork;

RingBuffer::~RingBuffer()
{
}

/*
* 初始化环形缓冲区
* 环形缓冲区这里可以是malloc申请的内存,也可以是Flash存储介质
* */

RingBuffer::RingBuffer(char* buf, size_t size)
{
	_pHead = buf;
	_pValid = _pValidTail = _pHead;
	_pTail = _pHead + size;
	_validLen = 0;
	_capacity = size;
}

/*
* function:向缓冲区中写入数据
* param:@buffer 写入的数据指针
* 		 @addLen 写入的数据长度
* return:-1:写入长度过大
* 		  -2:缓冲区没有初始化
* */

int RingBuffer::write(const char * buffer, int addLen)
{
	if (addLen > _capacity-_validLen) return -1;

	//将要存入的数据copy到_pValidTail处
	if (_pValidTail + addLen > _pTail)//需要分成两段copy
	{
		int len1 = _pTail - _pValidTail;
		int len2 = addLen - len1;
		memcpy(_pValidTail, buffer, len1);
		memcpy(_pHead, buffer + len1, len2);
		_pValidTail = _pHead + len2;//新的有效数据区结尾指针
	}
	else {
		memcpy(_pValidTail, buffer, addLen);
		_pValidTail += addLen;//新的有效数据区结尾指针
	}

	_validLen += addLen;

	return addLen;
}

/*
* function:从缓冲区内取出数据
* param   :@buffer:接受读取数据的buffer
*		    @len:将要读取的数据的长度
* return  :-1:没有初始化
*	 	    >0:实际读取的长度
* */
int RingBuffer::read(char * buffer, int len)
{
	if (_validLen == 0) return 0;

	if (len > _validLen) len = _validLen;

	if (_pValid + len > _pTail)//需要分成两段copy
	{
		int len1 = _pTail - _pValid;
		int len2 = len - len1;
		memcpy(buffer, _pValid, len1);//第一段
		memcpy(buffer + len1, _pHead, len2);//第二段，绕到整个存储区的开头
		_pValid = _pHead + len2;//更新已使用缓冲区的起始
	}
	else if (_pValid + len == _pTail) {
		memcpy(buffer, _pValid, len);//第一段
		_pValid = _pHead;
	}
	else {
		memcpy(buffer, _pValid, len);
		_pValid = _pValid + len;//更新已使用缓冲区的起始
	}
	_validLen -= len;//更新已使用缓冲区的长度

	return len;
}

/*
* function:从缓冲区内仅读出数据，不更新读写指针
* param   :@buffer:接受读取数据的buffer
*		    @len:将要读取的数据的长度
* return  :-1:没有初始化
*	 	    >0:实际读取的长度
* */
int RingBuffer::peek(char * buffer, int len)
{
	if (_validLen == 0) return 0;

	if (len > _validLen) len = _validLen;

	if (_pValid + len > _pTail)//需要分成两段copy
	{
		int len1 = _pTail - _pValid;
		int len2 = len - len1;
		memcpy(buffer, _pValid, len1);//第一段
		memcpy(buffer + len1, _pHead, len2);//第二段，绕到整个存储区的开头
	}
	else if (_pValid + len == _pTail) {
		memcpy(buffer, _pValid, len);//第一段
	}
	else {
		memcpy(buffer, _pValid, len);
	}

	return len;
}

void RingBuffer::consume(int len)
{
	if (len > _validLen) len = _validLen;

	if (_pValid + len > _pTail)//需要分成两段copy
	{
		int len1 = _pTail - _pValid;
		int len2 = len - len1;
		_pValid = _pHead + len2;//更新已使用缓冲区的起始
	}
	else if (_pValid + len == _pTail) {
		_pValid = _pHead;
	}
	else {
		_pValid = _pValid + len;//更新已使用缓冲区的起始
	}
	_validLen -= len;//更新已使用缓冲区的长度
}

char * RingBuffer::firstSegment(int * size) {
	if (_validLen == 0) { 
		*size = 0;
		return nullptr;
	}

	if (_pValidTail <= _pValid) {
		int len1 = _pTail - _pValid;
		*size = len1;
		return _pValid;
	}
	else {
		*size = _validLen;
		return _pValid;
	}
}
