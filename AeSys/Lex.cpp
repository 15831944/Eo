#include "stdafx.h"
#include "AeSys.h"
#include "Lex.h"
using namespace Lex;

namespace Lex {
	int g_TokenTypes[gc_MaximumNumberOfTokens];
	int g_Tokens;
	int g_NumberOfValues;
	int g_LocationOfValue[gc_MaximumNumberOfTokens];
	long g_Values[gc_ValuesMax];
}

void Lex::BreakExpression(int& firstTokenLocation, int& numberOfTokens, int* typeOfTokens, int* locationOfTokens) {
	auto NumberOfOpenParentheses {0};
	auto PreviousTokenType {0};
	int OperatorStack[32] {0};
	auto TopOfOperatorStack {1};
	OperatorStack[TopOfOperatorStack] = gc_TokenIdentifier;
	numberOfTokens = 0;
	auto CurrentTokenType {TokenType(firstTokenLocation)};
	while (CurrentTokenType != - 1) {
		switch (g_TokenTable[CurrentTokenType].Class) {
			case kConstant:
				typeOfTokens[numberOfTokens] = CurrentTokenType;
				locationOfTokens[numberOfTokens++] = firstTokenLocation;
				break;
			case kOpenParenthesis:
				OperatorStack[++TopOfOperatorStack] = CurrentTokenType;	// Push to operator stack
				NumberOfOpenParentheses++;
				break;
			case kCloseParenthesis:
				if (NumberOfOpenParentheses == 0) {
					break;
				}
				while (OperatorStack[TopOfOperatorStack] != gc_TokenLparen) { // Move operator to token stack
					typeOfTokens[numberOfTokens++] = OperatorStack[TopOfOperatorStack--];
				}
				TopOfOperatorStack--;		// Discard open parentheses
				NumberOfOpenParentheses--; 	// One less open parentheses
				break;
			case kBinaryArithmeticOp: case kOther:
				if (CurrentTokenType == gc_TokenBinaryPlus || CurrentTokenType == gc_TokenBinaryMinus) {
					const auto eClassPrv {g_TokenTable[PreviousTokenType].Class};
					if (eClassPrv != kConstant && eClassPrv != kIdentifier && eClassPrv != kCloseParenthesis) {
						CurrentTokenType = CurrentTokenType == gc_TokenBinaryPlus ? gc_TokenUnaryPlus : gc_TokenUnaryMinus;
					}
				}
				// Pop higher priority operators from stack
				while (g_TokenTable[OperatorStack[TopOfOperatorStack]].inStackPriority >= g_TokenTable[CurrentTokenType].inComingPriority) {
					typeOfTokens[numberOfTokens++] = OperatorStack[TopOfOperatorStack--];
				}
				// Push new operator onto stack
				OperatorStack[++TopOfOperatorStack] = CurrentTokenType;
				break;

				// TODO .. classes of tokens which might be implemented
			case kIdentifier: case kBinaryRelationalOp: case kBinaryLogicalOp: case kUnaryLogicalOp: case kAssignmentOp: default:
				break;
		}
		PreviousTokenType = CurrentTokenType;
		CurrentTokenType = TokenType(++firstTokenLocation);
	}
	if (NumberOfOpenParentheses > 0) {
		throw L"Unbalanced parentheses";
	}
	while (TopOfOperatorStack > 1) {
		typeOfTokens[numberOfTokens++] = OperatorStack[TopOfOperatorStack--];
	}
	if (numberOfTokens == 0) {
		throw L"Syntax error";
	}
}

