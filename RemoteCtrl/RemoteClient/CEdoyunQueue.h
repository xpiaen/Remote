#pragma once

template<class T>
class CEdoyunQueue
{//�̰߳�ȫ�Ķ��У�����IOCPʵ�֣�
public:
	CEdoyunQueue();
	~CEdoyunQueue();
	bool PushBack(const T& data);
	bool PopFront(T& data);
	size_t Size();
	void Clear();
private:
	static void threadEntry(void* arg);
	void threadMain();
private:
	std::list<T> m_listData;
	HANDLE m_hCompeletionPort
	HANDLE m_hThread;
public:
	typedef struct IocpParam {
		int nOperator;//����
		T strData;//����
		HANDLE hEvent;//pop������Ҫ������
		IocpParam(int op, const char* data,HANDLE he) {
			nOperator = op;
			strData = data;
			hEvent = he;
		}
		IocpParam() {
			nOperator = -1;
		}
	}PPARAM;//Post Parameter ����Ͷ����Ϣ�Ľṹ��
	enum {
		EQPush,
		EQPop,
		EQSize,
		EQClear
	};
};

