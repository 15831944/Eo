// From Examples\Editor\OSnapManager.cpp (last compare 20.5)
#include <limits>
#include <OdaCommon.h>
#include "OSnapManager.h"
#include <Gi/GiPathNode.h>
#include <OdRound.h>
#include <SaveState.h>
#include <DbBlockTableRecord.h>
#include <DbBlockReference.h>
#include <DbLayout.h>
#include <DbHostAppServices.h>
#include <Gi/GiLocalDrawableDesc.h>
#include <Gs/GsViewImpl.h>
constexpr long gc_SnapPointSize = 5;

OdBaseSnapManager::SubentId::SubentId(const OdGiPathNode& pathNode) {
	m_Marker = pathNode.selectionMarker();
	auto PathNode {&pathNode};
	do {
		m_Path.append(PathNode->persistentDrawableId());
		PathNode = PathNode->parent();
	} while (PathNode != nullptr);
}

bool OdBaseSnapManager::SubentId::operator==(const SubentId& other) const {
	if (m_Marker != other.m_Marker) {
		return false;
	}
	if (m_Path.size() != other.m_Path.size()) {
		return false;
	}
	for (unsigned i = 0; i < m_Path.size(); ++i) {
		if (m_Path[i] != other.m_Path[i]) {
			return false;
		}
	}
	return true;
}

OdBaseSnapManager::OdBaseSnapManager() noexcept
	: m_NearDist(std::numeric_limits<double>::max())
	, m_Redraw(m_Redraw) {}

unsigned OSnapManager::SnapModes() const noexcept {
	return m_SnapModes;
}

void OSnapManager::SetSnapModes(const unsigned snapModes) noexcept {
	m_SnapModes = snapModes;
}

void OdBaseSnapManager::Reset() {
	m_Centers.clear();
}

long OdBaseSnapManager::GetAperture(OdDbDatabase* database) const {
	return database->appServices()->getAPERTURE();
}

