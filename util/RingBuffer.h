/*
 * MIT License
 *
 * Copyright (c) 2019-2022 agdsdl <agdsdl@sina.com.cn>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
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
