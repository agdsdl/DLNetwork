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
* ��ʼ�����λ�����
* ���λ��������������malloc������ڴ�,Ҳ������Flash�洢����
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
* function:�򻺳�����д������
* param:@buffer д�������ָ��
* 		 @addLen д������ݳ���
* return:-1:д�볤�ȹ���
* 		  -2:������û�г�ʼ��
* */

int RingBuffer::write(const char * buffer, int addLen)
{
	if (addLen > _capacity-_validLen) return -1;

	//��Ҫ���������copy��_pValidTail��
	if (_pValidTail + addLen > _pTail)//��Ҫ�ֳ�����copy
	{
		int len1 = _pTail - _pValidTail;
		int len2 = addLen - len1;
		memcpy(_pValidTail, buffer, len1);
		memcpy(_pHead, buffer + len1, len2);
		_pValidTail = _pHead + len2;//�µ���Ч��������βָ��
	}
	else {
		memcpy(_pValidTail, buffer, addLen);
		_pValidTail += addLen;//�µ���Ч��������βָ��
	}

	_validLen += addLen;

	return addLen;
}

/*
* function:�ӻ�������ȡ������
* param   :@buffer:���ܶ�ȡ���ݵ�buffer
*		    @len:��Ҫ��ȡ�����ݵĳ���
* return  :-1:û�г�ʼ��
*	 	    >0:ʵ�ʶ�ȡ�ĳ���
* */
int RingBuffer::read(char * buffer, int len)
{
	if (_validLen == 0) return 0;

	if (len > _validLen) len = _validLen;

	if (_pValid + len > _pTail)//��Ҫ�ֳ�����copy
	{
		int len1 = _pTail - _pValid;
		int len2 = len - len1;
		memcpy(buffer, _pValid, len1);//��һ��
		memcpy(buffer + len1, _pHead, len2);//�ڶ��Σ��Ƶ������洢���Ŀ�ͷ
		_pValid = _pHead + len2;//������ʹ�û���������ʼ
	}
	else if (_pValid + len == _pTail) {
		memcpy(buffer, _pValid, len);//��һ��
		_pValid = _pHead;
	}
	else {
		memcpy(buffer, _pValid, len);
		_pValid = _pValid + len;//������ʹ�û���������ʼ
	}
	_validLen -= len;//������ʹ�û������ĳ���

	return len;
}

/*
* function:�ӻ������ڽ��������ݣ������¶�дָ��
* param   :@buffer:���ܶ�ȡ���ݵ�buffer
*		    @len:��Ҫ��ȡ�����ݵĳ���
* return  :-1:û�г�ʼ��
*	 	    >0:ʵ�ʶ�ȡ�ĳ���
* */
int RingBuffer::peek(char * buffer, int len)
{
	if (_validLen == 0) return 0;

	if (len > _validLen) len = _validLen;

	if (_pValid + len > _pTail)//��Ҫ�ֳ�����copy
	{
		int len1 = _pTail - _pValid;
		int len2 = len - len1;
		memcpy(buffer, _pValid, len1);//��һ��
		memcpy(buffer + len1, _pHead, len2);//�ڶ��Σ��Ƶ������洢���Ŀ�ͷ
	}
	else if (_pValid + len == _pTail) {
		memcpy(buffer, _pValid, len);//��һ��
	}
	else {
		memcpy(buffer, _pValid, len);
	}

	return len;
}

void RingBuffer::consume(int len)
{
	if (len > _validLen) len = _validLen;

	if (_pValid + len > _pTail)//��Ҫ�ֳ�����copy
	{
		int len1 = _pTail - _pValid;
		int len2 = len - len1;
		_pValid = _pHead + len2;//������ʹ�û���������ʼ
	}
	else if (_pValid + len == _pTail) {
		_pValid = _pHead;
	}
	else {
		_pValid = _pValid + len;//������ʹ�û���������ʼ
	}
	_validLen -= len;//������ʹ�û������ĳ���
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