void OdBaseSnapManager::subViewportDraw(OdGiViewportDraw* viewportDraw) const {
	OdGePoint3d Points[4];
	auto& ViewportGeometry {viewportDraw->geometry()};
	const auto& Viewport {viewportDraw->viewport()};
	const auto WorldToEyeTransform {Viewport.getWorldToEyeTransform()};
	Viewport.getNumPixelsInUnitSquare(Viewport.getCameraTarget(), reinterpret_cast<OdGePoint2d&>(Points[0]));
	const auto pix {1.0 / Points[0].x};
	const auto s {gc_SnapPointSize * pix};
	auto& SubEntityTraits {viewportDraw->subEntityTraits()};
	OdGiDrawFlagsHelper DrawFlagsHelper(SubEntityTraits, OdGiSubEntityTraits::kDrawNoPlotstyle);
	if (m_SnapMode > 0 && static_cast<unsigned>(m_SnapMode) < 100) {
		SubEntityTraits.setTrueColor(SnapTrueColor());
		SubEntityTraits.setFillType(kOdGiFillNever);
		SubEntityTraits.setSelectionMarker(kNullSubentIndex);
		Points[0] = WorldToEyeTransform * m_SnapPoint;
		Viewport.doPerspective(Points[0]);
		switch (m_SnapMode) {
			case OdDb::kOsModeEnd:
				Points[1].set(Points[0].x - s, Points[0].y + s, 0.0);
				Points[2].set(Points[0].x - s, Points[0].y - s, 0.0);
				Points[3].set(Points[0].x + s, Points[0].y - s, 0.0);
				Points[0].set(Points[0].x + s, Points[0].y + s, 0.0);
				ViewportGeometry.polygonEye(4, Points);
				Points[1].set(Points[1].x - pix, Points[1].y + pix, 0.0);
				Points[2].set(Points[2].x - pix, Points[2].y - pix, 0.0);
				Points[3].set(Points[3].x + pix, Points[3].y - pix, 0.0);
				Points[0].set(Points[0].x + pix, Points[0].y + pix, 0.0);
				ViewportGeometry.polygonEye(4, Points);
				break;
			case OdDb::kOsModeMid:
				Points[1].set(Points[0].x - s * 1.2, Points[0].y - s * 0.6, 0.0);
				Points[2].set(Points[0].x, Points[0].y + s * 1.4, 0.0);
				Points[3].set(Points[0].x + s * 1.2, Points[0].y - s * 0.6, 0.0);
				ViewportGeometry.polygonEye(3, Points + 1);
				Points[1].set(Points[1].x - pix, Points[1].y - pix, 0.0);
				Points[2].set(Points[2].x, Points[2].y + pix, 0.0);
				Points[3].set(Points[3].x + pix, Points[3].y - pix, 0.0);
				ViewportGeometry.polygonEye(3, Points + 1);
				break;
			case OdDb::kOsModeCen: {
				OdGiModelTransformSaver mt(ViewportGeometry, Viewport.getEyeToWorldTransform());
				ViewportGeometry.circle(Points[0], s * 1.4, OdGeVector3d::kZAxis);
			}
			break;
			case OdDb::kOsModeQuad:
				Points[1].set(Points[0].x - s, Points[0].y, 0.0);
				Points[2].set(Points[0].x, Points[0].y + s, 0.0);
				Points[3].set(Points[0].x + s, Points[0].y, 0.0);
				Points[0].set(Points[0].x, Points[0].y - s, 0.0);
				ViewportGeometry.polygonEye(4, Points);
				Points[1].set(Points[1].x - pix, Points[1].y, 0.0);
				Points[2].set(Points[2].x, Points[2].y + pix, 0.0);
				Points[3].set(Points[3].x + pix, Points[3].y, 0.0);
				Points[0].set(Points[0].x, Points[0].y - pix, 0.0);
				ViewportGeometry.polygonEye(4, Points);
				break;
			case OdDb::kOsModePerp:
				Points[1].set(Points[0].x - s, Points[0].y + s + pix, 0.0);
				Points[2].set(Points[0].x - s, Points[0].y - s, 0.0);
				Points[3].set(Points[0].x + s + pix, Points[0].y - s, 0.0);
				ViewportGeometry.polylineEye(3, Points + 1);
				Points[1].set(Points[1].x - pix, Points[1].y, 0.0);
				Points[2].set(Points[2].x - pix, Points[2].y - pix, 0.0);
				Points[3].set(Points[3].x, Points[3].y - pix, 0.0);
				ViewportGeometry.polylineEye(3, Points + 1);
				Points[1].set(Points[0].x - s, Points[0].y, 0.0);
				Points[2].set(Points[0].x, Points[0].y, 0.0);
				Points[3].set(Points[0].x, Points[0].y - s, 0.0);
				ViewportGeometry.polylineEye(3, Points + 1);
				Points[1].set(Points[1].x - pix, Points[1].y + pix, 0.0);
				Points[2].set(Points[2].x + pix, Points[2].y + pix, 0.0);
				Points[3].set(Points[3].x + pix, Points[3].y, 0.0);
				ViewportGeometry.polylineEye(3, Points + 1);
				break;
			case OdDb::kOsModeTan: {
				OdGiModelTransformSaver mt(ViewportGeometry, Viewport.getEyeToWorldTransform());
				ViewportGeometry.circle(Points[0], s, OdGeVector3d::kZAxis);
			}
				Points[1].set(Points[0].x - s, Points[0].y + s, 0.0);
				Points[2].set(Points[0].x + s, Points[0].y + s, 0.0);
				ViewportGeometry.polylineEye(2, Points + 1);
				Points[1].set(Points[1].x, Points[1].y + pix, 0.0);
				Points[2].set(Points[2].x, Points[2].y + pix, 0.0);
				ViewportGeometry.polylineEye(2, Points + 1);
				break;
			case OdDb::kOsModeNear:
				Points[1].set(Points[0].x - s, Points[0].y + s, 0.0);
				Points[2].set(Points[0].x + s, Points[0].y - s, 0.0);
				Points[3].set(Points[0].x - s, Points[0].y - s, 0.0);
				Points[0].set(Points[0].x + s, Points[0].y + s, 0.0);
				ViewportGeometry.polygonEye(4, Points);
				Points[1].set(Points[1].x - pix, Points[1].y + pix, 0.0);
				Points[2].set(Points[2].x + pix, Points[2].y - pix, 0.0);
				Points[3].set(Points[3].x - pix, Points[3].y - pix, 0.0);
				Points[0].set(Points[0].x + pix, Points[0].y + pix, 0.0);
				ViewportGeometry.polygonEye(4, Points);
				break;
			default:
				Points[1].set(Points[0].x - s, Points[0].y + s, 0.0);
				Points[2].set(Points[0].x + s, Points[0].y - s, 0.0);
				ViewportGeometry.polygonEye(2, Points + 1);
				Points[1].set(Points[0].x - s, Points[0].y - s, 0.0);
				Points[2].set(Points[0].x + s, Points[0].y + s, 0.0);
				ViewportGeometry.polygonEye(2, Points + 1);
				Points[1].set(Points[0].x - s - pix, Points[0].y + s + pix, 0.0);
				Points[2].set(Points[0].x - s - pix, Points[0].y - s - pix, 0.0);
				Points[3].set(Points[0].x + s + pix, Points[0].y - s - pix, 0.0);
				Points[0].set(Points[0].x + s + pix, Points[0].y + s + pix, 0.0);
				ViewportGeometry.polygonEye(4, Points);
				break;
		}
	}
	auto Marker {0};
	if (!m_Centers.empty()) {
		viewportDraw->subEntityTraits().setTrueColor(CenterTrueColor());
		for (unsigned i = 0; i < m_Centers.size(); i++, Marker++) {
			SubEntityTraits.setSelectionMarker(Marker);
			const auto& Center {WorldToEyeTransform * m_Centers[i].m_Point};
			Points[0].set(Center.x, Center.y + s, 0.0);
			Points[1].set(Center.x, Center.y - s, 0.0);
			ViewportGeometry.polygonEye(2, Points);
			Points[0].set(Center.x + s, Center.y, 0.0);
			Points[1].set(Center.x - s, Center.y, 0.0);
			ViewportGeometry.polygonEye(2, Points);
		}
	}
}

