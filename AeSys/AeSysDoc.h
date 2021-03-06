#pragma once
#include <DbSymUtl.h>
#include <Ed/EdCommandStack.h>
#include <DbLayoutManager.h>
#include "EoDbText.h"
#include "EoDbMaskedPrimitive.h"
#include "EoDbBlock.h"
#include "EoDbLayer.h"
#include "EoDbLinetypeTable.h"
# include "OdApplicationImpl.h"
class ExStringIO;
class EoDlgUserIoConsole;
struct EoGeUniquePoint;

class OdDbDatabaseDoc final : public OdDbDatabase {
	static AeSysDoc* g_DatabaseDocument;
	mutable AeSysDoc* m_pDoc;
public:
ODRX_DECLARE_MEMBERS(OdDbDatabaseDoc);

	OdDbDatabaseDoc() noexcept;

	AeSysDoc* Document() const noexcept;

	static void SetDocumentToAssign(AeSysDoc* document) noexcept;
};

using OdDbDatabaseDocPtr = OdSmartPtr<OdDbDatabaseDoc>;

class AeSysAppDocStaticRxObjects : public OdDbLayoutManagerReactor, public OdEdBaseIO {
	ODRX_NO_HEAP_OPERATORS()
};

class AeSysDoc : public COleDocument, protected OdStaticRxObject<AeSysAppDocStaticRxObjects> {
protected:
	using COleDocument::operator new;
	using COleDocument::operator delete;
	AeSysView* m_Viewer {nullptr};

	EoDlgUserIoConsole* UserIoConsole();

	bool m_Console {false};
	bool m_ConsoleResponded {false};
	int m_CommandActive {0};

	class DataSource final : COleDataSource {
		friend class AeSysDoc;
		OdString m_TemporaryPath;
	public:
		DataSource();

		void Create(AeSysDoc* document, const OdGePoint3d& point = OdGePoint3d::kOrigin);

		bool DoDragDrop(); // hides non-virtual function of parent
		void Empty(); // hides non-virtual function of parent
		~DataSource();
	};

	template <class T>
	struct AcadClipData {
		void Initialize() noexcept {
			memset(this, 0, sizeof(AcadClipData<T>));
		}

		void Read(CFile* file) {
			file->Read(this, sizeof(AcadClipData<T>));
		}
		T _tempFileName[0x104]; // name of the temp dwg file, where dragged entities are
		T _origFileName[0x104]; // original file name
		T _version[4]; // version of the original file, e.g. 'R15'
		int _one1; // seem to be always 1
		double _x; // pick point
		double _y;
		double _z;
		int _zero1; // seem to be always zero
		int _one2; // seem to be always 1
		int _unk[4];
		int _zero2[4]; // seem to be always zero
	};

	template <class T>
	struct AcadClipDataConstr : AcadClipData<T> {
		AcadClipDataConstr(const OdString& tempFileName, const OdString& origFileName, const OdGePoint3d& pickPoint) {
			AcadClipData<wchar_t>::Initialize();
			AcadClipData<wchar_t>::_one1 = 1;
			AcadClipData<wchar_t>::_one2 = 1;
			AcadClipData<wchar_t>::_version[0] = 'R';
			AcadClipData<wchar_t>::_version[1] = '1';
			AcadClipData<wchar_t>::_version[2] = '5';
			AcadClipData<wchar_t>::_version[3] = 0;
			AcadClipData<wchar_t>::_x = pickPoint.x;
			AcadClipData<wchar_t>::_y = pickPoint.y;
			AcadClipData<wchar_t>::_z = pickPoint.z;
			// <tas="_tempFileName & _origFileName not static. v16 compile fail."/>
			// memcpy(AcadClipData<wchar_t>::_tempFileName, static_cast<const T*>(tempFileName), odmin((0x100 * sizeof(T)), ((tempFileName.getLength() + 1) * sizeof(T))));
			// memcpy(AcadClipData<wchar_t>::_origFileName, static_cast<const T*>(origFileName), odmin((0x100 * sizeof(T)), ((origFileName.getLength() + 1) * sizeof(T))));
		}

