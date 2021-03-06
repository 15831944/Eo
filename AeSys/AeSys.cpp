#include "stdafx.h"
#include "ChildFrm.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "PrimState.h"
#include <DbLayoutPaperPE.h>
#include <RxDynamicModule.h>
#include <OdStreamBuf.h>
#include <Gi/GiWorldDraw.h>
#include <Gi/GiMaterialItem.h>
#include <ExFileUndoController.h>
#include <ExPageController.h>
#include <ExUndoController.h>
#include <memfilestreamimpl.h>
#include <FileDlgExt.h>
#include "EoAppAuditInfo.h"
#include "EoDbHatchPatternTable.h"
#include "EoDlgAudit.h"
#include "EoDlgModeLetter.h"
#include "EoDlgModeRevise.h"
#include "EoDlgPassword.h"
#include "EoDlgPlotStyleTableEditor.h"
#include "EoLoadApps.h"
#include "EoPreviewDib.h"
#include "Lex.h"
#include "EoDbHatch.h"
#include "EoViewCmd.h"
#include "EoSelectCmd.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
ATOM WINAPI RegisterPreviewWindowClass(HINSTANCE instance);

double g_PenWidths[] {
	0.0,
	0.0075,
	0.015,
	0.02,
	0.03,
	0.0075,
	0.015,
	0.0225,
	0.03,
	0.0075,
	0.015,
	0.0225,
	0.03,
	0.0075,
	0.015,
	0.0225
};
#include "PegColors.h"
COLORREF* g_CurrentPalette {g_ColorPalette};
CPrimState g_PrimitiveState;

// This is a legacy feature. All values are empty strings now for normal MouseButton command processing.
// User may still change through user interface, so must not assume empty.
CString AeSys::customLButtonDownCharacters(L"");
CString AeSys::customLButtonUpCharacters(L"" /* L"{13}" for VK_RETURN */);
CString AeSys::customRButtonDownCharacters(L"");
CString AeSys::customRButtonUpCharacters(L"" /* L"{27}" for VK_ESCAPE */);
#ifdef OD_OLE_SUPPORT
void rxInit_COleClientItem_handler();
void rxUninit_COleClientItem_handler();
#endif // OD_OLE_SUPPORT
OdStaticRxObject<CommandView> g_CommandView;
OdStaticRxObject<CommandSelect> g_CommandSelect;

static void AddPaperDrawingCustomization() {
	static class OdDbLayoutPaperPEImpl : public OdStaticRxObject<OdDbLayoutPaperPE> {
	public:
		bool drawPaper(const OdDbLayout* /*layout*/, OdGiWorldDraw* worldDraw, OdGePoint3d* points) override {
			worldDraw->geometry().polygon(4, points);
			return true;
		}

		bool drawBorder(const OdDbLayout* /*layout*/, OdGiWorldDraw* worldDraw, OdGePoint3d* points) override {
			worldDraw->geometry().polygon(4, points);
			return true;
		}

		bool drawMargins(const OdDbLayout* /*layout*/, OdGiWorldDraw* worldDraw, OdGePoint3d* points) override {
			if (points[0] == points[1] || points[1] == points[2]) {
				return true;
			}
			auto NumberOfDashes {15};
			OdGiGeometry& Geometry {worldDraw->geometry()};
			OdGePoint3d Dash1[2];
			OdGePoint3d Dash2[2];
			auto Step {(points[1] - points[0]) / (static_cast<double>(NumberOfDashes) * 2.0 + 1.0)};
			Dash1[0] = points[0];
			Dash2[0] = points[2];
			for (auto DashIndex = 0; DashIndex <= NumberOfDashes; ++DashIndex) {
				Dash1[1] = Dash1[0] + Step;
				Geometry.polyline(2, Dash1);
				Dash1[0] = Dash1[1] + Step;
				Dash2[1] = Dash2[0] - Step;
				Geometry.polyline(2, Dash2);
				Dash2[0] = Dash2[1] - Step;
			}
			NumberOfDashes = static_cast<int>((points[2] - points[1]).length() / Step.length() - 1.0) / 2;
			Step = (points[2] - points[1]) / (static_cast<double>(NumberOfDashes) * 2.0 + 1.0);
			Dash1[0] = points[1];
			Dash2[0] = points[3];
			for (auto DashIndex = 0; DashIndex <= NumberOfDashes; ++DashIndex) {
				Dash1[1] = Dash1[0] + Step;
				Geometry.polyline(2, Dash1);
				Dash1[0] = Dash1[1] + Step;
				Dash2[1] = Dash2[0] - Step;
				Geometry.polyline(2, Dash2);
				Dash2[0] = Dash2[1] - Step;
			}
			return true;
		}
	} PaperDrawProtocolObject;
	OdDbLayout::desc()->addX(OdDbLayoutPaperPE::desc(), &PaperDrawProtocolObject);
}

static void RemovePaperDrawingCustomization() {
	OdDbLayout::desc()->delX(OdDbLayoutPaperPE::desc());
}

static void AddMaterialTextureLoadingMonitor() {
	static class OdGiMaterialTextureLoadPEImpl : public OdStaticRxObject<OdGiMaterialTextureLoadPE> {
	public:
		void startTextureLoading(OdString& /*fileName*/, OdDbBaseDatabase* /*database*/) noexcept override {
			// Material texture to be loaded. Correct loading path here.
		}

		void textureLoaded(const OdString& fileName, OdDbBaseDatabase* /*database*/) override {
			TRACE1("Material texture loaded: %s\n", fileName.c_str());
		}
		/// <remarks> Called by texture loader after file loading, only if texture loading failed. </remarks>
		void textureLoadingFailed(const OdString& fileName, OdDbBaseDatabase* /*database*/) override {
			TRACE1("Failed to load material texture: %s\n", fileName.c_str());
		}
	} MaterialLoadProtocolObject;
	OdGiMaterialTextureEntry::desc()->addX(OdGiMaterialTextureLoadPE::desc(), &MaterialLoadProtocolObject);
}

void RemoveMaterialTextureLoadingMonitor() {
	OdGiMaterialTextureEntry::desc()->delX(OdGiMaterialTextureLoadPE::desc());
}

#include <DbLibraryInfo.h>
#include <summinfo.h>

class EoDlgAbout final : public CDialog {
public:
	EoDlgAbout() noexcept;

	enum { IDD = IDD_ABOUTBOX };

protected:
	void DoDataExchange(CDataExchange* dataExchange);

DECLARE_MESSAGE_MAP()

public:
	BOOL OnInitDialog();
};

EoDlgAbout::EoDlgAbout() noexcept
	: CDialog(IDD) {}

BOOL EoDlgAbout::OnInitDialog() {
	const auto LibraryInfo {oddbGetLibraryInfo()};
	const auto BuildComments {LibraryInfo->getBuildComments()};
	CDialog::OnInitDialog();
	SetDlgItemTextW(IDC_INFO_BUILD, BuildComments);
	return TRUE; // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void EoDlgAbout::DoDataExchange(CDataExchange* dataExchange) {
	CDialog::DoDataExchange(dataExchange);
}
#pragma warning (push)
#pragma warning (disable: 4191) // (level 3) 'operator': unsafe conversion from 'type_of_expression' to 'type_required'
BEGIN_MESSAGE_MAP(EoDlgAbout, CDialog)
END_MESSAGE_MAP()
BEGIN_MESSAGE_MAP(AeSys, CWinAppEx)
		ON_COMMAND(ID_APP_ABOUT, &AeSys::OnAppAbout)
		// Standard file based document commands
		ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
		ON_COMMAND(ID_FILE_OPEN, &AeSys::OnFileOpen)
		// Standard print setup command
		ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
		ON_COMMAND(ID_EDIT_CF_GROUPS, &AeSys::OnEditCfGroups)
		ON_UPDATE_COMMAND_UI(ID_EDIT_CF_GROUPS, &AeSys::OnUpdateEditCfGroups)
		ON_COMMAND(ID_EDIT_CF_IMAGE, &AeSys::OnEditCfImage)
		ON_UPDATE_COMMAND_UI(ID_EDIT_CF_IMAGE, &AeSys::OnUpdateEditCfImage)
		ON_COMMAND(ID_EDIT_CF_TEXT, &AeSys::OnEditCfText)
		ON_UPDATE_COMMAND_UI(ID_EDIT_CF_TEXT, &AeSys::OnUpdateEditCfText)
		ON_COMMAND(ID_HELP_CONTENTS, &AeSys::OnHelpContents)
		ON_COMMAND(ID_MODE_ANNOTATE, &AeSys::OnModeAnnotate)
		ON_UPDATE_COMMAND_UI(ID_MODE_ANNOTATE, &AeSys::OnUpdateModeAnnotate)
		ON_COMMAND(ID_MODE_CUT, &AeSys::OnModeCut)
		ON_UPDATE_COMMAND_UI(ID_MODE_CUT, &AeSys::OnUpdateModeCut)
		ON_COMMAND(ID_MODE_DIMENSION, &AeSys::OnModeDimension)
		ON_UPDATE_COMMAND_UI(ID_MODE_DIMENSION, &AeSys::OnUpdateModeDimension)
		ON_COMMAND(ID_MODE_DRAW, &AeSys::OnModeDraw)
		ON_UPDATE_COMMAND_UI(ID_MODE_DRAW, &AeSys::OnUpdateModeDraw)
		ON_COMMAND(ID_MODE_DRAW2, &AeSys::OnModeDraw2)
		ON_UPDATE_COMMAND_UI(ID_MODE_DRAW2, &AeSys::OnUpdateModeDraw2)
		ON_COMMAND(ID_MODE_EDIT, &AeSys::OnModeEdit)
		ON_UPDATE_COMMAND_UI(ID_MODE_EDIT, &AeSys::OnUpdateModeEdit)
		ON_COMMAND(ID_MODE_FIXUP, &AeSys::OnModeFixup)
		ON_UPDATE_COMMAND_UI(ID_MODE_FIXUP, &AeSys::OnUpdateModeFixup)
		ON_COMMAND(ID_MODE_LETTER, &AeSys::OnModeLetter)
		ON_COMMAND(ID_MODE_LPD, &AeSys::OnModeLpd)
		ON_UPDATE_COMMAND_UI(ID_MODE_LPD, &AeSys::OnUpdateModeLpd)
		ON_COMMAND(ID_MODE_NODAL, &AeSys::OnModeNodal)
		ON_UPDATE_COMMAND_UI(ID_MODE_NODAL, &AeSys::OnUpdateModeNodal)
		ON_COMMAND(ID_MODE_PIPE, &AeSys::OnModePipe)
		ON_UPDATE_COMMAND_UI(ID_MODE_PIPE, &AeSys::OnUpdateModePipe)
		ON_COMMAND(ID_MODE_POWER, &AeSys::OnModePower)
		ON_UPDATE_COMMAND_UI(ID_MODE_POWER, &AeSys::OnUpdateModePower)
		ON_COMMAND(ID_MODE_REVISE, &AeSys::OnModeRevise)
		ON_COMMAND(ID_MODE_TRAP, &AeSys::OnModeTrap)
		ON_UPDATE_COMMAND_UI(ID_MODE_TRAP, &AeSys::OnUpdateModeTrap)
		ON_COMMAND(ID_TRAPCOMMANDS_ADDGROUPS, &AeSys::OnTrapCommandsAddGroups)
		ON_UPDATE_COMMAND_UI(ID_TRAPCOMMANDS_ADDGROUPS, &AeSys::OnUpdateTrapCommandsAddGroups)
		ON_COMMAND(ID_TRAPCOMMANDS_HIGHLIGHT, &AeSys::OnTrapCommandsHighlight)
		ON_UPDATE_COMMAND_UI(ID_TRAPCOMMANDS_HIGHLIGHT, &AeSys::OnUpdateTrapCommandsHighlight)
		ON_COMMAND(ID_VIEW_MODEINFORMATION, &AeSys::OnViewModeInformation)
		ON_UPDATE_COMMAND_UI(ID_VIEW_MODEINFORMATION, &AeSys::OnUpdateViewModeInformation)
		ON_COMMAND(ID_FILE_PLOTSTYLEMANAGER, &AeSys::OnFilePlotStyleManager)
		ON_COMMAND(ID_TOOLS_LOADAPPLICATIONS, &AeSys::OnToolsLoadApplications)
		ON_COMMAND(ID_VECTORIZERTYPE_ADDVECTORIZERDLL, &AeSys::OnVectorizerTypeAddVectorizerDll)
		ON_UPDATE_COMMAND_UI(ID_VECTORIZERTYPE_ADDVECTORIZERDLL, &AeSys::OnUpdateVectorizerTypeAddVectorizerDll)
		ON_COMMAND(ID_VECTORIZERTYPE_CLEARMENU, &AeSys::OnVectorizerTypeClearMenu)
		ON_UPDATE_COMMAND_UI(ID_VECTORIZERTYPE_CLEARMENU, &AeSys::OnUpdateVectorizerTypeClearMenu)
END_MESSAGE_MAP()
#pragma warning (pop)

/// <remarks> Specialization of CCommandLineInfo to add the following switches: bat:, ld:, scr:, exe:, s:, and exit</remarks>
class CFullCommandLineInfo final : public CCommandLineInfo {
public:
	CString saveName;
	bool exit;
	CStringArray appsToLoad;
	CStringArray commandsToExecute;
	CString scriptToExecute;
	CString batToExecute;