void OdBaseSnapManager::InvalidateViewport(const HistEntryArray& centers) const {
	const auto WorldToDeviceTransform {m_View->worldToDeviceMatrix()};
	OdGsDCRect DcRectangle;
	for (const auto& Center : centers) {
		const auto Point {WorldToDeviceTransform * Center.m_Point};
		DcRectangle.m_min.x = OdRoundToLong(Point.x);
		DcRectangle.m_min.y = OdRoundToLong(Point.y);
		DcRectangle.m_max = DcRectangle.m_min;
		DcRectangle.m_min.x -= gc_SnapPointSize;
		DcRectangle.m_min.y -= gc_SnapPointSize;
		DcRectangle.m_max.x += gc_SnapPointSize;
		DcRectangle.m_max.y += gc_SnapPointSize;
		m_View->invalidate(DcRectangle);
	}
}

void OdBaseSnapManager::InvalidateViewport(const OdGePoint3d& point) const {
	OdGsDCRect DcRectangle;
	if (m_SnapMode != 0) {
		const auto WorldToDeviceTransform {m_View->worldToDeviceMatrix()};
		const auto Point {WorldToDeviceTransform * point};
		DcRectangle.m_min.x = OdRoundToLong(Point.x);
		DcRectangle.m_min.y = OdRoundToLong(Point.y);
		DcRectangle.m_max = DcRectangle.m_min;
		DcRectangle.m_min.x -= gc_SnapPointSize * 2;
		DcRectangle.m_min.y -= gc_SnapPointSize * 2;
		DcRectangle.m_max.x += gc_SnapPointSize * 2;
		DcRectangle.m_max.y += gc_SnapPointSize * 2;
		m_View->invalidate(DcRectangle);
	}
}

unsigned long OdBaseSnapManager::subSetAttributes(OdGiDrawableTraits* drawableTraits) const {
	return kDrawableNone;
}

bool OdBaseSnapManager::subWorldDraw(OdGiWorldDraw* worldDraw) const {
	return false;
}