void Lex::ConvertValToString(wchar_t* acVal, LexColumnDefinition* columnDefinition, wchar_t* acPic, int* aiLen) noexcept {
	const auto DataType {columnDefinition->dataType};
	const int DataDimension {LOWORD(columnDefinition->dataDefinition)};
	if (DataType == gc_TokenString) {
		*aiLen = DataDimension;
		acPic[0] = '\'';
		memmove(&acPic[1], acVal, static_cast<unsigned>(*aiLen));
		acPic[++*aiLen] = '\'';
		acPic[++*aiLen] = '\0';
	} else {
		wchar_t CharacterValue[32] {L"\0"};
		const auto LongValue {reinterpret_cast<long*>(CharacterValue)};
		const auto DoubleValue {reinterpret_cast<double*>(CharacterValue)};
		auto ValueLength {0};
		auto ValueIndex {0};
		auto LineLocation {0};
		int DataLength {HIWORD(columnDefinition->dataDefinition)};
		if (DataType != gc_TokenInteger) {
			DataLength = DataLength / 2;
		}
		if (DataDimension != DataLength) { // Matrix
			acPic[0] = '[';
			LineLocation++;
		}
		for (auto i1 = 0; i1 < DataLength; i1++) {
			LineLocation++;
			if (DataLength != 1 && i1 % DataDimension == 0) {
				acPic[LineLocation++] = '[';
			}
			if (DataType == gc_TokenInteger) {
				memcpy(LongValue, &acVal[ValueIndex], 4);
				ValueIndex += 4;
				_ltow(*LongValue, &acPic[LineLocation], 10);
				ValueLength = static_cast<int>(wcslen(&acPic[LineLocation]));
				LineLocation += ValueLength;
			} else {
				memcpy(DoubleValue, &acVal[ValueIndex], 8);
				ValueIndex += 8;
				if (DataType == gc_TokenReal) {
					// pCvtDoubToFltDecTxt(*dVal, 7, iLoc, cVal);
					wchar_t* NextToken {nullptr};
					auto szpVal {wcstok_s(CharacterValue, L" ", &NextToken)};
					wcscpy(&acPic[LineLocation], szpVal);
					LineLocation += static_cast<int>(wcslen(szpVal));
				} else if (DataType == gc_TokenLengthOperand) {
					//TODO: Length to length string
					LineLocation += ValueLength;
				} else if (DataType == gc_TokenAreaOperand) {
					// pCvtWrldToUsrAreaStr(*dVal, &acPic[iLnLoc], iVLen);
					LineLocation += ValueLength;
				}
			}
			if (DataLength != 1 && i1 % DataDimension == DataDimension - 1) {
				acPic[LineLocation++] = ']';
			}
		}
		if (DataDimension == DataLength) {
			*aiLen = LineLocation - 1;
		} else {
			acPic[LineLocation] = ']';
			*aiLen = LineLocation;
		}
	}
}

void Lex::ConvertValTyp(const int valueType, const int requiredType, long* definition, void* apVal) noexcept {
	if (valueType == requiredType) {
		return;
	}
	const auto pdVal {static_cast<double*>(apVal)};
	const auto piVal {static_cast<long*>(apVal)};
	if (valueType == gc_TokenString) {
		wchar_t szVal[256];
		wcscpy_s(szVal, 256, static_cast<wchar_t*>(apVal));
		if (requiredType == gc_TokenInteger) {
			piVal[0] = _wtoi(szVal);
			*definition = MAKELONG(1, 1);
		} else { // real
			pdVal[0] = _wtof(szVal);
			*definition = MAKELONG(1, 2);
		}
	} else if (valueType == gc_TokenInteger) {
		if (requiredType == gc_TokenString) {
			// <tas="integer to string not implemented"/>
		} else {
			pdVal[0] = static_cast<double>(piVal[0]);
			*definition = MAKELONG(1, 2);
		}
	} else { // real
		if (requiredType == gc_TokenString) { } else if (requiredType == gc_TokenInteger) { }
	}
}

void Lex::ConvertStringToVal(const int valueType, const long definition, wchar_t* szVal, long* lDefReq, void* p) {
	if (LOWORD(definition) <= 0) {
		throw L"Empty string";
	}
	wchar_t szTok[64];
	auto iNxt {0};
	const auto iTyp {Scan(szTok, szVal, iNxt)};
	if (valueType == gc_TokenInteger) { // Conversion to integer
		const auto pVal {static_cast<long*>(p)};
		if (iTyp == gc_TokenInteger) {
			*pVal = _wtol(szTok);
		} else if (iTyp == gc_TokenReal) {
			*pVal = static_cast<long>(_wtof(szTok));
		} else {
			throw L"String format conversion error";
		}
		*lDefReq = MAKELONG(1, 1);
	} else {
		const auto pVal {static_cast<double*>(p)};
		if (iTyp == gc_TokenInteger) {
			*pVal = static_cast<double>(_wtoi(szTok));
		} else if (iTyp == gc_TokenReal) {
			*pVal = _wtof(szTok);
		} else {
			throw L"String format conversion error";
		}
		*lDefReq = MAKELONG(1, 2);
	}
}