	CFullCommandLineInfo() noexcept
		: CCommandLineInfo()
		, exit {false} { }

	void ParseParam(const wchar_t* parameter, const BOOL flag, const BOOL last) override {
		auto Is {false};
		if (flag != 0) {
			if (_wcsnicmp(parameter, L"bat:", 4) == 0) {
				batToExecute = &parameter[4];
				Is = true;
			} else if (_wcsnicmp(parameter, L"ld:", 3) == 0) {
				appsToLoad.Add(&parameter[3]);
				Is = true;
			} else if (_wcsnicmp(parameter, L"scr:", 4) == 0) {
				scriptToExecute = &parameter[4];
				Is = true;
			} else if (_wcsnicmp(parameter, L"ex:", 3) == 0) {
				commandsToExecute.Add(&parameter[3]);
				Is = true;
			} else if (_wcsnicmp(parameter, L"s:", 2) == 0) {
				saveName = &parameter[2];
				Is = true;
			} else if (_wcsicmp(parameter, L"exit") == 0) {
				exit = true;
				Is = true;
			}
		}
		if (!Is || last != 0) {
			CCommandLineInfo::ParseParam(parameter, flag, last);
		}
	}

	void ParseParam(const char* /*parameter*/, BOOL /*flag*/, BOOL /*last*/) override { } // CCommandLineInfo (to suppress C4266 warning)
};

BOOL AeSys::ProcessShellCommand(CCommandLineInfo& commandLineInfo) {
	auto& FullCommandLineInfo {dynamic_cast<CFullCommandLineInfo&>(commandLineInfo)};
	if (!FullCommandLineInfo.batToExecute.IsEmpty()) {
		_wsystem(FullCommandLineInfo.batToExecute);
	}
	for (auto Index = 0; Index < FullCommandLineInfo.appsToLoad.GetCount(); ++Index) {
		odrxDynamicLinker()->loadModule(OdString(FullCommandLineInfo.appsToLoad.GetAt(Index)), false);
	}
	AeSysDoc* TemporaryDocument {nullptr};
	if (commandLineInfo.m_nShellCommand == CCommandLineInfo::FileOpen) {
		TemporaryDocument = dynamic_cast<AeSysDoc*>(OpenDocumentFile(commandLineInfo.m_strFileName));
		if (TemporaryDocument == nullptr) {
			return FALSE;
		}
		if (!FullCommandLineInfo.scriptToExecute.IsEmpty()) {
			CStdioFile ScriptFile(FullCommandLineInfo.scriptToExecute, CFile::modeRead);
			CString Command;
			while (ScriptFile.ReadString(Command) != 0) {
				if (!Command.IsEmpty() && Command[0] != _T('#')) {
					TemporaryDocument->ExecuteCommand(static_cast<const wchar_t*>(Command), true);
				}
			}
		}
		for (auto CommandIndex = 0; CommandIndex < FullCommandLineInfo.commandsToExecute.GetCount(); ++CommandIndex) {
			TemporaryDocument->ExecuteCommand(static_cast<const wchar_t*>(FullCommandLineInfo.commandsToExecute.GetAt(CommandIndex)), true);
		}
	} else {
		CWinAppEx::ProcessShellCommand(commandLineInfo);
	}
	if (!FullCommandLineInfo.saveName.IsEmpty()) {
		if (TemporaryDocument->OnSaveDocument(FullCommandLineInfo.saveName) == 0) {
			return FALSE;
		}
	}
	if (FullCommandLineInfo.exit) {
		PostQuitMessage(0);
	}
	return TRUE;
}

AeSys::AeSys() noexcept {
	EnableHtmlHelp();
	const CClientDC ClientDeviceContext(AfxGetMainWnd());
	m_HighColorMode = ClientDeviceContext.GetDeviceCaps(BITSPIXEL) > 8; // Detect color depth. 256 color toolbars can be used in the high or true color modes only
}

constexpr int gc_RegistryBufferSize = 1040;
constexpr int gc_RegistryMaxProfileName = 128;
constexpr int gc_RegistryMaxPath = 1024;

CString GetRegistryAcadLocation();

CString GetRegistryAcadProfilesKey();

bool GetRegistryString(HKEY key, const wchar_t* subKey, const wchar_t* name, wchar_t* value, int size) noexcept;

// get the value for the ACAD entry in the registry
static CString FindConfigPath(const CString& configType) {
	auto SubKey {GetRegistryAcadProfilesKey()};
	if (!SubKey.IsEmpty()) {
		SubKey += L"\\General";
		wchar_t SearchValue[gc_RegistryMaxPath] {L"\0"};
		if (GetRegistryString(HKEY_CURRENT_USER, SubKey, configType, SearchValue, gc_RegistryMaxPath)) {
			wchar_t ExpandedPath[gc_RegistryMaxPath] {L"\0"};
			ExpandEnvironmentStringsW(SearchValue, ExpandedPath, gc_RegistryMaxPath);
			return CString(ExpandedPath);
		}
	}
	return CString(L"\0");
}

static CString FindConfigFile(const CString& configType, CString file, OdDbSystemServices* systemServices) {
	const auto ConfigurationPath {FindConfigPath(configType)};
	if (!ConfigurationPath.IsEmpty()) {
		file = ConfigurationPath + L"\\" + file;
		if (systemServices->accessFile(file.GetString(), Oda::kFileRead)) {
			return file;
		}
	}
	return CString(L"\0");
}

AeSys theApp;

const ODCOLORREF* AeSys::CurrentPalette() const {
	return odcmAcadPalette(m_BackgroundColor);
}

OdGsDevicePtr AeSys::gsBitmapDevice(OdRxObject* /*view*/, OdDbBaseDatabase* /*database*/, const unsigned long flags) {
	try {
		OdGsModulePtr Module;
		if ((flags & kFor2dExportRender) != 0) { // Don't export HiddenLine viewports as bitmap in Pdf/Dwf/Svg exports.
			if ((flags & kFor2dExportRenderHLR) != 0) {
				return OdGsDevicePtr();
			}

			// Try to export shaded viewports using OpenGL device.
			Module = odrxDynamicLinker()->loadModule(OdWinOpenGLModuleName);
		}
		// Use currently selected device for thumbnails and etc.
		if (Module.isNull()) {
			Module = odrxDynamicLinker()->loadModule(m_VectorizerPath);
		}
		if (Module.isNull()) {
			return OdGsDevicePtr();
		}
		return Module->createBitmapDevice();
	} catch (const OdError&) { }
	return OdGsDevicePtr();
}

bool AeSys::getPassword(const OdString& drawingName, bool /*isXref*/, OdPassword& password) {
	EoDlgPassword PasswordDialog;
	PasswordDialog.m_sFileName = static_cast<const wchar_t*>(drawingName);
	if (PasswordDialog.DoModal() == IDOK) {
		password = PasswordDialog.m_password;
		if (drawingName.right(4) == L".dwg") {
			password.makeUpper();
		}
		return true;
	}
	return false;
}

OdDbPageControllerPtr AeSys::newPageController() {
	switch (m_PagingType & 0x0fU) {
		case 1: // OdDb::kUnload - Simple unloading of objects for partially loaded database.
			return OdRxObjectImpl<ExUnloadController>::createObject();
		case 2: // OdDb::kPage
		case 3: // OdDb::kUnload - Unloading of objects for partially loaded database and paging of objects thru ExPageController.
			return OdRxObjectImpl<ExPageController>::createObject();
		default: ;
	}
	// Paging is not used.
	return static_cast<OdDbPageController*>(nullptr);
}

unsigned AeSys::SetPagingType(const unsigned pagingType) noexcept {
	const auto OldType {m_PagingType};
	m_PagingType = pagingType;
	return OldType;
}

bool AeSys::SetUndoType(const bool useTempFiles) noexcept {
	const auto OldType {m_UseTempFiles};
	m_UseTempFiles = useTempFiles;
	return OldType;
}

OdString AeSys::fileDialog(int flags, const OdString& prompt, const OdString& defExt, const OdString& fileName, const OdString& filter) {
	if (!SupportFileSelectionViaDialog()) {
		return OdString(L"*unsupported*");
	}
	CFileDialog FileDialog(static_cast<BOOL>(flags == OdEd::kGfpForOpen), defExt, fileName, OFN_HIDEREADONLY | OFN_EXPLORER | OFN_PATHMUSTEXIST, filter, AfxGetMainWnd());
	FileDialog.m_ofn.lpstrTitle = prompt;
	if (FileDialog.DoModal() == IDOK) {
		return OdString(FileDialog.GetPathName());
	}
	throw OdEdCancel();
}

OdRxClass* AeSys::databaseClass() const {
	return OdDbDatabaseDoc::desc();
}

OdString AeSys::findFile(const OdString& fileToFind, OdDbBaseDatabase* database, const FindFileHint hint) {
	CString FilePathAndName;
	FilePathAndName.SetString(ExHostAppServices::findFile(fileToFind, database, hint));
	if (FilePathAndName.IsEmpty()) {
		auto SystemServices {odSystemServices()};
		CString FileToFind;
		FileToFind.SetString(fileToFind);
		auto Extension {FileToFind.Right(4)};
		Extension.MakeLower();
		if (Extension == L".pc3") {
			return FindConfigFile(L"PrinterConfigDir", FileToFind, SystemServices).GetString();
		}
		if (Extension == L".stb" || Extension == L".ctb") {
			return FindConfigFile(L"PrinterStyleSheetDir", FileToFind, SystemServices).GetString();
		}
		if (Extension == L".pmp") {
			return FindConfigFile(L"PrinterDescDir", FileToFind, SystemServices).GetString();
		}
		switch (hint) {
			case kFontFile: case kCompiledShapeFile: case kTrueTypeFontFile: case kPatternFile: case kFontMapFile: case kTextureMapFile:
				break;
			case kEmbeddedImageFile:
				if (FileToFind.Left(5).CompareNoCase(L"http:") == 0 || FileToFind.Left(6).CompareNoCase(L"https:") == 0) {
					// <tas="code section removed"</tas>
				}
				// fall through
			case kXRefDrawing: case kTXApplication: case kUnderlayFile: case kDefault: case kPhotometricWebFile:
				return FilePathAndName.GetString();
		}
		if (hint != kTextureMapFile && Extension != L".shx" && Extension != L".pat" && Extension != L".ttf" && Extension != L".ttc" && Extension != L".otf") {
			FileToFind += L".shx";
		} else if (hint == kTextureMapFile) {
			FileToFind.Replace(L"/", L"\\");
			FileToFind.Delete(0, FileToFind.ReverseFind(L'\\') + 1);
		}
		FilePathAndName = hint != kTextureMapFile ? FindConfigPath(L"ACAD") : FindConfigPath(L"AVEMAPS");
		CString Path;
		while (!FilePathAndName.IsEmpty()) {
			const auto PathDelimiter {FilePathAndName.Find(L";")};
			if (PathDelimiter == -1) {
				Path = FilePathAndName;
				FilePathAndName.Empty();
			} else {
				Path = FilePathAndName.Left(PathDelimiter);
				Path += L"\\" + FileToFind;
				if (SystemServices->accessFile(Path.GetString(), Oda::kFileRead)) {
					return Path.GetString();
				}
				FilePathAndName = FilePathAndName.Right(FilePathAndName.GetLength() - PathDelimiter - 1);
			}
		}
		if (hint == kTextureMapFile) {
			return FilePathAndName.GetString();
		}
		if (FilePathAndName.IsEmpty()) {
			const auto AcadLocation {GetRegistryAcadLocation()};
			if (!AcadLocation.IsEmpty()) {
				FilePathAndName = AcadLocation + L"\\Fonts\\" + FileToFind;
				if (!SystemServices->accessFile(FilePathAndName.GetString(), Oda::kFileRead)) {
					FilePathAndName = AcadLocation + L"\\Support\\" + FileToFind;
					if (!SystemServices->accessFile(FilePathAndName.GetString(), Oda::kFileRead)) {
						FilePathAndName.Empty();
					}
				}
			}
		}
	}
	return FilePathAndName.GetString();
}

CString AeSys::GetApplicationPath() {
	wchar_t FileName[MAX_PATH];
	if (GetModuleFileNameW(GetModuleHandleW(nullptr), FileName, MAX_PATH) != 0U) {
		const CString FilePath(FileName);
		const auto Delimiter {FilePath.ReverseFind('\\')};
		return FilePath.Left(Delimiter);
	}
	return L"";
}

void AeSys::auditPrintReport(OdAuditInfo* auditInfo, const OdString& /*line*/, int /*printDest*/) const {
	if (auditDialog != nullptr) {
		auditDialog->PrintReport(dynamic_cast<OdDbAuditInfo*>(auditInfo));
	}
}

OdDbUndoControllerPtr AeSys::newUndoController() {
	if (UndoType()) {
		auto FileUndoController {OdRxObjectImpl<ExFileUndoController>::createObject()};
		FileUndoController->setStorage(newUndoStream());
		return FileUndoController;
	}
	return OdRxObjectImpl<ExUndoController>::createObject();
}

OdStreamBufPtr AeSys::newUndoStream() {
	// OdMemFileStreamImpl = mix of memory and file streams
	return OdRxObjectImpl<OdMemFileStreamImpl<OdStreamBuf> >::createObject();
}

