#include "Global.h"
#include "TextService.h"
#include "LanguageBar.h"
#include "EditSession.h"

// {178A04CC-7A3E-47B9-99FE-C62D2F4CA20A}
static const GUID cPreservedKey_GUID_ToggleHangul =
{ 0x178a04cc, 0x7a3e, 0x47b9,{ 0x99, 0xfe, 0xc6, 0x2d, 0x2f, 0x4c, 0xa2, 0xa } };

/* Set Shift+Space as on off key */
static const TF_PRESERVEDKEY cShiftSpace = { VK_SPACE, TF_MOD_SHIFT };
static const TF_PRESERVEDKEY cHangul = { VK_HANGUL, TF_MOD_IGNORE_ALL_MODIFIER };
static const WCHAR ToggleDesc[] = L"Hangul Toggle";

// r -> s / s -> d / t -> f / d -> g / n -> j / e -> k / i -> l / o -> ; / k -> n
// f -> e / p -> r / g -> t / j -> y / l -> u / u -> i / y -> o / ; -> p /
static const UINT cColemakToQwerty[256] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x42, 0x43, 0x47, 0x4B, 0x45, 0x54, 0x48, 0x4C, 0x59, 0x4E, 0x55, 0x4D, 0x4A, 0xBA,
	0x52, 0x51, 0x53, 0x44, 0x46, 0x49, 0x56, 0x57, 0x58, 0x4F, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0x50, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

bool TextService::_InitKeyEventSink()
{
	DebugLogFile(L"%s\n", L"TextService::_InitKeyEventSink");

	ITfKeystrokeMgr* pKeystrokeMgr = nullptr;
	HRESULT hr = S_OK;

	if (FAILED(_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void **)&pKeystrokeMgr)))
	{
		DebugLogFile(L"\t%s\n", L"KeystrokeMgr fail");
		return false;
	}

	hr = pKeystrokeMgr->AdviseKeyEventSink(_ClientId, (ITfKeyEventSink *)this, TRUE);

	pKeystrokeMgr->Release();

	return (hr == S_OK);
}

void TextService::_UninitKeyEventSink()
{
	DebugLogFile(L"%s\n", L"TextService::_UninitKeyEventSink");

	ITfKeystrokeMgr* pKeystrokeMgr = nullptr;

	if (FAILED(_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void **)&pKeystrokeMgr)))
	{
		return;
	}

	pKeystrokeMgr->UnadviseKeyEventSink(_ClientId);

	pKeystrokeMgr->Release();
}

bool TextService::_InitPreservedKey() 
{
	DebugLogFile(L"%s\n", L"TextService::_InitPreservedKey");


	ITfKeystrokeMgr *pKeystrokeMgr;
	HRESULT hr;

	if (_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void **)&pKeystrokeMgr) != S_OK) {
		return false;
	}

	// register Hangul
	hr = pKeystrokeMgr->PreserveKey(_ClientId,
		cPreservedKey_GUID_ToggleHangul,
		&cHangul,
		ToggleDesc,
		(ULONG)wcslen(ToggleDesc));

	if (hr == S_OK) {
		// register Shift + Space
		hr = pKeystrokeMgr->PreserveKey(_ClientId,
			cPreservedKey_GUID_ToggleHangul,
			&cShiftSpace,
			ToggleDesc,
			(ULONG)wcslen(ToggleDesc));
	}

	pKeystrokeMgr->Release();

	return (hr == S_OK);
}

void TextService::_UninitPreservedKey() 
{
	DebugLogFile(L"%s\n", L"TextService::_UninitPreservedKey");

	ITfKeystrokeMgr *pKeystrokeMgr;

	if (_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void **)&pKeystrokeMgr) != S_OK) {
		return;
	}

	pKeystrokeMgr->UnpreserveKey(cPreservedKey_GUID_ToggleHangul, &cShiftSpace);
	pKeystrokeMgr->UnpreserveKey(cPreservedKey_GUID_ToggleHangul, &cHangul);

	pKeystrokeMgr->Release();
}

