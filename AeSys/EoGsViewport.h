#pragma once
#include "EoGePoint4d.h"

class EoGsViewport {
	double m_DeviceHeightInPixels {0.0};
	double m_DeviceWidthInPixels {0.0};
	double m_DeviceHeightInInches {0.0};
	double m_DeviceWidthInInches {0.0};
	double m_HeightInPixels {0.0};
	double m_WidthInPixels {0.0};
public:
	/// <remarks> Window coordinates are rounded to nearest whole number.</remarks>
	CPoint DoProjection(const EoGePoint4d& point) const noexcept;
	/// <remarks>Window coordinates are rounded to nearest whole number. Perspective division to yield normalized device coordinates.</remarks>
	void DoProjection(CPoint* pnt, int numberOfPoints, EoGePoint4d* points) const noexcept;
	/// <remarks>Window coordinates are rounded to nearest whole number. Perspective division to yield normalized device coordinates.</remarks>
	void DoProjection(CPoint* pnt, EoGePoint4dArray& points) const;

	void DoProjectionInverse(OdGePoint3d& point) const noexcept;

	double HeightInPixels() const noexcept;

	double HeightInInches() const noexcept;

	double WidthInPixels() const noexcept;

	double WidthInInches() const noexcept;

	void SetDeviceHeightInInches(double height) noexcept;

	void SetDeviceWidthInInches(double width) noexcept;

	void SetSize(int width, int height) noexcept;

	void SetDeviceHeightInPixels(double height) noexcept;

	void SetDeviceWidthInPixels(double width) noexcept;
};

using CViewports = CList<EoGsViewport>;