void AeSys::SetRecentCommand(const OdString& command) {
	if (!command.isEmpty() && command != m_RecentCommand) {
		m_RecentCommand = command;
		WriteProfileStringW(L"options", L"Recent Command", m_RecentCommand);
	}
}

CMenu* AeSys::CommandMenu(CMenu** toolsSubMenu) {
	MENUITEMINFO MenuItemInfo;
	MenuItemInfo.cbSize = sizeof(MENUITEMINFO);
	MenuItemInfo.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_SUBMENU | MIIM_ID;
	CString MenuName;
	CMenu* ToolsSubMenu {nullptr};
	auto TopMenu {CMenu::FromHandle(theApp.GetAeSysMenu())};
	for (auto Item = TopMenu->GetMenuItemCount() - 1; Item >= 0; Item--) {
		MenuItemInfo.dwTypeData = nullptr;
		TopMenu->GetMenuItemInfoW(static_cast<unsigned>(Item), &MenuItemInfo, TRUE);
		const auto SizeOfMenuName {++MenuItemInfo.cch};
		MenuItemInfo.dwTypeData = MenuName.GetBuffer(static_cast<int>(SizeOfMenuName));
		TopMenu->GetMenuItemInfoW(static_cast<unsigned>(Item), &MenuItemInfo, TRUE);
		MenuName.ReleaseBuffer();
		if (MenuItemInfo.fType == MFT_STRING && MenuName.Compare(L"&Tools") == 0) {
			ToolsSubMenu = CMenu::FromHandle(MenuItemInfo.hSubMenu);
			break;
		}
	}
	ASSERT(ToolsSubMenu != nullptr);
	if (toolsSubMenu != nullptr) {
		*toolsSubMenu = ToolsSubMenu;
	}
	CMenu* RegisteredCommandsSubMenu {nullptr};
	for (auto ToolsMenuItem = 0; ToolsMenuItem < ToolsSubMenu->GetMenuItemCount(); ToolsMenuItem++) {
		MenuItemInfo.dwTypeData = nullptr;
		ToolsSubMenu->GetMenuItemInfoW(static_cast<unsigned>(ToolsMenuItem), &MenuItemInfo, TRUE);
		const auto SizeOfMenuName {++MenuItemInfo.cch};
		MenuItemInfo.dwTypeData = MenuName.GetBuffer(static_cast<int>(SizeOfMenuName));
		ToolsSubMenu->GetMenuItemInfoW(static_cast<unsigned>(ToolsMenuItem), &MenuItemInfo, TRUE);
		MenuName.ReleaseBuffer();
		if (MenuItemInfo.fType == MFT_STRING && MenuName.CompareNoCase(L"Registered &Commands") == 0) {
			RegisteredCommandsSubMenu = CMenu::FromHandle(MenuItemInfo.hSubMenu);
			break;
		}
	}
	ENSURE(RegisteredCommandsSubMenu != nullptr);
	return RegisteredCommandsSubMenu;
}

void AeSys::RefreshCommandMenu() {
	CMenu* ToolsSubMenu {nullptr};
	auto RegisteredCommandsSubMenu {CommandMenu(&ToolsSubMenu)};
	for (auto Item = RegisteredCommandsSubMenu->GetMenuItemCount() - 1; Item >= 0; Item--) {
		auto SubMenu {RegisteredCommandsSubMenu->GetSubMenu(Item)};
		if (SubMenu != nullptr) {
			SubMenu->DestroyMenu();
		}
		RegisteredCommandsSubMenu->DeleteMenu(static_cast<unsigned>(Item), MF_BYPOSITION);
	}
	ENSURE(RegisteredCommandsSubMenu->GetMenuItemCount() == 0);
	MENUITEMINFO MenuItemInfo;
	MenuItemInfo.cbSize = sizeof(MENUITEMINFO);
	MenuItemInfo.fMask = MIIM_DATA;
	auto CommandStack {odedRegCmds()};
	const auto HasNoCommand {CommandStack->newIterator()->done()};
	const unsigned ToolsMenuItem {8}; // <tas="Until calculated ToolsMenu position finished. Menu resource which change the location of Registered Commands location break."</tas>
	ToolsSubMenu->EnableMenuItem(ToolsMenuItem, static_cast<unsigned>(MF_BYPOSITION | (HasNoCommand ? MF_GRAYED : MF_ENABLED)));
	unsigned CommandId {_APS_NEXT_COMMAND_VALUE + 100};
	if (!HasNoCommand) {
		auto CommandStackGroupIterator = CommandStack->newGroupIterator();
		while (!CommandStackGroupIterator->done()) {
			OdRxDictionaryPtr Group {CommandStackGroupIterator->object()};
			CMenu GroupMenu;
			GroupMenu.CreateMenu();
			OdRxIteratorPtr GroupCommandIterator {Group->newIterator(OdRx::kDictSorted)};
			OdString GroupName;
			while (!GroupCommandIterator->done()) {
				OdEdCommandPtr Command {GroupCommandIterator->object().get()};
				if (GroupName.isEmpty()) {
					GroupName = Command->groupName();
				}
				auto CommandName(Command->globalName());
				GroupMenu.AppendMenuW(MF_STRING, CommandId, CommandName);
				MenuItemInfo.dwItemData = static_cast<unsigned long>(reinterpret_cast<long>(Command.get()));
				SetMenuItemInfoW(GroupMenu.m_hMenu, CommandId, FALSE, &MenuItemInfo);
				GroupCommandIterator->next();
				CommandId++;
			}
			ENSURE(RegisteredCommandsSubMenu->AppendMenuW(MF_STRING | MF_POPUP, static_cast<UINT_PTR>(reinterpret_cast<LPARAM>(GroupMenu.Detach())), GroupName) != 0);
			CommandStackGroupIterator->next();
			GroupName.empty();
		}
	}
	m_NumCustomCommands = CommandId - _APS_NEXT_COMMAND_VALUE - 100;
}

void AeSys::AddReactor(const OdApplicationReactor* reactor) {
	if (applicationReactors.end() == std::find(applicationReactors.begin(), applicationReactors.end(), OdApplicationReactorPtr(reactor))) {
		applicationReactors.emplace_back(reactor);
	}
}

void AeSys::RemoveReactor(const OdApplicationReactor* reactor) {
	applicationReactors.erase(std::remove(applicationReactors.begin(), applicationReactors.end(), OdApplicationReactorPtr(reactor)), applicationReactors.end());
}

OdDbDatabasePtr AeSys::OpenFile(const wchar_t* pathName) {
	auto MainFrame {dynamic_cast<CMainFrame*>(GetMainWnd())};
	OdDbDatabasePtr Database;
	auto MtMode {getMtMode()};
	m_UseMtLoading ? (MtMode |= 1) : MtMode &= ~1;
	setMtMode(MtMode);

	// open an existing document
	MainFrame->StartTimer();
	try {
		const OdString PathName(pathName);
		if (PathName.right(4).iCompare(L".dgn") == 0) {
			// <tas="Likely will never support dgn"</tas>
			MainFrame->StopTimer(L"Loading");
		} else if (PathName.right(4).iCompare(L".dwf") == 0 || PathName.right(5).iCompare(L".dwfx") == 0) {
			// <tas="Will add support for dwf"</tas>
			MainFrame->StopTimer(L"Loading");
		} else if (m_Recover) {
			ODA_ASSERT(!auditDialog);
			auditDialog = new EoDlgAudit();
			ODA_ASSERT(auditDialog);
			ODA_VERIFY(auditDialog->Create(IDD_AUDITINFO));
			EoAppAuditInfo ApplicationAuditInformation;
			ApplicationAuditInformation.setHostAppServices(&theApp);
			Database = recoverFile(createFile(PathName), &ApplicationAuditInformation);
			MainFrame->StopTimer(L"Recovering");
			auditDialog->SetWindowTextW(L"Recover info " + PathName);
			auditDialog->ShowWindow(SW_SHOW);
			auditDialog = nullptr;
		} else {
			m_Loading = true;
			Database = readFile(PathName, false, m_Partial);
			MainFrame->StopTimer(L"Loading");
			m_Loading = false;
		}
	} catch (const OdError& Error) {
		Database = nullptr;
		MainFrame->SetStatusPaneTextAt(0, L"");
		ErrorMessageBox(L"Loading Error...", Error);
	} catch (const UserBreak&) {
		Database = nullptr;
		MainFrame->SetStatusPaneTextAt(0, L"");
		SetStatusPaneTextAt(1, L"Operation was canceled by user.");
	} catch (std::bad_alloc&) {
		Database = nullptr;
		MainFrame->SetStatusPaneTextAt(0, L"");
		SetStatusPaneTextAt(1, L"Memory Allocation Error...");
	}
	if (auditDialog != nullptr) { // Destroy audit dialog if recover failed
		delete auditDialog;
		auditDialog = nullptr;
	}
	return Database;
}

void AeSys::AddModeInformationToMessageList() const {
	auto ResourceString {LoadStringResource(m_CurrentMode)};
	auto NextToken {0};
	ResourceString = ResourceString.Tokenize(L"\n", NextToken);
	AddStringToMessageList(ResourceString);
}

void AeSys::AddStringToMessageList(const wchar_t* message) {
	auto MainFrame {dynamic_cast<CMainFrame*>(AfxGetMainWnd())};
	MainFrame->GetOutputPane().AddStringToMessageList(message);
	if (MainFrame->GetOutputPane().IsWindowVisible() == 0) {
		MainFrame->SetStatusPaneTextAt(gc_StatusInfo, message);
	}
}

void AeSys::AddStringToMessageList(const wchar_t* message, const wchar_t* string) const {
	CString FormatString;
	FormatString.Format(message, string);
	AddStringToMessageList(FormatString);
}

void AeSys::AddStringToMessageList(const unsigned stringResourceIdentifier) const {
	const auto ResourceString {LoadStringResource(stringResourceIdentifier)};
	AddStringToMessageList(ResourceString);
}

void AeSys::AddStringToMessageList(const unsigned stringResourceIdentifier, const wchar_t* string) const {
	const auto FormatSpecification {LoadStringResource(stringResourceIdentifier)};
	AddStringToMessageList(FormatSpecification, string);
}

void AeSys::AddStringToReportList(const wchar_t* message) {
	auto MainFrame {dynamic_cast<CMainFrame*>(AfxGetMainWnd())};
	MainFrame->GetOutputPane().AddStringToReportsList(message);
	if (MainFrame->GetOutputPane().IsWindowVisible() == 0) {
		MainFrame->SetStatusPaneTextAt(gc_StatusInfo, message);
	}
}

int AeSys::ArchitecturalUnitsFractionPrecision() const noexcept {
	return m_ArchitecturalUnitsFractionPrecision;
}
// Modifies the base accelerator table by defining the mode specific keys.
void AeSys::BuildModeSpecificAcceleratorTable() const {
	const auto MainFrame {dynamic_cast<CMainFrame*>(AfxGetMainWnd())};
	auto AcceleratorTableHandle {MainFrame->m_hAccelTable};
	DestroyAcceleratorTable(AcceleratorTableHandle);
	if (m_ModeResourceIdentifier == 0 || m_ModeResourceIdentifier == IDR_COMMAND_MODE) {
		MainFrame->m_hAccelTable = LoadAcceleratorsW(m_hInstance, MAKEINTRESOURCEW(IDR_COMMAND_MODE));
	} else {
		const auto ModeAcceleratorTableHandle {LoadAcceleratorsW(m_hInstance, MAKEINTRESOURCEW(m_ModeResourceIdentifier))};
		const auto ModeAcceleratorTableEntries {CopyAcceleratorTableW(ModeAcceleratorTableHandle, nullptr, 0)};
		AcceleratorTableHandle = LoadAcceleratorsW(m_hInstance, MAKEINTRESOURCEW(IDR_MAINFRAME));
		const auto AcceleratorTableEntries {CopyAcceleratorTableW(AcceleratorTableHandle, nullptr, 0)};
		const auto ModifiedAcceleratorTable {new tagACCEL[static_cast<unsigned>(AcceleratorTableEntries + ModeAcceleratorTableEntries)]};
		CopyAcceleratorTableW(ModeAcceleratorTableHandle, ModifiedAcceleratorTable, ModeAcceleratorTableEntries);
		CopyAcceleratorTableW(AcceleratorTableHandle, &ModifiedAcceleratorTable[ModeAcceleratorTableEntries], AcceleratorTableEntries);
		MainFrame->m_hAccelTable = ::CreateAcceleratorTable(ModifiedAcceleratorTable, AcceleratorTableEntries + ModeAcceleratorTableEntries);
		delete[] ModifiedAcceleratorTable;
	}
}

unsigned AeSys::ClipboardFormatIdentifierForEoGroups() const noexcept {
	return m_ClipboardFormatIdentifierForEoGroups;
}

unsigned AeSys::CurrentMode() const noexcept {
	return m_CurrentMode;
}

double AeSys::DeviceHeightInMillimeters() const noexcept {
	return m_DeviceHeightInMillimeters;
}

double AeSys::DeviceHeightInPixels() const noexcept {
	return m_DeviceHeightInPixels;
}

double AeSys::DeviceWidthInMillimeters() const noexcept {
	return m_DeviceWidthInMillimeters;
}

