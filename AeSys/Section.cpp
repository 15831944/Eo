#include "stdafx.h"
#include "Section.h"

Section::Section(const double width, const double depth, const long properties) noexcept {
	m_Width = width;
	m_Depth = depth;
	m_Properties = properties;
}

bool Section::operator==(const Section& other) noexcept {
	return Identical(other);
}

bool Section::operator!=(const Section& other) noexcept {
	return !Identical(other);
}

void Section::operator()(const double width, const double depth, const long properties) noexcept {
	m_Width = width;
	m_Depth = depth;
	m_Properties = properties;
}

void Section::SetWidth(const double width) noexcept {
	m_Width = width;
}

void Section::SetDepth(const double depth) noexcept {
	m_Depth = depth;
}

double Section::Width() const noexcept {
	return m_Width;
}

double Section::Depth() const noexcept {
	return m_Depth;
}

bool Section::Identical(const Section& section) const noexcept {
	return m_Width == section.m_Width && m_Depth == section.m_Depth && m_Properties == section.m_Properties;
}

bool Section::IsRectangular() const noexcept {
	return (m_Properties & mc_Rectangular) == mc_Rectangular;
}

bool Section::IsRound() const noexcept {
	return (m_Properties & mc_Round) == mc_Round;
}
