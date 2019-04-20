#include "stdafx.h"
#include "AeSysApp.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

EoDbSpline::EoDbSpline() noexcept {
}

EoDbSpline::EoDbSpline(const EoDbSpline& other) {
	m_LayerId = other.m_LayerId;
	m_EntityObjectId = other.m_EntityObjectId;

	m_ColorIndex = other.m_ColorIndex;
	m_LinetypeIndex = other.m_LinetypeIndex;

	m_Spline = other.m_Spline;
}

EoDbSpline::~EoDbSpline() {
}

const EoDbSpline& EoDbSpline::operator=(const EoDbSpline& other) {
	m_LayerId = other.m_LayerId;
	m_EntityObjectId = other.m_EntityObjectId;

	m_ColorIndex = other.m_ColorIndex;
	m_LinetypeIndex = other.m_LinetypeIndex;

	m_Spline = other.m_Spline;

	return (*this);
}

void EoDbSpline::AddReportToMessageList(const OdGePoint3d& point) const {
    CString Report(L"<BSpline>");
    Report += L" Color:" + FormatColorIndex();
    Report += L" Linetype:" + FormatLinetypeIndex();
    theApp.AddStringToMessageList(Report);
}

void EoDbSpline::AddToTreeViewControl(HWND tree, HTREEITEM parent) const noexcept {
	CMainFrame::InsertTreeViewControlItem(tree, parent, L"<BSpline>", this);
}

void EoDbSpline::AssociateWith(OdDbBlockTableRecordPtr& blockTableRecord) {
	OdDbSplinePtr SplineEntity = OdDbSpline::createObject();
	blockTableRecord->appendOdDbEntity(SplineEntity);
	SplineEntity->setDatabaseDefaults();

	SetEntityObjectId(SplineEntity->objectId());

	SplineEntity->setColorIndex(m_ColorIndex);
	SetLinetypeIndex(m_LinetypeIndex);
	
	int Degree;
	bool IsRational;
	bool IsPeriodic;
	OdGeKnotVector Knots;
	OdGePoint3dArray ControlPoints;
	OdGeDoubleArray Weights;
	m_Spline.getDefinitionData(Degree, IsRational, IsPeriodic, Knots, ControlPoints, Weights);

	SplineEntity->setNurbsData(Degree, false, false, false, ControlPoints, Knots, Weights, OdGeContext::gTol.equalPoint());
}

EoDbPrimitive* EoDbSpline::Clone(OdDbDatabasePtr& database) const {
	return (EoDbSpline::Create(*this, database));
}

void EoDbSpline::Display(AeSysView* view, CDC* deviceContext) {
	const OdInt16 ColorIndex = LogicalColorIndex();
	const OdInt16 LinetypeIndex = LogicalLinetypeIndex();

	pstate.SetPen(view, deviceContext, ColorIndex, LinetypeIndex);

	polyline::BeginLineStrip();

	EoGeNurbCurve3d::GeneratePoints(m_Spline);
	polyline::__End(view, deviceContext, LinetypeIndex);
}

void EoDbSpline::FormatExtra(CString& extra) const {
	extra.Empty();
	extra += L"Color;" + FormatColorIndex() + L"\t";
	extra += L"Linetype;" + FormatLinetypeIndex() + L"\t";
	CString NumberOfPoints;
	NumberOfPoints.Format(L"Number of Control Points;%d", m_Spline.numControlPoints());
	extra += NumberOfPoints;
}

void EoDbSpline::FormatGeometry(CString& geometry) const {
	CString ControlPointString;
	for (int ControlPointIndex = 0; ControlPointIndex < m_Spline.numControlPoints(); ControlPointIndex++) {
		const OdGePoint3d ControlPoint = m_Spline.controlPointAt(ControlPointIndex);
		ControlPointString.Format(L"Control Point;%f;%f;%f\t", ControlPoint.x, ControlPoint.y, ControlPoint.z);
		geometry += ControlPointString;
	}
}

void EoDbSpline::GetAllPoints(OdGePoint3dArray& points) const {
    points.setLogicalLength(m_Spline.numControlPoints());
    for (int ControlPointIndex = 0; ControlPointIndex < m_Spline.numControlPoints(); ControlPointIndex++) {
        points[ControlPointIndex] = m_Spline.controlPointAt(ControlPointIndex);
    }
}

OdGePoint3d EoDbSpline::GetCtrlPt() const {
	OdGePoint3d Point;
	if (!m_EntityObjectId.isNull()) {
		OdDbSplinePtr Spline = m_EntityObjectId.safeOpenObject();
		double EndParameter;
		Spline->getEndParam(EndParameter);
		Spline->getPointAtParam(EndParameter / 2., Point);
	}
	else {
		Point = m_Spline.controlPointAt(m_Spline.numControlPoints() / 2);
	}
	return (Point);
}

