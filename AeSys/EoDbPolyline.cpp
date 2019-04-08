#include "stdafx.h"
#include "AeSysApp.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

size_t EoDbPolyline::sm_EdgeToEvaluate = 0;
size_t EoDbPolyline::sm_Edge = 0;
size_t EoDbPolyline::sm_PivotVertex = 0;

EoDbPolyline::EoDbPolyline()
	: m_Flags(0)
    , m_ConstantWidth(0.)
    , m_Elevation(0.)
    , m_Thickness(0.)
    , m_Normal(OdGeVector3d::kZAxis) {
	m_Vertices.clear();
	m_StartWidths.clear();
	m_EndWidths.clear();
	m_Bulges.clear();
}

EoDbPolyline::EoDbPolyline(const EoDbPolyline& other) {
	m_Flags = other.m_Flags;
	m_LayerId = other.m_LayerId;
	m_EntityObjectId = other.m_EntityObjectId;

	m_ColorIndex = other.m_ColorIndex;
	m_LinetypeIndex = other.m_LinetypeIndex;
	
	m_ConstantWidth = other.m_ConstantWidth;
	m_Elevation = other.m_Elevation;
	m_Thickness = other.m_Thickness;
	m_Vertices.append(other.m_Vertices);
	m_StartWidths.append(other.m_StartWidths);
	m_EndWidths.append(other.m_EndWidths);
	m_Bulges.append(other.m_Bulges);
	m_Normal = other.m_Normal;
}

EoDbPolyline::~EoDbPolyline() {
}

const EoDbPolyline& EoDbPolyline::operator=(const EoDbPolyline& other) {
	m_Flags = other.m_Flags;
	m_LayerId = other.m_LayerId;
	m_EntityObjectId = other.m_EntityObjectId;

	m_ColorIndex = other.m_ColorIndex;
	m_LinetypeIndex = other.m_LinetypeIndex;
	
	m_ConstantWidth = other.m_ConstantWidth;
	m_Elevation = other.m_Elevation;
	m_Thickness = other.m_Thickness;
	m_Vertices.append(other.m_Vertices);
	m_StartWidths.append(other.m_StartWidths);
	m_EndWidths.append(other.m_EndWidths);
	m_Bulges.append(other.m_Bulges);
	m_Normal = other.m_Normal;

	return (*this);
}

void EoDbPolyline::AddReportToMessageList(const OdGePoint3d& point) const {
	const size_t NumberOfVertices = m_Vertices.size();

	if (sm_Edge > 0 && sm_Edge <= NumberOfVertices) {
		OdGePoint3d ptBeg;
		OdGePoint3d ptEnd;
		GetPointAt(sm_Edge - 1, ptBeg);
		GetPointAt(sm_Edge % NumberOfVertices, ptEnd);

		if (sm_PivotVertex < NumberOfVertices) {
			GetPointAt(sm_PivotVertex, ptBeg);
			GetPointAt(SwingVertex(), ptEnd);
		}
		double AngleInXYPlane;
		const double EdgeLength = OdGeVector3d(ptEnd - ptBeg).length();

		if (OdGeVector3d(ptBeg - point).length() > EdgeLength * .5) {
			AngleInXYPlane = EoGeLineSeg3d(ptEnd, ptBeg).AngleFromXAxis_xy();
		}
		else {
			AngleInXYPlane = EoGeLineSeg3d(ptBeg, ptEnd).AngleFromXAxis_xy();
		}
		CString Report(L"<Polyline-Edge>");
		Report += L" Color:" + FormatColorIndex();
		Report += L" Linetype:" + FormatLinetypeIndex();
		Report += L" [" + theApp.FormatLength(EdgeLength, theApp.GetUnits()) + L" @ " + theApp.FormatAngle(AngleInXYPlane) +L"]";
		theApp.AddStringToMessageList(Report);

		theApp.SetEngagedLength(EdgeLength);
		theApp.SetEngagedAngle(AngleInXYPlane);
	}
}

void EoDbPolyline::AddToTreeViewControl(HWND tree, HTREEITEM parent) const noexcept {
	CMainFrame::InsertTreeViewControlItem(tree, parent, L"<Polyline>", this);
}