void Lex::EvalTokenStream(int* aiTokId, long* definition, int* valueType, void* apOp) {
	wchar_t szTok[256] {L"\0"};
	auto lDef1 = MAKELONG(1, 1);
	int iDim1;
	auto iTyp1 {gc_TokenInteger};
	long lDef2 {0};
	int NumberOfTokens;
	int TypeOfTokens[32];
	int LocationOfTokens[32];
	BreakExpression(*aiTokId, NumberOfTokens, TypeOfTokens, LocationOfTokens);
	int iOpStkTyp[32] {0};
	long lOpStk[32][32] {{0}};
	long lOpStkDef[32] {0};
	const auto cOp1 {static_cast<wchar_t*>(apOp)};
	const auto dOp1 {static_cast<double*>(apOp)};
	const auto lOp1 {static_cast<long*>(apOp)};
	wchar_t cOp2[256] {L"\0"};
	const double* dOp2 {reinterpret_cast<double*>(cOp2)};
	const auto lOp2 {reinterpret_cast<long*>(cOp2)};
	auto OperandStackTop {0};
	auto TokenStackIndex {0}; // Start with first token
	while (TokenStackIndex < NumberOfTokens) {
		const auto iTokTyp {TypeOfTokens[TokenStackIndex]};
		const auto iTokLoc {LocationOfTokens[TokenStackIndex]};
		if (g_TokenTable[iTokTyp].Class == kIdentifier) { // symbol table stuff if desired
			throw L"Identifier token class not implemented";
		}
		if (g_TokenTable[iTokTyp].Class == kConstant) {
			iTyp1 = iTokTyp;
			lDef1 = g_Values[g_LocationOfValue[iTokLoc]];
			memcpy(cOp1, &g_Values[g_LocationOfValue[iTokLoc] + 1], static_cast<unsigned>(HIWORD(lDef1) * 4));
		} else { // Token is an operator .. Pop an operand from operand stack
			if (OperandStackTop == 0) {
				throw L"Operand stack is empty";
			}
			iTyp1 = iOpStkTyp[OperandStackTop];
			lDef1 = lOpStkDef[OperandStackTop];
			int iLen1 {HIWORD(lDef1)};
			memcpy(cOp1, &lOpStk[OperandStackTop--][0], static_cast<unsigned>(iLen1 * 4));
			if (g_TokenTable[iTokTyp].Class == kOther) { // intrinsics and oddball unary minus/plus
				if (iTyp1 == gc_TokenString) {
					iDim1 = LOWORD(lDef1);
					wcscpy_s(szTok, 256, cOp1);
					if (iTokTyp == gc_TokenToInteger) {
						iTyp1 = gc_TokenInteger;
						ConvertStringToVal(gc_TokenInteger, lDef1, szTok, &lDef1, cOp1);
					} else if (iTokTyp == gc_TokenReal) {
						iTyp1 = gc_TokenReal;
						ConvertStringToVal(gc_TokenReal, lDef1, szTok, &lDef1, cOp1);
					} else if (iTokTyp == gc_TokenString) { } else {
						throw L"String operand conversions error: unknown";
					}
				} else if (iTyp1 == gc_TokenInteger) {
					UnaryOp(iTokTyp, &iTyp1, &lDef1, lOp1);
				} else {
					UnaryOp(iTokTyp, &iTyp1, &lDef1, dOp1);
				}
			} else if (g_TokenTable[iTokTyp].Class == kBinaryArithmeticOp) { // Binary arithmetic operator
				if (OperandStackTop == 0) {
					throw L"Binary Arithmetic: Only one operand.";
				}
				auto iTyp2 {iOpStkTyp[OperandStackTop]}; // Pop second operand from operand stack
				lDef2 = lOpStkDef[OperandStackTop];
				int iLen2 {HIWORD(lDef2)};
				memcpy(cOp2, &lOpStk[OperandStackTop--][0], static_cast<unsigned>(iLen2 * 4));
				auto iTyp {EoMin(iTyp2, gc_TokenReal)};
				if (iTyp1 < iTyp) { // Convert first operand
					ConvertValTyp(iTyp1, iTyp, &lDef1, lOp1);
					iTyp1 = iTyp;
					iLen1 = HIWORD(lDef1);
				} else {
					iTyp = EoMin(iTyp1, gc_TokenReal);
					if (iTyp2 < iTyp) { // Convert second operand
						ConvertValTyp(iTyp2, iTyp, &lDef2, lOp2);
						iTyp2 = iTyp;
						iLen2 = HIWORD(lDef2);
					}
				}
				if (iTokTyp == gc_TokenBinaryPlus) {
					if (iTyp1 == gc_TokenString) {
						iDim1 = LOWORD(lDef1);
						const int iDim2 {LOWORD(lDef2)};
						const auto iDim {iDim2 + iDim1};
						wcscpy(cOp1, _tcscat(cOp2, cOp1));
						iLen1 = 1 + (iDim - 1) / 4;
						lDef1 = MAKELONG(iDim, iLen1);
					} else {
						if (iTyp1 == gc_TokenInteger) {
							lOp1[0] += lOp2[0];
						} else {
							dOp1[0] += dOp2[0];
						}
					}
				} else if (iTokTyp == gc_TokenBinaryMinus) {
					if (iTyp1 == gc_TokenString) {
						throw L"Can not subtract strings";
					}
					if (iTyp1 == gc_TokenInteger) {
						lOp1[0] = lOp2[0] - lOp1[0];
					} else {
						dOp1[0] = dOp2[0] - dOp1[0];
					}
				} else if (iTokTyp == gc_TokenMultiply) {
					if (iTyp1 == gc_TokenString) {
						throw L"Can not multiply strings";
					}
					if (iTyp1 == gc_TokenInteger) {
						lOp1[0] *= lOp2[0];
					} else {
						if (iTyp1 == gc_TokenReal) {
							iTyp1 = iTyp2;
						} else if (iTyp2 == gc_TokenReal) { } else if (iTyp1 == gc_TokenLengthOperand && iTyp2 == gc_TokenLengthOperand) {
							iTyp1 = gc_TokenAreaOperand;
						} else {
							throw L"Invalid mix of multiplicands";
						}
						dOp1[0] *= dOp2[0];
					}
				} else if (iTokTyp == gc_TokenDivide) {
					if (iTyp1 == gc_TokenString) {
						throw L"Can not divide strings";
					}
					if (iTyp1 == gc_TokenInteger) {
						if (lOp1[0] == 0) {
							throw L"Attempting to divide by 0";
						}
						lOp1[0] = lOp2[0] / lOp1[0];
					} else if (iTyp1 <= iTyp2) {
						if (dOp1[0] == 0.0) {
							throw L"Attempting to divide by 0.";
						}
						if (iTyp1 == iTyp2) {
							iTyp1 = gc_TokenReal;
						} else if (iTyp1 == gc_TokenReal) {
							iTyp1 = iTyp2;
						} else {
							iTyp1 = gc_TokenLengthOperand;
						}
						dOp1[0] = dOp2[0] / dOp1[0];
					} else {
						throw L"Division type error";
					}
				} else if (iTokTyp == gc_TokenExponentiate) {
					if (iTyp1 == gc_TokenInteger) {
						if (lOp1[0] >= 0 && lOp1[0] > DBL_MAX_10_EXP || lOp1[0] < 0 && lOp1[0] < DBL_MIN_10_EXP) {
							throw L"Exponentiation error";
						}
						lOp1[0] = static_cast<int>(pow(static_cast<double>(lOp2[0]), lOp1[0]));
					} else if (iTyp1 == gc_TokenReal) {
						const auto iExp {static_cast<int>(dOp1[0])};
						if (iExp >= 0 && iExp > DBL_MAX_10_EXP || iExp < 0 && iExp < DBL_MIN_10_EXP) {
							throw L"Exponentiation error";
						}
						dOp1[0] = pow(dOp2[0], dOp1[0]);
					}
				}
			} else if (g_TokenTable[iTokTyp].Class == kBinaryRelationalOp) {
				// if support for binary relational operators desired (== != > >= < <=)
				throw L"Binary relational operators not implemented";
			} else if (g_TokenTable[iTokTyp].Class == kBinaryLogicalOp) {
				// if support for binary logical operators desired (& |)
				throw L"Binary logical operators not implemented";
			} else if (g_TokenTable[iTokTyp].Class == kUnaryLogicalOp) {
				// if support for unary logical operator desired (!)
				throw L"Unary logical operator not implemented";
			}
		}
		OperandStackTop++; // Increment operand stack pointer
		iOpStkTyp[OperandStackTop] = iTyp1; // Push operand onto operand stack
		lOpStkDef[OperandStackTop] = lDef1;
		memcpy(&lOpStk[OperandStackTop][0], cOp1, static_cast<unsigned>(HIWORD(lDef1) * 4));
		TokenStackIndex++;
	}
	*valueType = iTyp1;
	*definition = lDef1;
}