void EoDbSpline::GetExtents(AeSysView* view, OdGeExtents3d& extents) const {
	if (!m_EntityObjectId.isNull()) {
		OdDbSplinePtr Spline = m_EntityObjectId.safeOpenObject();
		OdGeExtents3d Extents;
		Spline->getGeomExtents(Extents);
		extents.addExt(Extents);
		
	}
	else {
		// <tas="Extents should use the points on the curve and not the control points"</tas>
		for (OdUInt16 w = 0; w < m_Spline.numControlPoints(); w++) {
			extents.addPoint(m_Spline.controlPointAt(w));
		}
	}
}

OdGePoint3d EoDbSpline::GoToNxtCtrlPt() const {
	OdGePoint3d pt;

	if (sm_RelationshipOfPoint <= DBL_EPSILON)
		pt = m_Spline.endPoint();
	else if (sm_RelationshipOfPoint >= 1. - DBL_EPSILON)
		pt = m_Spline.startPoint();
	else if (m_Spline.endPoint().x > m_Spline.startPoint().x)
		pt = m_Spline.startPoint();
	else if (m_Spline.endPoint().x < m_Spline.startPoint().x)
		pt = m_Spline.endPoint();
	else if (m_Spline.endPoint().y > m_Spline.startPoint().y)
		pt = m_Spline.startPoint();
	else
		pt = m_Spline.endPoint();
	return (pt);
}

bool EoDbSpline::IsEqualTo(EoDbPrimitive* other) const {
	bool IsEqual = false;
	const OdDbObjectId OtherObjectId = other->EntityObjectId();
	if (!m_EntityObjectId.isNull() && !OtherObjectId.isNull()) {
		OdDbSplinePtr Spline = m_EntityObjectId.safeOpenObject();
		OdDbSplinePtr OtherSpline = OtherObjectId.safeOpenObject();
		IsEqual = Spline->isEqualTo(OtherSpline);
	}
	return IsEqual;
}

bool EoDbSpline::IsInView(AeSysView* view) const {
	EoGePoint4d pt[2];
	pt[0] = EoGePoint4d(m_Spline.controlPointAt(0), 1.);

	view->ModelViewTransformPoint(pt[0]);

	for (OdUInt16 w = 1; w < m_Spline.numControlPoints(); w++) {
		pt[1] = EoGePoint4d(m_Spline.controlPointAt(w), 1.);

		view->ModelViewTransformPoint(pt[1]);

		if (EoGePoint4d::ClipLine(pt[0], pt[1]))
			return true;

		pt[0] = pt[1];
	}
	return false;
}

bool EoDbSpline::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) const noexcept {
	return false;
}

OdGePoint3d EoDbSpline::SelectAtControlPoint(AeSysView*, const EoGePoint4d& point) const {
	sm_ControlPointIndex = SIZE_T_MAX;
	return (point.Convert3d());
}

bool EoDbSpline::SelectBy(const EoGePoint4d& point, AeSysView* view, OdGePoint3d& ptProj) const {
	polyline::BeginLineStrip();
	EoGeNurbCurve3d::GeneratePoints(m_Spline);

	return (polyline::SelectBy(point, view, sm_RelationshipOfPoint, ptProj));
}

bool EoDbSpline::SelectBy(const OdGePoint3d& lowerLeftCorner, const OdGePoint3d& upperRightCorner, AeSysView* view) const {
	OdGePoint3dArray ControlPoints;
	for (OdUInt16 w = 0; w < m_Spline.numControlPoints(); w++) {
		ControlPoints.append(m_Spline.controlPointAt(w));
	}
	return polyline::SelectUsingRectangle(view, lowerLeftCorner, upperRightCorner, ControlPoints);
}

void EoDbSpline::Set(int degree, const OdGeKnotVector& knots, const OdGePoint3dArray& controlPoints, const OdGeDoubleArray& weights, bool isPeriodic) {
	m_Spline.set(degree, knots, controlPoints, weights, isPeriodic);
}

