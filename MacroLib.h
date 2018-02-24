#pragma once

#include "util.h"
#include "os_version.h"


struct key_to_vk_type // Map key names to virtual keys.
{
	LPTSTR key_name;
	vk_type vk;
};


static key_to_vk_type g_key_to_vk[] =
{ {_T("Numpad0"), VK_NUMPAD0}
, {_T("Numpad1"), VK_NUMPAD1}
, {_T("Numpad2"), VK_NUMPAD2}
, {_T("Numpad3"), VK_NUMPAD3}
, {_T("Numpad4"), VK_NUMPAD4}
, {_T("Numpad5"), VK_NUMPAD5}
, {_T("Numpad6"), VK_NUMPAD6}
, {_T("Numpad7"), VK_NUMPAD7}
, {_T("Numpad8"), VK_NUMPAD8}
, {_T("Numpad9"), VK_NUMPAD9}
, {_T("NumpadMult"), VK_MULTIPLY}
, {_T("NumpadDiv"), VK_DIVIDE}
, {_T("NumpadAdd"), VK_ADD}
, {_T("NumpadSub"), VK_SUBTRACT}
// , {_T("NumpadEnter"), VK_RETURN}  // Must do this one via scan code, see below for explanation.
, {_T("NumpadDot"), VK_DECIMAL}
, {_T("Numlock"), VK_NUMLOCK}
, {_T("ScrollLock"), VK_SCROLL}
, {_T("CapsLock"), VK_CAPITAL}

, {_T("Escape"), VK_ESCAPE}  // So that VKtoKeyName() delivers consistent results, always have the preferred name first.
, {_T("Esc"), VK_ESCAPE}
, {_T("Tab"), VK_TAB}
, {_T("Space"), VK_SPACE}
, {_T("Backspace"), VK_BACK} // So that VKtoKeyName() delivers consistent results, always have the preferred name first.
, {_T("BS"), VK_BACK}

// These keys each have a counterpart on the number pad with the same VK.  Use the VK for these,
// since they are probably more likely to be assigned to hotkeys (thus minimizing the use of the
// keyboard hook, and use the scan code (SC) for their counterparts.  UPDATE: To support handling
// these keys with the hook (i.e. the sc_takes_precedence flag in the hook), do them by scan code
// instead.  This allows Numpad keys such as Numpad7 to be differentiated from NumpadHome, which
// would otherwise be impossible since both of them share the same scan code (i.e. if the
// sc_takes_precedence flag is set for the scan code of NumpadHome, that will effectively prevent
// the hook from telling the difference between it and Numpad7 since the hook is currently set
// to handle an incoming key by either vk or sc, but not both.

// Even though ENTER is probably less likely to be assigned than NumpadEnter, must have ENTER as
// the primary vk because otherwise, if the user configures only naked-NumPadEnter to do something,
// RegisterHotkey() would register that vk and ENTER would also be configured to do the same thing.
, {_T("Enter"), VK_RETURN}  // So that VKtoKeyName() delivers consistent results, always have the preferred name first.
, {_T("Return"), VK_RETURN}

, {_T("NumpadDel"), VK_DELETE}
, {_T("NumpadIns"), VK_INSERT}
, {_T("NumpadClear"), VK_CLEAR}  // same physical key as Numpad5 on most keyboards?
, {_T("NumpadUp"), VK_UP}
, {_T("NumpadDown"), VK_DOWN}
, {_T("NumpadLeft"), VK_LEFT}
, {_T("NumpadRight"), VK_RIGHT}
, {_T("NumpadHome"), VK_HOME}
, {_T("NumpadEnd"), VK_END}
, {_T("NumpadPgUp"), VK_PRIOR}
, {_T("NumpadPgDn"), VK_NEXT}

, {_T("PrintScreen"), VK_SNAPSHOT}
, {_T("CtrlBreak"), VK_CANCEL}  // Might want to verify this, and whether it has any peculiarities.
, {_T("Pause"), VK_PAUSE} // So that VKtoKeyName() delivers consistent results, always have the preferred name first.
, {_T("Break"), VK_PAUSE} // Not really meaningful, but kept for as a synonym of Pause for backward compatibility.  See CtrlBreak.
, {_T("Help"), VK_HELP}  // VK_HELP is probably not the extended HELP key.  Not sure what this one is.
, {_T("Sleep"), VK_SLEEP}

, {_T("AppsKey"), VK_APPS}

// UPDATE: For the NT/2k/XP version, now doing these by VK since it's likely to be
// more compatible with non-standard or non-English keyboards:
, {_T("LControl"), VK_LCONTROL} // So that VKtoKeyName() delivers consistent results, always have the preferred name first.
, {_T("RControl"), VK_RCONTROL} // So that VKtoKeyName() delivers consistent results, always have the preferred name first.
, {_T("LCtrl"), VK_LCONTROL} // Abbreviated versions of the above.
, {_T("RCtrl"), VK_RCONTROL} //
, {_T("LShift"), VK_LSHIFT}
, {_T("RShift"), VK_RSHIFT}
, {_T("LAlt"), VK_LMENU}
, {_T("RAlt"), VK_RMENU}
// These two are always left/right centric and I think their vk's are always supported by the various
// Windows API calls, unlike VK_RSHIFT, etc. (which are seldom supported):
, {_T("LWin"), VK_LWIN}
, {_T("RWin"), VK_RWIN}

// The left/right versions of these are handled elsewhere since their virtual keys aren't fully API-supported:
, {_T("Control"), VK_CONTROL} // So that VKtoKeyName() delivers consistent results, always have the preferred name first.
, {_T("Ctrl"), VK_CONTROL}  // An alternate for convenience.
, {_T("Alt"), VK_MENU}
, {_T("Shift"), VK_SHIFT}
/*
These were used to confirm the fact that you can't use RegisterHotkey() on VK_LSHIFT, even if the shift
modifier is specified along with it:
, {_T("LShift"), VK_LSHIFT}
, {_T("RShift"), VK_RSHIFT}
*/
, {_T("F1"), VK_F1}
, {_T("F2"), VK_F2}
, {_T("F3"), VK_F3}
, {_T("F4"), VK_F4}
, {_T("F5"), VK_F5}
, {_T("F6"), VK_F6}
, {_T("F7"), VK_F7}
, {_T("F8"), VK_F8}
, {_T("F9"), VK_F9}
, {_T("F10"), VK_F10}
, {_T("F11"), VK_F11}
, {_T("F12"), VK_F12}
, {_T("F13"), VK_F13}
, {_T("F14"), VK_F14}
, {_T("F15"), VK_F15}
, {_T("F16"), VK_F16}
, {_T("F17"), VK_F17}
, {_T("F18"), VK_F18}
, {_T("F19"), VK_F19}
, {_T("F20"), VK_F20}
, {_T("F21"), VK_F21}
, {_T("F22"), VK_F22}
, {_T("F23"), VK_F23}
, {_T("F24"), VK_F24}

// Mouse buttons:
, {_T("LButton"), VK_LBUTTON}
, {_T("RButton"), VK_RBUTTON}
, {_T("MButton"), VK_MBUTTON}
// Supported in only in Win2k and beyond:
, {_T("XButton1"), VK_XBUTTON1}
, {_T("XButton2"), VK_XBUTTON2}
// Custom/fake VKs for use by the mouse hook (supported only in WinNT SP3 and beyond?):
, {_T("WheelDown"), VK_WHEEL_DOWN}
, {_T("WheelUp"), VK_WHEEL_UP}
// Lexikos: Added fake VKs for support for horizontal scrolling in Windows Vista and later.
, {_T("WheelLeft"), VK_WHEEL_LEFT}
, {_T("WheelRight"), VK_WHEEL_RIGHT}

, {_T("Browser_Back"), VK_BROWSER_BACK}
, {_T("Browser_Forward"), VK_BROWSER_FORWARD}
, {_T("Browser_Refresh"), VK_BROWSER_REFRESH}
, {_T("Browser_Stop"), VK_BROWSER_STOP}
, {_T("Browser_Search"), VK_BROWSER_SEARCH}
, {_T("Browser_Favorites"), VK_BROWSER_FAVORITES}
, {_T("Browser_Home"), VK_BROWSER_HOME}
, {_T("Volume_Mute"), VK_VOLUME_MUTE}
, {_T("Volume_Down"), VK_VOLUME_DOWN}
, {_T("Volume_Up"), VK_VOLUME_UP}
, {_T("Media_Next"), VK_MEDIA_NEXT_TRACK}
, {_T("Media_Prev"), VK_MEDIA_PREV_TRACK}
, {_T("Media_Stop"), VK_MEDIA_STOP}
, {_T("Media_Play_Pause"), VK_MEDIA_PLAY_PAUSE}
, {_T("Launch_Mail"), VK_LAUNCH_MAIL}
, {_T("Launch_Media"), VK_LAUNCH_MEDIA_SELECT}
, {_T("Launch_App1"), VK_LAUNCH_APP1}
, {_T("Launch_App2"), VK_LAUNCH_APP2}

// Probably safest to terminate it this way, with a flag value.  (plus this makes it a little easier
// to code some loops, maybe).  Can also calculate how many elements are in the array using sizeof(array)
// divided by sizeof(element).  UPDATE: Decided not to do this in case ever decide to sort this array; don't
// want to rely on the fact that this will wind up in the right position after the sort (even though it
// should):
//, {_T(""), 0}
};