		AcadClipDataConstr() {
			AcadClipData<wchar_t>::Initialize();
		}
	};

	using AcadClipDataR15 = AcadClipDataConstr<char>;
	using AcadClipDataR21 = AcadClipDataConstr<wchar_t>;
public:
	class ClipboardData {
	public:
		static unsigned short formatR15;
		static unsigned short formatR16;
		static unsigned short formatR17;
		static unsigned short formatR18;
		static unsigned short formatR19;

		static bool IsAcadDataAvailable(COleDataObject* dataObject, const bool attach = false) {
			if (attach && dataObject->AttachClipboard() == 0) {
				return false;
			}
			return dataObject->IsDataAvailable(formatR15) != 0 || dataObject->IsDataAvailable(formatR16) != 0 || dataObject->IsDataAvailable(formatR17) != 0 || dataObject->IsDataAvailable(formatR18) != 0 || dataObject->IsDataAvailable(formatR19) != 0;
		}

		static OdSharedPtr<ClipboardData> Get(COleDataObject* dataObject, const bool attach = false) {
			if (attach && dataObject->AttachClipboard() == 0) {
				return nullptr;
			}
			OdSharedPtr<ClipboardData> Data {new ClipboardData()};
			if (Data->Read(dataObject)) {
				return Data;
			}
			return nullptr;
		}

		ClipboardData() noexcept = default;

		bool Read(COleDataObject* dataObject) {
			OdSharedPtr<CFile> File;
			if ((File = dataObject->GetFileData(formatR15)).get() != nullptr || (File = dataObject->GetFileData(formatR16)).get() != nullptr) {
				_isR15format = true;
				_data._r15.Read(File);
				return true;
			}
			if ((File = dataObject->GetFileData(formatR17)).get() != nullptr || (File = dataObject->GetFileData(formatR18)).get() != nullptr || (File = dataObject->GetFileData(formatR19)).get() != nullptr) {
				_isR15format = false;
				_data._r21.Read(File);
				return true;
			}
			return false;
		}

		OdString tempFileName() {
			return _isR15format ? OdString(_data._r15._tempFileName) : OdString(_data._r21._tempFileName);
		}

		OdGePoint3d pickPoint() {
			return _isR15format ? OdGePoint3d(_data._r15._x, _data._r15._y, _data._r15._z) : OdGePoint3d(_data._r21._x, _data._r21._y, _data._r21._z);
		}

	private:
		union Data {
			AcadClipData<char> _r15;
			AcadClipData<wchar_t> _r21;

			Data() noexcept {
				_r21.Initialize();
			}
		} _data;

		bool _isR15format {false};
	};

protected:
	AeSysDoc();

DECLARE_DYNCREATE(AeSysDoc)

	BOOL DoPromptFileName(CString& fileName, unsigned titleResourceId, unsigned long flags, BOOL openFileDialog, CDocTemplate* documentTemplate);

private:
	OdDbCommandContextPtr m_CommandContext;
	OdSmartPtr<EoDlgUserIoConsole> m_UserIoConsole;
	OdSmartPtr<ExStringIO> m_Macro;
public:
	OdDbCommandContextPtr CommandContext0();

	OdEdBaseIO* BaseIo() noexcept;

	static OdString CommandPrompt();

	static OdString RecentCommand();

	static OdString RecentCommandName();

	unsigned long getKeyState() noexcept override;

	OdGePoint3d getPoint(const OdString& prompt, int options, OdEdPointTracker* tracker) override;

	OdString getString(const OdString& prompt, int options, OdEdStringTracker* tracker) override;

	void putString(const OdString& string) override;

	// OdDbLayoutManagerReactor
	bool layoutSwitchable {false};

	void layoutSwitched(const OdString& newLayoutName, const OdDbObjectId& newLayout) override;

	bool disableClearSelection {false};
	bool partial {false};
	OdDb::DwgVersion saveAsVersion {OdDb::kDHL_CURRENT};
	OdDb::SaveType saveAsType {OdDb::kDwg};
	EoDb::FileTypes saveAsType_ {EoDb::kUnknown};