void Lex::Init() noexcept {
	g_Tokens = 0;
	g_NumberOfValues = 0;
}

void Lex::Parse(const wchar_t* szLine) {
	g_Tokens = 0;
	g_NumberOfValues = 0;
	wchar_t szTok[256] {L"\0"};
	auto iBeg {0};
	const auto iLnLen {static_cast<int>(wcslen(szLine))};
	while (iBeg < iLnLen) {
		const auto iTyp {Scan(szTok, szLine, iBeg)};
		if (iTyp == -1) {
			return;
		}
		if (g_Tokens == gc_MaximumNumberOfTokens) {
			return;
		}
		g_TokenTypes[g_Tokens] = iTyp;
		int iLen;
		int iDim;
		double dVal;
		switch (iTyp) {
			case gc_TokenIdentifier:
				iDim = static_cast<int>(wcslen(szTok));
				iLen = 1 + (iDim - 1) / 4;
				g_LocationOfValue[g_Tokens] = g_NumberOfValues + 1;
				g_Values[g_NumberOfValues + 1] = iDim + iLen * 65536;
				memcpy(&g_Values[g_NumberOfValues + 2], szTok, static_cast<unsigned>(iDim));
				g_NumberOfValues = g_NumberOfValues + 1 + iLen;
				break;
			case gc_TokenString:
				ParseStringOperand(szTok);
				break;
			case gc_TokenInteger:
				g_LocationOfValue[g_Tokens] = g_NumberOfValues;
				g_Values[g_NumberOfValues++] = MAKELONG(1, 1);
				g_Values[g_NumberOfValues++] = _wtoi(szTok);
				break;
			case gc_TokenReal: case gc_TokenLengthOperand:
				dVal = iTyp == gc_TokenReal ? _wtof(szTok) : AeSys::ParseLength(szTok);
				g_LocationOfValue[g_Tokens] = g_NumberOfValues;
				g_Values[g_NumberOfValues++] = MAKELONG(1, 2);
				memcpy(&g_Values[g_NumberOfValues++], &dVal, sizeof(double));
				g_NumberOfValues++;
				break;
			default: ;
		}
		g_Tokens++;
	}
}