struct key_to_sc_type // Map key names to scan codes.
{
	LPTSTR key_name;
	sc_type sc;
};


static key_to_sc_type g_key_to_sc[] =
	// Even though ENTER is probably less likely to be assigned than NumpadEnter, must have ENTER as
	// the primary vk because otherwise, if the user configures only naked-NumPadEnter to do something,
	// RegisterHotkey() would register that vk and ENTER would also be configured to do the same thing.
{ {_T("NumpadEnter"), SC_NUMPADENTER}

, {_T("Delete"), SC_DELETE}
, {_T("Del"), SC_DELETE}
, {_T("Insert"), SC_INSERT}
, {_T("Ins"), SC_INSERT}
// , {_T("Clear"), SC_CLEAR}  // Seems unnecessary because there is no counterpart to the Numpad5 clear key?
, {_T("Up"), SC_UP}
, {_T("Down"), SC_DOWN}
, {_T("Left"), SC_LEFT}
, {_T("Right"), SC_RIGHT}
, {_T("Home"), SC_HOME}
, {_T("End"), SC_END}
, {_T("PgUp"), SC_PGUP}
, {_T("PgDn"), SC_PGDN}

// If user specified left or right, must use scan code to distinguish *both* halves of the pair since
// each half has same vk *and* since their generic counterparts (e.g. CONTROL vs. L/RCONTROL) are
// already handled by vk.  Note: RWIN and LWIN don't need to be handled here because they each have
// their own virtual keys.
// UPDATE: For the NT/2k/XP version, now doing these by VK since it's likely to be
// more compatible with non-standard or non-English keyboards:
/*
, {_T("LControl"), SC_LCONTROL}
, {_T("RControl"), SC_RCONTROL}
, {_T("LShift"), SC_LSHIFT}
, {_T("RShift"), SC_RSHIFT}
, {_T("LAlt"), SC_LALT}
, {_T("RAlt"), SC_RALT}
*/
};


