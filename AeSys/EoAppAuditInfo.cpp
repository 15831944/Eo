#include "stdafx.h"
#include "AeSys.h"
#include "EoAppAuditInfo.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
EoAppAuditInfo::EoAppAuditInfo() noexcept {
	m_pHostAppServices = nullptr;
}

const OdDbAuditInfo::MsgInfo& EoAppAuditInfo::getLastInfo() {
	return OdDbAuditInfo::getLastInfo();
}

void EoAppAuditInfo::setLastInfo(MsgInfo& messageInfo) {
	OdDbAuditInfo::setLastInfo(messageInfo);
}

void EoAppAuditInfo::printInfo(const OdString& info) {
	MsgInfo MessageInfo;
	ODA_ASSERT(m_pHostAppServices);
	MessageInfo.bIsError = false;
	MessageInfo.strName = info;
	MessageInfo.strDefaultValue = L"";
	MessageInfo.strValue = L"";
	MessageInfo.strValidation = L"";
	setLastInfo(MessageInfo);
	m_pHostAppServices->auditPrintReport(this, info, getPrintDest());
	TRACE1("%ls\n", info.c_str());
}

void EoAppAuditInfo::printError(const OdString& name, const OdString& value, const OdString& validation, const OdString& defaultValue) {
	MsgInfo MessageInfo;
	ODA_ASSERT(m_pHostAppServices);
	MessageInfo.bIsError = true;
	MessageInfo.strName = name;
	MessageInfo.strDefaultValue = defaultValue;
	MessageInfo.strValue = value;
	MessageInfo.strValidation = validation;
	setLastInfo(MessageInfo);
	OdString Line;
	if (!name.isEmpty()) {
		Line += name + L". ";
	}
	if (!value.isEmpty()) {
		Line += L"An invalid " + value + L" was found. ";
	}
	if (!validation.isEmpty()) {
		Line += L"Validation: " + validation + L". ";
	}
	if (!defaultValue.isEmpty()) {
		Line += (fixErrors() ? L"Replaced by " : L"Default value is ") + defaultValue + L".";
	}
	m_pHostAppServices->auditPrintReport(this, Line, getPrintDest());
	TRACE1("%ls\n", Line.c_str());
}
