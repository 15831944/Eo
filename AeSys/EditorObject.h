#pragma once

// From Examples\Editor\EditorObject.h  (last compare 19.12)

#include "DbSSet.h"
#include "Ed/EdCommandStack.h"
#include "ExDbCommandContext.h"
#include "DbGsManager.h"
#include "StaticRxObject.h"
#include "OSnapManager.h"
#include "ExGripManager.h"
#include "ExEdInputParser.h"


class OdExZoomCmd : public OdEdCommand {
public:
	const OdString groupName() const override;
	const OdString globalName() const override;
	void execute(OdEdCommandContext* edCommandContext) override;
};

class OdEx3dOrbitCmd : public OdEdCommand {
public:
	const OdString groupName() const override;
	const OdString globalName() const override;
	void execute(OdEdCommandContext* edCommandContext) override;
};

class OdExDollyCmd : public OdEdCommand {
public:
	const OdString groupName() const override;
	const OdString globalName() const override;
	void execute(OdEdCommandContext* edCommandContext) override;
};

class OdExInteractivityModeCmd : public OdEdCommand {
public:
	const OdString groupName() const override;
	const OdString globalName() const override;
	void execute(OdEdCommandContext* edCommandContext) override;
};

class OdExCollideCmd : public OdEdCommand {
public:
	const OdString groupName() const override;
	const OdString globalName() const override;
	void execute(OdEdCommandContext* edCommandContext) override;
};

class OdExCollideAllCmd : public OdEdCommand {
public:
	const OdString groupName() const override;
	const OdString globalName() const override;
	void execute(OdEdCommandContext* edCommandContext) override;
};

class OdExEditorObject {

	OdGsLayoutHelperPtr m_pDevice;
	OdStaticRxObject<OSnapManager> m_osnapMan;
	OdStaticRxObject<OdExGripManager> m_gripManager;
	OdGsModelPtr m_p2dModel;
	OdDbCommandContext* m_pCmdCtx;

	OdStaticRxObject<OdExZoomCmd> m_cmd_ZOOM;
	OdStaticRxObject<OdEx3dOrbitCmd> m_cmd_3DORBIT;
	OdStaticRxObject<OdExDollyCmd> m_cmd_DOLLY;
	OdStaticRxObject<OdExInteractivityModeCmd> m_cmd_INTERACTIVITY;
	OdStaticRxObject<OdExCollideCmd> m_cmd_COLLIDE;
	OdStaticRxObject<OdExCollideAllCmd> m_cmd_COLLIDE_ALL;

	OdEdInputTrackerPtr m_InputTracker;
	OdGePoint3d m_basePt;
	const OdGePoint3d* m_pBasePt;

	enum Flags {
		kSnapOn = 4,
		kOrbitOn = 8,
		kDragging = 16,
		kTrackerHasDrawables = 32,
	};
	OdUInt32 m_flags;
public:
	const OdGsView* ActiveView() const;
	OdGsView* ActiveView();
	const OdGsView* ActiveTopView() const;
	OdGsView* ActiveTopView();
	OdDbObjectId ActiveViewportId() const;
	void UcsPlane(OdGePlane& plane) const;
	void Dolly(int x, int y);

	static void ZoomAt(OdGsView* view, int x, int y, short zDelta);
	static void Dolly(OdGsView* view, int x, int y);

public:
	enum _3DViewType {
		k3DViewTop,
		k3DViewBottom,
		k3DViewLeft,
		k3DViewRight,
		k3DViewFront,
		k3DViewBack,
		k3DViewSW,
		k3DViewSE,
		k3DViewNE,
		k3DViewNW
	};
	void Set3DView(_3DViewType type);
public:
	OdExEditorObject();

	void Initialize(OdGsDevice* device, OdDbCommandContext* dbCommandContext);
	OdGsLayoutHelper* Device() { return m_pDevice; }
	void Uninitialize();

	OdDbSelectionSetPtr workingSSet() const;
	void SetWorkingSelectionSet(OdDbSelectionSet* selectionSet);
	void SelectionSetChanged();

	OdGiDrawablePtr SnapDrawable() const;
	bool Unselect();

	OdEdCommandPtr Command(const OdString& commandName);

	OdGePoint3d ToEyeToWorld(int x, int y) const;
	bool ToUcsToWorld(OdGePoint3d& wcsPt) const;
	OdGePoint3d ToScreenCoord(int x, int y) const;
	OdGePoint3d ToScreenCoord(const OdGePoint3d& wcsPt) const;

	class OleDragCallback {
	public:
		virtual bool beginDragCallback(const OdGePoint3d& pt) = 0;
	};
	bool OnSize(unsigned int nFlags, int w, int h);
	bool OnPaintFrame(unsigned int nFlags = 0, OdGsDCRect* pUpdatedRect = 0);
	bool OnMouseLeftButtonClick(unsigned int nFlags, int x, int y, OleDragCallback* pDragCallback = 0);
	bool OnMouseMove(unsigned int flags, int x, int y);
	bool OnMouseWheel(unsigned int nFlags, int x, int y, short zDelta);
	bool OnMouseLeftButtonDoubleClick(unsigned int nFlags, int x, int y);
	bool OnMouseRightButtonDoubleClick(unsigned int nFlags, int x, int y);
	bool OnCtrlClick();
	void OnDestroy();

	bool HasSelection() const { return (workingSSet()->numEntities() > 0); }
	bool IsSnapOn() const noexcept { return GETBIT(m_flags, kSnapOn); }
	void SetSnapOn(bool snapOn) noexcept { SETBIT(m_flags, kSnapOn, snapOn); }
	bool IsOrbitOn() const noexcept { return GETBIT(m_flags, kOrbitOn); }

	void TurnOrbitOn(bool orbitOn);
	bool OnOrbitBeginDrag(int x, int y);
	bool OnOrbitEndDrag(int x, int y);

	bool OnZoomWindowBeginDrag(int x, int y);
	bool OnZoomWindowEndDrag(int x, int y);

	bool Snap(OdGePoint3d& point, const OdGePoint3d* lastPoint = 0);
	unsigned GetSnapModes() const;
	void SetSnapModes(bool bSnapOn, unsigned modes);
	void ResetSnapManager();
	void InitializeSnapping(OdGsView* view, OdEdInputTracker* tracker);
	void UninitializeSnapping(OdGsView* view);

	inline OdGsModel* GsModel() { return m_p2dModel.get(); }

	void Recalc_Entity_centers(void) {
		m_osnapMan.Recalc_Entity_centers();
	}

	void Set_Entity_centers() {
		if (HasDatabase()) {
			m_osnapMan.Set_Entity_centers(m_pCmdCtx->database());
		}
	}

	void SetTracker(OdEdInputTracker* tracker);

	bool TrackString(const OdString& value);
	bool TrackPoint(const OdGePoint3d& point);
	bool HasDatabase() const;
};


inline OdGiDrawablePtr OdExEditorObject::SnapDrawable() const {
	return &m_osnapMan;
}

inline void OdExEditorObject::ResetSnapManager() {
	m_osnapMan.reset();
}