void Lex::ParseStringOperand(const wchar_t* pszTok) {
	if (wcslen(pszTok) < 3) {
		theApp.AddStringToMessageList(IDS_MSG_ZERO_LENGTH_STRING);
		return;
	}
	const auto pszValues {reinterpret_cast<wchar_t*>(& g_Values[g_NumberOfValues + 2])};
	auto iDim {0};
	auto iNxt {1};
	while (pszTok[iNxt] != '\0') {
		if (pszTok[iNxt] == '"' && pszTok[iNxt + 1] == '"') {
			iNxt++;
		}
		pszValues[iDim++] = pszTok[iNxt++];
	}
	pszValues[--iDim] = '\0';
	const auto iLen = 1 + (iDim - 1) / 4;
	g_LocationOfValue[g_Tokens] = ++g_NumberOfValues;
	g_Values[g_NumberOfValues] = MAKELONG(iDim, iLen);
	g_NumberOfValues += iLen;
}

int Lex::Scan(wchar_t* token, const wchar_t* line, int& linePosition) {
	while (line[linePosition] == ' ') {
		linePosition++;
	}
	const auto iBegLoc {linePosition};
	auto iTokLoc {linePosition};
	auto Result {-1};
	auto iS {1};
	auto bDone {false};
	while (!bDone) {
		const auto Address {g_Base[iS] + line[linePosition]};
		if (g_Check[Address] == iS) {
			iS = g_Next[Address];
			if (g_TokenValue[iS] != 0) {
				Result = g_TokenValue[iS];
				iTokLoc = linePosition;
			}
			linePosition++;
		} else if (g_Default[iS] != 0) {
			iS = g_Default[iS];
		} else {
			bDone = true;
		}
	}
	const auto iLen {iTokLoc - iBegLoc + 1};
	wcsncpy(token, &line[iBegLoc], static_cast<unsigned>(iLen));
	token[iLen] = '\0';
	TRACE2("LinePointer = %d, TokenID = %d\n", linePosition, Result);
	if (Result == - 1) {
		linePosition = iBegLoc + 1;
	}
	return Result;
}

