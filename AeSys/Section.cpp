#include "StdAfx.h"

#include "Section.h"

Section::Section(double width, double depth, long properties) noexcept {
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

void Section::operator()(double width, double depth, long properties) noexcept {
	m_Width = width;
	m_Depth = depth;
	m_Properties = properties;
}

void Section::SetWidth(double width) noexcept {
	m_Width = width;
}

void Section::SetDepth(double depth) noexcept {
	m_Depth = depth;
}

double Section::Width() const noexcept {
	return m_Width;
}

double Section::Depth() const noexcept {
	return m_Depth;
}

bool Section::Identical(const Section& section) noexcept {
	return (m_Width == section.m_Width && m_Depth == section.m_Depth && m_Properties == section.m_Properties) ? true : false;
}

bool Section::IsRectangular() noexcept {
	return (m_Properties & Rectangular) == Rectangular;
}

bool Section::IsRound() noexcept {
	return (m_Properties & Round) == Round;
}