double AeSys::DeviceWidthInPixels() const noexcept {
	return m_DeviceWidthInPixels;
}

double AeSys::DimensionAngle() const noexcept {
	return m_DimensionAngle;
}

double AeSys::DimensionLength() const noexcept {
	return m_DimensionLength;
}

void AeSys::EditColorPalette() {
	CHOOSECOLORW ChooseColorInfo;
	::ZeroMemory(&ChooseColorInfo, sizeof(CHOOSECOLORW));
	ChooseColorInfo.lStructSize = sizeof(CHOOSECOLORW);
	ChooseColorInfo.rgbResult = g_ColorPalette[g_PrimitiveState.ColorIndex()];
	ChooseColorInfo.lpCustColors = g_ColorPalette;
	ChooseColorInfo.Flags = CC_FULLOPEN | CC_RGBINIT | CC_SOLIDCOLOR;
	ChooseColorW(&ChooseColorInfo);
	ChooseColorInfo.rgbResult = g_GreyPalette[g_PrimitiveState.ColorIndex()];
	ChooseColorInfo.lpCustColors = g_GreyPalette;
	ChooseColorW(&ChooseColorInfo);
	MessageBoxW(nullptr, L"The background color is no longer associated with the pen Color Palette.", L"Deprecation Notice", MB_OK | MB_ICONINFORMATION);
	AeSysDoc::GetDoc()->UpdateAllViews(nullptr);
}

double AeSys::EngagedAngle() const noexcept {
	return m_EngagedAngle;
}

double AeSys::EngagedLength() const noexcept {
	return m_EngagedLength;
}

CString AeSys::BrowseWithPreview(const HWND parentWindow, const wchar_t* filter, bool /*multiple*/) {
	CString FileName;
	const unsigned long Flags(OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST);
	const CString LibraryFileName(L"FileDlgExt" TD_DLL_VERSION_SUFFIX_STR L".dll");
	const auto LibraryModule {LoadLibraryW(LibraryFileName)};
	if (LibraryModule != nullptr) {
		const auto OpenWithPreviewDlgProc {reinterpret_cast<ODA_OPEN_DLGPROC>(GetProcAddress(LibraryModule, "CreateOpenWithPreviewDlg"))};
		if (OpenWithPreviewDlgProc != nullptr) {
			EoPreviewDib PreviewDeviceIndependentBitmap;
			OpenWithPreviewDlg* OpenWithPreviewDialog;
			(OpenWithPreviewDlgProc)(&PreviewDeviceIndependentBitmap, parentWindow, nullptr, filter, Flags, &OpenWithPreviewDialog);
			if (IDOK == OpenWithPreviewDialog->ShowModal()) {
				long BufferLength {MAX_PATH};
				OpenWithPreviewDialog->GetFullFileName(FileName.GetBuffer(BufferLength), BufferLength);
				FileName.ReleaseBuffer();
			}
			OpenWithPreviewDialog->ReleaseDlg();
		}
		FreeLibrary(LibraryModule);
	} else {
		CString Filter(filter);
		Filter.Replace('|', '\0');
		OPENFILENAME OpenFileName;
		::ZeroMemory(&OpenFileName, sizeof(OPENFILENAME));
		OpenFileName.lStructSize = sizeof(OPENFILENAME);
		OpenFileName.hwndOwner = parentWindow;
		OpenFileName.lpstrFilter = Filter;
		OpenFileName.nFilterIndex = 1;
		OpenFileName.lpstrFile = FileName.GetBuffer(MAX_PATH);
		OpenFileName.nMaxFile = MAX_PATH;
		OpenFileName.lpstrInitialDir = nullptr;
		OpenFileName.Flags = Flags;
		GetOpenFileNameW(&OpenFileName);
		FileName.ReleaseBuffer();
	}
	return FileName;
}

int AeSys::ExitInstance() {
	SetRegistryBase(L"ODA View");
	theApp.WriteInt(L"Discard Back Faces", static_cast<int>(m_DiscardBackFaces));
	theApp.WriteInt(L"Enable Double Buffer", static_cast<int>(m_EnableDoubleBuffer));
	theApp.WriteInt(L"Enable Blocks Cache", static_cast<int>(m_BlocksCache));
	theApp.WriteInt(L"Gs Device Multithreading", static_cast<int>(m_GsDevMultithreading));
	theApp.WriteInt(L"Mt Regenerate Threads Count", static_cast<int>(m_MtRegenerateThreads));
	theApp.WriteInt(L"Print/Preview via bitmap device", static_cast<int>(m_EnablePrintPreviewViaBitmap));
	theApp.WriteInt(L"UseGsModel", static_cast<int>(m_UseGsModel));
	theApp.WriteInt(L"Enable Software HLR", static_cast<int>(m_EnableHlr));
	theApp.WriteInt(L"Contextual Colors", static_cast<int>(m_ContextColors));
	theApp.WriteInt(L"TTF PolyDraw", static_cast<int>(m_TtfPolyDraw));
	theApp.WriteInt(L"TTF TextOut", static_cast<int>(m_TtfTextOut));
	theApp.WriteInt(L"TTF Cache", static_cast<int>(m_TtfCache));
	theApp.WriteInt(L"Dynamic Subentities Highlight", static_cast<int>(m_DynamicSubEntHlt));
	theApp.WriteInt(L"GDI Gradients as Bitmaps", static_cast<int>(m_GdiGradientsAsBitmap));
	theApp.WriteInt(L"GDI Gradients as Polys", static_cast<int>(m_GdiGradientsAsPolys));
	theApp.WriteInt(L"GDI Gradients as Polys Threshold", m_NGdiGradientsAsPolysThreshold);
	theApp.WriteInt(L"Disable Auto-Regen", static_cast<int>(m_DisableAutoRegenerate));
	theApp.WriteInt(L"Save round trip information", static_cast<int>(saveRoundTrip));
	theApp.WriteInt(L"Save Preview", static_cast<int>(savePreview));
	theApp.WriteInt(L"Background color", static_cast<int>(m_BackgroundColor));
	theApp.WriteInt(L"Save DWG with password", static_cast<int>(saveWithPassword));
	theApp.WriteString(L"recent GS", m_VectorizerPath);
	theApp.WriteString(L"Recent Command", m_RecentCommand);
	theApp.WriteInt(L"Fill TTF text", static_cast<int>(getTEXTFILL()));
	SetRegistryBase(L"Options");
	applicationOptions.Save();
	SetRegistryBase(L"MFC Auto");
	ReleaseSimplexStrokeFont();
	UninitializeTeigha();
	AfxOleTerm(FALSE);
	return CWinAppEx::ExitInstance();
}

CString AeSys::FormatAngle(const double angle, const int width, const int precision) {
	CString FormatSpecification;
	FormatSpecification.Format(L"%%%i.%if\u00b0", width, precision);
	CString AngleAsString;
	AngleAsString.Format(FormatSpecification, EoToDegree(angle));
	return AngleAsString;
}

CString AeSys::FormatLength(const double length, const Units units, const int width, const int precision) const {
	wchar_t LengthAsString[32];
	FormatLengthStacked(LengthAsString, 32, units, length, width, precision);
	return CString(LengthAsString).TrimLeft();
}

/// <summary>
///Produces a string formatted to type units from a "length" value
///ArchitecturalS units formatted as follows:
///	\S[feet]'[inches].[fraction numerator]/[fraction denominator];"
///Architectural units formatted as follows:
///	[feet]'[inches].[fraction numerator] [fraction denominator]"
///Engineering units formatted as follows:
///	[feet]'[inches].[decimal inches]"
///All other units formatted using floating decimal.
/// </summary>
void AeSys::FormatLengthStacked(wchar_t* lengthAsString, const unsigned bufSize, const Units units, const double length, const int width, const int precision) const {
	wchar_t Buffer[16] {L""};
	auto ScaledLength {length * AeSysView::GetActiveView()->WorldScale()};
	CString FormatSpecification;
	FormatSpecification.Format(L"%%%i.%if", width, precision);
	switch (units) {
		case kArchitectural: case kArchitecturalS: {
			wcscpy_s(lengthAsString, bufSize, length >= 0.0 ? L" " : L"-");
			ScaledLength = fabs(ScaledLength);
			auto Feet {static_cast<int>(ScaledLength / 12.0)};
			auto Inches {abs(static_cast<int>(fmod(ScaledLength, 12.0)))};
			const auto FractionPrecision {ArchitecturalUnitsFractionPrecision()};
			auto Numerator {lround(fabs(fmod(ScaledLength, 1.0)) * static_cast<double>(FractionPrecision))}; // Numerator of fractional component of inches
			if (Numerator == FractionPrecision) {
				if (Inches == 11) {
					Feet++;
					Inches = 0;
				} else {
					Inches++;
				}
				Numerator = 0;
			}
			_itow_s(Feet, Buffer, 16, 10);
			wcscat_s(lengthAsString, bufSize, Buffer);
			wcscat_s(lengthAsString, bufSize, L"'");
			_itow_s(Inches, Buffer, 16, 10);
			wcscat_s(lengthAsString, bufSize, Buffer);
			if (Numerator > 0) {
				wcscat_s(lengthAsString, bufSize, units == kArchitecturalS ? L"\\S" : L"\u00b7" /*middle dot*/);
				const auto Divisor {GreatestCommonDivisor(Numerator, FractionPrecision)};
				Numerator /= Divisor;
				const auto Denominator {FractionPrecision / Divisor}; // Add fractional component of inches
				_itow_s(Numerator, Buffer, 16, 10);
				wcscat_s(lengthAsString, bufSize, Buffer);
				wcscat_s(lengthAsString, bufSize, L"/");
				_itow_s(Denominator, Buffer, 16, 10);
				wcscat_s(lengthAsString, bufSize, Buffer);
				if (units == kArchitecturalS) {
					wcscat_s(lengthAsString, bufSize, L";");
				}
			}
			wcscat_s(lengthAsString, bufSize, L"\"");
			break;
		}
		case kEngineering: {
			wcscpy_s(lengthAsString, bufSize, length >= 0.0 ? L" " : L"-");
			ScaledLength = fabs(ScaledLength);
			const auto Precision {ScaledLength >= 1.0 ? precision - int(log10(ScaledLength)) - 1 : precision};
			if (Precision >= 0) {
				_itow_s(int(ScaledLength / 12.0), Buffer, 16, 10);
				wcscat_s(lengthAsString, bufSize, Buffer);
				ScaledLength = fmod(ScaledLength, 12.0);
				wcscat_s(lengthAsString, bufSize, L"'");
				_itow_s(int(ScaledLength), Buffer, 16, 10);
				wcscat_s(lengthAsString, bufSize, Buffer);
				if (Precision > 0) {
					FormatSpecification.Format(L"%%%i.%if", width, Precision);
					CString FractionalInches;
					FractionalInches.Format(FormatSpecification, ScaledLength);
					const auto DecimalPointPosition = FractionalInches.Find('.');
					FractionalInches = FractionalInches.Mid(DecimalPointPosition) + L"\"";
					wcscat_s(lengthAsString, bufSize, FractionalInches);
				}
			}
			break;
		}
		case kFeet:
			FormatSpecification.Append(L"'");
			swprintf_s(lengthAsString, bufSize, FormatSpecification, ScaledLength / 12.0);
			break;
		case kInches:
			FormatSpecification.Append(L"\"");
			swprintf_s(lengthAsString, bufSize, FormatSpecification, ScaledLength);
			break;
		case kMeters:
			FormatSpecification.Append(L"m");
			swprintf_s(lengthAsString, bufSize, FormatSpecification, ScaledLength * 0.0254);
			break;
		case kMillimeters:
			FormatSpecification.Append(L"mm");
			swprintf_s(lengthAsString, bufSize, FormatSpecification, ScaledLength * 25.4);
			break;
		case kCentimeters:
			FormatSpecification.Append(L"cm");
			swprintf_s(lengthAsString, bufSize, FormatSpecification, ScaledLength * 2.54);
			break;
		case kDecimeters:
			FormatSpecification.Append(L"dm");
			swprintf_s(lengthAsString, bufSize, FormatSpecification, ScaledLength * 0.254);
			break;
		case kKilometers:
			FormatSpecification.Append(L"km");
			swprintf_s(lengthAsString, bufSize, FormatSpecification, ScaledLength * 0.0000254);
			break;
		default:
			lengthAsString[0] = '\0';
			break;
	}
}

OdGePoint3d AeSys::GetCursorPosition() {
	auto ActiveView {AeSysView::GetActiveView()};
	return ActiveView != nullptr ? ActiveView->GetCursorPosition() : OdGePoint3d::kOrigin;
}

EoDb::FileTypes AeSys::GetFileType(const OdString& file) {
	auto Type(EoDb::kUnknown);
	const auto Extension {file.right(3)};
	if (!Extension.isEmpty()) {
		if (Extension.iCompare(L"peg") == 0) {
			Type = EoDb::kPeg;
		} else if (Extension.iCompare(L"tra") == 0) {
			Type = EoDb::kTracing;
		} else if (Extension.iCompare(L"jb1") == 0) {
			Type = EoDb::kJob;
		} else if (Extension.iCompare(L"dwg") == 0) {
			Type = EoDb::kDwg;
		} else if (Extension.iCompare(L"dxf") == 0) {
			Type = EoDb::kDxf;
		} else if (Extension.iCompare(L"dxb") == 0) {
			Type = EoDb::kDxb;
		}
	}
	return Type;
}

