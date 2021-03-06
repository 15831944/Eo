#include "stdafx.h"
#include "AeSysView.h"
#include "PrimState.h"
#include "EoGePolyline.h"

EoGeLineSeg3d::EoGeLineSeg3d(const OdGePoint3d& startPoint, const OdGePoint3d& endPoint)
	: OdGeLineSeg3d(startPoint, endPoint) {}

double EoGeLineSeg3d::AngleBetween_xy(const EoGeLineSeg3d& line) const {
	auto v1 {endPoint() - startPoint()};
	v1.z = 0.0;
	auto v2 {line.endPoint() - line.startPoint()};
	v2.z = 0.0;
	const auto dSumProd {v1.lengthSqrd() * v2.lengthSqrd()};
	if (dSumProd > DBL_EPSILON) {
		auto dVal {v1.dotProduct(v2) / sqrt(dSumProd)};
		dVal = EoMax(- 1.0, EoMin(1.0, dVal));
		return acos(dVal);
	}
	return 0.0;
}

double EoGeLineSeg3d::AngleFromXAxis_xy() const {
	const auto Vector {endPoint() - startPoint()};
	auto Angle {0.0};
	if (fabs(Vector.x) > DBL_EPSILON || fabs(Vector.y) > DBL_EPSILON) {
		Angle = atan2(Vector.y, Vector.x);
		if (Angle < 0.0) {
			Angle += Oda2PI;
		}
	}
	return Angle;
}

OdGePoint3d EoGeLineSeg3d::ConstrainToAxis(const double influenceAngle, const double axisOffsetAngle) const {
	EoGeMatrix3d TransformMatrix;
	TransformMatrix.setToTranslation(- startPoint().asVector());
	EoGeMatrix3d RotationMatrix;
	RotationMatrix.setToRotation(- EoToRadian(axisOffsetAngle), OdGeVector3d::kZAxis);
	TransformMatrix.preMultBy(RotationMatrix);
	auto pt {endPoint()};
	pt.transformBy(TransformMatrix);
	const auto dX {pt.x * pt.x};
	const auto dY {pt.y * pt.y};
	const auto dZ {pt.z * pt.z};
	auto dLen {sqrt(dX + dY + dZ)};
	if (dLen > DBL_EPSILON) { // Not a zero length line
		if (dX >= EoMax(dY, dZ)) { // Major component of line is along x-axis
			dLen = sqrt(dY + dZ);
			if (dLen > DBL_EPSILON) { // Not already on the x-axis
				if (dLen / fabs(pt.x) < tan(EoToRadian(influenceAngle))) { // Within cone of influence .. snap to x-axis
					pt.y = 0.0;
					pt.z = 0.0;
				}
			}
		} else if (dY >= dZ) { // Major component of line is along y-axis
			dLen = sqrt(dX + dZ);
			if (dLen > DBL_EPSILON) { // Not already on the y-axis
				if (dLen / fabs(pt.y) < tan(EoToRadian(influenceAngle))) { // Within cone of influence .. snap to y-axis
					pt.x = 0.0;
					pt.z = 0.0;
				}
			}
		} else {
			dLen = sqrt(dX + dY);
			if (dLen > DBL_EPSILON) { // Not already on the z-axis
				if (dLen / fabs(pt.z) < tan(EoToRadian(influenceAngle))) { // Within cone of influence .. snap to z-axis
					pt.x = 0.0;
					pt.y = 0.0;
				}
			}
		}
	}
	TransformMatrix.invert();
	pt.transformBy(TransformMatrix);
	return pt;
}

// <tas="CutAt point does not do on the line checks"</tas>
unsigned short EoGeLineSeg3d::CutAt(const OdGePoint3d& point, EoGeLineSeg3d& line) {
	unsigned short wRet = 0;
	line = *this;
	if (point != startPoint() && point != endPoint()) {
		line.SetEndPoint(point);
		SetStartPoint(point);
		wRet++;
	}
	return wRet;
}

int EoGeLineSeg3d::DirectedRelationshipOf(const OdGePoint3d& point) const {
	const auto Determinant {startPoint().x * (endPoint().y - point.y) - endPoint().x * (startPoint().y - point.y) + point.x * (startPoint().y - endPoint().y)};
	if (Determinant > DBL_EPSILON) {
		return 1;
	}
	if (Determinant < -DBL_EPSILON) {
		return -1;
	}
	return 0;
}