void EoDbPolyline::AssociateWith(OdDbBlockTableRecordPtr& blockTableRecord) {
	OdDbPolylinePtr PolylineEntity = OdDbPolyline::createObject();
	blockTableRecord->appendOdDbEntity(PolylineEntity);
	PolylineEntity->setDatabaseDefaults();
	
	SetEntityObjectId(PolylineEntity->objectId());
	
	PolylineEntity->setColorIndex(m_ColorIndex);
	SetLinetypeIndex(m_LinetypeIndex);
	PolylineEntity->setConstantWidth(m_ConstantWidth);
	PolylineEntity->setElevation(m_Elevation);
	PolylineEntity->setThickness(m_Thickness);
	for (size_t VertexIndex = 0; VertexIndex < m_Vertices.size(); VertexIndex++) {
		PolylineEntity->addVertexAt(VertexIndex, m_Vertices[VertexIndex]);
		PolylineEntity->setWidthsAt(VertexIndex, m_StartWidths[VertexIndex], m_EndWidths[VertexIndex]);
		PolylineEntity->setBulgeAt(VertexIndex, m_Bulges[VertexIndex]);
	}
	PolylineEntity->setClosed(m_Flags == EoDbPolyline::sm_Closed);
}

EoDbPrimitive* EoDbPolyline::Clone(OdDbDatabasePtr& database) const {
	return (EoDbPolyline::Create(*this, database));
}

void EoDbPolyline::Display(AeSysView* view, CDC* deviceContext) {
	const OdInt16 ColorIndex = LogicalColorIndex();
	const OdInt16 LinetypeIndex = LogicalLinetypeIndex();

	pstate.SetPen(view, deviceContext, ColorIndex, LinetypeIndex);

	if (IsClosed())
		polyline::BeginLineLoop();
	else
		polyline::BeginLineStrip();

	const OdGePoint3d Origin = OdGePoint3d::kOrigin + m_Normal * m_Elevation;
	const OdGeVector3d XAxis = ComputeArbitraryAxis(m_Normal);
	const OdGeVector3d YAxis = m_Normal.crossProduct(XAxis);

	OdGePlane Plane(Origin, XAxis, YAxis);
	OdGePoint3d Vertex;

	for (size_t VertexIndex = 0; VertexIndex < m_Vertices.size(); VertexIndex++) {
		Vertex.set(Plane, m_Vertices[VertexIndex]);
		polyline::SetVertex(Vertex);
	}
	polyline::__End(view, deviceContext, LinetypeIndex);
}

void EoDbPolyline::FormatExtra(CString& extra) const {
	extra.Empty();
	extra += L"Color;" + FormatColorIndex() + L"\t";
	extra += L"Linetype;" + FormatLinetypeIndex() + L"\t";
	CString NumberOfVertices;
	NumberOfVertices.Format(L"Number of Vertices;%d", m_Vertices.size());
	extra += NumberOfVertices;
}

void EoDbPolyline::FormatGeometry(CString& geometry) const {
	CString PointString;
	for (size_t VertexIndex = 0; VertexIndex < m_Vertices.size(); VertexIndex++) {
		OdGePoint3d Point;
		GetPointAt(VertexIndex, Point);
		PointString.Format(L"Point;%f;%f;%f\t", Point.x, Point.y, Point.z);
		geometry += PointString;
	}
}

void EoDbPolyline::GetAllPoints(OdGePoint3dArray& points) const {
	points.clear();
	for (size_t VertexIndex = 0; VertexIndex < points.size(); VertexIndex++) {
		GetPointAt(VertexIndex, points[VertexIndex]);
	}
}

OdGePoint3d EoDbPolyline::GetCtrlPt() const {
	OdGePoint3d StartPoint;
	OdGePoint3d EndPoint;
	const size_t NumberOfVertices = m_Vertices.size();
	GetPointAt(sm_Edge - 1, StartPoint);
	GetPointAt(sm_Edge % NumberOfVertices, EndPoint);

	return (EoGeLineSeg3d(StartPoint, EndPoint).midPoint());
}

void EoDbPolyline::GetExtents(AeSysView* view, OdGeExtents3d& extents) const {
	OdGePoint3d Point;
	for (size_t VertexIndex = 0; VertexIndex < m_Vertices.size(); VertexIndex++) {
		GetPointAt(VertexIndex, Point);
		extents.addPoint(Point);
	}
}

