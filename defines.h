
#ifndef defines_h
#define defines_h


#ifdef _MSC_VER
#pragma warning(disable:4800)
#endif

#define _CRT_SECURE_NO_WARNINGS


// AutoIt2 supports lines up to 16384 characters long, and we want to be able to do so too
// so that really long lines from aut2 scripts, such as a chain of IF commands, can be
// brought in and parsed.  In addition, it also allows continuation sections to be long.
#define LINE_SIZE (16384 + 1)  // +1 for terminator.  Don't increase LINE_SIZE above 65535 without considering ArgStruct::length's type (WORD).

// The following avoid having to link to OLDNAMES.lib, but they probably don't
// reduce code size at all.
#define stricmp(str1, str2) _stricmp(str1, str2)
#define strnicmp(str1, str2, size) _strnicmp(str1, str2, size)

// Items that may be needed for VC++ 6.X:
#ifndef SPI_GETFOREGROUNDLOCKTIMEOUT
	#define SPI_GETFOREGROUNDLOCKTIMEOUT        0x2000
	#define SPI_SETFOREGROUNDLOCKTIMEOUT        0x2001
#endif
#ifndef VK_XBUTTON1
	#define VK_XBUTTON1       0x05    /* NOT contiguous with L & RBUTTON */
	#define VK_XBUTTON2       0x06    /* NOT contiguous with L & RBUTTON */
	#define WM_NCXBUTTONDOWN                0x00AB
	#define WM_NCXBUTTONUP                  0x00AC
	#define WM_NCXBUTTONDBLCLK              0x00AD
	#define GET_WHEEL_DELTA_WPARAM(wParam)  ((short)HIWORD(wParam))
	#define WM_XBUTTONDOWN                  0x020B
	#define WM_XBUTTONUP                    0x020C
	#define WM_XBUTTONDBLCLK                0x020D
	#define GET_KEYSTATE_WPARAM(wParam)     (LOWORD(wParam))
	#define GET_NCHITTEST_WPARAM(wParam)    ((short)LOWORD(wParam))
	#define GET_XBUTTON_WPARAM(wParam)      (HIWORD(wParam))
	/* XButton values are WORD flags */
	#define XBUTTON1      0x0001
	#define XBUTTON2      0x0002
#endif
#ifndef HIMETRIC_INCH
	#define HIMETRIC_INCH 2540
#endif


#define IS_32BIT(signed_value_64) (signed_value_64 >= INT_MIN && signed_value_64 <= INT_MAX)
#define GET_BIT(buf,n) (((buf) & (1 << (n))) >> (n))
#define SET_BIT(buf,n,val) ((val) ? ((buf) |= (1<<(n))) : (buf &= ~(1<<(n))))

// FAIL = 0 to remind that FAIL should have the value zero instead of something arbitrary
// because some callers may simply evaluate the return result as true or false
// (and false is a failure):
enum ResultType {FAIL = 0, OK, WARN = OK, CRITICAL_ERROR  // Some things might rely on OK==1 (i.e. boolean "true")
	, CONDITION_TRUE, CONDITION_FALSE
	, LOOP_BREAK, LOOP_CONTINUE
	, EARLY_RETURN, EARLY_EXIT}; // EARLY_EXIT needs to be distinct from FAIL for ExitApp() and AutoExecSection().

#define SEND_MODES { _T("Event"), _T("Input"), _T("Play"), _T("InputThenPlay") } // Must match the enum below.
enum SendModes {SM_EVENT, SM_INPUT, SM_PLAY, SM_INPUT_FALLBACK_TO_PLAY, SM_INVALID}; // SM_EVENT must be zero.
// In above, SM_INPUT falls back to SM_EVENT when the SendInput mode would be defeated by the presence
// of a keyboard/mouse hooks in another script (it does this because SendEvent is superior to a
// crippled/interruptible SendInput due to SendEvent being able to dynamically adjust to changing
// conditions [such as the user releasing a modifier key during the Send]).  By contrast,
// SM_INPUT_FALLBACK_TO_PLAY falls back to the SendPlay mode.  SendInput has this extra fallback behavior
// because it's likely to become the most popular sending method.

enum ExitReasons {EXIT_NONE, EXIT_CRITICAL, EXIT_ERROR, EXIT_DESTROY, EXIT_LOGOFF, EXIT_SHUTDOWN
	, EXIT_WM_QUIT, EXIT_WM_CLOSE, EXIT_MENU, EXIT_EXIT, EXIT_RELOAD, EXIT_SINGLEINSTANCE};

enum WarnType {WARN_USE_UNSET_LOCAL, WARN_USE_UNSET_GLOBAL, WARN_LOCAL_SAME_AS_GLOBAL, WARN_USE_ENV, WARN_ALL};

enum WarnMode {WARNMODE_OFF, WARNMODE_OUTPUTDEBUG, WARNMODE_MSGBOX, WARNMODE_STDOUT};	// WARNMODE_OFF must be zero.

enum SingleInstanceType {ALLOW_MULTI_INSTANCE, SINGLE_INSTANCE_PROMPT, SINGLE_INSTANCE_REPLACE
	, SINGLE_INSTANCE_IGNORE, SINGLE_INSTANCE_OFF}; // ALLOW_MULTI_INSTANCE must be zero.

enum MenuTypeType {MENU_TYPE_NONE, MENU_TYPE_POPUP, MENU_TYPE_BAR}; // NONE must be zero.

// These are used for things that can be turned on, off, or left at a
// neutral default value that is neither on nor off.  INVALID must
// be zero:
enum ToggleValueType {TOGGLE_INVALID = 0, TOGGLED_ON, TOGGLED_OFF, ALWAYS_ON, ALWAYS_OFF, TOGGLE
	, TOGGLE_PERMIT, NEUTRAL, TOGGLE_SEND, TOGGLE_MOUSE, TOGGLE_SENDANDMOUSE, TOGGLE_DEFAULT
	, TOGGLE_MOUSEMOVE, TOGGLE_MOUSEMOVEOFF};

// Some things (such as ListView sorting) rely on SCS_INSENSITIVE being zero.
// In addition, BIF_InStr relies on SCS_SENSITIVE being 1:
enum StringCaseSenseType {SCS_INSENSITIVE, SCS_SENSITIVE, SCS_INSENSITIVE_LOCALE, SCS_INSENSITIVE_LOGICAL, SCS_INVALID};

enum RegSyntax {REG_OLD_SYNTAX, REG_NEW_SYNTAX, REG_EITHER_SYNTAX};