	[[nodiscard]] OdDbSelectionSetPtr SelectionSet() const;

	AeSysView* GetViewer() const noexcept;

	void OnCloseVectorizer(AeSysView* view);

	void SetVectorizer(AeSysView* view);

	void ExecuteCommand(const OdString& command, bool echo);

	OdDbDatabasePtr m_DatabasePtr;

	void DeleteSelection(bool force);

	void StartDrag(const OdGePoint3d& point);

	BOOL OnSaveDocument(const wchar_t* pathName) override;

	BOOL OnCmdMsg(unsigned commandId, int messageCategory, void* commandObject, AFX_CMDHANDLERINFO* handlerInfo) override;

	BOOL OnNewDocument() override;

	BOOL OnOpenDocument(const wchar_t* file) override;

	void DeleteContents() override;

	BOOL CanCloseFrame(CFrameWnd* frame) override;

	~AeSysDoc();

	BOOL DoSave(const wchar_t* pathName, BOOL replace = TRUE) override;
#ifdef _DEBUG
	void AssertValid() const override;

	void Dump(CDumpContext& dc) const override;
#endif
	OdSmartPtr<OdApplicationDocumentImpl> m_pRefDocument;
protected:
	void OnVectorize(const OdString& vectorizerPath);

	void AddRegisteredApp(const OdString& name);

private:
	OdString m_IdentifiedLayerName;
	EoDbLinetypeTable m_LinetypeTable;
	EoDbBlockTable m_BlockTable;
	EoDbLayerTable m_LayerTable;
	EoDbLayer* m_WorkLayer {nullptr};
	EoDbGroupList m_DeletedGroupList;
	EoDbGroupList m_TrappedGroupList;
	OdGePoint3d m_TrapPivotPoint;
	EoDbGroupList m_NodalGroupList;
	CObList m_MaskedPrimitives;
	CObList m_UniquePoints;
public:
	void UpdateGroupInAllViews(LPARAM hint, EoDbGroup* group);

	void UpdateGroupsInAllViews(LPARAM hint, EoDbGroupList* groups);

	void UpdateLayerInAllViews(LPARAM hint, EoDbLayer* layer);

	void UpdatePrimitiveInAllViews(LPARAM hint, EoDbPrimitive* primitive);

	void InitializeGroupAndPrimitiveEdit() const;

	/// <summary>Constructs 0 to many separate text primitives for each "\r\n" delimited text block</summary>
	void AddTextBlock(wchar_t* text);

	// Text Style Table interface
	/// <summary>Add a new text style to the text style table.</summary>
	static OdDbTextStyleTableRecordPtr AddNewTextStyle(const OdString& name, OdDbTextStyleTablePtr& textStyles);

	OdDbTextStyleTableRecordPtr AddStandardTextStyle();

	OdDbDimStyleTableRecordPtr AddStandardDimensionStyle();

	// Block Table interface
	EoDbBlockTable* BlockTable() noexcept;

	bool BlockTableIsEmpty() const;

	unsigned short BlockTableSize() const;

	int GetBlockReferenceCount(const CString& name);

	POSITION GetFirstBlockPosition() const;

	void GetNextBlock(POSITION& position, CString& name, EoDbBlock*& block) const;

	bool LookupBlock(const wchar_t* name, EoDbBlock*& block) const;
	/// <summary>Removes all blocks and defining primitives.</summary>
	void RemoveAllBlocks();

	void PurgeUnreferencedBlocks();

	void InsertBlock(const wchar_t* name, EoDbBlock* block);
	/// <summary>A layer is converted to a tracing or a job file</summary>
	bool LayerMelt(OdString& name);

	int LinetypeIndexReferenceCount(short linetypeIndex) const;

	void GetExtents___(AeSysView* view, OdGeExtents3d& extents);

	int NumberOfGroupsInWorkLayer() const;

	int NumberOfGroupsInActiveLayers() const;

