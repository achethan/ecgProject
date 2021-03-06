#ifndef BoundedBuffer_h
#define BoundedBuffer_h

#include <stdio.h>
#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

class BoundedBuffer
{
private:
	int cap; // max number of items in the buffer
	queue<vector<char>> q;	/* the queue of items in the buffer. Note
	that each item a sequence of characters that is best represented by a vector<char> for 2 reasons:
	1. An STL std::string cannot keep binary/non-printables
	2. The other alternative is keeping a char* for the sequence and an integer length (i.e., the items can be of variable length).
	While this would work, it is clearly more tedious */

	// add necessary synchronization variables and data structures 
	mutex mtx; //Lock 
	condition_variable empty; //Empty Cond Variable
	condition_variable full; //Full Cond Variable
	//int front, back;
	
	


public:
	BoundedBuffer(int _cap)
	{
         //front = back = 0;
		 cap = _cap;
	}
	~BoundedBuffer()
	{
          
	}

	void push(char* data, int len)
	{
		//1. Wait until there is room in the queue (i.e., queue lengh is less than cap)
		//2. Convert the incoming byte sequence given by data and len into a vector<char>
		//3. Then push the vector at the end of the queue
		unique_lock<mutex>lock(mtx);	 
        full.wait(lock, [this]{return q.size() < cap;});//Put lambda function as 2nd parameter
		vector<char>vec;
		int i = 0;
		while(i<len)
		{
			//cout<<"Data "<<*data<<endl;
			vec.push_back(*data);
			i++;
			data++;
		}
		q.push(vec);
		lock.unlock();
		empty.notify_one();
		
		//cout<<"SIZE OF QUEUE"<<q.size();
		//make sure lock.unlock is on crtical path
	}

	int pop(char* buf, int bufcap)
	{
		//1. Wait until the queue has at least 1 item
		//2. pop the front item of the queue. The popped item is a vector<char>
		//3. Convert the popped vector<char> into a char*, copy that into buf, make sure that vector<char>'s length is <= bufcap
		//4. Return the vector's length to the caller so that he knows many bytes were popped
		unique_lock<mutex>lock(mtx);
		empty.wait(lock, [this]{return q.size() > 0;});
		vector<char>vc = q.front();
		q.pop();
		int length = vc.size();
		//lock.unlock();
        full.notify_one();
		//lock.unlock();
		if(length <= bufcap)
		{
			memcpy(buf, vc.data(), length);
			//buf = vc.data();
		}
		
		return length;
		lock.unlock();
	}
};

#endif /* BoundedBuffer_ */
