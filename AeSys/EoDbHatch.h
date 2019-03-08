#pragma once

#include "DbHatch.h"

class EoDbHatch : public EoDbPrimitive {
	static size_t sm_EdgeToEvaluate;
	static size_t sm_Edge;
	static size_t sm_PivotVertex;

public:
	static double sm_PatternAngle;
	static double sm_PatternScaleX;
	static double sm_PatternScaleY;

	static OdStringArray sm_HatchNames;
	static int sm_HatchPatternOffsets[];
	static double sm_HatchPatternTable[];

public:
	enum InteriorStyle {
		kHollow,
		kSolid,
		kPattern,
		kHatch
	};

private:
	EoInt16	m_InteriorStyle;
	size_t m_InteriorStyleIndex;
	OdGePoint3d	m_HatchOrigin;
	OdGeVector3d m_HatchXAxis;
	OdGeVector3d m_HatchYAxis;
	OdGePoint3dArray m_Vertices;

	int m_NumberOfLoops;
	OdGePoint2dArray m_Vertices2d;
	OdGeDoubleArray m_Bulges;

public:	// Constructors and destructor
	EoDbHatch();
	EoDbHatch(const EoDbHatch& other);

	~EoDbHatch();

public: // Operators
	const EoDbHatch& operator=(const EoDbHatch& other);

public: // Methods - absolute virtuals
	void AddReportToMessageList(const OdGePoint3d& point) const;
	void AssociateWith(OdDbBlockTableRecordPtr blockTableRecord);
	void AddToTreeViewControl(HWND tree, HTREEITEM parent) const;
	EoDbPrimitive* Clone(OdDbDatabasePtr database) const;
	void Display(AeSysView* view, CDC* deviceContext);
	void FormatExtra(CString& extra) const;
	void FormatGeometry(CString& geometry) const;
	void GetAllPoints(OdGePoint3dArray& points) const;
	OdGePoint3d	GetCtrlPt() const;
	void GetExtents(AeSysView* view, OdGeExtents3d& extents) const;
	OdGePoint3d	GoToNxtCtrlPt() const;
	bool Is(EoUInt16 type) const;
	bool IsEqualTo(EoDbPrimitive* primitive) const {return false;}
	bool IsInView(AeSysView* view) const;
	bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) const;
	OdGePoint3d	SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) const;
	bool SelectBy(const OdGePoint3d& lowerLeftCorner, const OdGePoint3d& upperRightCorner, AeSysView* view) const;
	bool SelectBy(const EoGePoint4d& point, AeSysView* view, OdGePoint3d&) const;
	void TransformBy(const EoGeMatrix3d& transformMatrix);
	void TranslateUsingMask(const OdGeVector3d& translate, const DWORD);
	bool Write(EoDbFile& file) const;
	void Write(CFile& file, EoByte* buffer) const;

public: // Methods
	int Append(const OdGePoint3d& vertex);
	/// <summary>A Hatch is generated using line patterns.</summary>
	void DisplayHatch(AeSysView* view, CDC* deviceContext) const;
	void DisplaySolid(AeSysView* view, CDC* deviceContext) const;
	CString FormatInteriorStyle() const;
	OdGePoint3d GetPointAt(int pointIndex);
	void ModifyState();
	int NumberOfVertices() const;
	bool PivotOnGripPoint(AeSysView* view, const EoGePoint4d& point);
	OdGeVector3d RecomputeReferenceSystem();
	void RetrieveHatchPattern(const OdString& hatchPatternName, OdHatchPattern& hatchPattern) const;
	void SetHatchOrigin(const OdGePoint3d& origin);
	void SetHatchXAxis(const OdGeVector3d& xAxis);
	void SetHatchYAxis(const OdGeVector3d& yAxis);
	void SetHatRefVecs(double patternAngle, double patternScaleX, double patternScaleY);
	void SetInteriorStyle(EoInt16 interiorStyle);
	void SetInteriorStyleIndex(size_t styleIndex);
	void SetLoopAt(int loopIndex, OdDbHatchPtr& hatchEntity);
	void SetPatternReferenceSystem(const OdGePoint3d& origin, const OdGeVector3d& normal, double patternAngle, double patternScale);
	void SetVertices(const OdGePoint3dArray& vertices);
	size_t SwingVertex() const;

public:
	static EoDbHatch* ConstructFrom(EoDbFile& file);
	static EoDbHatch* ConstructFrom(EoByte* primitiveBuffer, int versionNumber);
	static EoDbHatch* Create(OdDbDatabasePtr database);
	static EoDbHatch* Create(const EoDbHatch& other, OdDbDatabasePtr database);

	static size_t Edge();
	static size_t LegacyInteriorStyleIndex(const OdString& name);
	static void SetEdgeToEvaluate(size_t edgeToEvaluate);
};