enum SymbolType // For use with ExpandExpression() and IsPureNumeric().
{
	// The sPrecedence array in ExpandExpression() must be kept in sync with any additions, removals,
	// or re-ordering of the below.  Also, IS_OPERAND() relies on all operand types being at the
	// beginning of the list:
	 PURE_NOT_NUMERIC // Must be zero/false because callers rely on that.
	, PURE_INTEGER, PURE_FLOAT
	, SYM_STRING = PURE_NOT_NUMERIC, SYM_INTEGER = PURE_INTEGER, SYM_FLOAT = PURE_FLOAT // Specific operand types.
#define IS_NUMERIC(symbol) ((symbol) == SYM_INTEGER || (symbol) == SYM_FLOAT) // Ordered for short-circuit performance.
	, SYM_MISSING // Only used in parameter lists.
	, SYM_VAR // An operand that is a variable's contents.
	, SYM_OPERAND // Generic/undetermined type of operand.
	, SYM_OBJECT // L31: Represents an IObject interface pointer.
	, SYM_DYNAMIC // An operand that needs further processing during the evaluation phase.
	, SYM_OPERAND_END // Marks the symbol after the last operand.  This value is used below.
	, SYM_BEGIN = SYM_OPERAND_END  // SYM_BEGIN is a special marker to simplify the code.
#define IS_OPERAND(symbol) ((symbol) < SYM_OPERAND_END)
	, SYM_POST_INCREMENT, SYM_POST_DECREMENT // Kept in this position for use by YIELDS_AN_OPERAND() [helps performance].
	, SYM_DOT // DOT must precede SYM_OPAREN so YIELDS_AN_OPERAND(SYM_GET) == TRUE, allowing auto-concat to work for it even though it is positioned after its second operand.
	, SYM_CPAREN, SYM_CBRACKET, SYM_CBRACE, SYM_OPAREN, SYM_OBRACKET, SYM_OBRACE, SYM_COMMA  // CPAREN (close-paren)/CBRACKET/CBRACE must come right before OPAREN for YIELDS_AN_OPERAND.
#define IS_OPAREN_LIKE(symbol) ((symbol) <= SYM_OBRACE && (symbol) >= SYM_OPAREN)
#define IS_CPAREN_LIKE(symbol) ((symbol) <= SYM_CBRACE && (symbol) >= SYM_CPAREN)
#define IS_OPAREN_MATCHING_CPAREN(sym_oparen, sym_cparen) ((sym_oparen - sym_cparen) == (SYM_OPAREN - SYM_CPAREN)) // Requires that (IS_OPAREN_LIKE(sym_oparen) || IS_CPAREN_LIKE(sym_cparen)) is true.
#define SYM_CPAREN_FOR_OPAREN(symbol) ((symbol) - (SYM_OPAREN - SYM_CPAREN)) // Caller must confirm it is OPAREN or OBRACKET.
#define SYM_OPAREN_FOR_CPAREN(symbol) ((symbol) + (SYM_OPAREN - SYM_CPAREN)) // Caller must confirm it is CPAREN or CBRACKET.
#define YIELDS_AN_OPERAND(symbol) ((symbol) < SYM_OPAREN) // CPAREN also covers the tail end of a function call.  Post-inc/dec yields an operand for things like Var++ + 2.  Definitely needs the parentheses around symbol.
	, SYM_ASSIGN, SYM_ASSIGN_ADD, SYM_ASSIGN_SUBTRACT, SYM_ASSIGN_MULTIPLY, SYM_ASSIGN_DIVIDE, SYM_ASSIGN_FLOORDIVIDE
	, SYM_ASSIGN_BITOR, SYM_ASSIGN_BITXOR, SYM_ASSIGN_BITAND, SYM_ASSIGN_BITSHIFTLEFT, SYM_ASSIGN_BITSHIFTRIGHT
	, SYM_ASSIGN_CONCAT // THIS MUST BE KEPT AS THE LAST (AND SYM_ASSIGN THE FIRST) BECAUSE THEY'RE USED IN A RANGE-CHECK.
#define IS_ASSIGNMENT_EXCEPT_POST_AND_PRE(symbol) (symbol <= SYM_ASSIGN_CONCAT && symbol >= SYM_ASSIGN) // Check upper bound first for short-circuit performance.
#define IS_ASSIGNMENT_OR_POST_OP(symbol) (IS_ASSIGNMENT_EXCEPT_POST_AND_PRE(symbol) || symbol == SYM_POST_INCREMENT || symbol == SYM_POST_DECREMENT)
	, SYM_IFF_ELSE, SYM_IFF_THEN // THESE TERNARY OPERATORS MUST BE KEPT IN THIS ORDER AND ADJACENT TO THE BELOW.
	, SYM_OR, SYM_AND // MUST BE KEPT IN THIS ORDER AND ADJACENT TO THE ABOVE because infix-to-postfix is optimized to check a range rather than a series of equalities.
	, SYM_LOWNOT  // LOWNOT is the word "not", the low precedence counterpart of !
	, SYM_EQUAL, SYM_EQUALCASE, SYM_NOTEQUAL // =, ==, <> ... Keep this in sync with IS_RELATIONAL_OPERATOR() below.
	, SYM_GT, SYM_LT, SYM_GTOE, SYM_LTOE  // >, <, >=, <= ... Keep this in sync with IS_RELATIONAL_OPERATOR() below.
#define IS_RELATIONAL_OPERATOR(symbol) (symbol >= SYM_EQUAL && symbol <= SYM_LTOE)
	, SYM_CONCAT
	, SYM_BITOR // Seems more intuitive to have these higher in prec. than the above, unlike C and Perl, but like Python.
	, SYM_BITXOR // SYM_BITOR (ABOVE) MUST BE KEPT FIRST AMONG THE BIT OPERATORS BECAUSE IT'S USED IN A RANGE-CHECK.
	, SYM_BITAND
	, SYM_BITSHIFTLEFT, SYM_BITSHIFTRIGHT // << >>  ALSO: SYM_BITSHIFTRIGHT MUST BE KEPT LAST AMONG THE BIT OPERATORS BECAUSE IT'S USED IN A RANGE-CHECK.
	, SYM_ADD, SYM_SUBTRACT
	, SYM_MULTIPLY, SYM_DIVIDE, SYM_FLOORDIVIDE
	, SYM_NEGATIVE, SYM_HIGHNOT, SYM_BITNOT, SYM_ADDRESS, SYM_DEREF  // Don't change position or order of these because Infix-to-postfix converter's special handling for SYM_POWER relies on them being adjacent to each other.
	, SYM_POWER    // See comments near precedence array for why this takes precedence over SYM_NEGATIVE.
	, SYM_PRE_INCREMENT, SYM_PRE_DECREMENT // Must be kept after the post-ops and in this order relative to each other due to a range check in the code.
	, SYM_FUNC     // A call to a function.
	, SYM_NEW      // new Class()
	, SYM_REGEXMATCH // L31: Experimental ~= RegExMatch operator, equivalent to a RegExMatch call in two-parameter mode.
	, SYM_COUNT    // Must be last because it's the total symbol count for everything above.
	, SYM_INVALID = SYM_COUNT // Some callers may rely on YIELDS_AN_OPERAND(SYM_INVALID)==false.
};
// These two are macros for maintainability (i.e. seeing them together here helps maintain them together).
#define SYM_DYNAMIC_IS_DOUBLE_DEREF(token) (token.buf) // SYM_DYNAMICs other than doubles have NULL buf, at least at the stage this macro is called.
#define SYM_DYNAMIC_IS_VAR_NORMAL_OR_CLIP(token) (!(token)->buf && ((token)->var->Type() == VAR_NORMAL || (token)->var->Type() == VAR_CLIPBOARD)) // i.e. it's an environment variable or the clipboard, not a built-in variable or double-deref.


