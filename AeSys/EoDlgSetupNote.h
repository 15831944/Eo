#pragma once
#include "EoDbFontDefinition.h"

class EoDlgSetupNote final : public CDialog {
DECLARE_DYNAMIC(EoDlgSetupNote)

	EoDlgSetupNote(CWnd* parent = nullptr);

	EoDlgSetupNote(EoDbFontDefinition* fontDefinition, CWnd* parent = nullptr);

	enum { IDD = IDD_SETUP_NOTE };

protected:
	void DoDataExchange(CDataExchange* dataExchange) final;

	BOOL OnInitDialog() final;

	void OnOK() final;

public:
	EoDbFontDefinition* fontDefinition {nullptr};
	CMFCFontComboBox mfcFontComboControl;
	double height {0.0};
	double widthFactor {0.0};
	double obliqueAngle {0.0};
	double rotationAngle {0.0};
DECLARE_MESSAGE_MAP()
};
