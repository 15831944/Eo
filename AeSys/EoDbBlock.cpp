#include "stdafx.h"

EoDbBlock::EoDbBlock() {
	m_wBlkTypFlgs = 0;
}
EoDbBlock::EoDbBlock(EoUInt16 flags, const OdGePoint3d& basePoint) {
	m_wBlkTypFlgs = flags;
	m_BasePoint = basePoint;
}
EoDbBlock::EoDbBlock(EoUInt16 flags, const OdGePoint3d&  basePoint, const OdString& pathName) {
	m_wBlkTypFlgs = flags;
	m_BasePoint = basePoint;
	m_strXRefPathName = pathName;
}

OdGePoint3d	EoDbBlock::BasePoint() const {
	return m_BasePoint;
}
EoUInt16 EoDbBlock::GetBlkTypFlgs() {
	return m_wBlkTypFlgs;
}
bool EoDbBlock::HasAttributes() {
	return (m_wBlkTypFlgs & 2) == 2;
}
bool EoDbBlock::IsAnonymous() {
	return (m_wBlkTypFlgs & 1) == 1;
}
bool EoDbBlock::IsFromExternalReference() {
	return (m_wBlkTypFlgs & 4) == 4;
}
void EoDbBlock::SetBlkTypFlgs(EoUInt16 flags) {
	m_wBlkTypFlgs = flags;
}
void EoDbBlock::SetBasePoint(const OdGePoint3d& basePoint) {
	m_BasePoint = basePoint;
}
