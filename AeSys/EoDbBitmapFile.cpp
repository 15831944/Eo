#include "stdafx.h"

#include "EoDbBitmapFile.h"

EoDbBitmapFile::EoDbBitmapFile(const CString& fileName) {
	CFileException e;
	if (CFile::Open(fileName, modeRead | shareDenyNone, &e)) {
	}
}

bool EoDbBitmapFile::Load(const CString& fileName, CBitmap& bmReference, CPalette& palReference) {
	auto Bitmap {static_cast<HBITMAP>(::LoadImageW(nullptr, fileName, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE))};
	
	if (Bitmap == nullptr) { return false; }
	
	bmReference.Attach(Bitmap);


	CClientDC ClientDeviceContext(nullptr);
	
	// Return now if device does not support palettes
	if ((ClientDeviceContext.GetDeviceCaps(RASTERCAPS) & RC_PALETTE) == 0) { return true; }

	DIBSECTION ds;
	bmReference.GetObjectW(sizeof(DIBSECTION), &ds);

	int NumberOfColors {0};

	if (ds.dsBmih.biClrUsed != 0) {
		NumberOfColors = static_cast<int>(ds.dsBmih.biClrUsed);
	} else {
		NumberOfColors = 1 << ds.dsBmih.biBitCount;
	}
	if (NumberOfColors > 256) { // Create a halftone palette
		palReference.CreateHalftonePalette(&ClientDeviceContext);
	} else { // Create a custom palette from the DIB section's color table
		auto RGBQuad {new RGBQUAD[static_cast<unsigned>(NumberOfColors)]};

		CDC dcMem;
		dcMem.CreateCompatibleDC(&ClientDeviceContext);

		auto Bitmap {dcMem.SelectObject(&bmReference)};
		::GetDIBColorTable((HDC) dcMem, 0, static_cast<unsigned>(NumberOfColors), RGBQuad);
		dcMem.SelectObject(Bitmap);

		const unsigned nSize {sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * (NumberOfColors - 1))};

		LOGPALETTE* pLogPal {(LOGPALETTE*) new unsigned char[nSize]};

		pLogPal->palVersion = 0x300;
		pLogPal->palNumEntries = unsigned short(NumberOfColors);

		for (int i = 0; i < NumberOfColors; i++) {
			pLogPal->palPalEntry[i].peRed = RGBQuad[i].rgbRed;
			pLogPal->palPalEntry[i].peGreen = RGBQuad[i].rgbGreen;
			pLogPal->palPalEntry[i].peBlue = RGBQuad[i].rgbBlue;
			pLogPal->palPalEntry[i].peFlags = 0;
		}
		palReference.CreatePalette(pLogPal);

		delete [] pLogPal;
		delete [] RGBQuad;
	}
	Close();

	return true;
}