struct ExprTokenType; // Forward declarations for use below.
struct IDebugProperties;


struct DECLSPEC_NOVTABLE IObject // L31: Abstract interface for "objects".
	: public IDispatch
{
	// See script_object.cpp for comments.
	virtual ResultType STDMETHODCALLTYPE Invoke(ExprTokenType &aResultToken, ExprTokenType &aThisToken, int aFlags, ExprTokenType *aParam[], int aParamCount) = 0;
	
#ifdef CONFIG_DEBUGGER
	virtual void DebugWriteProperty(IDebugProperties *, int aPage, int aPageSize, int aMaxDepth) = 0;
#endif
};


struct DECLSPEC_NOVTABLE IObjectComCompatible : public IObject
{
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
	//STDMETHODIMP_(ULONG) AddRef() = 0;
	//STDMETHODIMP_(ULONG) Release() = 0;
	STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
	STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
	STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);
};


#ifdef CONFIG_DEBUGGER

typedef void *DebugCookie;

struct DECLSPEC_NOVTABLE IDebugProperties
{
	// For simplicity/code size, the debugger handles failures internally
	// rather than returning an error code and requiring caller to handle it.
	virtual void WriteProperty(LPCSTR aName, LPTSTR aValue) = 0;
	virtual void WriteProperty(LPCSTR aName, __int64 aValue) = 0;
	virtual void WriteProperty(LPCSTR aName, IObject *aValue) = 0;
	virtual void WriteProperty(LPCSTR aName, ExprTokenType &aValue) = 0;
	virtual void WriteProperty(INT_PTR aKey, ExprTokenType &aValue) = 0;
	virtual void WriteProperty(IObject *aKey, ExprTokenType &aValue) = 0;
	virtual void BeginProperty(LPCSTR aName, LPCSTR aType, int aNumChildren, DebugCookie &aCookie) = 0;
	virtual void EndProperty(DebugCookie aCookie) = 0;
};

#endif


// Flags used when calling Invoke; also used by g_ObjGet etc.:
#define IT_GET				0
#define IT_SET				1
#define IT_CALL				2 // L40: MetaObject::Invoke relies on these being mutually-exclusive bits.
#define IT_BITMASK			3 // bit-mask for the above.

#define IF_METAOBJ			0x10000 // Indicates 'this' is a meta-object/base of aThisToken. Restricts some functionality and causes aThisToken to be inserted into the param list of called functions.
#define IF_METAFUNC			0x20000 // Indicates Invoke should call a meta-function before checking the object's fields.
#define IF_META				(IF_METAOBJ | IF_METAFUNC)	// Flags for regular recursion into base object.
#define IF_FUNCOBJ			0x40000 // Indicates 'this' is a function, being called via another object (aParam[0]).
#define IF_NEWENUM			0x80000 // Workaround for COM objects which don't resolve "_NewEnum" to DISPID_NEWENUM.
#define IF_CALL_FUNC_ONLY	0x100000 // Used by IDispatch: call only if value is a function.


// Helper function for event handlers and __Delete:
ResultType CallMethod(IObject *aInvokee, IObject *aThis, LPTSTR aMethodName
	, ExprTokenType *aParamValue = NULL, int aParamCount = 0, INT_PTR *aRetVal = NULL // For event handlers.
	, int aExtraFlags = 0); // For Object.__Delete().


struct DerefType; // Forward declarations for use below.
class Var;        //
struct ExprTokenType  // Something in the compiler hates the name TokenType, so using a different name.
{
	// Due to the presence of 8-byte members (double and __int64) this entire struct is aligned on 8-byte
	// vs. 4-byte boundaries.  The compiler defaults to this because otherwise an 8-byte member might
	// sometimes not start at an even address, which would hurt performance on Pentiums, etc.
	union // Which of its members is used depends on the value of symbol, below.
	{
		__int64 value_int64; // for SYM_INTEGER
		double value_double; // for SYM_FLOAT
		struct
		{
			union // These nested structs and unions minimize the token size by overlapping data.
			{
				IObject *object;
				DerefType *deref; // for SYM_FUNC
				Var *var;         // for SYM_VAR
				LPTSTR marker;     // for SYM_STRING and SYM_OPERAND.
			};
			union // Due to the outermost union, this doesn't increase the total size of the struct on x86 builds (but it does on x64). It's used by SYM_FUNC (helps built-in functions), SYM_DYNAMIC, SYM_OPERAND, and perhaps other misc. purposes.
			{
				LPTSTR buf;
				size_t marker_length; // Used only with aResultToken. TODO: Move into separate ResultTokenType struct.
			};
		};  
	};
	// Note that marker's str-length should not be stored in this struct, even though it might be readily
	// available in places and thus help performance.  This is because if it were stored and the marker
	// or SYM_VAR's var pointed to a location that was changed as a side effect of an expression's
	// call to a script function, the length would then be invalid.
	SymbolType symbol; // Short-circuit benchmark is currently much faster with this and the next beneath the union, perhaps due to CPU optimizations for 8-byte alignment.
	union
	{
		ExprTokenType *circuit_token; // Facilitates short-circuit boolean evaluation.
		LPTSTR mem_to_free; // Used only with aResultToken. TODO: Move into separate ResultTokenType struct.
	};
	// The above two probably need to be adjacent to each other to conserve memory due to 8-byte alignment,
	// which is the default alignment (for performance reasons) in any struct that contains 8-byte members
	// such as double and __int64.

	ExprTokenType() {}
	ExprTokenType(__int64 aValue) { SetValue(aValue); }
	ExprTokenType(double aValue) { SetValue(aValue); }
	ExprTokenType(IObject *aValue) { SetValue(aValue); }
	ExprTokenType(LPTSTR aValue) { SetValue(aValue); }
	
	void SetValue(__int64 aValue)
	{
		symbol = SYM_INTEGER;
		value_int64 = aValue;
	}
	void SetValue(int aValue) { SetValue((__int64)aValue); }
	void SetValue(UINT aValue) { SetValue((__int64)aValue); }
	void SetValue(UINT64 aValue) { SetValue((__int64)aValue); }
	void SetValue(double aValue)
	{
		symbol = SYM_FLOAT;
		value_double = aValue;
	}
	void SetValue(LPTSTR aValue)
	{
		ASSERT(aValue);
		symbol = SYM_STRING;
		marker = aValue;
	}
	void SetValue(IObject *aValue)
	// Caller must AddRef() if appropriate.
	{
		ASSERT(aValue);
		symbol = SYM_OBJECT;
		object = aValue;
	}
};
#define MAX_TOKENS 512 // Max number of operators/operands.  Seems enough to handle anything realistic, while conserving call-stack space.
#define STACK_PUSH(token_ptr) stack[stack_count++] = token_ptr
#define STACK_POP stack[--stack_count]  // To be used as the r-value for an assignment.