void EoDbSpline::SetControlPoints(const OdGePoint3dArray& controlPoints) {
	const int NumberOfControlPoints = controlPoints.size();
	const int Degree = EoMin(3, NumberOfControlPoints - 1);

	OdGeKnotVector Knots;
	EoGeNurbCurve3d::SetDefaultKnotVector(Degree, controlPoints, Knots);
	
	OdGeDoubleArray Weights;
	Weights.setLogicalLength(NumberOfControlPoints);

	if (!m_EntityObjectId.isNull()) {
		OdDbSplinePtr Spline = m_EntityObjectId.safeOpenObject(OdDb::kForWrite);
		Spline->setNurbsData(Degree, false, false, false, controlPoints, Knots, Weights, OdGeContext::gTol.equalPoint());
		}
	m_Spline.set(Degree, Knots, controlPoints, Weights);
}

void EoDbSpline::TransformBy(const EoGeMatrix3d& transformMatrix) {
	m_Spline.transformBy(transformMatrix);
}

void EoDbSpline::TranslateUsingMask(const OdGeVector3d& translate, const DWORD mask) {
	for (int ControlPointIndex = 0; ControlPointIndex < m_Spline.numControlPoints(); ControlPointIndex++)
		if (((mask >> ControlPointIndex) & 1UL) == 1) {
			m_Spline.setControlPointAt(ControlPointIndex, m_Spline.controlPointAt(ControlPointIndex) + translate);
		}
}

// <tas="Currently allowing 1st degree (only 2 control points) splines to be saved. This likely will not load in legacy apps"</tas>
bool EoDbSpline::Write(EoDbFile& file) const {
	file.WriteUInt16(kSplinePrimitive);
	file.WriteInt16(m_ColorIndex);
	file.WriteInt16(m_LinetypeIndex);
	file.WriteUInt16(OdUInt16(m_Spline.numControlPoints()));

	for (OdUInt16 ControlPointIndex = 0; ControlPointIndex < m_Spline.numControlPoints(); ControlPointIndex++) {
		file.WritePoint3d(m_Spline.controlPointAt(ControlPointIndex));
	}
	return true;
}

void EoDbSpline::Write(CFile& file, OdUInt8* buffer) const {
	buffer[3] = OdInt8((2 + m_Spline.numControlPoints() * 3) / 8 + 1);
	*((OdUInt16*) &buffer[4]) = OdUInt16(kSplinePrimitive);
	buffer[6] = OdInt8(m_ColorIndex == COLORINDEX_BYLAYER ? sm_LayerColorIndex : m_ColorIndex);
	buffer[7] = OdInt8(m_LinetypeIndex == LINETYPE_BYLAYER ? sm_LayerLinetypeIndex : m_LinetypeIndex);

	*((OdInt16*) &buffer[8]) = (OdInt16) m_Spline.numControlPoints();

	int i = 10;

	for (OdUInt16 w = 0; w < m_Spline.numControlPoints(); w++) {
		((EoVaxPoint3d*) &buffer[i])->Convert(m_Spline.controlPointAt(w));
		i += sizeof(EoVaxPoint3d);
	}
	file.Write(buffer, buffer[3] * 32);
}

// Static

EoDbSpline* EoDbSpline::ConstructFrom(OdUInt8* primitiveBuffer, int versionNumber) {
	// <tas="Has not been tested since the change to EoGeNurbCurve3d internals"</tas>
	OdInt16 ColorIndex;
	OdInt16 LinetypeIndex;

	OdGePoint3dArray ControlPoints;

	if (versionNumber == 1) {
		ColorIndex = OdInt16(primitiveBuffer[4] & 0x000f);
		LinetypeIndex = OdInt16((primitiveBuffer[4] & 0x00ff) >> 4);

		const OdUInt16 NumberOfControlPoints = OdUInt16(((EoVaxFloat*) &primitiveBuffer[8])->Convert());
		ControlPoints.setLogicalLength(NumberOfControlPoints);

		int BufferIndex = 12;

		for (OdUInt16 w = 0; w < NumberOfControlPoints; w++) {
			ControlPoints[w] = ((EoVaxPoint3d*) &primitiveBuffer[BufferIndex])->Convert() * 1.e-3;
			BufferIndex += sizeof(EoVaxPoint3d);
		}
	}
	else {
		ColorIndex = OdInt16(primitiveBuffer[6]);
		LinetypeIndex = OdInt16(primitiveBuffer[7]);

		const OdUInt16 NumberOfControlPoints = *((OdInt16*) &primitiveBuffer[8]);
		ControlPoints.setLogicalLength(NumberOfControlPoints);

		int BufferIndex = 10;

		for (OdUInt16 w = 0; w < NumberOfControlPoints; w++) {
			ControlPoints[w] = ((EoVaxPoint3d*) &primitiveBuffer[BufferIndex])->Convert();
			BufferIndex += sizeof(EoVaxPoint3d);
		}
	}
	EoDbSpline* SplinePrimitive = new EoDbSpline();
	SplinePrimitive->SetColorIndex(ColorIndex);
	SplinePrimitive->SetLinetypeIndex(LinetypeIndex);
	SplinePrimitive->SetControlPoints(ControlPoints);
	return (SplinePrimitive);
}