/// <summary> This function sets point to the 3D location of the vertex index in World Coordinates.</summary>
void EoDbPolyline::GetPointAt(int vertexIndex, OdGePoint3d& point) const {
	const OdGePoint3d Origin = OdGePoint3d::kOrigin + m_Normal * m_Elevation;
	const OdGeVector3d XAxis = ComputeArbitraryAxis(m_Normal);
	const OdGeVector3d YAxis = m_Normal.crossProduct(XAxis);
	OdGePlane Plane(Origin, XAxis, YAxis);
	point.set(Plane, m_Vertices[vertexIndex]);
}

OdGePoint3d EoDbPolyline::GoToNxtCtrlPt() const {
	const size_t NumberOfVertices = m_Vertices.size();
	if (sm_PivotVertex >= NumberOfVertices) { // have not yet rocked to a vertex
		const size_t StartVertexIndex = sm_Edge - 1;
		const size_t EndVertexIndex = sm_Edge % NumberOfVertices;

		OdGePoint3d StartPoint;
		GetPointAt(StartVertexIndex, StartPoint);
		OdGePoint3d EndPoint;
		GetPointAt(EndVertexIndex, EndPoint);

		if (EndPoint.x > StartPoint.x) {
			sm_PivotVertex = StartVertexIndex;
		}
		else if (EndPoint.x < StartPoint.x) {
			sm_PivotVertex = EndVertexIndex;
		}
		else if (EndPoint.y > StartPoint.y) {
			sm_PivotVertex = StartVertexIndex;
		}
		else {
			sm_PivotVertex = EndVertexIndex;
		}
	}
	else if (sm_PivotVertex == 0) {
		if (sm_Edge == 1) {
			sm_PivotVertex = 1;
		}
		else {
			sm_PivotVertex = NumberOfVertices - 1;
		}
	}
	else if (sm_PivotVertex == NumberOfVertices - 1) {
		if (sm_Edge == NumberOfVertices) {
			sm_PivotVertex = 0;
		}
		else {
			sm_PivotVertex--;
		}
	}
	else {
		if (sm_Edge == sm_PivotVertex) {
			sm_PivotVertex--;
		}
		else {
			sm_PivotVertex++;
		}
	}
	OdGePoint3d PivotPoint;
	GetPointAt(sm_PivotVertex, PivotPoint);
	return (PivotPoint);
}

bool EoDbPolyline::IsClosed() const noexcept {
	return (m_Flags != 0);
}

bool EoDbPolyline::IsInView(AeSysView* view) const {
	OdGePoint3d Point;
	EoGePoint4d	pt[2];
	GetPointAt(0, Point);
	pt[0] = EoGePoint4d(Point, 1.);
	view->ModelViewTransformPoint(pt[0]);

	for (size_t VertexIndex = 1; VertexIndex < m_Vertices.size(); VertexIndex++) {
		GetPointAt(VertexIndex, Point);
		pt[1] = EoGePoint4d(Point, 1.);
		view->ModelViewTransformPoint(pt[1]);

		if (EoGePoint4d::ClipLine(pt[0], pt[1]))
			return true;

		pt[0] = pt[1];
	}
	return false;
}

bool EoDbPolyline::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) const noexcept {
	// <tas="Polyline: need to implement IsPointOnControlPoint"</tas>
	return false;
}

bool EoDbPolyline::PivotOnGripPoint(AeSysView* view, const EoGePoint4d& point) noexcept {
	const size_t NumberOfVertices = m_Vertices.size();
	if (sm_PivotVertex >= NumberOfVertices) { // Not engaged at a vertex
		return false;
	}	
	OdGePoint3d Point;
	GetPointAt(sm_PivotVertex, Point);
	EoGePoint4d ptCtrl(Point, 1.);
	view->ModelViewTransformPoint(ptCtrl);

	if (ptCtrl.DistanceToPointXY(point) >= sm_SelectApertureSize) { // Not on proper vertex
		return false;
	}
	if (sm_PivotVertex == 0) {
		sm_Edge = sm_Edge == 1 ? NumberOfVertices : 1;
	}
	else if (sm_PivotVertex == NumberOfVertices - 1) {
		sm_Edge = (sm_Edge == NumberOfVertices) ? sm_Edge - 1 : NumberOfVertices;
	}
	else if (sm_PivotVertex == sm_Edge) {
		sm_Edge++;
	}
	else {
		sm_Edge--;
	}
	return true;
}