// But the array that goes with these actions is in globaldata.cpp because
// otherwise it would be a little cumbersome to declare the extern version
// of the array in here (since it's only extern to modules other than
// script.cpp):
enum enum_act {
// Seems best to make ACT_INVALID zero so that it will be the ZeroMemory() default within
// any POD structures that contain an action_type field:
  ACT_INVALID = FAIL  // These should both be zero for initialization and function-return-value purposes.
, ACT_ASSIGN, ACT_ASSIGNEXPR, ACT_EXPRESSION, ACT_ADD, ACT_SUB, ACT_MULT, ACT_DIV
, ACT_ASSIGN_FIRST = ACT_ASSIGN, ACT_ASSIGN_LAST = ACT_DIV
, ACT_STATIC
, ACT_IFIN, ACT_IFNOTIN, ACT_IFCONTAINS, ACT_IFNOTCONTAINS, ACT_IFIS, ACT_IFISNOT
, ACT_IFBETWEEN, ACT_IFNOTBETWEEN
, ACT_IFEXPR  // i.e. if (expr)
 // *** *** *** KEEP ALL OLD-STYLE/AUTOIT V2 IFs AFTER THIS (v1.0.20 bug fix). *** *** ***
 , ACT_FIRST_IF_ALLOWING_SAME_LINE_ACTION
 // *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
 // ACT_IS_IF_OLD() relies upon ACT_IFEQUAL through ACT_IFLESSOREQUAL being adjacent to one another
 // and it relies upon the fact that ACT_IFEQUAL is first in the series and ACT_IFLESSOREQUAL last.
, ACT_IFEQUAL = ACT_FIRST_IF_ALLOWING_SAME_LINE_ACTION, ACT_IFNOTEQUAL, ACT_IFGREATER, ACT_IFGREATEROREQUAL
, ACT_IFLESS, ACT_IFLESSOREQUAL
, ACT_FIRST_OPTIMIZED_IF = ACT_IFBETWEEN, ACT_LAST_OPTIMIZED_IF = ACT_IFLESSOREQUAL
, ACT_FIRST_COMMAND // i.e the above aren't considered commands for parsing/searching purposes.
, ACT_IFWINEXIST = ACT_FIRST_COMMAND
, ACT_IFWINNOTEXIST, ACT_IFWINACTIVE, ACT_IFWINNOTACTIVE
, ACT_IFINSTRING, ACT_IFNOTINSTRING
, ACT_IFEXIST, ACT_IFNOTEXIST, ACT_IFMSGBOX
, ACT_FIRST_IF = ACT_IFIN, ACT_LAST_IF = ACT_IFMSGBOX  // Keep this range updated with any new IFs that are added.
, ACT_ELSE
, ACT_MSGBOX, ACT_INPUTBOX, ACT_SPLASHTEXTON, ACT_SPLASHTEXTOFF, ACT_PROGRESS, ACT_SPLASHIMAGE
, ACT_TOOLTIP, ACT_TRAYTIP, ACT_INPUT
, ACT_TRANSFORM, ACT_STRINGLEFT, ACT_STRINGRIGHT, ACT_STRINGMID
, ACT_STRINGTRIMLEFT, ACT_STRINGTRIMRIGHT, ACT_STRINGLOWER, ACT_STRINGUPPER
, ACT_STRINGLEN, ACT_STRINGGETPOS, ACT_STRINGREPLACE, ACT_STRINGSPLIT, ACT_SPLITPATH, ACT_SORT
, ACT_ENVGET, ACT_ENVSET, ACT_ENVUPDATE
, ACT_RUNAS, ACT_RUN, ACT_RUNWAIT, ACT_URLDOWNLOADTOFILE
, ACT_GETKEYSTATE
, ACT_SEND, ACT_SENDRAW, ACT_SENDINPUT, ACT_SENDPLAY, ACT_SENDEVENT
, ACT_CONTROLSEND, ACT_CONTROLSENDRAW, ACT_CONTROLCLICK, ACT_CONTROLMOVE, ACT_CONTROLGETPOS, ACT_CONTROLFOCUS
, ACT_CONTROLGETFOCUS, ACT_CONTROLSETTEXT, ACT_CONTROLGETTEXT, ACT_CONTROL, ACT_CONTROLGET
, ACT_SENDMODE, ACT_SENDLEVEL, ACT_COORDMODE, ACT_SETDEFAULTMOUSESPEED
, ACT_CLICK, ACT_MOUSEMOVE, ACT_MOUSECLICK, ACT_MOUSECLICKDRAG, ACT_MOUSEGETPOS
, ACT_STATUSBARGETTEXT
, ACT_STATUSBARWAIT
, ACT_CLIPWAIT, ACT_KEYWAIT
, ACT_SLEEP, ACT_RANDOM
, ACT_GOTO, ACT_GOSUB, ACT_ONEXIT, ACT_HOTKEY, ACT_SETTIMER, ACT_CRITICAL, ACT_THREAD, ACT_RETURN, ACT_EXIT
, ACT_LOOP, ACT_FOR, ACT_WHILE, ACT_UNTIL, ACT_BREAK, ACT_CONTINUE // Keep LOOP, FOR, WHILE and UNTIL together and in this order for range checks in various places.
, ACT_TRY, ACT_CATCH, ACT_FINALLY, ACT_THROW // Keep TRY, CATCH and FINALLY together and in this order for range checks.
, ACT_BLOCK_BEGIN, ACT_BLOCK_END
, ACT_WINACTIVATE, ACT_WINACTIVATEBOTTOM
, ACT_WINWAIT, ACT_WINWAITCLOSE, ACT_WINWAITACTIVE, ACT_WINWAITNOTACTIVE
, ACT_WINMINIMIZE, ACT_WINMAXIMIZE, ACT_WINRESTORE
, ACT_WINHIDE, ACT_WINSHOW
, ACT_WINMINIMIZEALL, ACT_WINMINIMIZEALLUNDO
, ACT_WINCLOSE, ACT_WINKILL, ACT_WINMOVE, ACT_WINMENUSELECTITEM, ACT_PROCESS
, ACT_WINSET, ACT_WINSETTITLE, ACT_WINGETTITLE, ACT_WINGETCLASS, ACT_WINGET, ACT_WINGETPOS, ACT_WINGETTEXT
, ACT_SYSGET, ACT_POSTMESSAGE, ACT_SENDMESSAGE
// Keep rarely used actions near the bottom for parsing/performance reasons:
, ACT_PIXELGETCOLOR, ACT_PIXELSEARCH, ACT_IMAGESEARCH
, ACT_GROUPADD, ACT_GROUPACTIVATE, ACT_GROUPDEACTIVATE, ACT_GROUPCLOSE
, ACT_DRIVESPACEFREE, ACT_DRIVE, ACT_DRIVEGET
, ACT_SOUNDGET, ACT_SOUNDSET, ACT_SOUNDGETWAVEVOLUME, ACT_SOUNDSETWAVEVOLUME, ACT_SOUNDBEEP, ACT_SOUNDPLAY
, ACT_FILEAPPEND, ACT_FILEREAD, ACT_FILEREADLINE, ACT_FILEDELETE, ACT_FILERECYCLE, ACT_FILERECYCLEEMPTY
, ACT_FILEINSTALL, ACT_FILECOPY, ACT_FILEMOVE, ACT_FILECOPYDIR, ACT_FILEMOVEDIR
, ACT_FILECREATEDIR, ACT_FILEREMOVEDIR
, ACT_FILEGETATTRIB, ACT_FILESETATTRIB, ACT_FILEGETTIME, ACT_FILESETTIME
, ACT_FILEGETSIZE, ACT_FILEGETVERSION
, ACT_SETWORKINGDIR, ACT_FILESELECTFILE, ACT_FILESELECTFOLDER, ACT_FILEGETSHORTCUT, ACT_FILECREATESHORTCUT
, ACT_INIREAD, ACT_INIWRITE, ACT_INIDELETE
, ACT_REGREAD, ACT_REGWRITE, ACT_REGDELETE, ACT_SETREGVIEW
, ACT_OUTPUTDEBUG
, ACT_SETKEYDELAY, ACT_SETMOUSEDELAY, ACT_SETWINDELAY, ACT_SETCONTROLDELAY, ACT_SETBATCHLINES
, ACT_SETTITLEMATCHMODE, ACT_SETFORMAT, ACT_FORMATTIME
, ACT_SUSPEND, ACT_PAUSE
, ACT_AUTOTRIM, ACT_STRINGCASESENSE, ACT_DETECTHIDDENWINDOWS, ACT_DETECTHIDDENTEXT, ACT_BLOCKINPUT
, ACT_SETNUMLOCKSTATE, ACT_SETSCROLLLOCKSTATE, ACT_SETCAPSLOCKSTATE, ACT_SETSTORECAPSLOCKMODE
, ACT_KEYHISTORY, ACT_LISTLINES, ACT_LISTVARS, ACT_LISTHOTKEYS
, ACT_EDIT, ACT_RELOAD, ACT_MENU, ACT_GUI, ACT_GUICONTROL, ACT_GUICONTROLGET
, ACT_EXITAPP
, ACT_SHUTDOWN
, ACT_FILEENCODING
, ACT_HOTKEY_IF
// Make these the last ones before the count so they will be less often processed.  This helps
// performance because this one doesn't actually have a keyword so will never result
// in a match anyway.  UPDATE: No longer used because Run/RunWait is now required, which greatly
// improves syntax checking during load:
//, ACT_EXEC
// It's safer to use g_ActionCount, which is calculated immediately after the array is declared
// and initialized, at which time we know its true size.  However, the following lets us detect
// when the size of the array doesn't match the enum (in debug mode):
#ifdef _DEBUG
, ACT_COUNT
#endif
};