int Lex::TokenType(const int aiTokId) noexcept {
	return aiTokId >= 0 && aiTokId < g_Tokens ? g_TokenTypes[aiTokId] : - 1;
}

void Lex::UnaryOp(const int aiTokTyp, int* valueType, long* definition, double* adOp) {
	wchar_t Token[32];
	int Dimension {LOWORD(*definition)};
	int Length {HIWORD(*definition)};
	switch (aiTokTyp) {
		case gc_TokenUnaryMinus:
			for (auto i = 0; i < Length / 2; i++) {
				adOp[i] = -adOp[i];
			}
			break;
		case gc_TokenUnaryPlus:
			break;
		case gc_TokenAbs:
			adOp[0] = fabs(adOp[0]);
			break;
		case gc_TokenAcos: {
			if (fabs(adOp[0]) > 1.0) {
				throw L"Math error: acos of a value greater than 1.";
			}
			adOp[0] = acos(EoToDegree(adOp[0]));
			break;
		}
		case gc_TokenAsin: {
			if (fabs(adOp[0]) > 1.0) {
				throw L"Math error: asin of a value greater than 1.";
			}
			adOp[0] = asin(EoToDegree(adOp[0]));
			break;
		}
		case gc_TokenAtan:
			adOp[0] = atan(EoToDegree(adOp[0]));
			break;
		case gc_TokenCos:
			adOp[0] = cos(EoToRadian(adOp[0]));
			break;
		case gc_TokenToReal:
			break;
		case gc_TokenExp:
			adOp[0] = exp(adOp[0]);
			break;
		case gc_TokenToInteger: // Conversion to integer
			ConvertValTyp(gc_TokenReal, gc_TokenInteger, definition, static_cast<void*>(adOp));
			*valueType = gc_TokenInteger;
			break;
		case gc_TokenLn: {
			if (adOp[0] <= 0.0) {
				throw L"Math error: ln of a non-positive number";
			}
			adOp[0] = log(adOp[0]);
			break;
		}
		case gc_TokenLog: {
			if (adOp[0] <= 0.0) {
				throw L"Math error: log of a non-positive number";
			}
			adOp[0] = log10(adOp[0]);
			break;
		}
		case gc_TokenSin:
			adOp[0] = sin(EoToRadian(adOp[0]));
			break;
		case gc_TokenSqrt: {
			if (adOp[0] < 0.0) {
				throw L"Math error: sqrt of a negative number";
			}
			adOp[0] = sqrt(adOp[0]);
			break;
		}
		case gc_TokenTan:
			adOp[0] = tan(EoToRadian(adOp[0]));
			break;
		case gc_TokenToString: {
			*valueType = gc_TokenString;
			LexColumnDefinition ColumnDefinition {*definition, gc_TokenReal};
			ConvertValToString(reinterpret_cast<wchar_t*>(adOp), &ColumnDefinition, Token, &Dimension);
			Length = 1 + (Dimension - 1) / 4;
			wcscpy(reinterpret_cast<wchar_t*>(adOp), Token);
			*definition = MAKELONG(Dimension, Length);
			break;
		}
		default:
			throw L"Unknown operation";
	}
}