// Can calc the counts only after the arrays are initialized above:
static int g_key_to_vk_count = _countof(g_key_to_vk);
static int g_key_to_sc_count = _countof(g_key_to_sc);

class CMacroLib
{

public:
	CMacroLib(void);
	~CMacroLib(void);

	ResultType ImageSearch(int aLeft, int aTop, int aRight, int aBottom, LPTSTR aImageFile, int &nRetPosX, int &nRetPosY);

	ResultType ControlSend(CWnd* pWnd, LPTSTR aKeysToSend, LPTSTR aTitle, LPTSTR aText, LPTSTR aExcludeTitle, LPTSTR aExcludeText, bool aSendRaw);

	void SetClipBoard(CString strText);

	void MouseMove(CWnd* pWnd, int nX, int nY);

	void MouseClick(CWnd* pWnd, int nX, int nY, int nDelay = 100);

private:
	LPCOLORREF getbits(HBITMAP ahImage, HDC hdc, LONG &aWidth, LONG &aHeight, bool &aIs16Bit, int aMinColorDepth = 8);

	void SendKeys(LPTSTR aKeys, bool aSendRaw, SendModes aSendModeOrig, HWND aTargetWindow = NULL);

	modLR_type GetModifierLRState(bool aExplicitlyGet = false);