COLORREF AeSys::GetHotColor(const short colorIndex) noexcept {
	return g_ColorPalette[colorIndex];
}

HINSTANCE AeSys::GetInstance() const noexcept {
	return m_hInstance;
}

HMENU AeSys::GetAeSysMenu() const noexcept {
	return m_AeSysMenuHandle;
}

HMENU AeSys::GetAeSysSubMenu(const int position) const noexcept {
	return GetSubMenu(m_AeSysMenuHandle, position);
}

AeSys::Units AeSys::GetUnits() const noexcept {
	return m_Units;
}

/// <summary>Finds the greatest common divisor of arbitrary integers.</summary>
/// <returns>First number if second number is zero, greatest common divisor otherwise.</returns>
int AeSys::GreatestCommonDivisor(const int number1, const int number2) const noexcept {
	auto ReturnValue {abs(number1)};
	auto Divisor {abs(number2)};
	while (Divisor != 0) {
		const auto Remainder {ReturnValue % Divisor};
		ReturnValue = Divisor;
		Divisor = Remainder;
	}
	return ReturnValue;
}

bool AeSys::HighColorMode() const noexcept {
	return m_HighColorMode;
}

OdGePoint3d AeSys::HomePointGet(const int i) noexcept {
	if (i >= 0 && i < 9) {
		return m_HomePoints[i];
	}
	return OdGePoint3d::kOrigin;
}

void AeSys::HomePointSave(const int i, const OdGePoint3d& point) noexcept {
	if (i >= 0 && i < 9) {
		m_HomePoints[i] = point;
	}
}

void AeSys::InitializeGlobals(CDC* deviceContext) {
	g_PrimitiveState.SetHatchInteriorStyle(EoDbHatch::kHatch);
	g_PrimitiveState.SetHatchInteriorStyleIndex(1);
	EoDbHatch::patternScaleX = 0.1;
	EoDbHatch::patternScaleY = 0.1;
	EoDbHatch::patternAngle = 0.0;
	const EoDbCharacterCellDefinition CharacterCellDefinition;
	g_PrimitiveState.SetCharacterCellDefinition(CharacterCellDefinition);
	const EoDbFontDefinition FontDefinition;
	g_PrimitiveState.SetFontDefinition(deviceContext, FontDefinition);
	SetUnits(kInches);
	SetArchitecturalUnitsFractionPrecision(8);
	SetDimensionLength(0.125);
	SetDimensionAngle(45.0);
	m_TrapHighlighted = true;
	m_TrapHighlightColor = 15;

	//Document->InitializeGroupAndPrimitiveEdit();
	g_PrimitiveState.SetPen(nullptr, deviceContext, 1, 1);
	g_PrimitiveState.SetPointDisplayMode(1);
}

bool AeSys::InitializeOda() {
	try {
		odInitialize(this);
		EoLoadApps::rxInit();
		OdApplicationReactor::rxInit();
		OdApplicationDocument::rxInit();
		odrxDynamicLinker()->loadModule(OdGripPointsModuleName, false); // GripPoints module
		odrxDynamicLinker()->loadModule(OdDynBlocksModuleName, false); // Core dynamic blocks support
		odrxDynamicLinker()->loadModule(Od3DSolidHistoryTxModuleName, false);
		odrxDynamicLinker()->loadModule(DbConstraintsModuleName, false); // Drawings constraints module
		odrxDynamicLinker()->loadModule(TfModuleName, false); // ODA Platform format import/export module
		// odrxDynamicLinker()->loadModule(ExDimAssocModuleName, false); // interface for recalculation of associative dimensions, leaders, mleaders
		odrxDynamicLinker()->loadModule(OdDbCommandsModuleName, false); // DbCommands module (ERASE,EXPLODE,PURGE, etc.)
		odrxDynamicLinker()->loadModule(OdExCustObjsModuleName, false);
		odrxDynamicLinker()->loadModule(OdRecomputeDimBlockModuleName, false);
		odrxDynamicLinker()->loadModule(OdAcIdViewObjModuleName, false);
		odrxDynamicLinker()->loadModule(OdExCommandsModuleName, false);
		odrxDynamicLinker()->loadModule(OdRasterExportModuleName, false);
		odrxDynamicLinker()->loadModule(OdExDeepCloningModuleName, false);
		odrxDynamicLinker()->loadModule(OdExDynamicBlocksModuleName, false); // Sample commands illustrating dynamic blocks usage
		odrxDynamicLinker()->loadModule(OdAveModuleName, false); // Support of old style materials, lights, etc. (AVE_*)
		odrxDynamicLinker()->loadModule(OdExFieldEvaluatorModuleName, false); // Field evaluator for "AcVar", and "AcDiesel" fields
		odrxDynamicLinker()->loadModule(OdAutoSurfServicesModuleName, false); // OdAsSurfBody object support module (loads dependency TD_Br_20.5_15.dll if necessary)
		odrxDynamicLinker()->loadModule(OdModelDocObjModuleName, false);
		odrxDynamicLinker()->loadModule(OdPlotSettingsValidatorModuleName, false); // PlotSettingsValidator module (To include support for plot settings)
		AddPaperDrawingCustomization();
		AddMaterialTextureLoadingMonitor();
		OdDbDatabaseDoc::rxInit();
#ifdef OD_OLE_SUPPORT
		::rxInit_COleClientItem_handler();
#endif // OD_OLE_SUPPORT
		auto CommandStack {odedRegCmds()};
		CommandStack->addCommand(&g_CommandView);
		CommandStack->addCommand(&g_CommandSelect);

		/* <tas>
		rxInitMaterialsEditorObjects();
		   </tas> */
	} catch (const OdError& Error) {
		theApp.ErrorMessageBox(L"odInitialize error", Error);
		return false;
	} catch (...) {
		MessageBoxW(nullptr, L"odInitialize error", L"Open Design Alliance", MB_ICONERROR | MB_OK);
		return false;
	}
	return true;
}

BOOL AeSys::InitInstance() {
	// InitCommonControlsEx() is required on Windows XP if an application manifest specifies use of ComCtl32.dll version 6 or later to enable visual styles. Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitializeCommonControls;
	InitializeCommonControls.dwSize = sizeof InitializeCommonControls;

	// Load animate control, header, hot key, list-view, progress bar, status bar, tab, tooltip, toolbar, trackbar, tree-view, and up-down control classes.
	InitializeCommonControls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitializeCommonControls);
	if (AfxOleInit() == 0) { // Failed to initialize OLE support for the application.
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}
	AfxEnableControlContainer();
	EnableTaskbarInteraction();

	// Standard initialization
	// Application settings to be stored in the registry instead of INI files.
	SetRegistryKey(L"Engineers Office");
	LoadStdProfileSettings(8U); // Load the list of most recently used (MRU) files and last preview state.
	SetRegistryBase(L"Options");
	applicationOptions.Load();
	SetRegistryBase(L"ODA View");
	m_DiscardBackFaces = GetInt(L"Discard Back Faces", 1) != 0;
	m_EnableDoubleBuffer = GetInt(L"Enable Double Buffer", 1) != 0; // <tas="true unless debugging"</tas>
	m_BlocksCache = GetInt(L"Enable Blocks Cache") != 0;
	m_GsDevMultithreading = GetInt(L"Gs Device Multithreading") != 0;
	m_MtRegenerateThreads = static_cast<unsigned>(GetInt(L"Mt Regenerate Threads Count", 4));
	m_EnablePrintPreviewViaBitmap = GetInt(L"Print/Preview via bitmap device", 1) != 0;
	m_UseGsModel = GetInt(L"UseGsModel", 1) != 0;
	m_EnableHlr = GetInt(L"Enable Software HLR") != 0;
	m_ContextColors = GetInt(L"Contextual Colors", 1) != 0;
	m_TtfPolyDraw = GetInt(L"TTF PolyDraw") != 0;
	m_TtfTextOut = GetInt(L"TTF TextOut") != 0;
	m_TtfCache = GetInt(L"TTF Cache") != 0;
	m_DynamicSubEntHlt = GetInt(L"Dynamic Subentities Highlight") != 0;
	m_GdiGradientsAsBitmap = GetInt(L"GDI Gradients as Bitmaps") != 0;
	m_GdiGradientsAsPolys = GetInt(L"GDI Gradients as Polys") != 0;
	m_NGdiGradientsAsPolysThreshold = gsl::narrow_cast<unsigned char>(GetInt(L"GDI Gradients as Polys Threshold", 10));
	m_DisableAutoRegenerate = GetInt(L"Disable Auto-Regen") != 0;

	//	m_displayFields = GetProfileInt(_T("options"), _T("Field display format"), 0);
	saveRoundTrip = GetInt(L"Save round trip information", 1) != 0;
	savePreview = GetInt(L"Save Preview") != 0;
	m_BackgroundColor = static_cast<unsigned>(GetInt(L"Background color", static_cast<int>(g_ViewBackgroundColor)));
	saveWithPassword = GetInt(L"Save DWG with password") != 0;
	m_VectorizerPath = GetString(L"recent GS", OdWinDirectXModuleName);
	m_RecentCommand = GetString(L"Recent Command", L"");
	const auto FillTtf = GetInt(L"Fill TTF text", 1);
	setTEXTFILL(FillTtf != 0);
	SetRegistryBase(L"MFC Auto");
	Lex::Init();

	// Initialize application managers for usage. They are automatically constructed if not yet present
	InitContextMenuManager(); // Manages shortcut menus, also known as context menus.
	InitKeyboardManager(); // Manages shortcut key tables for the main frame window and child frame windows.
	InitTooltipManager(); // Maintains runtime information about tooltips.
	CMFCToolTipInfo ToolTipInfo;
	ToolTipInfo.m_bVislManagerTheme = TRUE;
	GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL, RUNTIME_CLASS(CMFCToolTipCtrl), &ToolTipInfo);
	EnableUserTools(ID_TOOLS_ENTRY, ID_USER_TOOL1, ID_USER_TOOL10, RUNTIME_CLASS(CUserTool), IDR_MENU_ARGS, IDR_MENU_DIRS);
	CMFCButton::EnableWindowsTheming();
	CWinAppEx::InitInstance();
	if (!InitializeOda()) {
		return FALSE;
	}

	// Register the application's document templates.  Document templates serve as the connection between documents, frame windows and views.
	const auto AeSysDocTemplate {new CMultiDocTemplate(IDR_AESYSTYPE, RUNTIME_CLASS(AeSysDoc), RUNTIME_CLASS(CChildFrame), RUNTIME_CLASS(AeSysView))};
	if (AeSysDocTemplate == nullptr) {
		return FALSE;
	}
	AddDocTemplate(AeSysDocTemplate);
	const auto TracingDocTemplate {new CMultiDocTemplate(IDR_TRACINGTYPE, RUNTIME_CLASS(AeSysDoc), RUNTIME_CLASS(CChildFrame), RUNTIME_CLASS(AeSysView))};
	if (TracingDocTemplate == nullptr) {
		return FALSE;
	}
	AddDocTemplate(TracingDocTemplate);

	// Create main MDI Frame window
	auto MainFrame {new CMainFrame};
	if (MainFrame == nullptr || MainFrame->LoadFrame(IDR_MAINFRAME) == 0) {
		delete MainFrame;
		return FALSE;
	}
	m_pMainWnd = MainFrame;
	m_pMainWnd->DragAcceptFiles();
	const auto DeviceContext {MainFrame->GetDC()};
	m_DeviceWidthInPixels = static_cast<double>(DeviceContext->GetDeviceCaps(HORZRES));
	m_DeviceHeightInPixels = static_cast<double>(DeviceContext->GetDeviceCaps(VERTRES));
	m_DeviceWidthInMillimeters = static_cast<double>(DeviceContext->GetDeviceCaps(HORZSIZE));
	m_DeviceHeightInMillimeters = static_cast<double>(DeviceContext->GetDeviceCaps(VERTSIZE));
	InitializeGlobals(DeviceContext);
	MainFrame->ReleaseDC(DeviceContext);
	m_AeSysMenuHandle = LoadMenuW(m_hInstance, MAKEINTRESOURCEW(IDR_AESYSTYPE));
	RefreshCommandMenu();

	// Parse command line for standard shell commands, DDE, file open
	CFullCommandLineInfo CommandLineInfo;
	ParseCommandLine(CommandLineInfo);
	if (CommandLineInfo.m_nShellCommand == CCommandLineInfo::FileNew) {
		CommandLineInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
	}
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Dispatch commands specified on the command line
	if (ProcessShellCommand(CommandLineInfo) == 0) {
		return FALSE;
	}
	if (RegisterPreviewWindowClass(m_hInstance) == 0U) {
		return FALSE;
	}
	SetShadowFolderPath(L"AeSys Shadow Folder");
	LoadSimplexStrokeFont(ResourceFolderPath() + L"Simplex.psf");
	EoDbHatchPatternTable::LoadHatchesFromFile(ResourceFolderPath() + L"Hatches\\DefaultSet.pat");
	LoadPenWidthsFromFile(ResourceFolderPath() + L"Pens\\Widths.txt");
	//LoadColorPalletFromFile(ResourceFolder + L"Pens\\Colors\\Default.txt"));
	// This is the private data format used to pass EoGroups from one instance to another
	m_ClipboardFormatIdentifierForEoGroups = RegisterClipboardFormatW(L"EoGroups");
	m_ThisThreadId = GetCurrentThreadId();
	m_pMainWnd->ShowWindow(m_nCmdShow);
	m_pMainWnd->UpdateWindow();
	InitPlotStyleSheetEnv();
	return TRUE;
}

