#pragma once

class EoDbDwgToPegFile {
	OdDbDatabasePtr m_DatabasePtr_;

public:
	EoDbDwgToPegFile(OdDbDatabasePtr database);

	~EoDbDwgToPegFile();
	void ConvertToPeg(AeSysDoc* document);

	void ConvertHeaderSection(AeSysDoc* document);
	void ConvertViewportTable(AeSysDoc* document);
	void ConvertLayerTable(AeSysDoc* document);

	/// <summary>
	/// Load all block containers, local or external. An external reference contains the name and the filename of the external drawing.
	/// Local blocks containers are an unordered list of drawing entities. 
	/// The two type of local block containers are layout and non-layout.
	/// </summary>
	void ConvertBlockTable(AeSysDoc* document);
	void ConvertBlocks(AeSysDoc* document);
	void ConvertEntities(AeSysDoc* document);
	void ConvertBlock(OdDbBlockTableRecordPtr block, AeSysDoc* document);
};
