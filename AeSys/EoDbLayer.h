#pragma once

class EoDbLayer : public EoDbGroupList {
private:
	OdDbLayerTableRecordPtr m_Layer;
	EoUInt16 m_StateFlags;	
	EoUInt16 m_TracingFlags;

public:
	enum StateFlags {
		kIsResident = 0x0001,	// entry in table list is saved
		kIsInternal = 0x0002,	// group list saved within drawing
		kIsCurrent = 0x0004,	// may have groups added (0 or 1), displayed using hot color set
		kIsActive = 0x0008,		// may have groups modified (0 or more), displayed using warm color set
		kIsLocked = 0x0010,		// tracing which is viewed or layer which is static (no additions or modifications), displayed using warm color set
		kIsOff = 0x0020
	};
	EoDbLayer(OdDbLayerTableRecordPtr layer);
	EoDbLayer(const OdString& name, EoUInt16 flags);

	~EoDbLayer() {}
	COLORREF Color() const;
	EoInt16 ColorIndex() const;
	void Display(AeSysView* view, CDC* deviceContext);
	void Display_(AeSysView* view, CDC* deviceContext, bool identifyTrap);
	bool IsActive() const;
	bool IsCurrent() const;
	bool IsInternal() const;
	bool IsLocked() const;
	bool IsOff() const;
	bool IsResident() const;
	EoInt16 LinetypeIndex();
	OdString LinetypeName();
	void MakeActive();
	void MakeCurrent();
	void MakeInternal(bool isInternal);
	void MakeResident(bool isResident);
	OdString Name() const;
	void PenTranslation(EoUInt16, EoInt16*, EoInt16*);
	void SetColorIndex(EoInt16 colorIndex);
	void SetIsFrozen(bool isFrozen);
	void SetIsLocked(bool isLocked);
	void SetIsOff(bool isOff);
	void SetLinetype(OdDbObjectId linetype);
	void SetName(const OdString& name);
	void SetStateFlags(EoUInt16 flags);
	void SetTransparency(const OdCmTransparency& transparency);
	EoUInt16 StateFlags() const;
	OdDbLayerTableRecordPtr TableRecord() const;
};

typedef CTypedPtrArray<CObArray, EoDbLayer*> EoDbLayerTable;
