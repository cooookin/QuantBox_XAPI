#include "stdafx.h"
#include "MdUserApi.h"
#include "../include/QueueEnum.h"

#include "../include/ApiHeader.h"
#include "../include/ApiStruct.h"

#include "../include/toolkit.h"

#include "../QuantBox_Queue/MsgQueue.h"

#include "../include/XApiC.h"



#include "TypeConvert.h"

#include <string.h>
#include <cfloat>

#include <mutex>
#include <vector>
//#include <tchar.h>
#include <windows.h>

using namespace std;


#define WM_USER_STOCK	2000

CMdUserApi* CMdUserApi::pThis = nullptr;

LRESULT CALLBACK CMdUserApi::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_USER_STOCK)
	{
		return pThis->_OnMsg(wParam, lParam);
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT CMdUserApi::_OnMsg(WPARAM wParam, LPARAM lParam)
{
	RCV_DATA* pHeader;
	int i;

	pHeader = (RCV_DATA*)lParam;

	//	���ڴ����ٶ�������������,��ý� pHeader->m_pData ָ������ݱ���, ��������
	switch (wParam)
	{
	case RCV_REPORT:						// ��Ʊ����
		for (i = 0; i < pHeader->m_nPacketNum; i++)
		{
			// ���ݴ���
			m_msgQueue->m_bDirectOutput;
			OnRtnDepthMarketData((RCV_REPORT_STRUCTEx*)&(pHeader->m_pReport[i]));
		}
		break;

	case RCV_FILEDATA:						// �ļ�
		switch (pHeader->m_wDataType)
		{
		case FILE_HISTORY_EX:				// ����������
			break;

		case FILE_MINUTE_EX:				// ������������
			break;
		default:
			return 0;
		}
		break;

	default:
		return 0;							// unknown data
	}
	return 1;
}

void* __stdcall Query(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	// ���ڲ����ã����ü���Ƿ�Ϊ��
	CMdUserApi* pApi = (CMdUserApi*)pApi2;
	pApi->QueryInThread(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
	return nullptr;
}

CMdUserApi::CMdUserApi(void)
{
	//m_pApi = nullptr;
	m_lRequestID = 0;
	m_nSleep = 1;

	// �Լ�ά��������Ϣ����
	m_msgQueue = new CMsgQueue();
	m_msgQueue_Query = new CMsgQueue();

	m_msgQueue_Query->Register((void*)Query, this);
	m_msgQueue_Query->StartThread();

	//m_msgQueue->m_bDirectOutput = true;

	m_bRunning = false;
	m_hThread = nullptr;
	m_hWnd = nullptr;
	m_hModule = nullptr;
	m_pStock_Init = nullptr;
	m_pStock_Quit = nullptr;

	pThis = this;
}

CMdUserApi::~CMdUserApi(void)
{
	Disconnect();
}

void CMdUserApi::QueryInThread(char type, void* pApi1, void* pApi2, double double1, double double2, void* ptr1, int size1, void* ptr2, int size2, void* ptr3, int size3)
{
	int iRet = 0;
	switch (type)
	{
	case E_Init:
		iRet = _Init();
		break;
	//case E_ReqUserLoginField:
	//	iRet = _ReqUserLogin(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
	//	break;
	default:
		break;
	}

	if (0 == iRet)
	{
		//���سɹ�����ӵ��ѷ��ͳ�
		m_nSleep = 1;
	}
	else
	{
		m_msgQueue_Query->Input_Copy(type, pApi1, pApi2, double1, double2, ptr1, size1, ptr2, size2, ptr3, size3);
		//ʧ�ܣ���4���ݽ�����ʱ����������1s
		m_nSleep *= 4;
		m_nSleep %= 1023;
	}
	this_thread::sleep_for(chrono::milliseconds(m_nSleep));
}

void CMdUserApi::Register(void* pCallback,void* pClass)
{
	m_pClass = pClass;
	if (m_msgQueue == nullptr)
		return;

	m_msgQueue_Query->Register((void*)Query,this);
	m_msgQueue->Register(pCallback,this);
	if (pCallback)
	{
		m_msgQueue_Query->StartThread();
		m_msgQueue->StartThread();
	}
	else
	{
		m_msgQueue_Query->StopThread();
		m_msgQueue->StopThread();
	}
}

ConfigInfoField* CMdUserApi::Config(ConfigInfoField* pConfigInfo)
{
	return nullptr;
}

//bool CMdUserApi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
//{
//	bool bRet = ((pRspInfo) && (pRspInfo->ErrorID != 0));
//	if(bRet)
//	{
//		ErrorField* pField = (ErrorField*)m_msgQueue->new_block(sizeof(ErrorField));
//
//		pField->ErrorID = pRspInfo->ErrorID;
//		strcpy(pField->ErrorMsg, pRspInfo->ErrorMsg);
//
//		m_msgQueue->Input_NoCopy(ResponeType::OnRtnError, m_msgQueue, m_pClass, bIsLast, 0, pField, sizeof(ErrorField), nullptr, 0, nullptr, 0);
//	}
//	return bRet;
//}
//
//bool CMdUserApi::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
//{
//	bool bRet = ((pRspInfo) && (pRspInfo->ErrorID != 0));
//
//	return bRet;
//}


void CMdUserApi::Connect(const string& szPath,
	ServerInfoField* pServerInfo,
	UserInfoField* pUserInfo,
	int count)
{
	m_szPath = szPath;
	memcpy(&m_ServerInfo, pServerInfo, sizeof(ServerInfoField));
	memcpy(&m_UserInfo, pUserInfo, sizeof(UserInfoField));

	StartThread();

	//m_msgQueue_Query->Input_NoCopy(RequestType::E_Init, m_msgQueue_Query, this, 0, 0,
	//	nullptr, 0, nullptr, 0, nullptr, 0);
}

int CMdUserApi::_Init()
{
	//StartThread();

	return 0;
}

void CMdUserApi::Disconnect()
{
	StopThread();

	// ������ѯ����
	if (m_msgQueue_Query)
	{
		m_msgQueue_Query->StopThread();
		m_msgQueue_Query->Register(nullptr,nullptr);
		m_msgQueue_Query->Clear();
		delete m_msgQueue_Query;
		m_msgQueue_Query = nullptr;
	}

	//if(m_pApi)
	//{
	//	m_pApi->RegisterSpi(NULL);
	//	m_pApi->Release();
	//	m_pApi = NULL;

		// ȫ������ֻ�����һ��
		// �������dll����Ƶ��߳�������ʧ��ʱֱ���˳�����������ط�Ҫ��һ���жϣ�������
		if (m_msgQueue)
		{
			m_msgQueue->Clear();
			m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Disconnected, 0, nullptr, 0, nullptr, 0, nullptr, 0);
			// ��������
			m_msgQueue->Process();
		}
	//}

	// ������Ӧ����
	if (m_msgQueue)
	{
		m_msgQueue->StopThread();
		m_msgQueue->Register(nullptr,nullptr);
		m_msgQueue->Clear();
		delete m_msgQueue;
		m_msgQueue = nullptr;
	}
}

//����ص����ñ�֤�˺������췵��
void CMdUserApi::OnRtnDepthMarketData(RCV_REPORT_STRUCTEx *pDepthMarketData)
{
	DepthMarketDataField* pField = (DepthMarketDataField*)m_msgQueue->new_block(sizeof(DepthMarketDataField));

	strcpy(pField->InstrumentID, OldSymbol_2_NewSymbol(pDepthMarketData->m_szLabel, pDepthMarketData->m_wMarket));
	strcpy(pField->ExchangeID, Market_2_Exchange(pDepthMarketData->m_wMarket));

	//if (pDepthMarketData->m_szLabel[1] >= 'A')
	//if (strcmp(pField->InstrumentID, "000001") == 0)
	//{
	//	int a = 1;
	//}

	sprintf(pField->Symbol, "%s.%s", pField->InstrumentID, pField->ExchangeID);

	GetExchangeTime(pDepthMarketData->m_time, &pField->TradingDay, &pField->ActionDay, &pField->UpdateTime);
	
	pField->LastPrice = my_round(pDepthMarketData->m_fNewPrice);
	pField->Volume = pDepthMarketData->m_fVolume;
	pField->Turnover = pDepthMarketData->m_fAmount;

	pField->OpenPrice = my_round(pDepthMarketData->m_fOpen);
	pField->HighestPrice = my_round(pDepthMarketData->m_fHigh);
	pField->LowestPrice = my_round(pDepthMarketData->m_fLow);

	pField->PreClosePrice = my_round(pDepthMarketData->m_fLastClose);

	int nLots = 1;

	do
	{
		if (pDepthMarketData->m_fBuyVolume[0] == 0)
			break;
		pField->BidPrice1 = my_round(pDepthMarketData->m_fBuyPrice[0]);
		pField->BidVolume1 = pDepthMarketData->m_fBuyVolume[0] * nLots;

		if (pDepthMarketData->m_fBuyVolume[1] == 0)
			break;
		pField->BidPrice2 = my_round(pDepthMarketData->m_fBuyPrice[1]);
		pField->BidVolume2 = pDepthMarketData->m_fBuyVolume[1] * nLots;

		if (pDepthMarketData->m_fBuyVolume[2] == 0)
			break;
		pField->BidPrice3 = my_round(pDepthMarketData->m_fBuyPrice[2]);
		pField->BidVolume3 = pDepthMarketData->m_fBuyVolume[2] * nLots;

		if (pDepthMarketData->m_fBuyVolume4 == 0)
			break;
		pField->BidPrice4 = my_round(pDepthMarketData->m_fBuyPrice4);
		pField->BidVolume4 = pDepthMarketData->m_fBuyVolume4 * nLots;

		if (pDepthMarketData->m_fBuyVolume5 == 0)
			break;
		pField->BidPrice5 = my_round(pDepthMarketData->m_fBuyPrice5);
		pField->BidVolume5 = pDepthMarketData->m_fBuyVolume5 * nLots;
	} while (false);

	do
	{
		if (pDepthMarketData->m_fSellVolume[0] == 0)
			break;
		pField->AskPrice1 = my_round(pDepthMarketData->m_fSellPrice[0]);
		pField->AskVolume1 = pDepthMarketData->m_fSellVolume[0] * nLots;

		if (pDepthMarketData->m_fSellVolume[1] == 0)
			break;
		pField->AskPrice2 = my_round(pDepthMarketData->m_fSellPrice[1]);
		pField->AskVolume2 = pDepthMarketData->m_fSellVolume[1] * nLots;

		if (pDepthMarketData->m_fSellVolume[2] == 0)
			break;
		pField->AskPrice3 = my_round(pDepthMarketData->m_fSellPrice[2]);
		pField->AskVolume3 = pDepthMarketData->m_fSellVolume[2] * nLots;

		if (pDepthMarketData->m_fSellVolume4 == 0)
			break;
		pField->AskPrice4 = my_round(pDepthMarketData->m_fSellPrice4);
		pField->AskVolume4 = pDepthMarketData->m_fSellVolume4 * nLots;

		if (pDepthMarketData->m_fSellVolume5 == 0)
			break;
		pField->AskPrice5 = my_round(pDepthMarketData->m_fSellPrice5);
		pField->AskVolume5 = pDepthMarketData->m_fSellVolume5 * nLots;
	} while (false);

	m_msgQueue->Input_NoCopy(ResponeType::OnRtnDepthMarketData, m_msgQueue, m_pClass, 0, 0, pField, sizeof(DepthMarketDataField), nullptr, 0, nullptr, 0);
}

void CMdUserApi::StartThread()
{
	if (nullptr == m_hThread)
	{
		m_bRunning = true;
		m_hThread = new thread(ProcessThread, this);
	}
}

void CMdUserApi::StopThread()
{
	m_bRunning = false;
	//m_cv.notify_all();
	lock_guard<mutex> cl(m_mtx_del);
	if (m_hThread)
	{
		m_hThread->join();
		delete m_hThread;
		m_hThread = nullptr;
	}
}


void CMdUserApi::RunInThread()
{
	m_hModule = X_LoadLib(m_ServerInfo.Address);
	if (m_hModule == nullptr)
	{
		RspUserLoginField* pField = (RspUserLoginField*)m_msgQueue->new_block(sizeof(RspUserLoginField));

		pField->ErrorID = GetLastError();
		strncpy(pField->ErrorMsg, X_GetLastError(), sizeof(ErrorMsgType));

		m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Disconnected, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
		return;
	}

	m_pStock_Init = (pFunStock_Init)X_GetFunction(m_hModule, "Stock_Init");
	m_pStock_Quit = (pFunStock_Quit)X_GetFunction(m_hModule, "Stock_Quit");
	if (m_pStock_Init == nullptr)
	{
		RspUserLoginField* pField = (RspUserLoginField*)m_msgQueue->new_block(sizeof(RspUserLoginField));

		pField->ErrorID = GetLastError();
		strncpy(pField->ErrorMsg, X_GetLastError(), sizeof(ErrorMsgType));

		m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Disconnected, 0, pField, sizeof(RspUserLoginField), nullptr, 0, nullptr, 0);
		return;
	}

	m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Initialized, 0, nullptr, 0, nullptr, 0, nullptr, 0);
	m_msgQueue->Input_NoCopy(ResponeType::OnConnectionStatus, m_msgQueue, m_pClass, ConnectionStatus::Done, 0, nullptr, 0, nullptr, 0, nullptr, 0);


	m_hWnd = CreateWindowA(
		"static",
		"MsgRecv",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		NULL,
		NULL);

	if (m_hWnd != NULL && IsWindow(m_hWnd))
	{
		SetWindowLong(m_hWnd, GWL_WNDPROC, (LONG)WndProc);
	}
	m_pStock_Init(m_hWnd, WM_USER_STOCK, RCV_WORK_SENDMSG);

	MSG msg;
	while (m_bRunning && GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	m_pStock_Quit(m_hWnd);
	DestroyWindow(m_hWnd);

	X_FreeLib(m_hModule);
	m_hModule = nullptr;
	m_hWnd = nullptr;

	// �����߳�
	m_hThread = nullptr;
	m_bRunning = false;
}