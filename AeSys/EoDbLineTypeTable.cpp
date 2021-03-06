#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include <DbLinetypeTable.h>
#include <DbLinetypeTableRecord.h>
#include "EoDbLinetypeTable.h"
const wchar_t* EoDbLinetypeTable::m_LegacyLinetypes[] = {
	L"0",
	L"Continuous",
	L"2",
	L"3",
	L"4",
	L"5",
	L"6",
	L"7",
	L"8",
	L"9",
	L"10",
	L"11",
	L"12",
	L"13",
	L"14",
	L"15",
	L"16",
	L"17",
	L"BORDER",
	L"BORDER2",
	L"BORDERX2",
	L"CENTER",
	L"CENTER2",
	L"CENTERX2",
	L"DASHDOT",
	L"DASHDOT2",
	L"DASHDOTX2",
	L"DASHED",
	L"DASHED2",
	L"DASHEDX2",
	L"DIVIDE",
	L"DIVIDE2",
	L"DIVIDEX2",
	L"DOT",
	L"DOT2",
	L"DOTX2",
	L"HIDDEN",
	L"HIDDEN2",
	L"HIDDENX2",
	L"PHANTOM",
	L"PHANTOM2",
	L"PHANTOMX2"
};
/// <remarks>
/// "ByBlock" and "ByLayer" is not be permitted in all legacy file versions. This should be managed in the outbound conversions back to legacy file.
/// </remarks>
unsigned short EoDbLinetypeTable::LegacyLinetypeIndex(const OdString& name) {
	unsigned short Index = 0;
	if (name.iCompare(L"ByBlock") == 0) {
		Index = EoDbPrimitive::mc_LinetypeByblock;
	} else if (name.iCompare(L"ByLayer") == 0) {
		Index = EoDbPrimitive::mc_LinetypeBylayer;
	} else {
		while (Index < c_NumberOfLegacyLinetypes && name.iCompare(m_LegacyLinetypes[Index]) != 0) {
			Index++;
		}
		Index = Index < c_NumberOfLegacyLinetypes ? Index : 0U;
	}
	return Index;
}

OdString EoDbLinetypeTable::LegacyLinetypeName(const int index) {
	const auto Index {index < c_NumberOfLegacyLinetypes ? index : 1};
	return m_LegacyLinetypes[Index];
}

void EoDbLinetypeTable::LoadLinetypesFromTxtFile(OdDbDatabasePtr database, const CString& fileName) {
	OdDbLinetypeTablePtr Linetypes {database->getLinetypeTableId().safeOpenObject(OdDb::kForWrite)};
	CStdioFile StreamFile;
	if (StreamFile.Open(fileName, CFile::modeRead | CFile::typeText) != 0) {
		unsigned short MaxNumberOfDashes {12};
		auto DashLengths {new double[MaxNumberOfDashes]};
		CString Line;
		while (StreamFile.ReadString(Line) != 0) {
			auto NextToken {0};
			_wtoi(Line.Tokenize(L"=", NextToken)); // Label is unused
			OdString Name {Line.Tokenize(L",", NextToken).GetString()};
			OdString Comments {Line.Tokenize(L"\n", NextToken).GetString()};
			StreamFile.ReadString(Line);
			NextToken = 0;
			const auto NumberOfDashes {static_cast<unsigned short>(_wtoi(Line.Tokenize(L",\n", NextToken)))};
			if (NumberOfDashes > MaxNumberOfDashes) {
				delete [] DashLengths;
				DashLengths = new double[NumberOfDashes];
				MaxNumberOfDashes = NumberOfDashes;
			}
			auto PatternLength {0.0};
			for (unsigned DashIndex = 0; DashIndex < NumberOfDashes; DashIndex++) {
				DashLengths[DashIndex] = _wtof(Line.Tokenize(L",\n", NextToken));
				PatternLength += DashLengths[DashIndex];
			}
			if (Linetypes->getAt(Name).isNull()) {
				auto Linetype {OdDbLinetypeTableRecord::createObject()};
				Linetype->setName(Name);
				Linetype->setComments(Comments);
				Linetype->setNumDashes(NumberOfDashes);
				Linetype->setPatternLength(PatternLength);
				for (auto DashIndex = 0; DashIndex < NumberOfDashes; DashIndex++) {
					Linetype->setDashLengthAt(DashIndex, DashLengths[DashIndex]);
					Linetype->setShapeStyleAt(DashIndex, OdDbObjectId::kNull);
					Linetype->setShapeNumberAt(DashIndex, 0);
					Linetype->setTextAt(DashIndex, L" ");
					Linetype->setShapeScaleAt(DashIndex, 1.0);
					Linetype->setShapeOffsetAt(DashIndex, OdGeVector2d(0.0, 0.0));
					Linetype->setShapeRotationAt(DashIndex, 0.0);
					Linetype->setShapeIsUcsOrientedAt(DashIndex, false);
				}
				Linetypes->add(Linetype);
			}
		}
		delete [] DashLengths;
	}
}