void Lex::UnaryOp(const int aiTokTyp, int* valueType, long* definition, long* alOp) {
	wchar_t Token[32];
	int Dimension {LOWORD(*definition)};
	switch (aiTokTyp) {
		case gc_TokenUnaryMinus:
			alOp[0] = - alOp[0];
			break;
		case gc_TokenUnaryPlus:
			break;
		case gc_TokenAbs:
			alOp[0] = labs(alOp[0]);
			break;
		case gc_TokenToInteger:
			break;
		case gc_TokenToReal:
			ConvertValTyp(gc_TokenInteger, gc_TokenReal, definition, static_cast<void*>(alOp));
			*valueType = gc_TokenReal;
			break;
		case gc_TokenToString: {
			*valueType = gc_TokenString;
			LexColumnDefinition ColumnDefinition {*definition, gc_TokenInteger};
			ConvertValToString(reinterpret_cast<wchar_t*>(alOp), &ColumnDefinition, Token, &Dimension);
			const auto Length {1 + (Dimension - 1) / 4};
			wcscpy(reinterpret_cast<wchar_t*>(alOp), Token);
			*definition = MAKELONG(Dimension, Length);
			break;
		}
		default:
			throw L"Unknown operation";
	}
}

wchar_t* Lex::ScanForChar(const wchar_t c, wchar_t* * ppStr) noexcept {
	const auto p {SkipWhiteSpace(*ppStr)};
	if (*p == c) {
		*ppStr = p + 1;
		return p;
	}
	return nullptr; // not found
}

wchar_t* Lex::SkipWhiteSpace(wchar_t* pszString) noexcept {
	while (pszString != nullptr && *pszString != 0U && isspace(*pszString) != 0) {
		pszString++;
	}
	return pszString;
}

wchar_t* Lex::ScanForString(wchar_t* * ppStr, wchar_t* pszTerm, wchar_t* * ppArgBuf) noexcept {
	auto pIn {SkipWhiteSpace(*ppStr)};
	const auto pStart {*ppArgBuf};
	auto pOut {pStart};
	const auto bInQuotes {*pIn == '"'};
	if (bInQuotes) {
		pIn++;
	}
	do {
		if (bInQuotes) {
			if (*pIn == '"' && *(pIn + 1) != '"') { // Skip over the quote
				pIn++;
				break;
			}
		} else if (isalnum(*pIn) != 0) { } else { // allow some peg specials
			if (!(*pIn == '_' || *pIn == '$' || *pIn == '.' || *pIn == '-' || *pIn == ':' || *pIn == '\\')) {
				break;
			}
		}
		if (*pIn == '"' && *(pIn + 1) == '"') { // Skip the escaping first quote
			pIn++;
		}
		if (*pIn == '\\' && *(pIn + 1) == '\\') { // Skip the escaping backslash
			pIn++;
		}
		*pOut++ = *pIn++; // the char to the arg buffer
	} while (*pIn != 0U);
	*pOut++ = '\0'; // Set up the terminating char and update the scan pointer
	*pszTerm = *pIn;
	if (*pIn != 0U) {
		*ppStr = pIn + 1;
	} else {
		*ppStr = pIn;
	}
	*ppArgBuf = pOut; // Update the arg buffer to the next free bit
	return pStart;
}
