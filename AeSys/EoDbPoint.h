#pragma once

#include "EoDbPrimitive.h"

class EoDbPoint : public EoDbPrimitive {
	OdInt16	m_PointDisplayMode;
	OdGePoint3d	m_Position;
	OdUInt16 m_NumberOfDatums;
	double* m_Data;

public: // Constructors and destructor

	EoDbPoint() noexcept;
	EoDbPoint(const OdGePoint3d& point) noexcept;

	EoDbPoint(const EoDbPoint& other);
	const EoDbPoint& operator=(const EoDbPoint& other);

	~EoDbPoint();

public: // Methods - absolute virtuals

	void AddReportToMessageList(const OdGePoint3d& point) const override;
	void AddToTreeViewControl(HWND tree, HTREEITEM parent) const noexcept override;
	EoDbPrimitive* Clone(OdDbBlockTableRecordPtr blockTableRecord) const override;
	void Display(AeSysView* view, CDC* deviceContext) override;
	void FormatExtra(CString& extra) const override;
	void FormatGeometry(CString& geometry) const override;
	void GetAllPoints(OdGePoint3dArray& points) const override;
	OdGePoint3d GetCtrlPt() const noexcept override;
	void GetExtents(AeSysView* view, OdGeExtents3d& extents) const override;
	OdGePoint3d GoToNxtCtrlPt() const noexcept override;
	bool Is(OdUInt16 type) const noexcept override { return type == EoDb::kPointPrimitive; }
	bool IsEqualTo(EoDbPrimitive* primitive) const override;
	bool IsInView(AeSysView* view) const override;
	bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) const override;
	OdGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) const override;
	bool SelectBy(const OdGePoint3d& lowerLeftCorner, const OdGePoint3d& upperRightCorner, AeSysView* view) const override;
	bool SelectBy(const EoGePoint4d& point, AeSysView* view, OdGePoint3d&) const override;
	void TransformBy(const EoGeMatrix3d& transformMatrix) override;
	void TranslateUsingMask(const OdGeVector3d& translate, const DWORD) override;
	bool Write(EoDbFile& file) const override;
	void Write(CFile& file, OdUInt8* buffer) const override;

public: // Methods

	double DataAt(OdUInt16 dataIndex) const noexcept;
	void ModifyState() noexcept override;
	OdInt16 PointDisplayMode() const noexcept;
	OdGePoint3d Position() const noexcept;
	void SetData(OdUInt16 numberOfDatums, double* data);
	void SetPointDisplayMode(OdInt16 displayMode) noexcept;

public: // Methods - static

	static EoDbPoint* Create(OdDbPointPtr& point);

	static OdDbPointPtr Create(OdDbBlockTableRecordPtr& blockTableRecord);
	static OdDbPointPtr Create(OdDbBlockTableRecordPtr& blockTableRecord, EoDbFile& file);
	static OdDbPointPtr Create(OdDbBlockTableRecordPtr blockTableRecord, OdUInt8* primitiveBuffer, int versionNumber);
};
