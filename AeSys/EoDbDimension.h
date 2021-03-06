#pragma once
#include <DbAlignedDimension.h>
#include "EoGeLineSeg3d.h"
#include "EoDbFontDefinition.h"
#include "EoDbPrimitive.h"

class EoDbDimension final : public EoDbPrimitive {
DECLARE_DYNAMIC(EoDbDimension)

	EoGeLineSeg3d m_Line;
	short m_TextColorIndex {1};
	EoDbFontDefinition m_FontDefinition;
	EoGeReferenceSystem m_ReferenceSystem;
	CString m_strText;

	EoDbDimension() = default;

	EoDbDimension(const EoDbDimension& other);

	EoDbDimension& operator=(const EoDbDimension& other);

	~EoDbDimension() = default;

	void AddReportToMessageList(const OdGePoint3d& point) const override;

	void AddToTreeViewControl(HWND tree, HTREEITEM parent) const noexcept override;

	[[nodiscard]] EoDbPrimitive* Clone(OdDbBlockTableRecordPtr blockTableRecord) const override;

	void Display(AeSysView* view, CDC* deviceContext) override;

	void FormatExtra(CString& extra) const override;

	void FormatGeometry(CString& geometry) const override;

	void GetAllPoints(OdGePoint3dArray& points) const override;

	[[nodiscard]] OdGePoint3d GetCtrlPt() const override;

	void GetExtents(AeSysView* view, OdGeExtents3d& extents) const override;

	[[nodiscard]] OdGePoint3d GoToNxtCtrlPt() const override;

	bool IsEqualTo(EoDbPrimitive* primitive) const noexcept override;

	bool IsInView(AeSysView* view) const override;

	bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) const override;

	OdGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) const override;
	/// <summary>Evaluates whether a line intersects a dimension line.</summary>
	bool SelectUsingLineSeg(const EoGeLineSeg3d& lineSeg, AeSysView* view, OdGePoint3dArray& intersections) override;

	bool SelectUsingRectangle(const OdGePoint3d& lowerLeftCorner, const OdGePoint3d& upperRightCorner, AeSysView* view) const override;

	bool SelectUsingPoint(const EoGePoint4d& point, AeSysView* view, OdGePoint3d& projectedPoint) const override;

	void TransformBy(const EoGeMatrix3d& transformMatrix) override;

	void TranslateUsingMask(const OdGeVector3d& translate, unsigned mask) override;

	bool Write(EoDbFile& file) const override;

	void Write(CFile& file, unsigned char* buffer) const override;

	void CutAt(const OdGePoint3d& point, EoDbGroup* newGroup) override;

	void CutAt2Points(OdGePoint3d* points, EoDbGroupList* groups, EoDbGroupList* newGroups, OdDbDatabasePtr database) override;

	void ModifyState() noexcept override;

	void GetBoundingBox(OdGePoint3dArray& boundingBox, double spaceFactor) const;

	[[nodiscard]] const EoDbFontDefinition& FontDefinition() const noexcept;

	[[nodiscard]] const EoGeLineSeg3d& Line() const noexcept;

	void GetPts(OdGePoint3d& startPoint, OdGePoint3d& endPoint) const;

	[[nodiscard]] EoGeReferenceSystem ReferenceSystem() const;

	[[nodiscard]] double Length() const;

	[[nodiscard]] double ParametricRelationshipOf(const OdGePoint3d& point) const;

	void SetDefaultNote();

	void SetFontDefinition(const EoDbFontDefinition& fontDefinition) noexcept;

	void SetStartPoint(const OdGePoint3d& startPoint);

	void SetEndPoint(const OdGePoint3d& endPoint);

	void SetReferenceSystem(const EoGeReferenceSystem& referenceSystem) noexcept;

	void SetText(const CString& text);

	void SetTextHorizontalAlignment(EoDb::HorizontalAlignment horizontalAlignment) noexcept;

	void SetTextColorIndex(short colorIndex) noexcept;

	void SetTextVerticalAlignment(EoDb::VerticalAlignment verticalAlignment) noexcept;

	const CString& Text() const noexcept;

	const short& TextColorIndex() const noexcept;

private:
	static unsigned short sm_wFlags;	// bit 1 clear if dimension selected at note, set if dimension selected at line
public:
	static EoDbDimension* Create(OdDbAlignedDimensionPtr& alignedDimension);

	static OdDbAlignedDimensionPtr Create(OdDbBlockTableRecordPtr blockTableRecord);

	static OdDbAlignedDimensionPtr Create(OdDbBlockTableRecordPtr blockTableRecord, EoDbFile& file);

	static OdDbAlignedDimensionPtr Create(OdDbBlockTableRecordPtr blockTableRecord, unsigned char* primitiveBuffer, int versionNumber);
};