OdGePoint3d EoDbPolyline::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) const {
	sm_ControlPointIndex = SIZE_T_MAX;
	double dApert = sm_SelectApertureSize;

	sm_PivotVertex = m_Vertices.size();

	for (size_t VertexIndex = 0; VertexIndex < m_Vertices.size(); VertexIndex++) {
		OdGePoint3d Point;
		GetPointAt(VertexIndex, Point);
		EoGePoint4d pt(Point, 1.);
		view->ModelViewTransformPoint(pt);

		const double dDis = point.DistanceToPointXY(pt);

		if (dDis < dApert) {
			sm_ControlPointIndex = VertexIndex;
			dApert = dDis;

			sm_Edge = VertexIndex + 1;
			sm_PivotVertex = VertexIndex;
		}
	}
	OdGePoint3d ControlPoint(OdGePoint3d::kOrigin);
	if (sm_ControlPointIndex != SIZE_T_MAX) {
		GetPointAt(sm_ControlPointIndex, ControlPoint);
	}
	return (ControlPoint);
}

bool EoDbPolyline::SelectBy(const EoGePoint4d& point, AeSysView* view, OdGePoint3d& ptProj) const {
	const size_t NumberOfVertices = m_Vertices.size();
	if (sm_EdgeToEvaluate > 0 && sm_EdgeToEvaluate <= NumberOfVertices) { // Evaluate specified edge of polyline
		OdGePoint3d StartPoint;
		GetPointAt(sm_EdgeToEvaluate - 1, StartPoint);
		OdGePoint3d EndPoint;
		GetPointAt(sm_EdgeToEvaluate % NumberOfVertices, EndPoint);
		EoGePoint4d ptBeg(StartPoint, 1.);
		EoGePoint4d ptEnd(EndPoint, 1.);

		view->ModelViewTransformPoint(ptBeg);
		view->ModelViewTransformPoint(ptEnd);

		EoGeLineSeg3d LineSegment(ptBeg.Convert3d(), ptEnd.Convert3d());
		if (LineSegment.IsSelectedBy_xy(point.Convert3d(), view->SelectApertureSize(), ptProj, sm_RelationshipOfPoint)) {
			ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
			return true;
		}
	}
	else { // Evaluate entire polyline
		size_t NumberofEdges = NumberOfVertices;
		if (!IsClosed())
			NumberofEdges--;

		OdGePoint3d StartPoint;
		GetPointAt(0, StartPoint);
		EoGePoint4d ptBeg(StartPoint, 1.);
		view->ModelViewTransformPoint(ptBeg);

		for (size_t VertexIndex = 1; VertexIndex <= NumberofEdges; VertexIndex++) {
			OdGePoint3d EndPoint;
			GetPointAt(VertexIndex % NumberOfVertices, EndPoint);
			EoGePoint4d ptEnd(EndPoint, 1.);
			view->ModelViewTransformPoint(ptEnd);

			EoGeLineSeg3d LineSegment(ptBeg.Convert3d(), ptEnd.Convert3d());
			if (LineSegment.IsSelectedBy_xy(point.Convert3d(), view->SelectApertureSize(), ptProj, sm_RelationshipOfPoint)) {
				ptProj.z = ptBeg.z + sm_RelationshipOfPoint * (ptEnd.z - ptBeg.z);
				sm_Edge = VertexIndex;
				sm_PivotVertex = NumberOfVertices;
				return true;
			}
			ptBeg = ptEnd;
		}
	}
	return false;
}

bool EoDbPolyline::SelectBy(const OdGePoint3d& lowerLeftCorner, const OdGePoint3d& upperRightCorner, AeSysView* view) const {
	OdGePoint3dArray Points;
	for (size_t VertexIndex = 0; VertexIndex < m_Vertices.size(); VertexIndex++) {
		OdGePoint3d Point;
		GetPointAt(VertexIndex, Point);
		Points.append(Point);
	}
	return polyline::SelectUsingRectangle(view, lowerLeftCorner, upperRightCorner, Points);
}

void EoDbPolyline::SetClosed(bool closed) {
	if (!m_EntityObjectId.isNull()) {
		OdDbPolylinePtr PolylineEntity = m_EntityObjectId.safeOpenObject(OdDb::kForWrite);
		PolylineEntity->setClosed(closed);
	}
	if (closed) {
		m_Flags |= sm_Closed;
	}
	else {
		m_Flags &= ~sm_Closed;
	}
}

void EoDbPolyline::AppendVertex(const OdGePoint2d& vertex, double bulge, double startWidth, double endWidth) {
	const size_t VertexIndex = m_Vertices.append(vertex);
	m_Bulges.append(bulge);
	m_StartWidths.append(startWidth);
	m_EndWidths.append(endWidth);
}