EoDbSpline* EoDbSpline::Create(const EoDbSpline& other, OdDbDatabasePtr& database) {
    OdDbBlockTableRecordPtr BlockTableRecord = database->getModelSpaceId().safeOpenObject(OdDb::kForWrite);
    OdDbSplinePtr SplineEntity = other.EntityObjectId().safeOpenObject()->clone();
    BlockTableRecord->appendOdDbEntity(SplineEntity);

    EoDbSpline* Spline = new EoDbSpline(other);
    Spline->SetEntityObjectId(SplineEntity->objectId());

    return Spline;
}

EoDbSpline* EoDbSpline::Create(OdDbDatabasePtr& database) {
	OdDbBlockTableRecordPtr BlockTableRecord = database->getModelSpaceId().safeOpenObject(OdDb::kForWrite);

	OdDbSplinePtr SplineEntity = OdDbSpline::createObject();
	SplineEntity->setDatabaseDefaults(database);
	BlockTableRecord->appendOdDbEntity(SplineEntity);

	EoDbSpline* Spline = new EoDbSpline();
	Spline->SetEntityObjectId(SplineEntity->objectId());
	
	Spline->SetColorIndex(pstate.ColorIndex());
	Spline->SetLinetypeIndex(pstate.LinetypeIndex());
	
	return Spline;
}

OdDbSplinePtr EoDbSpline::Create(OdDbBlockTableRecordPtr& blockTableRecord) {
    OdDbSplinePtr Spline = OdDbSpline::createObject();
    Spline->setDatabaseDefaults(blockTableRecord->database());

    blockTableRecord->appendOdDbEntity(Spline);
    Spline->setColorIndex(pstate.ColorIndex());

    const auto Linetype {EoDbPrimitive::LinetypeObjectFromIndex(pstate.LinetypeIndex())};

    Spline->setLinetype(Linetype);

    return Spline;
}

OdDbSplinePtr EoDbSpline::Create(OdDbBlockTableRecordPtr& blockTableRecord, EoDbFile& file) {
    OdDbSplinePtr Spline = OdDbSpline::createObject();
    Spline->setDatabaseDefaults(blockTableRecord->database());

    blockTableRecord->appendOdDbEntity(Spline);

    Spline->setColorIndex(file.ReadInt16());

    const auto Linetype {EoDbPrimitive::LinetypeObjectFromIndex(file.ReadInt16())};

    Spline->setLinetype(Linetype);

    const auto NumberOfControlPoints = file.ReadUInt16();

    const int Degree = EoMin(3, NumberOfControlPoints - 1);

    OdGePoint3dArray ControlPoints;
    for (int ControlPointIndex = 0; ControlPointIndex < NumberOfControlPoints; ControlPointIndex++) {
        ControlPoints.append(file.ReadPoint3d());
    }
    OdGeKnotVector Knots;
    EoGeNurbCurve3d::SetDefaultKnotVector(Degree, ControlPoints, Knots);
    OdGeDoubleArray Weights;
    Weights.setLogicalLength(NumberOfControlPoints);

    Spline->setNurbsData(Degree, false, false, false, ControlPoints, Knots, Weights, OdGeContext::gTol.equalPoint());
    
    return Spline;
}

EoDbSpline* EoDbSpline::Create(OdDbSplinePtr& spline) {
    EoDbSpline* Spline = new EoDbSpline();
    Spline->SetEntityObjectId(spline->objectId());
    Spline->SetColorIndex_(spline->colorIndex());
    Spline->SetLinetypeIndex_(EoDbLinetypeTable::LegacyLinetypeIndex(spline->linetype()));

    int Degree;
    bool Rational;
    bool Closed;
    bool Periodic;
    OdGePoint3dArray ControlPoints;
    OdGeDoubleArray Weights;
    OdGeKnotVector Knots;
    double Tolerance;

    spline->getNurbsData(Degree, Rational, Closed, Periodic, ControlPoints, Knots, Weights, Tolerance);
    
    // <tas="Only creating non-periodic splines."></tas>
    if (Periodic) {
        ATLTRACE2(atlTraceGeneral, 0, L"Periodic %s was not converted ...\n", (LPCWSTR) spline->desc()->name());
    } else {
        Spline->Set(Degree, Knots, ControlPoints, Weights, Periodic);
        // ConvertCurveData(entity, Spline);
    }

    return Spline;
}