enum enum_act_old {
  OLD_INVALID = FAIL  // These should both be zero for initialization and function-return-value purposes.
  , OLD_SETENV, OLD_ENVADD, OLD_ENVSUB, OLD_ENVMULT, OLD_ENVDIV
  // ACT_IS_IF_OLD() relies on the items in this next line being adjacent to one another and in this order:
  , OLD_IFEQUAL, OLD_IFNOTEQUAL, OLD_IFGREATER, OLD_IFGREATEROREQUAL, OLD_IFLESS, OLD_IFLESSOREQUAL
  , OLD_WINGETACTIVETITLE, OLD_WINGETACTIVESTATS
};

// It seems best not to include ACT_SUSPEND in the below, since the user may have marked
// a large number of subroutines as "Suspend, Permit".  Even PAUSE is iffy, since the user
// may be using it as "Pause, off/toggle", but it seems best to support PAUSE because otherwise
// hotkey such as "#z::pause" would not be able to unpause the script if its MaxThreadsPerHotkey
// was 1 (the default).
#define ACT_IS_ALWAYS_ALLOWED(ActionType) (ActionType == ACT_EXITAPP || ActionType == ACT_PAUSE \
	|| ActionType == ACT_EDIT || ActionType == ACT_RELOAD || ActionType == ACT_KEYHISTORY \
	|| ActionType == ACT_LISTLINES || ActionType == ACT_LISTVARS || ActionType == ACT_LISTHOTKEYS)
#define ACT_IS_ASSIGN(ActionType) (ActionType <= ACT_ASSIGN_LAST && ActionType >= ACT_ASSIGN_FIRST) // Ordered for short-circuit performance.
#define ACT_IS_IF(ActionType) (ActionType >= ACT_FIRST_IF && ActionType <= ACT_LAST_IF)
#define ACT_IS_LOOP(ActionType) (ActionType >= ACT_LOOP && ActionType <= ACT_WHILE)
#define ACT_IS_LINE_PARENT(ActionType) (ACT_IS_IF(ActionType) || ActionType == ACT_ELSE \
	|| ACT_IS_LOOP(ActionType) || (ActionType >= ACT_TRY && ActionType <= ACT_FINALLY))
#define ACT_IS_IF_OLD(ActionType, OldActionType) (ActionType >= ACT_FIRST_IF_ALLOWING_SAME_LINE_ACTION && ActionType <= ACT_LAST_IF) \
	&& (ActionType < ACT_IFEQUAL || ActionType > ACT_IFLESSOREQUAL || (OldActionType >= OLD_IFEQUAL && OldActionType <= OLD_IFLESSOREQUAL))
	// All the checks above must be done so that cmds such as IfMsgBox (which are both "old" and "new")
	// can support parameters on the same line or on the next line.  For example, both of the above are allowed:
	// IfMsgBox, No, Gosub, XXX
	// IfMsgBox, No
	//     Gosub, XXX

// For convenience in many places.  Must cast to int to avoid loss of negative values.
#define BUF_SPACE_REMAINING ((int)(aBufSize - (aBuf - aBuf_orig)))

// MsgBox timeout value.  This can't be zero because that is used as a failure indicator:
// Also, this define is in this file to prevent problems with mutual
// dependency between script.h and window.h.  Update: It can't be -1 either because
// that value is used to indicate failure by DialogBox():
#define AHK_TIMEOUT -2
// And these to prevent mutual dependency problem between window.h and globaldata.h:
#define MAX_MSGBOXES 7 // Probably best not to change this because it's used by OurTimers to set the timer IDs, which should probably be kept the same for backward compatibility.
#define MAX_INPUTBOXES 4
#define MAX_PROGRESS_WINDOWS 10  // Allow a lot for downloads and such.
#define MAX_PROGRESS_WINDOWS_STR _T("10") // Keep this in sync with above.
#define MAX_SPLASHIMAGE_WINDOWS 10
#define MAX_SPLASHIMAGE_WINDOWS_STR _T("10") // Keep this in sync with above.
#define MAX_MSG_MONITORS 500

// IMPORTANT: Before ever changing the below, note that it will impact the IDs of menu items created
// with the MENU command, as well as the number of such menu items that are possible (currently about
// 65500-11000=54500). See comments at ID_USER_FIRST for details:
#define GUI_CONTROL_BLOCK_SIZE 1000
#define MAX_CONTROLS_PER_GUI (GUI_CONTROL_BLOCK_SIZE * 11) // Some things rely on this being less than 0xFFFF and an even multiple of GUI_CONTROL_BLOCK_SIZE.
#define NO_CONTROL_INDEX MAX_CONTROLS_PER_GUI // Must be 0xFFFF or less.
#define NO_EVENT_INFO 0 // For backward compatibility with documented contents of A_EventInfo, this should be kept as 0 vs. something more special like UINT_MAX.

#define MAX_TOOLTIPS 20
#define MAX_TOOLTIPS_STR _T("20")   // Keep this in sync with above.
#define MAX_FILEDIALOGS 4
#define MAX_FOLDERDIALOGS 4