void EoDbPolyline::SetConstantWidth(double constantWidth) noexcept {
	m_ConstantWidth = constantWidth;
}

void EoDbPolyline::SetElevation(double elevation) {
	if (!m_EntityObjectId.isNull()) {
		OdDbPolylinePtr PolylineEntity = m_EntityObjectId.safeOpenObject(OdDb::kForWrite);
		PolylineEntity->setElevation(elevation);
	}
	m_Elevation = elevation;
}

void EoDbPolyline::SetNormal(const OdGeVector3d& normal) {
	if (!m_EntityObjectId.isNull()) {
		OdDbPolylinePtr PolylineEntity = m_EntityObjectId.safeOpenObject(OdDb::kForWrite);
		PolylineEntity->setNormal(normal);
	}
	m_Normal = normal;
}

void EoDbPolyline::SetPoints(const OdGePoint3dArray& points) {
	if (!m_EntityObjectId.isNull()) {
		OdDbPolylinePtr PolylineEntity = m_EntityObjectId.safeOpenObject(OdDb::kForWrite);
		for (size_t VertexIndex = 0; VertexIndex < points.size(); VertexIndex++) {
			PolylineEntity->setPointAt(VertexIndex, points[VertexIndex].convert2d());
		}
		PolylineEntity->setClosed(m_Flags == EoDbPolyline::sm_Closed);
	}
}

void EoDbPolyline::TransformBy(const EoGeMatrix3d& transformMatrix) {
	// <tas="TransformBy broken. Need to go to world and back?"</tas>
	for (size_t VertexIndex = 0; VertexIndex < m_Vertices.size(); VertexIndex++) {
		//m_Vertices[VertexIndex].transformBy(transformMatrix);
	}
}

void EoDbPolyline::TranslateUsingMask(const OdGeVector3d& translate, const DWORD mask) {
	// <tas="TranslateUsingMask broken. Need to go to world and back? This type of operation could get polyline vertex off plane"</tas>
	for (size_t VertexIndex = 0; VertexIndex < m_Vertices.size(); VertexIndex++) {
	//	if (((mask >> VertexIndex) & 1UL) == 1)
	//		m_Vertices[VertexIndex] += translate;
	}
}

bool EoDbPolyline::Write(EoDbFile& file) const {
	file.WriteUInt16(kPolylinePrimitive);
	file.WriteInt16(m_ColorIndex);
	file.WriteInt16(m_LinetypeIndex);
    file.WriteUInt16(m_Flags);
    file.WriteDouble(m_ConstantWidth);
    file.WriteDouble(m_Elevation);
    file.WriteDouble(m_Thickness);
    file.WriteVector3d(m_Normal);

    file.WriteUInt16(OdUInt16(m_Vertices.size()));

    for (size_t VertexIndex = 0; VertexIndex < m_Vertices.size(); VertexIndex++) {
		file.WritePoint2d(m_Vertices[VertexIndex]);
        file.WriteDouble(m_StartWidths[VertexIndex]);
        file.WriteDouble(m_EndWidths[VertexIndex]);
        file.WriteDouble(m_Bulges[VertexIndex]);
	}
	return true;
}

/// <remarks> Job (.jb1) files did not have a polyline primitive</remarks>
void EoDbPolyline::Write(CFile& file, OdUInt8* buffer) const noexcept {
};

size_t EoDbPolyline::SwingVertex() const {
	const size_t NumberOfVertices = m_Vertices.size();
	size_t SwingVertex;

	if (sm_PivotVertex == 0) {
		SwingVertex = (sm_Edge == 1) ? 1 : NumberOfVertices - 1;
	}
	else if (sm_PivotVertex == NumberOfVertices - 1) {
		SwingVertex = (sm_Edge == NumberOfVertices) ? 0 : sm_PivotVertex - 1;
	}
	else {
		SwingVertex = (sm_Edge == sm_PivotVertex) ? sm_PivotVertex - 1 : sm_PivotVertex + 1;
	}
	return (SwingVertex);
}

size_t EoDbPolyline::Edge() noexcept {
	return sm_Edge;
}

void EoDbPolyline::SetEdgeToEvaluate(size_t edgeToEvaluate) noexcept {
	sm_EdgeToEvaluate = edgeToEvaluate;
}