void EoGeLineSeg3d::Display(AeSysView* view, CDC* deviceContext) const {
	const auto LinetypeIndex {g_PrimitiveState.LinetypeIndex()};
	if (EoDbPrimitive::IsSupportedLinetype(LinetypeIndex)) {
		EoGePoint4d pt[] = {EoGePoint4d(startPoint(), 1.0), EoGePoint4d(endPoint(), 1.0)};
		view->ModelViewTransformPoints(2, pt);
		if (EoGePoint4d::ClipLine(pt[0], pt[1])) {
			CPoint pnt[2];
			view->DoViewportProjection(pnt, 2, pt);
			deviceContext->Polyline(pnt, 2);
		}
	} else {
		polyline::BeginLineStrip();
		polyline::SetVertex(startPoint());
		polyline::SetVertex(endPoint());
		polyline::__End(view, deviceContext, LinetypeIndex);
	}
}

void EoGeLineSeg3d::Extents(OdGePoint3d& minimum, OdGePoint3d& maximum) const {
	minimum.x = EoMin(startPoint().x, endPoint().x);
	minimum.y = EoMin(startPoint().y, endPoint().y);
	minimum.z = EoMin(startPoint().z, endPoint().z);
	maximum.x = EoMax(startPoint().x, endPoint().x);
	maximum.y = EoMax(startPoint().y, endPoint().y);
	maximum.z = EoMax(startPoint().z, endPoint().z);
}

bool EoGeLineSeg3d::GetParallels(const double distanceBetweenLines, const double eccentricity, EoGeLineSeg3d& leftLine, EoGeLineSeg3d& rightLine) const {
	leftLine = *this;
	rightLine = *this;
	const auto LengthOfLines {length()};
	if (LengthOfLines > FLT_EPSILON) {
		const auto X {(endPoint().y - startPoint().y) * distanceBetweenLines / LengthOfLines};
		const auto Y {(endPoint().x - startPoint().x) * distanceBetweenLines / LengthOfLines};
		leftLine.translateBy(OdGeVector3d(- X * eccentricity, Y * eccentricity, 0.0));
		rightLine.translateBy(OdGeVector3d(X * (1.0 - eccentricity), - Y * (1.0 - eccentricity), 0.0));
		return true;
	}
	return false;
}

bool EoGeLineSeg3d::IntersectWith_xy(const EoGeLineSeg3d& line, OdGePoint3d& intersection) const {
	auto Start1End1 {endPoint() - startPoint()};
	const auto Start2End2 {line.endPoint() - line.startPoint()};
	const auto Determinant {Start1End1.x * Start2End2.y - Start2End2.x * Start1End1.y};
	if (fabs(Determinant) > DBL_EPSILON) {
		const auto Start1Start2 {line.startPoint() - startPoint()};
		const auto dT {(Start1Start2.y * Start2End2.x - Start2End2.y * Start1Start2.x) / Determinant};
		Start1End1 *= dT;
		intersection = startPoint() - Start1End1;
		return true;
	}
	return false;
}

bool EoGeLineSeg3d::IntersectWithInfinite(const EoGeLineSeg3d& line, OdGePoint3d& intersection) const {
	OdGeLine3d InfiniteFirstLine;
	getLine(InfiniteFirstLine);
	OdGeLine3d InfiniteSecondLine;
	line.getLine(InfiniteSecondLine);
	return InfiniteFirstLine.intersectWith(InfiniteSecondLine, intersection);
}

bool EoGeLineSeg3d::IsContainedBy_xy(const OdGePoint3d& lowerLeftPoint, const OdGePoint3d& upperRightPoint) const {
	OdGePoint3d Points[2];
	Points[0] = startPoint();
	Points[1] = endPoint();
	const auto dX {endPoint().x - startPoint().x};
	const auto dY {endPoint().y - startPoint().y};
	auto i {1};
	unsigned Out[2];
	Out[0] = RelationshipToRectangleOf(Points[0], lowerLeftPoint, upperRightPoint);
	for (;;) {
		Out[i] = RelationshipToRectangleOf(Points[i], lowerLeftPoint, upperRightPoint);
		if (Out[0] == 0 && Out[1] == 0) {
			return true;
		}
		if ((Out[0] & Out[1]) != 0) {
			return false;
		}
		i = Out[0] == 0 ? 1 : 0;
		if ((Out[i] & 1) == 1) { // Above window
			Points[i].x = Points[i].x + dX * (upperRightPoint.y - Points[i].y) / dY;
			Points[i].y = upperRightPoint.y;
		} else if ((Out[i] & 2) == 2) { // Below window
			Points[i].x = Points[i].x + dX * (lowerLeftPoint.y - Points[i].y) / dY;
			Points[i].y = lowerLeftPoint.y;
		} else if ((Out[i] & 4) == 4) {
			Points[i].y = Points[i].y + dY * (upperRightPoint.x - Points[i].x) / dX;
			Points[i].x = upperRightPoint.x;
		} else {
			Points[i].y = Points[i].y + dY * (lowerLeftPoint.x - Points[i].x) / dX;
			Points[i].x = lowerLeftPoint.x;
		}
	}
}