#define MAX_NUMBER_LENGTH 255                   // Large enough to allow custom zero or space-padding via %10.2f, etc.
#define MAX_NUMBER_SIZE (MAX_NUMBER_LENGTH + 1) // But not too large because some things might rely on this being fairly small.
#define MAX_INTEGER_LENGTH 20                     // Max length of a 64-bit number when expressed as decimal or
#define MAX_INTEGER_SIZE (MAX_INTEGER_LENGTH + 1) // hex string; e.g. -9223372036854775808 or (unsigned) 18446744073709551616 or (hex) -0xFFFFFFFFFFFFFFFF.

// Hot-strings:
// memmove() and proper detection of long hotstrings rely on buf being at least this large:
#define HS_BUF_SIZE (MAX_HOTSTRING_LENGTH * 2 + 10)
#define HS_BUF_DELETE_COUNT (HS_BUF_SIZE / 2)
#define HS_MAX_END_CHARS 100

// Bitwise storage of boolean flags.  This section is kept in this file because
// of mutual dependency problems between hook.h and other header files:
typedef UCHAR HookType;
#define HOOK_KEYBD 0x01
#define HOOK_MOUSE 0x02
#define HOOK_FAIL  0xFF

/*
#define EXTERN_G extern global_struct *g
#define EXTERN_OSVER extern OS_Version g_os
#define EXTERN_CLIPBOARD extern Clipboard g_clip
#define EXTERN_SCRIPT extern Script g_script
#define CLOSE_CLIPBOARD_IF_OPEN	if (g_clip.mIsOpen) g_clip.Close()
#define CLIPBOARD_CONTAINS_ONLY_FILES (!IsClipboardFormatAvailable(CF_NATIVETEXT) && IsClipboardFormatAvailable(CF_HDROP))
*/

// These macros used to keep app responsive during a long operation.  In v1.0.39, the
// hooks have a dedicated thread.  However, mLastPeekTime is still compared to 5 rather
// than some higher value for the following reasons:
// 1) Want hotkeys pressed during a long operation to take effect as quickly as possible.
//    For example, in games a hotkey's response time is critical.
// 2) Benchmarking shows less than a 0.5% performance improvement from this comparing
//    to a higher value (even one as high as 500), even when the system is under heavy
//    load from other processes).
//
// mLastPeekTime is global/static so that recursive functions, such as FileSetAttrib(),
// will sleep as often as intended even if the target files require frequent recursion.
// The use of a global/static is not friendly to recursive calls to the function (i.e. calls
// made as a consequence of the current script subroutine being interrupted by another during
// this instance's MsgSleep()).  However, it doesn't seem to be that much of a consequence
// since the exact interval/period of the MsgSleep()'s isn't that important.  It's also
// pretty unlikely that the interrupting subroutine will also just happen to call the same
// function rather than some other.
//
// Older comment that applies if there is ever again no dedicated thread for the hooks:
// These macros were greatly simplified when it was discovered that PeekMessage(), when called
// directly as below, is enough to prevent keyboard and mouse lag when the hooks are installed
#define LONG_OPERATION_INIT MSG msg; DWORD tick_now; DWORD LastPeekTime = GetTickCount(); DWORD PeekFrequency = 5;

// MsgSleep() is used rather than SLEEP_WITHOUT_INTERRUPTION to allow other hotkeys to
// launch and interrupt (suspend) the operation.  It seems best to allow that, since
// the user may want to press some fast window activation hotkeys, for example, during
// the operation.  The operation will be resumed after the interrupting subroutine finishes.
// Notes applying to the macro:
// Store tick_now for use later, in case the Peek() isn't done, though not all callers need it later.
// ...
// Since the Peek() will yield when there are no messages, it will often take 20ms or more to return
// (UPDATE: this can't be reproduced with simple tests, so either the OS has changed through service
// packs, or Peek() yields only when the OS detects that the app is calling it too often or calling
// it in certain ways [PM_REMOVE vs. PM_NOREMOVE seems to make no difference: either way it doesn't yield]).
// Therefore, must update tick_now again (its value is used by macro and possibly by its caller)
// to avoid having to Peek() immediately after the next iteration.
// ...
// The code might bench faster when "g_script.mLastPeekTime = tick_now" is a separate operation rather
// than combined in a chained assignment statement.

#define SLEEP(x) \
{\
	LONG_OPERATION_INIT \
	int nSleepIdx = 0; \
	while(++nSleepIdx <= x){ \
		tick_now = GetTickCount();\
		if (tick_now - LastPeekTime > PeekFrequency)\
		{\
			if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){	TranslateMessage(&msg); DispatchMessage(&msg);}\
			tick_now = GetTickCount();\
			LastPeekTime = tick_now;\
		}\
		Sleep(1);\
	}\
}



#define LONG_OPERATION_UPDATE \
{\
	tick_now = GetTickCount();\
	if (tick_now - LastPeekTime > PeekFrequency)\
	{\
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){	TranslateMessage(&msg); DispatchMessage(&msg);}\
		tick_now = GetTickCount();\
		LastPeekTime = tick_now;\
	}\
	Sleep(1);\
}

// Same as the above except for SendKeys() and related functions (uses SLEEP_WITHOUT_INTERRUPTION vs. MsgSleep).
#define LONG_OPERATION_UPDATE_FOR_SENDKEYS \
{\
	tick_now = GetTickCount();\
	if (tick_now - LastPeekTime > PeekFrequency)\
	{\
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){	TranslateMessage(&msg); DispatchMessage(&msg);}\
		tick_now = GetTickCount();\
		LastPeekTime = tick_now;\
		Sleep(1);\
	}\
}

// Defining these here avoids awkwardness due to the fact that globaldata.cpp
// does not (for design reasons) include globaldata.h:
typedef UCHAR ActionTypeType; // If ever have more than 256 actions, will have to change this (but it would increase code size due to static data in g_act).
#pragma pack(push, 1) // v1.0.45: Reduces code size a little without impacting runtime performance because this struct is hardly ever accessed during runtime.
struct Action
{
	LPTSTR Name;
	// Just make them int's, rather than something smaller, because the collection
	// of actions will take up very little memory.  Int's are probably faster
	// for the processor to access since they are the native word size, or something:
	// Update for v1.0.40.02: Now that the ARGn macros don't check mArgc, MaxParamsAu2WithHighBit
	// is needed to allow MaxParams to stay pure, which in turn prevents Line::Perform()
	// from accessing a NULL arg in the sArgDeref array (i.e. an arg that exists only for
	// non-AutoIt2 scripts, such as the extra ones in StringGetPos).
	// Also, changing these from ints to chars greatly reduces code size since this struct
	// is used by g_act to build static data into the code.  Testing shows that the compiler
	// will generate a warning even when not in debug mode in the unlikely event that a constant
	// larger than 127 is ever stored in one of these:
	char MinParams, MaxParams, MaxParamsAu2WithHighBit;
	// Array indicating which args must be purely numeric.  The first arg is
	// number 1, the second 2, etc (i.e. it doesn't start at zero).  The list
	// is ended with a zero, much like a string.  The compiler will notify us
	// (verified) if MAX_NUMERIC_PARAMS ever needs to be increased:
	#define MAX_NUMERIC_PARAMS 7
	ActionTypeType NumericParams[MAX_NUMERIC_PARAMS];
};
#pragma pack(pop)