bool OdBaseSnapManager::Snap(OdGsView* view, OdGePoint3d& point, const OdGePoint3d* lastPoint) {
	const auto TrackerSnapInfo {dynamic_cast<OdEdPointTrackerWithSnapInfo*>(m_SnapInputTracker)};
	if (TrackerSnapInfo != nullptr) {
		TrackerSnapInfo->m_SnapContext.mValid = false;
	}
	m_Redraw = false;
	m_SnapPoints.clear();
	m_View = view;
	m_PickPoint = &point;
	m_LastPoint = lastPoint;
	auto PreviousCenters(m_Centers);
	const auto PreviousPoint {m_SnapPoint};
	const auto PreviousMode {m_SnapMode};
	if (m_SnapMode == 0 || !Checkpoint(m_SnapMode, m_SnapPoint)) {
		m_NearDist = std::numeric_limits<double>::max();
		m_SnapPoint = OdGePoint3d(1e100, 1e100, 1e100);
		m_SnapMode = OdDb::OsnapMode(100);
	}
	const auto pt {(view->worldToDeviceMatrix() * point).convert2d()};
	OdGsDCPoint DcPoints[2];
	const auto Aperture {GetAperture(dynamic_cast<OdDbDatabase*>(view->userGiContext()->database()))};
	DcPoints[0].x = OdRoundToLong(pt.x) - Aperture;
	DcPoints[1].x = DcPoints[0].x + Aperture * 2;
	DcPoints[0].y = OdRoundToLong(pt.y) - Aperture;
	DcPoints[1].y = DcPoints[0].y + Aperture * 2;
	m_HitRadius = static_cast<double>(Aperture);
	m_WorldToDevice = view->worldToDeviceMatrix().getCsXAxis().length();
	auto pViewImpl {dynamic_cast<OdGsViewImpl*>(view)};
	if (pViewImpl != nullptr) {
		pViewImpl->setSnapping(true);
	}
	m_SelectedEntityData.clear();
	view->select(DcPoints, 2, this);
	if (!m_SelectedEntityData.empty()) { // only one can be selected currently
		CheckSnapPoints(m_SelectedEntityData[0], pViewImpl->worldToEyeMatrix());
	}
	if (pViewImpl != nullptr) {
		pViewImpl->setSnapping(false);
	}
	if (m_SnapMode > 0 && static_cast<unsigned>(m_SnapMode) < 100) {
		point = m_SnapPoint;
	} else {
		if (PreviousMode > 0 && static_cast<unsigned>(PreviousMode) < 100) {
			InvalidateViewport(PreviousPoint);
		}
		m_SnapMode = OdDb::OsnapMode(0);
	}
	auto Result {true};
	if (m_SnapPoint == PreviousPoint) {
		Result = false;
	} else {
		if (PreviousPoint.x < 1e100) {
			InvalidateViewport(PreviousPoint);
		}
		if (m_SnapPoint.x < 1e100) {
			InvalidateViewport(m_SnapPoint);
		}
	}
	return Result || m_Redraw; // <tas"This was bitwise or. Changed to logical or."/>
}

bool OdBaseSnapManager::selected(const OdGiDrawableDesc&) {
	return false;
}

inline bool OdBaseSnapManager::Checkpoint(const OdDb::OsnapMode objectSnapMode, const OdGePoint3d& point) {
	const auto WorldToDeviceTransform {m_View->worldToDeviceMatrix()};
	const auto p1((WorldToDeviceTransform * *m_PickPoint).convert2d());
	const auto p2((WorldToDeviceTransform * point).convert2d());
	const auto dist {(p1 - p2).length()};
	auto TrackerSnapInfo {dynamic_cast<OdEdPointTrackerWithSnapInfo*>(m_SnapInputTracker)};
	if (dist < m_HitRadius) {
		if (dist < m_NearDist) {
			m_NearDist = dist;
			m_SnapPoint = point;
			m_SnapMode = objectSnapMode;
			if (TrackerSnapInfo != nullptr) {
				TrackerSnapInfo->m_SnapContext.mPoint = point;
				TrackerSnapInfo->m_SnapContext.mMode = objectSnapMode;
				TrackerSnapInfo->m_SnapContext.mValid = true;
			}
			return true;
		}
		if (dist == m_NearDist) {
			return true;
		}
	}
	return false;
}

const int nMaxHist = 7;

bool OdBaseSnapManager::AppendToQueue(HistEntryArray& histEntries, const HistEntry& histEntry) {
	if (!histEntries.contains(histEntry)) {
		if (histEntries.size() > nMaxHist) {
			histEntries.erase(histEntries.begin());
		}
		histEntries.append(histEntry);
		return true;
	}
	return false;
}