	void BuildVisibleGroupList(AeSysView* view) const;

	/// <summary>Displays drawing and determines which groups are detectable.</summary>
	void DisplayAllLayers(AeSysView* view, CDC* deviceContext) const;

	// Layer Table interface
	void AddLayer(EoDbLayer* layer);

	OdDbObjectId AddLayerTo(OdDbLayerTablePtr layers, EoDbLayer* layer);

	EoDbLayer* AnyLayerRemove(EoDbGroup* group);

	EoDbLayer* GetLayerAt(const OdString& name) const;

	EoDbLayer* GetLayerAt(int layerIndex) const;

	[[nodiscard]] int GetLayerTableSize() const;

	[[nodiscard]] int FindLayerAt(const OdString& name) const;

	OdDbLayerTablePtr LayerTable(OdDb::OpenMode openMode = OdDb::kForRead);

	void RemoveAllLayers();

	void RemoveLayerAt(int layerIndex);

	void RemoveEmptyLayers();

	EoDbLayer* SelectLayerBy(const OdGePoint3d& point);

	void PenTranslation(unsigned numberOfColors, std::vector<int>& newColors, std::vector<int>& pCol);

	void PurgeDuplicateObjects();

	int RemoveEmptyNotesAndDelete();

	int RemoveEmptyGroups();

	void ResetAllViews() const;

	void AddGroupToAllViews(EoDbGroup* group) const;

	void AddGroupsToAllViews(EoDbGroupList* groups) const;

	void RemoveGroupFromAllViews(EoDbGroup* group) const;

	void RemoveAllGroupsFromAllViews() const;

	// <Work Layer> interface
	void AddWorkLayerGroup(EoDbGroup* group);

	void AddWorkLayerGroups(EoDbGroupList* groups);

	POSITION FindWorkLayerGroup(EoDbGroup* group) const;

	[[nodiscard]] POSITION GetFirstWorkLayerGroupPosition() const;

	[[nodiscard]] EoDbGroup* GetLastWorkLayerGroup() const;

	[[nodiscard]] POSITION GetLastWorkLayerGroupPosition() const;

	EoDbGroup* GetNextWorkLayerGroup(POSITION& position) const;

	EoDbGroup* GetPreviousWorkLayerGroup(POSITION& position) const;

	[[nodiscard]] EoDbLayer* GetWorkLayer() const noexcept;

	void InitializeWorkLayer();

	OdDbObjectId SetCurrentLayer(OdDbLayerTableRecordPtr layerTableRecord);
	// </Work Layer>
	void WriteShadowFile();

	// Deleted groups interface
	POSITION DeletedGroupsAddHead(EoDbGroup* group);

	POSITION DeletedGroupsAddTail(EoDbGroup* group);

	EoDbGroup* DeletedGroupsRemoveHead();

	void DeletedGroupsRemoveGroups();

	EoDbGroup* DeletedGroupsRemoveTail();
	/// <summary>Restores the last group added to the deleted group list.</summary>
	void DeletedGroupsRestore();
	
	// trap interface
	void AddGroupsToTrap(EoDbGroupList* groups);

	POSITION AddGroupToTrap(EoDbGroup* group);
	/// <summary>Builds a single group from two or more groups in trap.</summary>
	/// <remarks>The new group is added to the hot layer even if the trap contained groups from one or more warm layers.</remarks>
	void CompressTrappedGroups();

	void CopyTrappedGroups(const OdGeVector3d& translate);
	/// <summary>The current trap is copied to the clipboard. This is done with two independent clipboard formats. The standard enhanced metafile and the private EoDbGroupList which is read exclusively by Peg.</summary>
	void CopyTrappedGroupsToClipboard(AeSysView* view);

	void DeleteAllTrappedGroups();
	/// <summary>Expands compressed groups.</summary>
	/// <remarks>The new groups are added to the hot layer even if the trap contained groups from one or more warm layers.</remarks>
	void ExpandTrappedGroups();

	POSITION FindTrappedGroup(EoDbGroup* group) const;