// Values are hard-coded for some of the below because they must not deviate from the documented, numerical
// TitleMatchModes:
enum TitleMatchModes {MATCHMODE_INVALID = FAIL, FIND_IN_LEADING_PART = 1, FIND_ANYWHERE = 2, FIND_EXACT = 3, FIND_REGEX, FIND_FAST, FIND_SLOW};

typedef UINT GuiIndexType; // Some things rely on it being unsigned to avoid the need to check for less-than-zero.
typedef UINT GuiEventType; // Made a UINT vs. enum so that illegal/underflow/overflow values are easier to detect.

// The following array and enum must be kept in sync with each other:
#define GUI_EVENT_NAMES {_T(""), _T("Normal"), _T("DoubleClick"), _T("RightClick"), _T("ColClick")}
enum GuiEventTypes {GUI_EVENT_NONE  // NONE must be zero for any uses of ZeroMemory(), synonymous with false, etc.
	, GUI_EVENT_NORMAL, GUI_EVENT_DBLCLK // Try to avoid changing this and the other common ones in case anyone automates a script via SendMessage (though that does seem very unlikely).
	, GUI_EVENT_RCLK, GUI_EVENT_COLCLK
	, GUI_EVENT_FIRST_UNNAMED  // This item must always be 1 greater than the last item that has a name in the GUI_EVENT_NAMES array below.
	, GUI_EVENT_DROPFILES = GUI_EVENT_FIRST_UNNAMED
	, GUI_EVENT_CLOSE, GUI_EVENT_ESCAPE, GUI_EVENT_RESIZE, GUI_EVENT_CONTEXTMENU
	, GUI_EVENT_DIGIT_0 = 48}; // Here just as a reminder that this value and higher are reserved so that a single printable character or digit (mnemonic) can be sent, and also so that ListView's "I" notification can add extra data into the high-byte (which lies just to the left of the "I" character in the bitfield).

typedef USHORT CoordModeType;

// Bit-field offsets:
#define COORD_MODE_PIXEL   0
#define COORD_MODE_MOUSE   2
#define COORD_MODE_TOOLTIP 4
#define COORD_MODE_CARET   6
#define COORD_MODE_MENU    8

#define COORD_MODE_WINDOW  0
#define COORD_MODE_CLIENT  1
#define COORD_MODE_SCREEN  2
#define COORD_MODE_MASK    3
#define COORD_MODES { _T("Window"), _T("Client"), _T("Screen") }

#define COORD_CENTERED (INT_MIN + 1)
#define COORD_UNSPECIFIED INT_MIN
#define COORD_UNSPECIFIED_SHORT SHRT_MIN  // This essentially makes coord -32768 "reserved", but it seems acceptable given usefulness and the rarity of a real coord like that.

typedef UINT_PTR EventInfoType;

typedef UCHAR SendLevelType;
// Setting the max level to 100 is somewhat arbitrary. It seems that typical usage would only
// require a few levels at most. We do want to keep the max somewhat small to keep the range
// for magic values that get used in dwExtraInfo to a minimum, to avoid conflicts with other
// apps that may be using the field in other ways.
const SendLevelType SendLevelMax = 100;
// Using int as the type for level so this can be used as validation before converting to SendLevelType.
inline bool SendLevelIsValid(int level) { return level >= 0 && level <= SendLevelMax; }

// Create some "fake" virtual keys to simplify sections of the code.
// According to winuser.h, the following ranges (among others)
// are considered "unassigned" rather than "reserved", so should be
// fairly safe to use for the foreseeable future.  0xFF should probably
// be avoided since it's sometimes used as a failure indictor by API
// calls.  And 0x00 should definitely be avoided because it is used
// to indicate failure by many functions that deal with virtual keys.
// 0x88 - 0x8F : unassigned
// 0x97 - 0x9F : unassigned (this range seems less likely to be used)
#define VK_NEW_MOUSE_FIRST 0x9A
#define VK_LBUTTON_LOGICAL 0x9A // v1.0.43: Added to support swapping of left/right mouse buttons in Control Panel.
#define VK_RBUTTON_LOGICAL 0x9B //
#define VK_WHEEL_LEFT      0x9C // v1.0.48: Lexikos: Fake virtual keys for support for horizontal scrolling in
#define VK_WHEEL_RIGHT     0x9D // Windows Vista and later.
#define VK_WHEEL_DOWN      0x9E
#define VK_WHEEL_UP        0x9F
#define IS_WHEEL_VK(aVK) ((aVK) >= VK_WHEEL_LEFT && (aVK) <= VK_WHEEL_UP)
#define VK_NEW_MOUSE_LAST  0x9F




// These are the only keys for which another key with the same VK exists.  Therefore, use scan code for these.
// If use VK for some of these (due to them being more likely to be used as hotkeys, thus minimizing the
// use of the keyboard hook), be sure to use SC for its counterpart.
// Always use the compressed version of scancode, i.e. 0x01 for the high-order byte rather than vs. 0xE0.
#define SC_NUMPADENTER 0x11C
#define SC_INSERT 0x152
#define SC_DELETE 0x153
#define SC_HOME 0x147
#define SC_END 0x14F
#define SC_UP 0x148
#define SC_DOWN 0x150
#define SC_LEFT 0x14B
#define SC_RIGHT 0x14D
#define SC_PGUP 0x149
#define SC_PGDN 0x151

// These are the same scan codes as their counterpart except the extended flag is 0 rather than
// 1 (0xE0 uncompressed):
#define SC_ENTER 0x1C
// In addition, the below dual-state numpad keys share the same scan code (but different vk's)
// regardless of the state of numlock:
#define SC_NUMPADDEL 0x53
#define SC_NUMPADINS 0x52
#define SC_NUMPADEND 0x4F
#define SC_NUMPADHOME 0x47
#define SC_NUMPADCLEAR 0x4C
#define SC_NUMPADUP 0x48
#define SC_NUMPADDOWN 0x50
#define SC_NUMPADLEFT 0x4B
#define SC_NUMPADRIGHT 0x4D
#define SC_NUMPADPGUP 0x49
#define SC_NUMPADPGDN 0x51

#define SC_NUMPADDOT SC_NUMPADDEL
#define SC_NUMPAD0 SC_NUMPADINS
#define SC_NUMPAD1 SC_NUMPADEND
#define SC_NUMPAD2 SC_NUMPADDOWN
#define SC_NUMPAD3 SC_NUMPADPGDN
#define SC_NUMPAD4 SC_NUMPADLEFT
#define SC_NUMPAD5 SC_NUMPADCLEAR
#define SC_NUMPAD6 SC_NUMPADRIGHT
#define SC_NUMPAD7 SC_NUMPADHOME
#define SC_NUMPAD8 SC_NUMPADUP
#define SC_NUMPAD9 SC_NUMPADPGUP

