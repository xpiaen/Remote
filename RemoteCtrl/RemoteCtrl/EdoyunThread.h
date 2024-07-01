#pragma once
#include "pch.h"
#include <windows.h>
#include <atomic>
#include <vector>
#include <mutex>

class ThreadFuncBase {};
typedef int (ThreadFuncBase::* FUNCTYPE)();

class ThreadWorker {
public:
	ThreadWorker():thiz(NULL), func(NULL) {}
	ThreadWorker(ThreadFuncBase* obj, FUNCTYPE f):thiz(obj), func(f) {}
	ThreadWorker(const ThreadWorker& other){
		thiz = other.thiz;
		func = other.func;
	}
	ThreadWorker& operator=(const ThreadWorker& other) {
		if (this != &other) {
			thiz = other.thiz;
			func = other.func;
		}
		return *this;
	}
	int operator()() {
		if (thiz) {
			return (thiz->*func)();
		}
		else return -1;
	}
	bool IsValid() const {
		return (thiz != NULL) && (func != NULL);
	}
private:
	ThreadFuncBase* thiz;
	int (ThreadFuncBase::* func)();
};

class EdoyunThread
{
public:
	EdoyunThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}

	~EdoyunThread() {
		Stop();
	}

	//true��ʾ�ɹ������߳�   false��ʾ����ʧ��
	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(ThreadProc, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	bool IsValid() {//����true��ʾ��Ч   false��ʾ�߳��쳣���Ѿ���ֹ
		if(m_hThread == NULL || m_hThread == INVALID_HANDLE_VALUE)return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	bool Stop() {
		if (m_bStatus == false)return true;
		m_bStatus = false;
		DWORD ret = WaitForSingleObject(m_hThread, 1000) == WAIT_OBJECT_0;
		if (ret == WAIT_TIMEOUT) {
			TerminateThread(m_hThread, -1);
		}
		UpdateWorker();
		return ret == WAIT_OBJECT_0;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {
		if ((m_worker.load() != NULL) && (m_worker.load() != &worker)) {
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		if(m_worker.load() == &worker)return;
		if (!worker.IsValid()) {
			m_worker.store(NULL);
			return;
		}
		m_worker.store(new ::ThreadWorker(worker));
	}

	bool Isidle() {//����true��ʾ�߳̿���   false��ʾ�й���Ҫ����
		::ThreadWorker* pWorker = m_worker.load();
		if (pWorker == NULL) return true;
		return !pWorker->IsValid();
	}
private:
	void ThreadWorker() {
		while (m_bStatus) {
			if (m_worker.load() == NULL) {
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker.load();
			if (worker.IsValid()) {
				if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) {
					int ret = worker();
					if (ret != 0) {
						CString str;
						str.Format(_T("thread warning code  %d\r\n"), ret);
						OutputDebugString(str);
					}
					if (ret < 0)m_worker.store(NULL);
				}
			}
			else {
				Sleep(1);
			}
		}
	}
	static void ThreadProc(void* arg) {
		EdoyunThread* pThis = (EdoyunThread*)arg;
		if (pThis) {
			pThis->ThreadWorker();
		}
		_endthread();
	}
private:
	HANDLE m_hThread;
	bool m_bStatus;//false ��ʾ�߳̽�Ҫ�ر�   true��ʾ�߳���������
	std::atomic<::ThreadWorker*> m_worker;
};

class EdoyunThreadPool 
{
public:
	EdoyunThreadPool(size_t size){
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++) {
			m_threads[i] = new EdoyunThread();
		}
	}
	~EdoyunThreadPool() {
		Stop();
		for (auto thread : m_threads) {
			delete thread;
		}
		m_threads.clear();
	}
	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->Start() == false) {
				ret = false;
				break;
			}
		}
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); i++) {
				m_threads[i]->Stop();
			}
		}
		return ret;
	}
	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i]->Stop();
		}
	}
	//�����߳�����   -1��ʾû�п����߳�
	//TODO:�Ż��������е�m_threadsȫ���ŵ�һ���б��У���Ҫʱ���б���ȡ��һ���߳������������߳���Ż��б�
	int DispatchWorker(const ::ThreadWorker& worker) {
		m_lock.lock();
		int index = -1;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->Isidle()) {
				m_threads[i]->UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}
	bool CheckThreadValid(size_t index) {
		if (index < m_threads.size()) {
			return m_threads[index]->IsValid();
		}
		return false;
	}
private:
	std::mutex m_lock;
	std::vector<EdoyunThread*> m_threads;
};