bool AeSys::IsClipboardDataGroups() const noexcept {
	return m_ClipboardDataEoGroups;
}

bool AeSys::IsClipboardDataImage() const noexcept {
	return m_ClipboardDataImage;
}

bool AeSys::IsClipboardDataText() const noexcept {
	return m_ClipboardDataText;
}

bool AeSys::IsTrapHighlighted() const noexcept {
	return m_TrapHighlighted;
}

void AeSys::LoadColorPalletFromFile(const CString& fileName) {
	CStdioFile StreamFile;
	if (StreamFile.Open(fileName, CFile::modeRead | CFile::typeText) != 0) {
		wchar_t Line[128] {L"\0"};
		while (StreamFile.ReadString(Line, sizeof Line / sizeof(wchar_t) - 1) != nullptr && _tcsnicmp(Line, L"<Colors>", 8) != 0) { }
		while (StreamFile.ReadString(Line, sizeof Line / sizeof(wchar_t) - 1) != nullptr && *Line != '<') {
			wchar_t* NextToken {nullptr};
			const auto Index {wcstok_s(Line, L"=", &NextToken)};
			auto Red {wcstok_s(nullptr, L",", &NextToken)};
			auto Green {wcstok_s(nullptr, L",", &NextToken)};
			auto Blue {wcstok_s(nullptr, L",", &NextToken)};
			g_ColorPalette[_wtoi(Index)] = RGB(_wtoi(Red), _wtoi(Green), _wtoi(Blue));
			Red = wcstok_s(nullptr, L",", &NextToken);
			Green = wcstok_s(nullptr, L",", &NextToken);
			Blue = wcstok_s(nullptr, L"\n", &NextToken);
			g_GreyPalette[_wtoi(Index)] = RGB(_wtoi(Red), _wtoi(Green), _wtoi(Blue));
		}
	}
}

void AeSys::LoadModeResources(const unsigned mode) {
	BuildModeSpecificAcceleratorTable();
	m_CurrentMode = mode;
	AddModeInformationToMessageList();
	auto ActiveView {AeSysView::GetActiveView()};
	if (ActiveView != nullptr) {
		ActiveView->SetModeCursor(m_CurrentMode);
		ActiveView->ModeLineDisplay();
		ActiveView->RubberBandingDisable();
	}
}

void AeSys::LoadPenWidthsFromFile(const CString& fileName) {
	CStdioFile StreamFile;
	if (StreamFile.Open(fileName, CFile::modeRead | CFile::typeText) != 0) {
		wchar_t PenWidths[64];
		while (StreamFile.ReadString(PenWidths, sizeof PenWidths / sizeof(wchar_t) - 1) != nullptr) {
			wchar_t* NextToken {nullptr};
			const auto PenIndex {_wtoi(wcstok_s(PenWidths, L"=", &NextToken))};
			const auto Width {_wtof(wcstok_s(nullptr, L",\n", &NextToken))};
			if (PenIndex >= 0 && PenIndex < static_cast<int>(sizeof g_PenWidths / sizeof g_PenWidths[0])) {
				g_PenWidths[PenIndex] = Width;
			}
		}
	}
}
/// <remarks> Font stroke table encoded as follows:
/// b0 - b11  relative y displacement
/// b12 - b23 relative x displacement
/// b24 - b31 operation code (5 for line, else move)
/// The font is exactly 16384 bytes and defines a 96 character font set with a maximum of 4096 stokes
/// </remarks>
void AeSys::LoadSimplexStrokeFont(const CString& pathName) {
	const auto OpenHandle {CreateFileW(pathName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
	if (OpenHandle != INVALID_HANDLE_VALUE) {
		if (SetFilePointer(OpenHandle, 0, nullptr, FILE_BEGIN) != INVALID_SET_FILE_POINTER) {
			if (m_SimplexStrokeFont == nullptr) {
				m_SimplexStrokeFont = new char[16384];
			}
			unsigned long NumberOfBytesRead;
			if (ReadFile(OpenHandle, m_SimplexStrokeFont, 16384U, &NumberOfBytesRead, nullptr) == 0) {
				ReleaseSimplexStrokeFont();
			}
		}
		CloseHandle(OpenHandle);
	} else {
		const auto ResourceHandle {FindResourceW(nullptr, MAKEINTRESOURCEW(IDR_PEGSTROKEFONT), L"STROKEFONT")};
		if (ResourceHandle != nullptr) {
			const auto ResourceSize {SizeofResource(nullptr, ResourceHandle)};
			m_SimplexStrokeFont = new char[ResourceSize];
			const auto Resource {LockResource(LoadResource(nullptr, ResourceHandle))};
			memcpy_s(m_SimplexStrokeFont, ResourceSize, Resource, ResourceSize);
		}
	}
}

CString AeSys::LoadStringResource(const unsigned resourceIdentifier) {
	CString String;
	VERIFY(String.LoadStringW(resourceIdentifier) == TRUE);
	return String;
}

bool AeSys::ModeInformationOverView() const noexcept {
	return m_ModeInformationOverView;
}

void AeSys::OnAppAbout() {
	EoDlgAbout Dialog;
	Dialog.DoModal();
}

void AeSys::OnEditCfGroups() noexcept {
	m_ClipboardDataEoGroups = !m_ClipboardDataEoGroups;
}

void AeSys::OnEditCfImage() noexcept {
	m_ClipboardDataImage = !m_ClipboardDataImage;
}

void AeSys::OnEditCfText() noexcept {
	m_ClipboardDataText = !m_ClipboardDataText;
}

void AeSys::OnFileOpen() {
	const unsigned long Flags {OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST};
	auto Filter {LoadStringResource(IDS_OPENFILE_FILTER)};
	CFileDialog FileDialog(TRUE, nullptr, nullptr, Flags, Filter);
	CString FileName;
	FileDialog.m_ofn.lpstrFile = FileName.GetBuffer(MAX_PATH);
	auto Title {LoadStringResource(AFX_IDS_OPENFILE)};
	FileDialog.m_ofn.lpstrTitle = Title;
	const auto Result {FileDialog.DoModal()};
	FileName.ReleaseBuffer();
	if (Result == IDOK) {
		OpenDocumentFile(FileName);
	}
}

void AeSys::OnFilePlotStyleManager() {
	OPENFILENAME OpenFileName;
	::ZeroMemory(&OpenFileName, sizeof(OPENFILENAME));
	OpenFileName.lStructSize = sizeof(OPENFILENAME);
	OpenFileName.hwndOwner = nullptr;
	OpenFileName.hInstance = theApp.GetInstance();
	OpenFileName.lpstrFilter = L"Plot Style Files\0*.ctb;*.stb\0All Files\0*.*\0\0";
	OpenFileName.lpstrFile = new wchar_t[MAX_PATH];
	*OpenFileName.lpstrFile = 0;
	OpenFileName.nMaxFile = MAX_PATH;
	OpenFileName.lpstrTitle = L"Select Plot Style File";
	OpenFileName.Flags = OFN_FILEMUSTEXIST;
	OpenFileName.lpstrDefExt = L"stb";
	if (GetOpenFileNameW(&OpenFileName) != 0) {
		const OdString FileName(OpenFileName.lpstrFile);
		delete OpenFileName.lpstrFile;
		auto SystemServices {odSystemServices()};
		try {
			if (SystemServices->accessFile(FileName, Oda::kFileRead)) {
				auto StreamBuffer(SystemServices->createFile(FileName));
				OdPsPlotStyleTablePtr PlotStyleTable;
				if (StreamBuffer.get() != nullptr) {
					OdPsPlotStyleServicesPtr PlotStyleServices {odrxDynamicLinker()->loadApp(ODPS_PLOTSTYLE_SERVICES_APPNAME)};
					if (PlotStyleServices.get() != nullptr) {
						PlotStyleTable = PlotStyleServices->loadPlotStyleTable(StreamBuffer);
					}
				}
				EoDlgPlotStyleManager PsTableEditorDlg(theApp.GetMainWnd());
				PsTableEditorDlg.SetFileBufPath(FileName);
				PsTableEditorDlg.SetPlotStyleTable(PlotStyleTable);
				if (PsTableEditorDlg.DoModal() == IDOK) {
					PlotStyleTable->copyFrom(PsTableEditorDlg.GetPlotStyleTable());
				}
			}
		} catch (...) { }
	}
}

void AeSys::OnHelpContents() {
	// <tas="HtmlHelp undefined x64 build. No testing done."/>
	// ::HtmlHelpW(AfxGetMainWnd()->GetSafeHwnd(), L"..\\AeSys\\hlp\\AeSys.chm", HH_DISPLAY_TOPIC, NULL);
}

void AeSys::ResetModes() {
	m_ModeResourceIdentifier = IDR_COMMAND_MODE;
	m_PrimaryMode = 0;
	m_CurrentMode = 0;
	auto ActiveView {AeSysView::GetActiveView()};
	if (ActiveView != nullptr) {
		auto CursorHandle {static_cast<HCURSOR>(LoadImageW(theApp.GetInstance(), MAKEINTRESOURCEW(IDR_COMMAND_MODE), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE))};
		VERIFY(CursorHandle);
		SetCursor(CursorHandle);
		SetClassLongW(ActiveView->GetSafeHwnd(), GCLP_HCURSOR, reinterpret_cast<long>(CursorHandle));
		ActiveView->ModeLineDisplay();
		ActiveView->RubberBandingDisable();
	}
	BuildModeSpecificAcceleratorTable();
}

void AeSys::OnModeAnnotate() {
	m_ModeResourceIdentifier = IDR_ANNOTATE_MODE;
	m_PrimaryMode = ID_MODE_ANNOTATE;
	LoadModeResources(ID_MODE_ANNOTATE);
}

void AeSys::OnModeCut() {
	m_ModeResourceIdentifier = IDR_CUT_MODE;
	m_PrimaryMode = ID_MODE_CUT;
	LoadModeResources(ID_MODE_CUT);
}

void AeSys::OnModeDimension() {
	m_ModeResourceIdentifier = IDR_DIMENSION_MODE;
	m_PrimaryMode = ID_MODE_DIMENSION;
	LoadModeResources(ID_MODE_DIMENSION);
}

void AeSys::OnModeDraw() {
	m_ModeResourceIdentifier = IDR_DRAW_MODE;
	m_PrimaryMode = ID_MODE_DRAW;
	LoadModeResources(ID_MODE_DRAW);
}

void AeSys::OnModeDraw2() {
	m_ModeResourceIdentifier = IDR_DRAW2_MODE;
	m_PrimaryMode = ID_MODE_DRAW2;
	LoadModeResources(ID_MODE_DRAW2);
}

void AeSys::OnModeEdit() {
	m_ModeResourceIdentifier = IDR_EDIT_MODE;
	LoadModeResources(ID_MODE_EDIT);
}

void AeSys::OnModeFixup() {
	m_ModeResourceIdentifier = IDR_FIXUP_MODE;
	LoadModeResources(ID_MODE_FIXUP);
}

void AeSys::OnModeLetter() {
	EoDlgModeLetter Dialog;
	Dialog.DoModal();
}

void AeSys::OnModeLpd() {
	m_ModeResourceIdentifier = IDR_LPD_MODE;
	LoadModeResources(ID_MODE_LPD);
}

void AeSys::OnModeNodal() {
	m_ModeResourceIdentifier = IDR_NODAL_MODE;
	LoadModeResources(ID_MODE_NODAL);
}

void AeSys::OnModePipe() {
	m_ModeResourceIdentifier = IDR_PIPE_MODE;
	LoadModeResources(ID_MODE_PIPE);
}

void AeSys::OnModePower() {
	m_ModeResourceIdentifier = IDR_POWER_MODE;
	LoadModeResources(ID_MODE_POWER);
}

void AeSys::OnModeRevise() {
	EoDlgModeRevise Dialog;
	Dialog.DoModal();
}

void AeSys::OnModeTrap() {
	if (m_TrapModeAddGroups) {
		m_ModeResourceIdentifier = IDR_TRAP_MODE;
		LoadModeResources(ID_MODE_TRAP);
	} else {
		m_ModeResourceIdentifier = IDR_TRAPR_MODE;
		LoadModeResources(ID_MODE_TRAPR);
	}
}

void AeSys::OnToolsLoadApplications() {
	EoLoadApps LoadAppsDialog;
	LoadAppsDialog.DoModal();
}

void AeSys::OnTrapCommandsAddGroups() {
	m_TrapModeAddGroups = !m_TrapModeAddGroups;
	m_CurrentMode = static_cast<unsigned>(m_TrapModeAddGroups ? ID_MODE_TRAP : ID_MODE_TRAPR);
	OnModeTrap();
}

void AeSys::OnTrapCommandsHighlight() noexcept {
	m_TrapHighlighted = !m_TrapHighlighted;
	//LPARAM Hint = m_TrapHighlighted ? kGroupsSafeTrap : kGroupsSafe;
	//UpdateGroupsInAllViews(Hint, &m_TrappedGroupList);
}

void AeSys::OnUpdateEditCfGroups(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_ClipboardDataEoGroups));
}

