#include "stdafx.h"
#include "AeSysApp.h"
#include "AeSysDoc.h"

#include "EoDbLinetypeTable.h"

const wchar_t* EoDbLinetypeTable::LegacyLinetypes[] = {
	L"0", L"Continuous",
	L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9",
	L"10", L"11", L"12", L"13", L"14", L"15", L"16", L"17",
	L"BORDER", L"BORDER2", L"BORDERX2", L"CENTER", L"CENTER2", L"CENTERX2", L"DASHDOT", L"DASHDOT2",
	L"DASHDOTX2", L"DASHED", L"DASHED2", L"DASHEDX2", L"DIVIDE", L"DIVIDE2", L"DIVIDEX2", L"DOT",
	L"DOT2", L"DOTX2", L"HIDDEN", L"HIDDEN2", L"HIDDENX2", L"PHANTOM", L"PHANTOM2", L"PHANTOMX2"
};
/// <remarks>
/// "ByBlock" and "ByLayer" is not be permitted in all legacy file versions. This should be managed in the outbound conversions back to legacy file.
/// </remarks>
EoUInt16 EoDbLinetypeTable::LegacyLinetypeIndex(const OdString& name) {
	EoUInt16 Index = 0;
	if (name.iCompare(L"ByBlock") == 0) {
		Index = EoDbPrimitive::LINETYPE_BYBLOCK;
	}
	else if (name.iCompare(L"ByLayer") == 0) {
		Index = EoDbPrimitive::LINETYPE_BYLAYER;
	}
	else {
		while (Index < NumberOfLegacyLinetypes && name.iCompare(LegacyLinetypes[Index]) != 0) {
			Index++;
		}
		Index = (Index < NumberOfLegacyLinetypes) ? Index : 0;
	}
	return Index;
}
OdString EoDbLinetypeTable::LegacyLinetypeName(const int index) {
	int Index = (index < NumberOfLegacyLinetypes) ? index : 1;
	return LegacyLinetypes[Index];
}
void EoDbLinetypeTable::LoadLinetypesFromTxtFile(OdDbDatabasePtr database, const CString& fileName) {
	OdDbLinetypeTablePtr Linetypes = database->getLinetypeTableId().safeOpenObject(OdDb::kForWrite);

	CStdioFile fl;
	if (fl.Open(fileName, CFile::modeRead | CFile::typeText)) {

		EoUInt16 MaxNumberOfDashes = 12;
		double* DashLengths = new double[MaxNumberOfDashes];
		
		CString Line;

		while (fl.ReadString(Line) != 0) {
			int NextToken = 0;
			/*EoUInt16 Label =*/ EoUInt16(_wtoi(Line.Tokenize(L"=", NextToken)));

			OdString Name = Line.Tokenize(L",", NextToken);
			OdString Comments = Line.Tokenize(L"\n", NextToken);

			fl.ReadString(Line);

			NextToken = 0;
			EoUInt16 NumberOfDashes = EoUInt16(_wtoi(Line.Tokenize(L",\n", NextToken)));

			if (NumberOfDashes > MaxNumberOfDashes) {
				delete [] DashLengths;
				DashLengths = new double[NumberOfDashes];
				MaxNumberOfDashes = NumberOfDashes;
			}
			double PatternLength = 0.;
			for (EoUInt16 DashIndex = 0; DashIndex < NumberOfDashes; DashIndex++) {
				DashLengths[DashIndex] = _wtof(Line.Tokenize(L",\n", NextToken));
				PatternLength += DashLengths[DashIndex];
			}
			if (Linetypes->getAt(Name).isNull()) {
				OdDbLinetypeTableRecordPtr Linetype = OdDbLinetypeTableRecord::createObject();
				Linetype->setName(Name);
				Linetype->setComments(Comments);
				Linetype->setNumDashes(NumberOfDashes);
				Linetype->setPatternLength(PatternLength);
		
				for (int DashIndex = 0; DashIndex < NumberOfDashes; DashIndex++) {
					Linetype->setDashLengthAt(DashIndex, DashLengths[DashIndex]);
					Linetype->setShapeStyleAt(DashIndex, OdDbObjectId::kNull);
					Linetype->setShapeNumberAt(DashIndex, 0);
					Linetype->setTextAt(DashIndex, L" ");
					Linetype->setShapeScaleAt(DashIndex, 1.);
					Linetype->setShapeOffsetAt(DashIndex, OdGeVector2d(0., 0.));
					Linetype->setShapeRotationAt(DashIndex, 0.);
					Linetype->setShapeIsUcsOrientedAt(DashIndex, false);
				}
				Linetypes->add(Linetype);
			}
		}
		delete [] DashLengths;
	}
}