EoDbPolyline* EoDbPolyline::Create(const EoDbPolyline& polyline, OdDbDatabasePtr database) {
    OdDbBlockTableRecordPtr BlockTableRecord = database->getModelSpaceId().safeOpenObject(OdDb::kForWrite);
    OdDbPolylinePtr PolylineEntity = polyline.EntityObjectId().safeOpenObject()->clone();
    BlockTableRecord->appendOdDbEntity(PolylineEntity);

    EoDbPolyline* Polyline = new EoDbPolyline(polyline);
    Polyline->SetEntityObjectId(PolylineEntity->objectId());

    return Polyline;
}

EoDbPolyline* EoDbPolyline::Create(OdDbDatabasePtr database) {
	EoDbPolyline* PolylinePrimitive = new EoDbPolyline();
	PolylinePrimitive->SetColorIndex(pstate.ColorIndex());
	PolylinePrimitive->SetLinetypeIndex(pstate.LinetypeIndex());
	OdDbBlockTableRecordPtr BlockTableRecord = database->getModelSpaceId().safeOpenObject(OdDb::kForWrite);
	PolylinePrimitive->AssociateWith(BlockTableRecord);
	return PolylinePrimitive;
}

OdDbPolylinePtr EoDbPolyline::Create(OdDbBlockTableRecordPtr blockTableRecord) {
    OdDbPolylinePtr Polyline = OdDbPolyline::createObject();
    Polyline->setDatabaseDefaults(blockTableRecord->database());

    blockTableRecord->appendOdDbEntity(Polyline);
    Polyline->setColorIndex(pstate.ColorIndex());

    const OdDbObjectId Linetype = EoDbPrimitive::LinetypeObjectFromIndex(pstate.LinetypeIndex());

    Polyline->setLinetype(Linetype);

    return Polyline;
}

OdDbPolylinePtr EoDbPolyline::Create(OdDbBlockTableRecordPtr blockTableRecord, EoDbFile& file) {
    OdDbPolylinePtr Polyline = OdDbPolyline::createObject();
    Polyline->setDatabaseDefaults(blockTableRecord->database());

    blockTableRecord->appendOdDbEntity(Polyline);

    Polyline->setColorIndex(file.ReadInt16());

    const OdDbObjectId Linetype = EoDbPrimitive::LinetypeObjectFromIndex(file.ReadInt16());

    Polyline->setLinetype(Linetype);

    OdUInt16 Flags = file.ReadUInt16();
    auto Closed {(Flags && sm_Closed) == sm_Closed};
    Polyline->setClosed(Closed);

    Polyline->setConstantWidth(file.ReadDouble());
    Polyline->setElevation(file.ReadDouble());
    Polyline->setThickness(file.ReadDouble());
    Polyline->setNormal(file.ReadVector3d());

    const OdUInt16 NumberOfVertices = file.ReadUInt16();

    for (size_t VertexIndex = 0; VertexIndex < NumberOfVertices; VertexIndex++) {
        Polyline->addVertexAt(VertexIndex, file.ReadPoint2d());
        Polyline->setWidthsAt(VertexIndex, file.ReadDouble(), file.ReadDouble());
        Polyline->setBulgeAt(VertexIndex, file.ReadDouble());
    }
    return (Polyline);
}

EoDbPolyline* EoDbPolyline::Create(OdDbPolylinePtr polyline) {
    EoDbPolyline* Polyline = new EoDbPolyline();
    Polyline->SetEntityObjectId(polyline->objectId());
    Polyline->SetColorIndex_(polyline->colorIndex());
    Polyline->SetLinetypeIndex_(EoDbLinetypeTable::LegacyLinetypeIndex(polyline->linetype()));

    const size_t NumberOfVertices {polyline->numVerts()};

    auto Vertex {OdGePoint2d::kOrigin};
    auto StartWidth {0.};
    auto EndWidth {0.};
    
    for (size_t VertexIndex = 0; VertexIndex < NumberOfVertices; VertexIndex++) {
        polyline->getPointAt(VertexIndex, Vertex);
        polyline->getWidthsAt(VertexIndex, StartWidth, EndWidth);

        Polyline->AppendVertex(Vertex, polyline->getBulgeAt(VertexIndex), StartWidth, EndWidth);
    }
    Polyline->SetClosed(polyline->isClosed());
    Polyline->SetConstantWidth(polyline->getConstantWidth());
    Polyline->SetNormal(polyline->normal());
    Polyline->SetElevation(polyline->elevation());

    return Polyline;
}
