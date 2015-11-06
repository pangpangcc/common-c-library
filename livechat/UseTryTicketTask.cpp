/*
 * author: Samson.Fan
 *   date: 2015-04-01
 *   file: UseTryTicketTask.cpp
 *   desc: 使用试聊券Task实现类
 */

#include "UseTryTicketTask.h"
#include "ITaskManager.h"
#include "ILiveChatClient.h"
#include "AmfPublicParse.h"
#include <json/json.h>

// 请求参数定义
#define TARGETID_PARAM		"targetId"	// 对方用户Id
#define EVENT_PARAM			"event"		// 语音Id

UseTryTicketTask::UseTryTicketTask(void)
{
	m_listener = NULL;

	m_seq = 0;
	m_errType = LCC_ERR_FAIL;
	m_errMsg = "";

	m_userId = "";
}

UseTryTicketTask::~UseTryTicketTask(void)
{
}

// 初始化
bool UseTryTicketTask::Init(ILiveChatClientListener* listener)
{
	bool result = false;
	if (NULL != listener)
	{
		m_listener = listener;
		result = true;
	}
	return result;
}
	
// 处理已接收数据
bool UseTryTicketTask::Handle(const TransportProtocol* tp)
{
	bool result = false;

	string targetId("");
	TRY_TICKET_EVENT eventCode = TTE_UNKNOW;
		
	AmfParser parser;
	amf_object_handle root = parser.Decode((char*)tp->data, tp->GetDataLength());
	if (!root.isnull()) {
		// 解析成功协议
		if (root->type == DT_OBJECT) {
			// targetId
			amf_object_handle targetIdObject = root->get_child(TARGETID_PARAM);
			result = !targetIdObject.isnull() && targetIdObject->type == DT_STRING;
			if (result) {
				targetId = targetIdObject->strValue;
			}

			// event
			amf_object_handle eventObject = root->get_child(EVENT_PARAM);
			result = !eventObject.isnull() && eventObject->type == DT_INTEGER;
			if (result) {
				eventCode = (eventObject->intValue < TTE_UNKNOW ? (TRY_TICKET_EVENT)eventObject->intValue : TTE_UNKNOW);
			}

			if (result) {
				m_errType = LCC_ERR_SUCCESS;
				m_errMsg = "";
			}
		}

		// 解析失败协议
		if (!result) {
			int errType = 0;
			string errMsg = "";
			if (GetAMFProtocolError(root, errType, errMsg)) {
				m_errType = (LCC_ERR_TYPE)errType;
				m_errMsg = errMsg;
				result = true;
			}
		}
	}

	// 协议解析失败
	if (!result) {
		m_errType = LCC_ERR_PROTOCOLFAIL;
		m_errMsg = "";
	}

	// 通知listener
	if (NULL != m_listener) {
		m_listener->OnUseTryTicket(m_userId, m_errType, m_errMsg, targetId, eventCode);
	}
	
	return result;
}
	
// 获取待发送的数据，可先获取data长度，如：GetSendData(NULL, 0, dataLen);
bool UseTryTicketTask::GetSendData(void* data, unsigned int dataSize, unsigned int& dataLen)
{
	bool result = false;
	
	// 构造json协议
	Json::Value root(m_userId);
	Json::FastWriter writer;
	string json = writer.write(root);

	// 填入buffer
	if (json.length() < dataSize) {
		memcpy(data, json.c_str(), json.length());
		dataLen = json.length();

		result  = true;
	}
	return result;
}

// 获取待发送数据的类型
TASK_PROTOCOL_TYPE UseTryTicketTask::GetSendDataProtocolType()
{
	return JSON_PROTOCOL;
}
	
// 获取命令号
int UseTryTicketTask::GetCmdCode()
{
	return TCMD_USETRYTICKET;
}

// 设置seq
void UseTryTicketTask::SetSeq(unsigned int seq)
{
	m_seq = seq;
}

// 获取seq
unsigned int UseTryTicketTask::GetSeq()
{
	return m_seq;
}

// 是否需要等待回复。若false则发送后释放(delete掉)，否则发送后会被添加至待回复列表，收到回复后释放
bool UseTryTicketTask::IsWaitToRespond()
{
	return true;
}

// 获取处理结果
void UseTryTicketTask::GetHandleResult(LCC_ERR_TYPE& errType, string& errMsg)
{
	errType = m_errType;
	errMsg = m_errMsg;
}

// 初始化参数
bool UseTryTicketTask::InitParam(const string& userId)
{
	bool result = false;
	if (!userId.empty())
	{
		m_userId = userId;

		result = true;
	}
	
	return result;
}

// 未完成任务的断线通知
void UseTryTicketTask::OnDisconnect()
{
	if (NULL != m_listener) {
		m_listener->OnUseTryTicket(m_userId, LCC_ERR_CONNECTFAIL, "", "", TTE_UNKNOW);
	}
}