WORD TextService::_ConvertVKeyToAscii(UINT code, BYTE* lpKeyState)
{
	//
	// Map virtual key to scan code
	//
	UINT scanCode = 0;
	scanCode = MapVirtualKey(code, 0);

	//
	// Keyboard state
	//
	if (!GetKeyboardState(lpKeyState))
	{
		return 0;
	}

	//
	// Map virtual key to character code
	//
	WORD wch = '\0';
	if (ToAscii(code, scanCode, lpKeyState, &wch, 0) == 1)
	{
		return wch;
	}

	return 0;
}

void TextService::_Automata(UINT key, UINT scancode)
{
	BYTE pKeyState[256] = { '\0' };
	UINT converted_key;

	// TODO: Check if colemak2 keyboard is loaded
	converted_key = cColemakToQwerty[key&0xFF];
	WORD code = _ConvertVKeyToAscii((UINT)converted_key, pKeyState);
	DebugLogFile(L"%s %x\n", L"TextService::_Automata (ascii)", code);

	_keyEaten = false;

	// 만일 CTRL, ALT, WIN(LWIN, RWIN) 키 중 아무거나 눌린 상태라면 영어 자판으로 처리함.
	if ((pKeyState[VK_CONTROL] & 0x80) || (pKeyState[VK_MENU] & 0x80) ||
		(pKeyState[VK_LWIN] & 0x80) || (pKeyState[VK_RWIN] & 0x80)) {
		DebugLogFile(L"Modifier Keys are down.");
	}

	if (key == VK_SHIFT)
	{
		_keyEaten = true;
		return;
	}

	if (key == VK_BACK)
	{
		DebugLogFile(L"\t%s\n", L"BACKSPACE");
		_keyEaten = true;
		gNavilIME.HangulBackspace();
		if (gNavilIME.HangulGetPreedit(0) == 0)
		{
			// 자모를 다 지우고 libhangul 버퍼가 비어 있는 상황
			_keyEaten = false;
		}
		return;
	}

	// ESC가 눌리면 조합을 중지하고 flush하고 영문 모드로 바꾼다.
	if (key == VK_ESCAPE)
	{
		DebugLogFile(L"\t%s\n", L"-> toggle Hangul ESC");
		_pHangulTurnOnOffStatus->ToggleStatus();

		// 키를 먹지 않고 그대로 내려 보낸다.
		_keyEaten = false;

		return;
	}

	if (gNavilIME.HangulProcess((int)code))
	{
		DebugLogFile(L"\t%s %x\n", L"proc:", code);

		_keyEaten = true;
	}
}

/*
ITfKeyEventSink interface functions
*/
// 키 입력 포커스를 얻었을 때 호출된다.
STDMETHODIMP TextService::OnSetFocus(BOOL fForeground)
{
	DebugLogFile(L"%s\n", L"TextService::OnSetFocus");
	return S_OK;
}

// Notepad는 TestKeyDown/Up이 먼저 호출되고 pIsEaten이 true 일 때만 KeyDown/Up이 호출된다.
// 그런데 Wordpad는 TestKeyDown/Up이 호출되지 않고 바로 KeyDown/Up이 호출된다. 난장판이구만..
STDMETHODIMP TextService::OnTestKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pIsEaten)
{
	DebugLogFile(L"%s\n", L"TextService::OnTestKeyDown");

	// MS Word에서는 space 혹은 enter 다음에 TestKeyDown을 두번 호출한다.
	// 진짜 keycode을 얹어서 두 번 호출한다...-_-; 그래서 처리를 해 주지 않으면 space 다음에 쌍자음이 입력된다.
	// _testKeyHappend 변수는 OnTestKeyDown이 처리되면 true로 바뀌고 키 입력 처리가 OnKeyDown에서 완료되면 false로 바뀌므로 _testKeyHappend 변수 값이 true인 동안에는 OnTestKeyDown에서 automata 처리를 하지 않고 입력값 자체도 무시해버려야(Eaten=true) 한다.
	if (gNavilIME.GetHangulMode() == false)
	{
		// TODO: 영어 자판 처리는 이곳에서,
		_keyEaten = false;
		*pIsEaten = false;
		return S_OK;
	}
	if (_testKeyHappened == true)
	{
		DebugLogFile(L"\t->%s\n", L"Waiting KeyDown done..");
		_keyEaten = true;
		*pIsEaten = true;
		return S_OK;
	}
	
	// lParam[23:16] : scan code.
	_Automata((UINT)wParam, (UINT)((lParam >> 16)&0xFF));
	*pIsEaten = _keyEaten;
	_testKeyHappened = true;

	if (*pIsEaten == false)
	{
		_EndComposition(pContext);
		gNavilIME.HangulFlush();
		_testKeyHappened = false;
	}

	return S_OK;
}