	void ParseClickOptions(LPTSTR aOptions, int &aX, int &aY, vk_type &aVK, KeyEventTypes &aEventType, int &aRepeatCount, bool &aMoveOffset);

	int ConvertMouseButton(LPTSTR aBuf, bool aAllowWheel = true, bool aUseLogicalButton = false);

	void MouseMove(int &aX, int &aY, DWORD &aEventFlags, int aSpeed, bool aMoveOffset);

	void DoMouseDelay();

	void MouseEvent(DWORD aEventFlags, DWORD aData, DWORD aX = COORD_UNSPECIFIED, DWORD aY = COORD_UNSPECIFIED);

	void DoIncrementalMouseMove(int aX1, int aY1, int aX2, int aY2, int aSpeed);

	void SendKey(vk_type aVK, sc_type aSC, modLR_type aModifiersLR, modLR_type aModifiersLRPersistent, int aRepeatCount, KeyEventTypes aEventType, modLR_type aKeyAsModifiersLR, HWND aTargetWindow, int aX = COORD_UNSPECIFIED, int aY = COORD_UNSPECIFIED, bool aMoveOffset = false);

	bool IsMouseVK(vk_type aVK);


	void SetModifierLRState(modLR_type aModifiersLRnew, modLR_type aModifiersLRnow, HWND aTargetWindow	, bool aDisguiseDownWinAlt, bool aDisguiseUpWinAlt, DWORD aExtraInfo = KEY_IGNORE_ALL_EXCEPT_MODIFIER);

	void MouseClick(vk_type aVK, int aX, int aY, int aRepeatCount, int aSpeed, KeyEventTypes aEventType, bool aMoveOffset = false);

	vk_type TextToVK(LPTSTR aText, modLR_type *pModifiersLR = NULL, bool aExcludeThoseHandledByScanCode = false, bool aAllowExplicitVK = true, HKL aKeybdLayout = GetKeyboardLayout(0));
	vk_type CharToVKAndModifiers(TCHAR aChar, modLR_type *pModifiersLR, HKL aKeybdLayout);

	HWND GetNonChildParent(HWND aWnd);

	HWND SetForegroundWindowEx(HWND aTargetWindow);

	HWND AttemptSetForeground(HWND aTargetWindow, HWND aForeWindow);

	sc_type TextToSC(LPTSTR aText);
	vk_type sc_to_vk(sc_type aSC);

	modLR_type KeyToModifiersLR(vk_type aVK, sc_type aSC = 0, bool *pIsNeutral = NULL);
	void SendKeySpecial(TCHAR aChar, int aRepeatCount);
	void SendUnicodeChar(wchar_t aChar, int aModifiers = -1);

	vk_type TextToSpecial(LPTSTR aText, size_t aTextLength, KeyEventTypes &aEventType, modLR_type &aModifiersLR, bool aUpdatePersistent);

	void KeyEvent(KeyEventTypes aEventType, vk_type aVK, sc_type aSC = 0, HWND aTargetWindow = NULL, bool aDoKeyDelay = false, DWORD aExtraInfo = KEY_IGNORE_ALL_EXCEPT_MODIFIER);

	sc_type vk_to_sc(vk_type aVK, bool aReturnSecondary = false);

	void DoKeyDelay(int aDelay = (sSendMode == SM_PLAY) ? -1 : 10);

	void SendASC(LPCTSTR aAscii);

	
private:
	OS_Version g_os;

	LPTSTR m_strKeysToSend;

	
};