bool EoGeLineSeg3d::IsSelectedBy_xy(const OdGePoint3d& point, const double pickAperture, OdGePoint3d& ptProj, double& relationship) const {
	if (point.x < EoMin(startPoint().x, endPoint().x) - pickAperture) {
		return false;
	}
	if (point.x > EoMax(startPoint().x, endPoint().x) + pickAperture) {
		return false;
	}
	if (point.y < EoMin(startPoint().y, endPoint().y) - pickAperture) {
		return false;
	}
	if (point.y > EoMax(startPoint().y, endPoint().y) + pickAperture) {
		return false;
	}
	const auto StartX {startPoint().x - point.x};
	const auto StartY {startPoint().y - point.y};
	const auto StartEndX {endPoint().x - startPoint().x};
	const auto StartEndY {endPoint().y - startPoint().y};
	const auto Divisor {StartEndX * StartEndX + StartEndY * StartEndY};
	double DistanceSquared;
	if (Divisor <= DBL_EPSILON) {
		relationship = 0.0;
		DistanceSquared = StartX * StartX + StartY * StartY;
	} else {
		relationship = -(StartX * StartEndX + StartY * StartEndY) / Divisor;
		relationship = EoMax(0.0, EoMin(1.0, relationship));
		const auto X {StartX + relationship * StartEndX};
		const auto Y {StartY + relationship * StartEndY};
		DistanceSquared = X * X + Y * Y;
	}
	if (DistanceSquared > pickAperture * pickAperture) {
		return false;
	}
	ptProj.x = startPoint().x + relationship * StartEndX;
	ptProj.y = startPoint().y + relationship * StartEndY;
	return true;
}

bool EoGeLineSeg3d::ParametricRelationshipOf(const OdGePoint3d& point, double& relationship) const {
	const auto Vector {endPoint() - startPoint()};
	if (fabs(Vector.x) > DBL_EPSILON) {
		relationship = (point.x - startPoint().x) / Vector.x;
		return true;
	}
	if (fabs(Vector.y) > DBL_EPSILON) {
		relationship = (point.y - startPoint().y) / Vector.y;
		return true;
	}
	if (fabs(Vector.z) > DBL_EPSILON) {
		relationship = (point.z - startPoint().z) / Vector.z;
		return true;
	}
	return false;
}

OdGePoint3d EoGeLineSeg3d::ProjPt(const OdGePoint3d& point) const {
	auto vBegEnd {endPoint() - startPoint()};
	const auto Sum {vBegEnd.lengthSqrd()};
	if (Sum > DBL_EPSILON) {
		const auto vBegPt {point - startPoint()};
		const auto Scale {vBegPt.dotProduct(vBegEnd) / Sum};
		vBegEnd *= Scale;
	}
	return startPoint() + vBegEnd;
}

int EoGeLineSeg3d::ProjPtFrom_xy(const double parallelDistance, const double perpendicularDistance, OdGePoint3d& projectedPoint) const {
	auto X {endPoint().x - startPoint().x};
	auto Y {endPoint().y - startPoint().y};
	auto Length {sqrt(X * X + Y * Y)};
	if (Length <= DBL_EPSILON) {
		return FALSE;
	}
	double Ratio;
	projectedPoint = startPoint();
	if (fabs(parallelDistance) > DBL_EPSILON) {
		Ratio = parallelDistance / Length;
		Length = parallelDistance;
		X = Ratio * X;
		Y = Ratio * Y;
		projectedPoint.x = startPoint().x + X;
		projectedPoint.y = startPoint().y + Y;
	}
	if (fabs(perpendicularDistance) > DBL_EPSILON) {
		Ratio = perpendicularDistance / Length;
		projectedPoint.x -= Ratio * Y;
		projectedPoint.y += Ratio * X;
	}
	return TRUE;
}

OdGePoint3d EoGeLineSeg3d::ProjToBegPt(const double distance) const {
	auto vEndBeg {startPoint() - endPoint()};
	const auto dLen {vEndBeg.length()};
	if (dLen > DBL_EPSILON) {
		vEndBeg *= distance / dLen;
	}
	return endPoint() + vEndBeg;
}

OdGePoint3d EoGeLineSeg3d::ProjToEndPt(const double distance) const {
	auto vBegEnd {endPoint() - startPoint()};
	const auto Length {vBegEnd.length()};
	if (Length > DBL_EPSILON) {
		vBegEnd *= distance / Length;
	}
	return startPoint() + vBegEnd;
}

void EoGeLineSeg3d::SetEndPoint(const OdGePoint3d& endPoint) {
	set(startPoint(), endPoint);
}

void EoGeLineSeg3d::SetStartPoint(const OdGePoint3d& startPoint) {
	set(startPoint, endPoint());
}