void AeSys::OnUpdateEditCfImage(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_ClipboardDataImage));
}

void AeSys::OnUpdateEditCfText(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_ClipboardDataText));
}

void AeSys::OnUpdateModeAnnotate(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_CurrentMode == ID_MODE_ANNOTATE));
}

void AeSys::OnUpdateModeCut(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_CurrentMode == ID_MODE_CUT));
}

void AeSys::OnUpdateModeDimension(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_CurrentMode == ID_MODE_DIMENSION));
}

void AeSys::OnUpdateModeDraw(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_CurrentMode == ID_MODE_DRAW));
}

void AeSys::OnUpdateModeDraw2(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_CurrentMode == ID_MODE_DRAW2));
}

void AeSys::OnUpdateModeEdit(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_CurrentMode == ID_MODE_EDIT));
}

void AeSys::OnUpdateModeFixup(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_CurrentMode == ID_MODE_FIXUP));
}

void AeSys::OnUpdateModeLpd(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_CurrentMode == ID_MODE_LPD));
}

void AeSys::OnUpdateModeNodal(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_CurrentMode == ID_MODE_NODAL));
}

void AeSys::OnUpdateModePipe(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_CurrentMode == ID_MODE_PIPE));
}

void AeSys::OnUpdateModePower(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_CurrentMode == ID_MODE_POWER));
}

void AeSys::OnUpdateModeTrap(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_CurrentMode == ID_MODE_TRAP));
}

void AeSys::OnUpdateTrapCommandsAddGroups(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_TrapModeAddGroups));
}

void AeSys::OnUpdateTrapCommandsHighlight(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_TrapHighlighted));
}

void AeSys::OnUpdateViewModeInformation(CCmdUI* commandUserInterface) {
	commandUserInterface->SetCheck(static_cast<int>(m_ModeInformationOverView));
}

COLORREF AppGetTextCol() noexcept {
	return ~(g_ViewBackgroundColor | 0xff000000);
}

void AeSys::OnViewModeInformation() {
	m_ModeInformationOverView = !m_ModeInformationOverView;
	AeSysDoc::GetDoc()->UpdateAllViews(nullptr);
}

double AeSys::ParseLength(const wchar_t* lengthAsString) {
	wchar_t* StopString;
	auto ReturnValue {wcstod(lengthAsString, &StopString)};
	switch (toupper(static_cast<int>(StopString[0]))) {
		case '\'': // Feet and maybe inches
			ReturnValue *= 12.0; // Reduce to inches
			ReturnValue += wcstod(&StopString[1], &StopString); // Begin scan for inches at character following foot delimiter
			break;
		case 'M': // meters or millimeters
			if (toupper(static_cast<int>(StopString[1])) == 'M') {
				ReturnValue *= 0.03937007874015748;
			} else {
				ReturnValue *= 39.37007874015748;
			}
			break;
		case 'C': // centimeters
			ReturnValue *= 0.3937007874015748;
			break;
		case 'D': // decimeters
			ReturnValue *= 3.937007874015748;
			break;
		case 'K': // kilometers
			ReturnValue *= 39370.07874015748;
		default: ;
	}
	return ReturnValue / AeSysView::GetActiveView()->WorldScale();
}

double AeSys::ParseLength(const Units units, const wchar_t* lengthAsString) {
	try {
		auto TokenIndex {0};
		long DataDefinition;
		int TokenType;
		double ReturnValue[32];
		Lex::Parse(lengthAsString);
		Lex::EvalTokenStream(&TokenIndex, &DataDefinition, &TokenType, static_cast<void*>(ReturnValue));
		if (TokenType == Lex::gc_TokenLengthOperand) {
			return ReturnValue[0];
		}
		Lex::ConvertValTyp(TokenType, Lex::gc_TokenReal, &DataDefinition, ReturnValue);
		switch (units) {
			case kArchitectural: case kArchitecturalS: case kEngineering: case kFeet:
				ReturnValue[0] *= 12.0;
				break;
			case kMeters:
				ReturnValue[0] *= 39.37007874015748;
				break;
			case kMillimeters:
				ReturnValue[0] *= 0.03937007874015748;
				break;
			case kCentimeters:
				ReturnValue[0] *= 0.3937007874015748;
				break;
			case kDecimeters:
				ReturnValue[0] *= 3.937007874015748;
				break;
			case kKilometers:
				ReturnValue[0] *= 39370.07874015748;
				break;
			case kInches: default:
				break;
		}
		ReturnValue[0] /= AeSysView::GetActiveView()->WorldScale();
		return ReturnValue[0];
	} catch (const wchar_t* Message) {
		MessageBoxW(nullptr, Message, nullptr, MB_ICONWARNING | MB_OK);
		return 0.0;
	}
}

double AeSys::PenWidthsGet(const short colorIndex) noexcept {
	return g_PenWidths[colorIndex];
}
/// <remarks> Processing occurs immediately before the framework loads the application state from the registry. </remarks>
void AeSys::PreLoadState() {
	GetContextMenuManager()->AddMenu(L"My menu", IDR_CONTEXT_MENU);

	// <tas="add other context menus here"/>
}

int AeSys::PrimaryMode() const noexcept {
	return m_PrimaryMode;
}

const OdString AeSys::product() {
	return L"AeSys Application";
}

bool GetRegistryString(const HKEY key, const wchar_t* subKey, const wchar_t* name, wchar_t* value, const int size) noexcept {
	auto ReturnValue {false};
	HKEY OpenedKey;
	if (RegOpenKeyExW(key, subKey, 0, KEY_READ, &OpenedKey) == ERROR_SUCCESS) {
		unsigned long RegistryBufferSize {gc_RegistryBufferSize};
		unsigned char Data[gc_RegistryBufferSize] {0};
		wchar_t DataW[gc_RegistryBufferSize] {L""};
		if (RegQueryValueExW(OpenedKey, name, nullptr, nullptr, Data, &RegistryBufferSize) == ERROR_SUCCESS) {
			memcpy_s(&DataW, gc_RegistryBufferSize, &Data, RegistryBufferSize);
			ReturnValue = true;
		} else {
			if (ERROR_SUCCESS == RegEnumKeyExW(OpenedKey, 0, DataW, &RegistryBufferSize, nullptr, nullptr, nullptr, nullptr)) {
				ReturnValue = true;
			}
		}
		if (size < gc_RegistryBufferSize) {
			swprintf_s(value, static_cast<size_t>(size), L"%s\0", DataW);
		} else {
			wcsncpy(value, DataW, static_cast<size_t>(size - 1));
			value[size - 1] = '\0';
		}
		RegCloseKey(OpenedKey);
	}
	return ReturnValue;
}

CString GetRegistryAcadLocation() {
	CString SubKey {L"SOFTWARE\\Autodesk\\AutoCAD"};
	wchar_t Version[32] {L""};
	if (!GetRegistryString(HKEY_LOCAL_MACHINE, SubKey, L"CurVer", Version, 32)) {
		return L"";
	}
	SubKey += L"\\";
	SubKey += Version;
	wchar_t SubVersion[32] {L""};
	if (!GetRegistryString(HKEY_LOCAL_MACHINE, SubKey, L"CurVer", SubVersion, 32)) {
		return L"";
	}
	SubKey += L"\\";
	SubKey += SubVersion;
	wchar_t SearchPaths[gc_RegistryMaxPath] {L""};
	if (!GetRegistryString(HKEY_LOCAL_MACHINE, SubKey, L"AcadLocation", SearchPaths, gc_RegistryMaxPath)) {
		return L"";
	}
	return CString(SearchPaths);
}

CString GetRegistryAcadProfilesKey() {
	CString SubKey {L"SOFTWARE\\Autodesk\\AutoCAD"};
	wchar_t Version[32] {L"\0"};
	if (!GetRegistryString(HKEY_CURRENT_USER, SubKey, L"CurVer", Version, 32)) {
		return L"";
	}
	SubKey += L"\\";
	SubKey += Version;
	wchar_t SubVersion[32] {L"\0"};
	if (!GetRegistryString(HKEY_CURRENT_USER, SubKey, L"CurVer", SubVersion, 32)) {
		return L"";
	}
	SubKey += L"\\";
	SubKey += SubVersion;
	SubKey += L"\\Profiles";
	wchar_t Profile[gc_RegistryMaxProfileName] {L"\0"};
	if (!GetRegistryString(HKEY_CURRENT_USER, SubKey, L"", Profile, gc_RegistryMaxProfileName)) {
		return L"";
	}
	SubKey += L"\\";
	SubKey += Profile;
	return SubKey;
}

OdString AeSys::getSubstituteFont(const OdString& /*fontName*/, OdFontType /*fontType*/) {
	return OdString(L"simplex.shx");
}

OdString AeSys::getFontMapFileName() const {
	wchar_t FontMapFile[gc_RegistryMaxPath] {L"\0"};
	wchar_t ExpandedPath[gc_RegistryMaxPath] {L"\0"};
	auto SubKey {GetRegistryAcadProfilesKey()};
	if (!SubKey.IsEmpty()) {
		SubKey += L"\\Editor Configuration";
		if (!GetRegistryString(HKEY_CURRENT_USER, SubKey, L"FontMappingFile", FontMapFile, gc_RegistryMaxPath)) {
			return L"";
		}
		ExpandEnvironmentStringsW(FontMapFile, ExpandedPath, gc_RegistryMaxPath);
		return OdString(ExpandedPath);
	}
	return L"C:\\acad.fmp";
}

OdString AeSys::getTempPath() const {
	wchar_t TempPath[MAX_PATH] {L"\0"};
	auto SubKey {GetRegistryAcadProfilesKey()};
	if (!SubKey.IsEmpty()) {
		SubKey += L"\\General Configuration";
		if (!GetRegistryString(HKEY_CURRENT_USER, SubKey, L"TempDirectory", TempPath, MAX_PATH)) {
			return OdDbHostAppServices::getTempPath();
		}
		if (_waccess(TempPath, 0) != 0) {
			return OdDbHostAppServices::getTempPath();
		}
		CString Result(TempPath, static_cast<int>(wcslen(TempPath)));
		if (Result.GetAt(Result.GetLength() - 1) != '\\') {
			Result += '\\';
		}
		return Result.GetString();
	}
	return OdDbHostAppServices::getTempPath();
}

void AeSys::ReleaseSimplexStrokeFont() const noexcept {
	delete[] m_SimplexStrokeFont;
}

OdString AeSys::RecentGsDevicePath() const {
	return m_VectorizerPath;
}

void AeSys::SetRecentGsDevicePath(const OdString& vectorizerPath) {
	WriteProfileStringW(L"options", L"recent GS", vectorizerPath);
	m_VectorizerPath = vectorizerPath;
}

void AeSys::SetStatusPaneTextAt(const int index, const wchar_t* newText) {
	dynamic_cast<CMainFrame*>(GetMainWnd())->SetStatusPaneTextAt(index, newText);
}

OdDbHostAppProgressMeter* AeSys::newProgressMeter() {
	if (m_ThisThreadId != GetCurrentThreadId()) {
		return nullptr;
	}
	return ExHostAppServices::newProgressMeter();
}

void AeSys::start(const OdString& displayString) {
	m_ProgressMessage = static_cast<const wchar_t*>(displayString);
	m_ProgressPosition = 0;
	m_ProgressPercent = -1;
	// <tas="m_tbExt.SetProgressState(::AfxGetMainWnd()->GetSafeHwnd(), CTaskBarWin7Ext::PS_Normal);"</tas>
	// <tas="m_tbExt.SetProgressValue(::AfxGetMainWnd()->GetSafeHwnd(), 0, 100);"</tas>
}

void AeSys::stop() {
	m_ProgressPosition = m_ProgressLimit;
	meterProgress();
	// <tas="m_tbExt.SetProgressState(::AfxGetMainWnd()->GetSafeHwnd(), CTaskBarWin7Ext::PS_NoProgress);"</tas>
	// <tas="m_tbExt.FlashWindow(::AfxGetMainWnd()->GetSafeHwnd());"</tas>
}