void OdBaseSnapManager::CheckSnapPoints(const SelectedEntityData& selectedEntityData, const OdGeMatrix3d& worldToEyeTransform) {
	const auto ModelToWorldTransform {selectedEntityData.ModelToWorldTransform};
	const auto InsertionMatrix {ModelToWorldTransform != OdGeMatrix3d::kIdentity};
	const auto ModelPickPoint {ModelToWorldTransform * *m_PickPoint};
	OdGePoint3d ModelLastPoint;
	auto nSnapModes {SnapModes()};
	if (m_LastPoint != nullptr) {
		ModelLastPoint = ModelToWorldTransform * *m_LastPoint;
	} else {
		nSnapModes &= ~(ToSnapModes(OdDb::kOsModePerp) | ToSnapModes(OdDb::kOsModeTan));
	}
	auto PointTrackerWithSnapInfo {dynamic_cast<OdEdPointTrackerWithSnapInfo*>(m_SnapInputTracker)};
	OdDbEntityPtr Entity {selectedEntityData.SubentId.m_Path.first().safeOpenObject()};
	const auto Marker {selectedEntityData.SubentId.m_Marker};
	if (PointTrackerWithSnapInfo == nullptr) {
		for (auto ObjectSnapMode = OdDb::kOsModeEnd; ObjectSnapMode <= OdDb::kOsModeNear; ObjectSnapMode = OdDb::OsnapMode(ObjectSnapMode + 1)) {
			if (nSnapModes & ToSnapModes(ObjectSnapMode)) // so not all types are tested
			{
				OdResult Result;
				if (InsertionMatrix) {
					Result = Entity->getOsnapPoints(ObjectSnapMode, Marker, ModelPickPoint, ModelLastPoint, worldToEyeTransform, m_SnapPoints, ModelToWorldTransform);
				} else {
					Result = Entity->getOsnapPoints(ObjectSnapMode, Marker, ModelPickPoint, ModelLastPoint, worldToEyeTransform, m_SnapPoints);
				}
				if (Result == eOk) {
					for (auto& SnapPoint : m_SnapPoints) {
						SnapPoint.transformBy(ModelToWorldTransform);
						Checkpoint(ObjectSnapMode, SnapPoint);
						switch (ObjectSnapMode) {
							case OdDb::kOsModeCen:
								AppendToQueue(m_Centers, HistEntry(selectedEntityData.SubentId, SnapPoint));
								m_Redraw = true;
								break;
							case OdDb::kOsModeEnd:
								break;
							case OdDb::kOsModeMid:
								break;
							case OdDb::kOsModeNode:
								break;
							case OdDb::kOsModeQuad:
								break;
							case OdDb::kOsModeIntersec:
								break;
							case OdDb::kOsModeIns:
								break;
							case OdDb::kOsModePerp:
								break;
							case OdDb::kOsModeTan:
								break;
							case OdDb::kOsModeNear:
								break;
							case OdDb::kOsModeApint:
								break;
							case OdDb::kOsModePar:
								break;
							case OdDb::kOsModeStart:
								break;
							default:
								break;
						}
					}
					m_SnapPoints.clear();
				}
			}
		}
	} else {
		if (!PointTrackerWithSnapInfo->IsTargetEntity(Entity)) {
			return;
		}
		OdSaveState<double> SavedHitRadius(m_HitRadius, 1500.0);
		OdArray<OdDb::OsnapMode> snapModes;
		PointTrackerWithSnapInfo->GetSnapModes(Entity, snapModes);
		for (auto it = snapModes.begin(); it != snapModes.end(); it++) {
			if (Entity->getOsnapPoints(*it, Marker, ModelPickPoint, ModelLastPoint, worldToEyeTransform, m_SnapPoints) == eOk) {
				PointTrackerWithSnapInfo->m_SnapContext.mEntityObjectId = Entity->objectId();
				PointTrackerWithSnapInfo->m_SnapContext.mMarker = Marker;
				for (auto& SnapPoint : m_SnapPoints) {
					SnapPoint.transformBy(ModelToWorldTransform);
					Checkpoint(*it, SnapPoint);
				}
			}
		}
	}
}

