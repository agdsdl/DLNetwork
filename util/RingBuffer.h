#pragma once
#include "platform.h"

namespace DLNetwork {
class RingBuffer
{
public:
	RingBuffer(char* buf, size_t size);
	~RingBuffer();

	/*
	* function:�򻺳�����д������
	* param:@buffer д�������ָ��
	* 		 @addLen д������ݳ���
	* return:-1:д�볤�ȹ���
	*		  >0:д�볤��
	* */
	int write(const char* buffer, int addLen);

	/*
	* function:�ӻ�������ȡ������
	* param   :@buffer:���ܶ�ȡ���ݵ�buffer
	*		    @len:��Ҫ��ȡ�����ݵĳ���
	* return  :-1:û�г�ʼ��
	*	 	    >0:ʵ�ʶ�ȡ�ĳ���
	* */
	int read(char* buffer, int len);

	/*
	* function:��ȡ��ʹ�û������ĳ���
	* return  :��ʹ�õ�buffer����
	* */
	int dataSize(void) {
		return _validLen;
	}
	int remainSize() {
		return _capacity - _validLen;
	}
	/*
	* function: ��len�ֽ�����Ϊ������
	*/
	void consume(int len);
	/*
	* function:�ӻ������ڽ��������ݣ������¶�дָ��
	* param   :@buffer:���ܶ�ȡ���ݵ�buffer
	*		    @len:��Ҫ��ȡ�����ݵĳ���
	* return  :-1:û�г�ʼ��
	*	 	    >0:ʵ�ʶ�ȡ�ĳ���
	* */
	int peek(char* buffer, int len);

	char* firstSegment(int* size);
private:
	int _validLen = 0;//��ʹ�õ����ݳ���
	char* _pHead = nullptr;//���δ洢�����׵�ַ
	char* _pTail = nullptr;//���δ洢���Ľ�β��ַ
	char* _pValid = nullptr;//��ʹ�õĻ��������׵�ַ
	char* _pValidTail = nullptr;//��ʹ�õĻ�������β��ַ
	size_t _capacity = 0;
};

} //DLNetwork