	[[nodiscard]] POSITION GetFirstTrappedGroupPosition() const;

	EoDbGroup* GetNextTrappedGroup(POSITION& position);

	EoDbGroupList* GroupsInTrap() noexcept;

	[[nodiscard]] bool IsTrapEmpty() const;

	void ModifyTrappedGroupsColorIndex(short colorIndex);

	void ModifyTrappedGroupsLinetypeIndex(short linetypeIndex);

	void ModifyTrappedGroupsNoteAttributes(EoDbFontDefinition& fontDef, EoDbCharacterCellDefinition& cellDef, int attributes);

	void RemoveAllTrappedGroups();

	EoDbGroup* RemoveLastTrappedGroup();

	POSITION RemoveTrappedGroup(EoDbGroup* group);

	void RemoveTrappedGroupAt(POSITION position);

	void SetTrapPivotPoint(const OdGePoint3d& pivotPoint) noexcept;

	void SquareTrappedGroups(AeSysView* view);

	void TracingFuse(OdString& nameAndLocation);

	bool TracingLoadLayer(const OdString& file, EoDbLayer* layer);

	bool TracingOpen(const OdString& fileName);

	void TransformTrappedGroups(const EoGeMatrix3d& transformMatrix);

	[[nodiscard]] int TrapGroupCount() const;

	[[nodiscard]] OdGePoint3d TrapPivotPoint() const noexcept;
	
	// Nodal list interface (includes list of groups, primitives and unique points)
	void DeleteNodalResources();
	/// <summary>Maintains a list of the primitives with at least one identified node.</summary>
	void UpdateNodalList(EoDbGroup* group, EoDbPrimitive* primitive, unsigned mask, unsigned bit, OdGePoint3d point);

	POSITION AddNodalGroup(EoDbGroup* group);

	POSITION FindNodalGroup(EoDbGroup* group) const;

	[[nodiscard]] POSITION GetFirstNodalGroupPosition() const;

	EoDbGroup* GetNextNodalGroup(POSITION& position);

	void RemoveAllNodalGroups();

	POSITION AddMaskedPrimitive(EoDbMaskedPrimitive* maskedPrimitive);

	[[nodiscard]] POSITION GetFirstMaskedPrimitivePosition() const;

	EoDbMaskedPrimitive* GetNextMaskedPrimitive(POSITION& position);

	void RemoveAllMaskedPrimitives();

	unsigned GetPrimitiveMask(EoDbPrimitive* primitive);

	void AddPrimitiveBit(EoDbPrimitive* primitive, unsigned bit);

	void RemovePrimitiveBit(EoDbPrimitive* primitive, unsigned bit);

	int AddUniquePoint(const OdGePoint3d& point);

	void DisplayUniquePoints();

	[[nodiscard]] POSITION GetFirstUniquePointPosition() const {
		return m_UniquePoints.GetHeadPosition();
	}

	EoGeUniquePoint* GetNextUniquePoint(POSITION& position);

	void RemoveUniquePointAt(POSITION position);

	void RemoveAllUniquePoints();

	int RemoveUniquePoint(const OdGePoint3d& point);
	
	// Generated message map functions
	void OnPurgeUnreferencedBlocks();

	void OnClearActiveLayers();

	void OnClearAllLayers();

	void OnClearAllTracings();

	void OnClearMappedTracings();

	void OnClearViewedTracings();

	void OnClearWorkingLayer();
	/// <summary>The current view is copied to the clipboard as an enhanced metafile.</summary>
	void OnEditImageToClipboard();

	void OnEditSegToWork();
	/// <summary>Pastes clipboard to drawing. Only EoGroups format handled and no translation is performed.</summary>
	void OnEditTrace();

	void OnEditTrapCopy();

	void OnEditTrapCut();

	void OnEditTrapDelete();
	/// <summary>Initializes current trap and all trap component lists.</summary>
	void OnEditTrapQuit();
	/// <summary>Pastes clipboard to drawing. If the clipboard has the EoGroups format, all other formats are ignored.</summary>
	void OnEditTrapPaste();
	/// <summary>Adds all groups in the work layer to the trap.</summary>
	void OnEditTrapWork();
	/// <summary>Add all groups in all work and active layers to the trap.</summary>
	void OnEditTrapWorkAndActive();