unsigned long OdBaseSnapManager::selected(const OdGiPathNode& pathNode, const OdGiViewport& viewInfo) {
	if (pathNode.transientDrawable() == this) {
		const auto Marker {pathNode.selectionMarker()};
		if (Marker > -1) {
			if (SnapModes() & ToSnapModes(OdDb::kOsModeCen) && static_cast<OdGsMarker>(m_Centers.size()) > Marker) {
				Checkpoint(OdDb::kOsModeCen, m_Centers[Marker].m_Point);
			}
		}
		return static_cast<unsigned long>(kContinue);
	}
	const auto Entity {OdDbEntity::cast(OdDbObjectId(pathNode.persistentDrawableId()).openObject())};
	if (Entity.isNull()) {
		return static_cast<unsigned long>(kSkipDrawable);
	}
	m_SelectedEntityData.append()->set(pathNode);
	return static_cast<unsigned long>(kSkipDrawable);
}

void OdBaseSnapManager::RecalculateEntityCenters() {
	for (auto i = m_Centers.size() - 1; i >= 0; --i) {
		auto SubentId = m_Centers[i].m_SubentId;
		if (SubentId.m_Path.empty()) {
			continue;
		}
		auto Entity {OdDbEntity::cast(SubentId.m_Path[0].openObject())};
		if (Entity.isNull()) {
			m_Centers.erase(m_Centers.begin() + i);
			continue;
		}
		OdGePoint3dArray SnapPoints;
		Entity->getOsnapPoints(OdDb::kOsModeCen, OdGsMarker(), OdGePoint3d(), OdGePoint3d(), OdGeMatrix3d(), SnapPoints);
		if (!SnapPoints.empty()) {
			m_Centers[i].m_Point = SnapPoints[0]; // recalculation center
		}
	}
}

bool OdBaseSnapManager::SetEntityCenters(OdRxObject* rxObject) {
	m_Centers.clear();
	const auto Database {OdDbDatabase::cast(rxObject).get()};
	if (Database == nullptr) {
		return false;
	}
	OdDbBlockTableRecordPtr BlockTableRecord {Database->getActiveLayoutBTRId().safeOpenObject()}; // Layout table
	if (Database->getModelSpaceId() != BlockTableRecord->objectId()) { // it's not ModelSpace, it's PaperSpace which can have many ModelSpace
		OdSmartPtr<OdDbLayout> Layout {BlockTableRecord->getLayoutId().safeOpenObject()};
		if (Layout->overallVportId() != OdDbObjectId(Database->activeViewportId())) {
			BlockTableRecord = Database->getModelSpaceId().safeOpenObject(); // get active ModelSpace for PaperSpace
		}
	}
	SetEntityCenters(BlockTableRecord);
	return true;
}

void OdBaseSnapManager::SetEntityCenters(OdDbBlockTableRecord* blockTableRecord, const OdGeMatrix3d& matrix) {
	OdGiDrawableDesc* DrawableDescriptor {nullptr};
	OdGiLocalDrawableDesc LocalDrawableDescriptor(DrawableDescriptor); // need for build OdGiPathNode
	for (auto BlockTableIterator = blockTableRecord->newIterator(); !BlockTableIterator->done() && m_Centers.size() < nMaxHist; BlockTableIterator->step()) {
		auto Entity {BlockTableIterator->entity()};
		auto BlockReference {OdDbBlockReference::cast(Entity)};
		if (!BlockReference.isNull()) {
			auto BlockTableRecord {OdDbBlockTableRecord::cast(BlockReference->blockTableRecord().openObject())};
			if (!BlockTableRecord.isNull()) {
				SetEntityCenters(BlockTableRecord, BlockReference->blockTransform());
			}
			continue;
		}
		if (Entity.isNull()) {
			continue;
		}
		LocalDrawableDescriptor.persistId = Entity->objectId();
		OdGePoint3dArray SnapPoints;
		Entity->getOsnapPoints(OdDb::kOsModeCen, OdGsMarker(), OdGePoint3d::kOrigin, OdGePoint3d::kOrigin, OdGeMatrix3d(), SnapPoints);
		for (unsigned i = 0; i < SnapPoints.size() && m_Centers.size() < nMaxHist; i++) {
			m_Centers.append(HistEntry(LocalDrawableDescriptor, SnapPoints[i].transformBy(matrix)));
		}
	}
}

void OdBaseSnapManager::Track(OdEdInputTracker* inputTracker) {
	m_SnapInputTracker = inputTracker;
}