void AeSys::meterProgress() {
	bool UpdateProgress;
	int Percent;
	{
		TD_AUTOLOCK_P_DEF(meterMutex);
		const auto OldPercent {m_ProgressPercent};
		m_ProgressPercent = static_cast<int>(static_cast<double>(m_ProgressPosition++) / static_cast<double>(m_ProgressLimit) * 100);
		Percent = m_ProgressPercent;
		UpdateProgress = OldPercent != m_ProgressPercent;
	}
	if (UpdateProgress) {
		struct StatusUpdater {
		private:
			int m_Percent;
			CMainFrame* m_MainFrame;
			AeSys* m_Application;
		public:
			StatusUpdater(const int percent, CMainFrame* mainFrame, AeSys* application) noexcept
				: m_Percent(percent)
				, m_MainFrame(mainFrame)
				, m_Application(application) { }

			static void Exec(void* statusUpdater) {
				const auto StatusUpdaterExec {static_cast<StatusUpdater*>(statusUpdater)};
				CString ProgressMessage;
				ProgressMessage.Format(L"%s %d", StatusUpdaterExec->m_Application->m_ProgressMessage.GetString(), StatusUpdaterExec->m_Percent);
				// <tas="pExec->m_MainFrame->m_wndStatusBar.SetPaneText(0, ProgressMessage);:</tas>
				// <tas="pExec->m_Application->m_tbExt.SetProgressValue(::AfxGetMainWnd()->GetSafeHwnd(), (ULONG) pExec->m_ProgressPercent, 100);"</tas>
				MSG Message;
				while (PeekMessageW(&Message, StatusUpdaterExec->m_MainFrame->m_hWnd, WM_KEYUP, WM_KEYUP, 1) != 0) {
					auto Dup {false};
					if (Message.wParam == VK_ESCAPE && !Dup) {
						Dup = true;
						ProgressMessage.Format(L"Are you sure you want to terminate\n%s ?", StatusUpdaterExec->m_Application->m_ProgressMessage.GetString());
						// <tas="pExec->m_Application->m_tbExt.SetProgressState(::AfxGetMainWnd()->GetSafeHwnd(), CTaskBarWin7Ext::PS_Paused);"</tas>
						if (AfxMessageBox(ProgressMessage, MB_YESNO | MB_ICONQUESTION) == IDYES) {
							// <tas="pExec->m_Application->m_tbExt.SetProgressState(::AfxGetMainWnd()->GetSafeHwnd(), CTaskBarWin7Ext::PS_NoProgress);"</tas>
							throw UserBreak();
						} 
						// <tas="pExec->m_Application->m_tbExt.SetProgressState(::AfxGetMainWnd()->GetSafeHwnd(), CTaskBarWin7Ext::PS_Normal);:</tas>
					}
				}
			}
		} ExecArg(Percent, dynamic_cast<CMainFrame*>(GetMainWnd()), this);
		odExecuteMainThreadAction(StatusUpdater::Exec, &ExecArg);
	}
}

void AeSys::setLimit(const int max) noexcept {
	m_ProgressLimit = max != 0 ? max : 1;
}

int AeSys::ConfirmMessageBox(const unsigned stringResourceIdentifier, const wchar_t* string) {
	const auto FormatSpecification {LoadStringResource(stringResourceIdentifier)};
	CString FormattedResourceString;
	FormattedResourceString.Format(FormatSpecification, string);
	auto NextToken {0};
	const auto Text {FormattedResourceString.Tokenize(L"\t", NextToken)};
	const auto Caption {FormattedResourceString.Tokenize(L"\n", NextToken)};
	return MessageBoxW(nullptr, Text, Caption, MB_ICONINFORMATION | MB_YESNOCANCEL | MB_DEFBUTTON2);
}

void AeSys::ErrorMessageBox(const wchar_t* caption, const OdError& error) {
	const auto MainWindow {GetMainWnd()};
	const auto ParentWindow {MainWindow != nullptr ? MainWindow->m_hWnd : nullptr};
	MessageBoxW(ParentWindow, error.description(), caption, MB_OK | MB_ICONERROR);
}

static bool g_IgnoreWarnings = false;

void AeSys::warning(const char* warnVisGroup, const OdString& text) {
	if (m_UseMtLoading) {
		// TODO: MT: Implement correct processing of AeSys::warning() in Multithreading loading mode
		return;
	}
	if (!g_IgnoreWarnings && (warnVisGroup == nullptr || *warnVisGroup == 0)) {
		unsigned Type {MB_ICONWARNING};
#ifdef _DEBUG
		Type |= static_cast<unsigned>(MB_CANCELTRYCONTINUE);
#else
		Type |= static_cast<unsigned>(MB_YESNO);
#endif
		const auto ReturnValue {MessageBoxW(nullptr, text + L"\n\nDo you want to proceed ?", L"Warning!", Type)};
		if (ReturnValue == IDCANCEL || ReturnValue == IDNO) {
			throw UserBreak();
		}
		if (ReturnValue == IDTRYAGAIN) {
			g_IgnoreWarnings = true;
		}
	}
}

void AeSys::WarningMessageBox(const unsigned stringResourceIdentifier) {
	const auto ResourceString {LoadStringResource(stringResourceIdentifier)};
	auto NextToken {0};
	const auto Text {ResourceString.Tokenize(L"\t", NextToken)};
	const auto Caption {ResourceString.Tokenize(L"\n", NextToken)};
	MessageBoxW(nullptr, Text, Caption, MB_ICONWARNING | MB_OK);
}

void AeSys::WarningMessageBox(const unsigned stringResourceIdentifier, const wchar_t* string) {
	const auto FormatSpecification {LoadStringResource(stringResourceIdentifier)};
	CString FormattedResourceString;
	FormattedResourceString.Format(FormatSpecification, string);
	auto NextToken {0};
	const auto Text {FormattedResourceString.Tokenize(L"\t", NextToken)};
	const auto Caption {FormattedResourceString.Tokenize(L"\n", NextToken)};
	MessageBoxW(nullptr, Text, Caption, MB_ICONWARNING | MB_OK);
}

void AeSys::InitPlotStyleSheetEnv() {
	const auto StyleSheetFiles {FindConfigPath(L"PrinterStyleSheetDir")};
	_wputenv_s(L"DDPLOTSTYLEPATHS", StyleSheetFiles);
}

CString AeSys::ResourceFolderPath() {
	return GetApplicationPath() + L"\\res\\";
}

void AeSys::SetArchitecturalUnitsFractionPrecision(const int precision) noexcept {
	if (precision > 0) {
		m_ArchitecturalUnitsFractionPrecision = precision;
	}
}

void AeSys::SetDimensionAngle(const double angle) noexcept {
	m_DimensionAngle = angle;
}

void AeSys::SetDimensionLength(const double length) noexcept {
	m_DimensionLength = length;
}

void AeSys::SetEngagedAngle(const double angle) noexcept {
	m_EngagedAngle = angle;
}

void AeSys::SetEngagedLength(const double length) noexcept {
	m_EngagedLength = length;
}

int AeSys::SetShadowFolderPath(const CString& folder) {
	wchar_t Path[MAX_PATH];
	if (SHGetSpecialFolderPathW(m_pMainWnd->GetSafeHwnd(), Path, CSIDL_PERSONAL, TRUE) != 0) {
		m_ShadowFolderPath = Path;
	} else {
		m_ShadowFolderPath.Empty();
	}
	m_ShadowFolderPath += L"\\" + folder + L"\\";
	return _wmkdir(m_ShadowFolderPath);
}

void AeSys::SetUnits(const Units units) noexcept {
	m_Units = units;
}

CString AeSys::ShadowFolderPath() const noexcept {
	return m_ShadowFolderPath;
}

char* AeSys::SimplexStrokeFont() const noexcept {
	return m_SimplexStrokeFont;
}

short AeSys::TrapHighlightColor() const noexcept {
	return m_TrapHighlightColor;
}

void AeSys::UninitializeTeigha() {
	OdApplicationReactor::rxUninit();
	OdApplicationDocument::rxUninit();
	EoLoadApps::rxUninit();
	try {
		/* <tas>
		rxUninitMaterialsEditorObjects();
		   </tas> */
		auto CommandStack {odedRegCmds()};
		CommandStack->removeCmd(&g_CommandSelect);
		CommandStack->removeCmd(&g_CommandView);
		OdDbDatabaseDoc::rxUninit();
#ifdef OD_OLE_SUPPORT
		::rxUninit_COleClientItem_handler();
#endif // OD_OLE_SUPPORT
		RemovePaperDrawingCustomization();
		RemoveMaterialTextureLoadingMonitor();
		odUninitialize();
	} catch (const OdError& Error) {
		theApp.ErrorMessageBox(L"", Error);
	}
}

void AeSys::UpdateMdiTabs(const BOOL resetMdiChild) {
	dynamic_cast<CMainFrame*>(AfxGetMainWnd())->UpdateMdiTabs(resetMdiChild);
}

BOOL AeSys::OnIdle(const long count) {
	for (auto& ApplicationReactor : applicationReactors) {
		ApplicationReactor->OnIdle(count);
	}
	return __super::OnIdle(count);
}

BOOL AeSys::PreTranslateMessage(MSG* message) {
	for (auto& ApplicationReactor : applicationReactors) {
		ApplicationReactor->OnPreTranslateMessage(message);
	}
	return __super::PreTranslateMessage(message);
}

/// <section="vectorizer menu - add new and clear all">
bool AddGsMenuItem(CMenu* vectorizePopupMenu, unsigned long& numberOfVectorizers, const wchar_t* vectorizerPath) {
	if (ID_VECTORIZER_FIRST + numberOfVectorizers <= ID_VECTORIZER_LAST) {
		vectorizePopupMenu->InsertMenuW(numberOfVectorizers, MF_BYPOSITION, ID_VECTORIZER_FIRST + numberOfVectorizers, vectorizerPath);
		MENUITEMINFO MenuItemInfo;
		MenuItemInfo.cbSize = sizeof MenuItemInfo;
		MenuItemInfo.fMask = MIIM_DATA;
		MenuItemInfo.dwItemData = static_cast<unsigned long>(theApp.GetGsMenuItemMarker());
		VERIFY(::SetMenuItemInfoW(vectorizePopupMenu->m_hMenu, numberOfVectorizers, TRUE, &MenuItemInfo));
		if (theApp.RecentGsDevicePath().iCompare(OdString(vectorizerPath)) == 0) {
			vectorizePopupMenu->CheckMenuItem(numberOfVectorizers, MF_BYPOSITION | MF_CHECKED);
		}
		++numberOfVectorizers;
		return true;
	}
	return false;
}

void AeSys::OnVectorizerTypeAddVectorizerDll() {
	const unsigned long Flags {OFN_HIDEREADONLY | OFN_EXPLORER | OFN_PATHMUSTEXIST};
	CString Filter {L"Graphic System DLL (*." VECTORIZATION_MODULE_EXTENSION_W L")|*." VECTORIZATION_MODULE_EXTENSION_W L"|Windows DLL (*.dll)|*.dll||"};
	CFileDialog FileDialog(TRUE, VECTORIZATION_MODULE_EXTENSION_W, L"", Flags, Filter, AfxGetMainWnd());
	FileDialog.m_ofn.lpstrTitle = L"Select Graphic System DLL";
	auto ApplicationPath {GetApplicationPath()};
	FileDialog.m_ofn.lpstrInitialDir = ApplicationPath.GetBuffer(ApplicationPath.GetLength());
	if (FileDialog.DoModal() == IDOK) {
		m_VectorizerPath = static_cast<const wchar_t*>(FileDialog.GetFileName());
		m_VectorizerPath.replace(TD_DLL_VERSION_SUFFIX_STR, L"");
		const auto TopMenu {CMenu::FromHandle(theApp.GetAeSysMenu())};
		auto VectorizePopupMenu {TopMenu->GetSubMenu(3)};
		AddGsMenuItem(VectorizePopupMenu, m_NumGsMenuItems, m_VectorizerPath);
		WriteProfileStringW(L"options\\vectorizers", m_VectorizerPath, L"");
		GetMainWnd()->SendMessageW(WM_COMMAND, ID_VECTORIZERTYPE);
	}
	ApplicationPath.ReleaseBuffer();
}

void AeSys::OnUpdateVectorizerTypeAddVectorizerDll(CCmdUI* /*commandUserInterface*/) {
	if (m_NumGsMenuItems == 0) {
		const auto TopMenu {CMenu::FromHandle(theApp.GetAeSysMenu())};
		const auto VectorizePopupMenu {TopMenu->GetSubMenu(3)};
		CRegKey RegistryKey;
		RegistryKey.Create(HKEY_CURRENT_USER, L"Software\\Engineers Office\\AeSys\\options\\vectorizers");
		CString Path;
		unsigned long PathSize;
		for (;;) {
			PathSize = _MAX_FNAME + _MAX_EXT;
			const auto Status {RegEnumValueW(RegistryKey, m_NumGsMenuItems, Path.GetBuffer(static_cast<int>(PathSize)), &PathSize, nullptr, nullptr, nullptr, nullptr)};
			Path.ReleaseBuffer();
			if (Status == ERROR_SUCCESS) {
				if (!AddGsMenuItem(VectorizePopupMenu, m_NumGsMenuItems, Path)) {
					break;
				}
			} else {
				break;
			}
		}
	}
}

void AeSys::OnVectorizerTypeClearMenu() {
	const auto TopMenu {CMenu::FromHandle(theApp.GetAeSysMenu())};
	auto VectorizePopupMenu {TopMenu->GetSubMenu(3)};
	while (VectorizePopupMenu->GetMenuItemCount() > 3) {
		VectorizePopupMenu->RemoveMenu(0, MF_BYPOSITION);
	}
	CRegKey RegistryKey;
	RegistryKey.Create(HKEY_CURRENT_USER, L"Software\\Engineers Office\\AeSys\\options");
	RegistryKey.RecurseDeleteKey(L"vectorizers");
	SetRecentGsDevicePath(L"");
	m_NumGsMenuItems = 0;
}

void AeSys::OnUpdateVectorizerTypeClearMenu(CCmdUI* commandUserInterface) {
	commandUserInterface->Enable(static_cast<BOOL>(m_NumGsMenuItems > 0));
}
/// </section>
