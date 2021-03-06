#include "stdafx.h"
#include "EoGsModelTransform.h"

EoGsModelTransform::EoGsModelTransform() {
	m_CurrentModelTransform.setToIdentity();
}

EoGeMatrix3d EoGsModelTransform::ModelMatrix() const noexcept {
	return m_CurrentModelTransform;
}

unsigned EoGsModelTransform::Depth() const noexcept {
	return m_Depth;
}

void EoGsModelTransform::PopModelTransform() {
	m_Depth--;
	m_CurrentModelTransform = m_TransformMatrixList.RemoveTail();
}

// <tas="Have not tested with nested levels. postMultBy could need to be changed to preMultBy"</tas>
void EoGsModelTransform::PushModelTransform(const EoGeMatrix3d& transformation) {
	m_Depth++;
	m_TransformMatrixList.AddTail(m_CurrentModelTransform);
	m_CurrentModelTransform.postMultBy(transformation);
}
