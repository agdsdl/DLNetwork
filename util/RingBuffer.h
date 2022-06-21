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
	* function:向缓冲区中写入数据
	* param:@buffer 写入的数据指针
	* 		 @addLen 写入的数据长度
	* return:-1:写入长度过大
	*		  >0:写入长度
	* */
	int write(const char* buffer, int addLen);

	/*
	* function:从缓冲区内取出数据
	* param   :@buffer:接受读取数据的buffer
	*		    @len:将要读取的数据的长度
	* return  :-1:没有初始化
	*	 	    >0:实际读取的长度
	* */
	int read(char* buffer, int len);

	/*
	* function:获取已使用缓冲区的长度
	* return  :已使用的buffer长度
	* */
	int dataSize(void) {
		return _validLen;
	}
	int remainSize() {
		return _capacity - _validLen;
	}
	/*
	* function: 置len字节数据为已消费
	*/
	void consume(int len);
	/*
	* function:从缓冲区内仅读出数据，不更新读写指针
	* param   :@buffer:接受读取数据的buffer
	*		    @len:将要读取的数据的长度
	* return  :-1:没有初始化
	*	 	    >0:实际读取的长度
	* */
	int peek(char* buffer, int len);

	char* firstSegment(int* size);
private:
	int _validLen = 0;//已使用的数据长度
	char* _pHead = nullptr;//环形存储区的首地址
	char* _pTail = nullptr;//环形存储区的结尾地址
	char* _pValid = nullptr;//已使用的缓冲区的首地址
	char* _pValidTail = nullptr;//已使用的缓冲区的尾地址
	size_t _capacity = 0;
};

} //DLNetwork