	void OnFile();

	void OnFileManage();

	void OnFilePageSetup();

	void OnFileQuery();

	void OnFileTracing();

	void OnHelpKey();
#ifdef OD_OLE_SUPPORT
	void OnInsertOleobject();
#endif // OD_OLE_SUPPORT
	void OnInsertTracing();

	void OnLayerActive();

	void OnLayerCurrent();

	void OnLayerLock();

	void OnLayerMelt();

	void OnLayerOff();

	void OnLayersSetAllActive();

	void OnLayersSetAllLocked();

	void OnPurgeUnusedLayers();

	void OnPurgeDuplicateObjects();

	void OnPurgeEmptyNotes();

	void OnPurgeEmptyGroups();

	void OnPensEditColors();

	void OnPensLoadColors();

	void OnPensRemoveUnusedLinetypes();

	void OnPensTranslate();
	/// <summary>Breaks a primitive into a simpler set of primitives.</summary>
	void OnPrimBreak();
	/// <summary>Searches for closest detectible primitive. If found, primitive is lifted from its group, inserted into a new group which is added to deleted group list. The primitive resources are not freed.</summary>
	void OnToolsPrimitiveDelete();

	void OnPrimExtractNum();

	void OnPrimExtractStr();
	/// <summary>Positions the cursor at a "control" point on the current engaged group.</summary>
	void OnPrimGotoCenterPoint();
	/// <summary>Picks a primitive and modifies its attributes to current settings.</summary>
	void OnPrimModifyAttributes();

	void OnToolsPrimitiveSnapToEndPoint();
	/// <summary>Reduces complex primitives and group references to a simpler form</summary>
	void OnToolsGroupBreak();
	/// <summary>
	/// Searches for closest detectible group.  If found, group is removed
	/// from all general group lists and added to deleted group list.
	/// Notes: The group resources are not freed.
	/// </summary>
	void OnToolsGroupDelete();

	void OnToolsGroupDeleteLast();
	/// <summary>Exchanges the first and last groups on the deleted group list.</summary>
	void OnToolsGroupExchange();

	void OnToolsGroupRestore();

	void OnSetupFillHatch();

	void OnSetupFillHollow() noexcept;

	void OnSetupFillPattern() noexcept;

	void OnSetupFillSolid() noexcept;

	void OnSetupGotoPoint();

	void OnSetupNote();

	void OnSetupOptionsDraw();

	void OnSetupPenColor();

	void OnSetupLayerProperties();

	void OnSetupLinetype();

	void OnSetupSavePoint();

	void OnTracingActive();

	void OnTracingCurrent();

	void OnTracingFuse();

	void OnTracingLock();

	void OnTracingOff();

	void OnTrapCommandsBlock();

	void OnTrapCommandsCompress();

	void OnTrapCommandsExpand();

	void OnTrapCommandsFilter();

	void OnTrapCommandsInvert();

	void OnTrapCommandsQuery();

	void OnTrapCommandsSquare();

	void OnTrapCommandsUnblock();
	// Returns a pointer to the currently active document.
	static AeSysDoc* GetDoc();

DECLARE_MESSAGE_MAP()

public:
	void OnViewSetActiveLayout();

	void OnDrawingUtilitiesAudit();
	// <command_console>
	void OnEditClearSelection();

	void OnEditSelectAll();

	void OnEditConsole();

	void OnEditExplode();

	void OnEditEntget();

	void OnViewNamedViews();

	void OnEditUndo();

	void OnUpdateEditUndo(CCmdUI* commandUserInterface);

	void OnEditRedo();

	void OnUpdateEditRedo(CCmdUI* commandUserInterface);
	// </command_console>
	void OnVectorizerType();

	void OnUpdateVectorizerType(CCmdUI* commandUserInterface);
};