// 키가 눌렸을 때
STDMETHODIMP TextService::OnKeyDown(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pIsEaten)
{
	DebugLogFile(L"%s\n", L"TextService::OnKeyDown");

	if (_testKeyHappened == false)
	{
		OnTestKeyDown(pContext, wParam, lParam, pIsEaten);
		_testKeyHappened = false;

		if (*pIsEaten == false)
		{
			return S_OK;
		}
	}

	*pIsEaten = _keyEaten;
	_testKeyHappened = false;

	UINT commit = gNavilIME.HangulGetCommit(0);
	if (commit != 0)
	{
		DebugLogFile(L"\t%s -> %x\n", L"commit", commit);
		if (_IsComposing())	{
			_HandleComposition(pContext, (WCHAR)commit);
			_EndComposition(pContext);
		}
		else {
			_AppendText(pContext, (WCHAR)commit);
		}
		return S_OK;
	}
	UINT preedit = gNavilIME.HangulGetPreedit(0);
	if (preedit != 0) {
		DebugLogFile(L"\t%s -> %x\n", L"preedit", preedit);
		_HandleComposition(pContext, (WCHAR)preedit);
	}

	return S_OK;
}

STDMETHODIMP TextService::OnTestKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pIsEaten)
{
	DebugLogFile(L"%s\n", L"TextService::OnTestKeyUp");

	*pIsEaten = _keyEaten;
	_testKeyHappened = false;

	return S_OK;
}

// 키가 올라올 때
STDMETHODIMP TextService::OnKeyUp(ITfContext *pContext, WPARAM wParam, LPARAM lParam, BOOL *pIsEaten)
{
	DebugLogFile(L"%s\n", L"TextService::OnKeyUp");

	*pIsEaten = _keyEaten;

	if (*pIsEaten)
	{
		UINT commit = gNavilIME.HangulGetCommit(0);
		if (commit != 0)
		{
			UINT preedit = gNavilIME.HangulGetPreedit(0);
			if (preedit != 0) {
				DebugLogFile(L"\t%s -> %x\n", L"preedit (in KeyUp)", preedit);
				_HandleComposition(pContext, (WCHAR)preedit);
			}
			else
			{
				for (UINT i = 1; i < 8; i++) {
					UINT remainedCommit = gNavilIME.HangulGetCommit(i);
					DebugLogFile(L"\t%s[%d] -> %x\n", L"remained commit (in KeyUp)", i, remainedCommit);

					if (remainedCommit != 0)
					{
						_AppendText(pContext, (WCHAR)remainedCommit);
					}
					else
					{
						break;
					}
				}
				gNavilIME.HangulFlush();
			}
		}
	}

	return S_OK;
}

// _InitPreservedKey에서 등록한 핫 키가 눌리면 호출된다
STDMETHODIMP TextService::OnPreservedKey(ITfContext *pContext, REFGUID rguid, BOOL *pIsEaten)
{
	DebugLogFile(L"%s\n", L"TextService::OnPreservedKey");

	if (IsEqualGUID(rguid, cPreservedKey_GUID_ToggleHangul)) {
		DebugLogFile(L"\t%s\n", L"-> toggle Hangul");
		_pHangulTurnOnOffStatus->ToggleStatus();
		
		if (_IsComposing()) {
			gNavilIME.HangulFlush();
			_EndComposition(pContext);
		}

		*pIsEaten = TRUE;
	}
	else {
		*pIsEaten = FALSE;
	}

	return S_OK;
}