// These both have a unique vk and a unique sc (on most keyboards?), but they're listed here because
// MapVirtualKey doesn't support them under Win9x (except maybe NumLock itself):
#define SC_NUMLOCK 0x145
#define SC_NUMPADDIV 0x135
#define SC_NUMPADMULT 0x037
#define SC_NUMPADSUB 0x04A
#define SC_NUMPADADD 0x04E

// Note: A KeyboardProc() (hook) actually receives 0x36 for RSHIFT under both WinXP and Win98se, not 0x136.
// All the below have been verified to be accurate under Win98se and XP (except rctrl and ralt in XP).
#define SC_LCONTROL 0x01D
#define SC_RCONTROL 0x11D
#define SC_LSHIFT 0x02A
#define SC_RSHIFT 0x136 // Must be extended, at least on WinXP, or there will be problems, e.g. SetModifierLRState().
#define SC_LALT 0x038
#define SC_RALT 0x138
#define SC_LWIN 0x15B
#define SC_RWIN 0x15C

#define SC_APPSKEY 0x15D

#define DEFAULT_MOUSE_SPEED 2
#define MAX_MOUSE_SPEED 100

typedef UCHAR modLR_type; // Only the left-right win/alt/ctrl/shift rather than their generic counterparts.
#define MODLR_MAX 0xFF
#define MODLR_COUNT 8
#define MOD_LCONTROL 0x01
#define MOD_RCONTROL 0x02
#define MOD_LALT 0x04
#define MOD_RALT 0x08
#define MOD_LSHIFT 0x10
#define MOD_RSHIFT 0x20
#define MOD_LWIN 0x40
#define MOD_RWIN 0x80

#define IsKeyDown9xNT(vk) ((GetAsyncKeyState(vk) & 0x8000) || ((GetKeyState(vk) & 0x8000)))
#define IsKeyDown2kXP(vk) (GetKeyState(vk) & 0x8000)
#define IsKeyDownAsync(vk) (GetAsyncKeyState(vk) & 0x8000)
#define IsKeyToggledOn(vk) (GetKeyState(vk) & 0x01)

#define MAX_INITIAL_EVENTS_SI 500UL  // sizeof(INPUT) == 28 as of 2006. Since Send is called so often, and since most Sends are short, reducing the load on the stack is also a deciding factor for these.
#define MAX_INITIAL_EVENTS_PB 1500UL // sizeof(PlaybackEvent) == 8, so more events are justified before resorting to malloc().

#define STD_MODS_TO_DISGUISE (MOD_LALT|MOD_RALT|MOD_LWIN|MOD_RWIN)

#define KEY_IGNORE 0xFFC3D44F
#define KEY_PHYS_IGNORE (KEY_IGNORE - 1)  // Same as above but marked as physical for other instances of the hook.
#define KEY_IGNORE_ALL_EXCEPT_MODIFIER (KEY_IGNORE - 2) 

#define KEY_IGNORE_LEVEL(LEVEL) (KEY_IGNORE_ALL_EXCEPT_MODIFIER - LEVEL)
#define KEY_IGNORE_MIN KEY_IGNORE_LEVEL(SendLevelMax)
#define KEY_IGNORE_MAX KEY_IGNORE // There are two extra values above KEY_IGNORE_LEVEL(0)

enum KeyStateTypes {KEYSTATE_LOGICAL, KEYSTATE_PHYSICAL, KEYSTATE_TOGGLE}; // For use with GetKeyJoyState(), etc.
enum KeyEventTypes {KEYDOWN, KEYUP, KEYDOWNANDUP};


typedef USHORT sc_type; // Scan code.
typedef UCHAR vk_type;  // Virtual key.
typedef UINT mod_type;  // Standard Windows modifier type for storing MOD_CONTROL, MOD_WIN, MOD_ALT, MOD_SHIFT.

struct PlaybackEvent
{
	UINT message;
	union
	{
		struct
		{
			sc_type sc; // Placed above vk for possibly better member stacking/alignment.
			vk_type vk;
		};
		struct
		{
			// Screen coordinates, which can be negative.  SHORT vs. INT is used because the likelihood
			// have having a virtual display surface wider or taller than 32,767 seems too remote to
			// justify increasing the struct size, which would impact the stack space and dynamic memory
			// used by every script every time it uses the playback method to send keystrokes or clicks.
			// Note: WM_LBUTTONDOWN uses WORDs vs. SHORTs, but they're not really comparable because
			// journal playback/record both use screen coordinates but WM_LBUTTONDOWN et. al. use client
			// coordinates.
			SHORT x;
			SHORT y;
		};
		DWORD time_to_wait; // This member is present only when message==0; otherwise, a struct is present.
	};
};


static modLR_type sModifiersLR_persistent;
static bool StoreCapslockMode;
static UINT sEventCount, sMaxEvents;

static LPINPUT sEventSI;        // No init necessary.  An array that's allocated/deallocated by SendKeys().
static PlaybackEvent *&sEventPB = (PlaybackEvent *&)sEventSI;
static UINT sCurrentEvent;
static modLR_type sEventModifiersLR; // Tracks the modifier state to following the progress/building of the SendInput array.
static POINT sSendInputCursorPos;    // Tracks/predicts cursor position as SendInput array is built.
static HookType sHooksToRemoveDuringSendInput;
static SendModes sSendMode = SM_EVENT; // Whether a SendInput or Hook array is currently being constructed.
static bool sAbortArraySend;         // No init needed.
static bool sFirstCallForThisEvent;  //
static bool sInBlindMode;            //
static DWORD sThisEventTime;         //










// Same reason as above struct.  It's best to keep this struct as small as possible
// because it's used as a local (stack) var by at least one recursive function:
// Each instance of this struct generally corresponds to a quasi-thread.  The function that creates
// a new thread typically saves the old thread's struct values on its stack so that they can later
// be copied back into the g struct when the thread is resumed:



#define ERRORLEVEL_SAVED_SIZE 128 // The size that can be remembered (saved & restored) if a thread is interrupted. Big in case user put something bigger than a number in g_ErrorLevel.

#ifdef UNICODE
#define WINAPI_SUFFIX "W"
#define PROCESS_API_SUFFIX "W" // used by Process32First and Process32Next
#else
#define WINAPI_SUFFIX "A"
#define PROCESS_API_SUFFIX
#endif

#define _TSIZE(a) ((a)*sizeof(TCHAR))
#define CP_AHKNOBOM 0x80000000
#define CP_AHKCP    (~CP_AHKNOBOM)

// Use #pragma message(MY_WARN(nnnn) "warning messages") to generate a warning like a compiler's warning
#define __S(x) #x
#define _S(x) __S(x)
#define MY_WARN(n) __FILE__ "(" _S(__LINE__) ") : warning C" __S(n) ": "

// These will be removed when things are done.
#ifdef CONFIG_UNICODE_CHECK
#define UNICODE_CHECK __declspec(deprecated(_T("Please check what you want are bytes or characters.")))
UNICODE_CHECK inline size_t CHECK_SIZEOF(size_t n) { return n; }
#define SIZEOF(c) CHECK_SIZEOF(sizeof(c))
#pragma deprecated(memcpy, memset, memmove, malloc, realloc, _alloca, alloca, toupper, tolower)
#else
#define UNICODE_CHECK
#endif

#endif
