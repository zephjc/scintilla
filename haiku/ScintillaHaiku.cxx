// Scintilla source code edit control
// ScintillaHaiku.cxx - Haiku specific subclass of ScintillaBase
// Copyright 2011 by Andrea Anzani <andrea.anzani@gmail.com>
// Copyright 2014-2015 by Kacper Kasper <kacperkasper@gmail.com>
// The License.txt file describes the conditions under which this software may be distributed.
// IME support implementation based on an excellent article:
// https://www.haiku-os.org/documents/dev/developing_ime_aware_applications
// with portions from BTextView.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include <assert.h>

#include <map>
#include <vector>
#include <algorithm>

#include <Bitmap.h>
#include <Clipboard.h>
#include <Input.h>
#include <MenuItem.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Picture.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <String.h>
#include <SupportDefs.h>
#include <View.h>
#include <Window.h>

#include "Platform.h"
#include "Scintilla.h"
#include "ScintillaView.h"

#include "ILexer.h"
#ifdef SCI_LEXER
#include "SciLexer.h"
#include "PropSetSimple.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "LexerModule.h"
#include "ExternalLexer.h"
#endif
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "AutoComplete.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "CaseFolder.h"
#include "Decoration.h"
#include "Document.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"
#include "ScintillaBase.h"

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

static const int32 sTickMessage = 'TCKM';
static const BString sMimeRectangularMarker("text/x-rectangular-marker");

class CallTipView : public BView {
public:
	CallTipView(BRect rect, BView *parent, CallTip *ct);
	
	void Draw(BRect updateRect);

private:
	BView *fParent;
	CallTip *fCT;
};

CallTipView::CallTipView(BRect rect, BView *parent, CallTip *ct):
	BView(rect, "callTipView", 0, B_WILL_DRAW), fParent(parent), fCT(ct) {}

void CallTipView::Draw(BRect updateRect) {
	Surface *surfaceWindow = Surface::Allocate(SC_TECHNOLOGY_DEFAULT);
	if (surfaceWindow) {
		surfaceWindow->Init(static_cast<BView*>(this), static_cast<BView*>(fParent));
		fCT->PaintCT(surfaceWindow);
		surfaceWindow->Release();
		delete surfaceWindow;
	}
}

class ScintillaHaiku : public ScintillaBase, public BView {
public:
	ScintillaHaiku();
	ScintillaHaiku(BRect rect);

	// functions from Scintilla:
	void Initialise();
	void Finalise();
	void SetVerticalScrollPos();
	void SetHorizontalScrollPos();
	bool ModifyScrollBars(int, int);
	void Copy();
	void Paste();
	void ClaimSelection();
	void NotifyChange();
	void NotifyParent(SCNotification);
	void CopyToClipboard(const SelectionText& selectedText);
	bool FineTickerAvailable();
	bool FineTickerRunning(TickReason reason);
	void FineTickerStart(TickReason reason, int millis, int tolerance);
	void FineTickerCancel(TickReason reason);
	void SetMouseCapture(bool);
	bool HaveMouseCapture();
	sptr_t WndProc(unsigned int, uptr_t, sptr_t);
	sptr_t DefWndProc(unsigned int, uptr_t, sptr_t);
	void CreateCallTipWindow(PRectangle rect);
	void AddToPopUp(const char *label, int cmd, bool enabled);
	void StartDrag();
	
	bool Drop(BMessage* message, const BPoint& point, const BPoint& offset, bool move);
	
	void Draw(BRect rect);
	void KeyDown(const char* bytes, int32 len);
	void AttachedToWindow();
	void MakeFocus(bool focus = true);
	void MouseDown(BPoint point);
	void MouseUp(BPoint point);
	void MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	void FrameResized(float width, float height);
	void WindowActivated(bool active);

	void MessageReceived(BMessage *msg);
	void ScrollTo(BPoint p);

private:
	bool capturedMouse;
	bool wordwiseNavigationShortcuts;
	bool wordwiseSelectionShortcuts;
	BMessageRunner* timers[tickDwell + 1];

	void _Activate();
	void _Deactivate();

	void _IMStarted(BMessage* message);
	void _IMStopped();
	void _IMChanged(BMessage* message);
	void _IMLocationRequest(BMessage* message);
	void _IMCancel();

	BMessenger msgr_input;
	bool im_started;
	int32 im_pos;
	int32 im_length;
	const char* im_string;
};

ScintillaHaiku::ScintillaHaiku():
	BView("ScintillaHaikuView", B_WILL_DRAW | B_FRAME_EVENTS) {
	capturedMouse = false;
	
	Initialise();
}

ScintillaHaiku::ScintillaHaiku(BRect rect):
	BView(rect, "ScintillaHaikuView", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS) {
    capturedMouse = false;
    
	Initialise();
}

void ScintillaHaiku::_IMStarted(BMessage* message) {
	im_started = (message->FindMessenger("be:reply_to", &msgr_input) == B_OK);
	im_pos = CurrentPosition();
	im_length = 0;
	im_string = NULL;
}

void ScintillaHaiku::_IMStopped() {
	msgr_input = BMessenger();
	im_started = false;
	im_pos = -1;
	im_length = -1;
	im_string = NULL;
}

void ScintillaHaiku::_IMChanged(BMessage* message) {
	if(!im_started) return;
	
	im_string = NULL;
	message->FindString("be:string", &im_string);
	if(im_string == NULL) im_string = "";
	WndProc(SCI_DELETERANGE, (uptr_t) im_pos, (sptr_t) im_length);
	im_length = strlen(im_string);
	
	if(message->GetBool("be:confirmed")) {
		const char* currPos = im_string;
		const char* prevPos = currPos;
		
		while(*currPos != '\0') {
			if((*currPos & 0xC0) == 0xC0) {
				++currPos;
				while((*currPos & 0xC0) == 0x80)
					++currPos;
			} else if((*currPos & 0xC0) == 0x80) {
				prevPos = ++currPos;
			} else {
				++currPos;
			}
			KeyDown(prevPos, currPos - prevPos);
			prevPos = currPos;
		}
	} else {
		WndProc(SCI_INSERTTEXT, im_pos, (sptr_t) im_string);
		WndProc(SCI_SETCURRENTPOS, im_pos + im_length, 0);

		int32 start = 0, end = 0;
		for(int32 i = 0; message->FindInt32("be:clause_start", i, &start) == B_OK &&
						message->FindInt32("be:clause_end", i, &end) == B_OK; i++) {
			if(end > start) {
				WndProc(SCI_STARTSTYLING, im_pos + start, 0);
				WndProc(SCI_SETSTYLING, end - start, 253);
			}
		}
		
		for(int32 i = 0; message->FindInt32("be:selection", i * 2, &start) == B_OK &&
						message->FindInt32("be:selection", i * 2 + 1, &end) == B_OK; i++) {
			if(end > start) {
				WndProc(SCI_STARTSTYLING, im_pos + start, 0);
				WndProc(SCI_SETSTYLING, end - start, 254);
			}
		}
	}
}

int32 UTF8NextChar(const char* str, uint32 offset) {
	if(offset >= strlen(str))
		return offset;
		
	for(++offset; (str[offset] & 0xC0) == 0x80; ++offset)
		;
	
	return offset;
}

void ScintillaHaiku::_IMLocationRequest(BMessage* message) {
	if(!im_started) return;
	
	if(im_string == NULL || *im_string == 0) return;
	
	BMessage reply(B_INPUT_METHOD_EVENT);
	reply.AddInt32("be:opcode", B_INPUT_METHOD_LOCATION_REQUEST);
	
	BPoint left_top = ConvertToScreen(BPoint(0, 0));
	uint32 offset = 0;
	uint32 limit = strlen(im_string);
	
	while(offset < limit) {
		Point loc = LocationFromPosition(im_pos + offset);
		BPoint pt = left_top;
		pt.x += loc.x;
		pt.y += loc.y;
		reply.AddPoint("be:location_reply", pt);
		reply.AddFloat("be:height_reply", vs.lineHeight);
		
		offset = UTF8NextChar(im_string, offset);
	}
	
	msgr_input.SendMessage(&reply);
}

void ScintillaHaiku::_IMCancel() {
	if(im_started == true) {
		BMessage message(B_INPUT_METHOD_EVENT);
		message.AddInt32("be:opcode", B_INPUT_METHOD_STOPPED);
		msgr_input.SendMessage(&message);
	}
}

void ScintillaHaiku::Initialise() {
	for (TickReason tr = tickCaret; tr <= tickDwell; tr = static_cast<TickReason>(tr + 1)) {
		timers[tr] = NULL;
	}

	wordwiseNavigationShortcuts = false;
	wordwiseSelectionShortcuts = false;

	if(imeInteraction == imeInline)
		SetFlags(Flags() | B_INPUT_METHOD_AWARE);

	im_started = false;
	im_pos = -1;

	WndProc(SCI_STYLESETFORE, 253, 0xFF0000);
	WndProc(SCI_STYLESETFORE, 254, 0x0000FF);
}

void ScintillaHaiku::Finalise() {
	for (TickReason tr = tickCaret; tr <= tickDwell; tr = static_cast<TickReason>(tr + 1)) {
		FineTickerCancel(tr);
	}
	ScintillaBase::Finalise();
}

int ConvertSingleKey(char beos) {
	int sck_key = -1;
	switch(beos) {
		case B_RIGHT_ARROW: sck_key = SCK_RIGHT; break;
		case B_LEFT_ARROW: sck_key = SCK_LEFT; break;
		case B_UP_ARROW: sck_key = SCK_UP; break;
		case B_DOWN_ARROW: sck_key = SCK_DOWN; break;
		case B_BACKSPACE: sck_key = SCK_BACK; break;
		case B_RETURN: sck_key = SCK_RETURN; break;
		case B_TAB: sck_key = SCK_TAB; break;
		case B_ESCAPE: sck_key = SCK_ESCAPE; break;
		case B_INSERT: sck_key = SCK_INSERT; break;
		case B_DELETE: sck_key = SCK_DELETE; break;
		case B_HOME: sck_key = SCK_HOME; break;
		case B_END: sck_key = SCK_END; break;
		case B_PAGE_UP: sck_key = SCK_PRIOR; break;
		case B_PAGE_DOWN: sck_key = SCK_NEXT; break;
		case B_FUNCTION_KEY: break;
		default: sck_key = (int) beos; break;
	}
 	return sck_key;
}

void ScintillaHaiku::SetVerticalScrollPos() {
	if(LockLooper()) {
		ScrollBar(B_VERTICAL)->SetValue(topLine + 1);
		UnlockLooper();
	}
}

void ScintillaHaiku::SetHorizontalScrollPos() {
	if(LockLooper()) {
 		ScrollBar(B_HORIZONTAL)->SetValue(xOffset + 1);
 		UnlockLooper();
	}
}

bool ScintillaHaiku::ModifyScrollBars(int nMax, int nPage) {
 	int pageScroll = LinesToScroll();
 	PRectangle rcText = GetTextRectangle();
	int horizEndPreferred = scrollWidth;
	if (horizEndPreferred < 0)
		horizEndPreferred = 0;
	unsigned int pageWidth = rcText.Width();
	unsigned int pageIncrement = pageWidth / 3;
	unsigned int charWidth = vs.styles[STYLE_DEFAULT].aveCharWidth;

	if(LockLooper()) {
		ScrollBar(B_HORIZONTAL)->SetRange(1, horizEndPreferred + 1);
		ScrollBar(B_HORIZONTAL)->SetSteps(charWidth, pageIncrement);
		ScrollBar(B_VERTICAL)->SetRange(1, nMax - pageScroll + 1);
		UnlockLooper();
		return true;
	}
	return false;
}

void ScintillaHaiku::Copy() {
	if(!sel.Empty()) {
		SelectionText st;
		CopySelectionRange(&st);
		CopyToClipboard(st);
	}
}

void ScintillaHaiku::Paste() {
	const char* textPointer;
	char* text = NULL;
	ssize_t textLen = 0;
	bool isRectangular = false;
	BMessage* clip = NULL;
	if(be_clipboard->Lock()) {
		if((clip = be_clipboard->Data())) {
			isRectangular = (clip->FindData(sMimeRectangularMarker,
				B_MIME_TYPE, (const void **) &textPointer, &textLen) == B_OK);
			
			if(isRectangular == false) {
				clip->FindData("text/plain", B_MIME_TYPE,
					(const void **)&textPointer, &textLen);
			}
			
			text = new char[textLen];
			memcpy(text, textPointer, textLen);
		}
		be_clipboard->Unlock();
	}

	UndoGroup ug(pdoc);
	ClearSelection(multiPasteMode == SC_MULTIPASTE_EACH);
	InsertPasteShape(text, textLen, isRectangular ? pasteRectangular : pasteStream);
	EnsureCaretVisible();
	delete text;
}

void ScintillaHaiku::ClaimSelection() {
 	// Haiku doesn't have 'primary selection'
}

void ScintillaHaiku::NotifyChange() {
	// We don't send anything here
}

void ScintillaHaiku::NotifyParent(SCNotification n) {
 	BMessage message(B_SCINTILLA_NOTIFICATION);
 	message.AddData("notification", B_ANY_TYPE, &n, sizeof(SCNotification));
 	BMessenger(this).SendMessage(&message);
}

void ScintillaHaiku::MessageReceived(BMessage* msg) {
	if(msg->WasDropped()) {
		BPoint dropOffset;
		BPoint dropPoint = msg->DropPoint(&dropOffset);
		ConvertFromScreen(&dropPoint);
		
		void* from = NULL;
		bool move = false;
		if (msg->FindPointer("be:originator", &from) == B_OK
				&& from == this)
			move = true;
		
		if(!Drop(msg, dropPoint, dropOffset, move)) {
			BView::MessageReceived(msg);
		}
	}
	
	switch(msg->what) {
	//case B_SIMPLE_DATA: {
		
	//} break;
	case B_INPUT_METHOD_EVENT: {
		int32 opcode;
		if(msg->FindInt32("be:opcode", &opcode) != B_OK) break;

		switch(opcode) {
		case B_INPUT_METHOD_STARTED:
			_IMStarted(msg);
			break;
		case B_INPUT_METHOD_STOPPED:
			_IMStopped();
			break;
		case B_INPUT_METHOD_CHANGED:
			_IMChanged(msg);
			break;
		case B_INPUT_METHOD_LOCATION_REQUEST:
			_IMLocationRequest(msg);
			break;
		default:
			BView::MessageReceived(msg);
		}
	} break;
	case 'nava': {
		int32 key = msg->GetInt32("key", 0);
		int32 modifiers = msg->GetInt32("modifiers", 0);
		bool shift = (modifiers & B_SHIFT_KEY);
		bool ctrl = (modifiers & B_CONTROL_KEY);
		bool alt = (modifiers & B_COMMAND_KEY);

		bool consumed;
		Editor::KeyDown(ConvertSingleKey(static_cast<char>(key)), shift, alt, ctrl, &consumed);
	} break;
	case sTickMessage: {
		int32 reason;
		if(msg->FindInt32("reason", &reason) == B_OK) {
			TickFor((TickReason) reason);
		}
	} break;
	default:
		Command(msg->what);
		BView::MessageReceived(msg);
	break;
	}
}

void ScintillaHaiku::CopyToClipboard(const SelectionText& selectedText) {
	
	BMessage* clip = NULL;
	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		if ((clip = be_clipboard->Data())) {
			clip->AddData("text/plain", B_MIME_TYPE, selectedText.Data(), selectedText.Length());
			if(selectedText.rectangular) {
				clip->AddData(sMimeRectangularMarker, B_MIME_TYPE, selectedText.Data(), selectedText.Length());
			}
			be_clipboard->Commit();
		}
	}
	be_clipboard->Unlock();
}

bool ScintillaHaiku::FineTickerAvailable() {
	return true;
}

bool ScintillaHaiku::FineTickerRunning(TickReason reason) {
	return timers[reason] != NULL;
}

void ScintillaHaiku::FineTickerStart(TickReason reason, int millis, int tolerance) {
	FineTickerCancel(reason);
	BMessage message(sTickMessage);
	message.AddInt32("reason", reason);
	timers[reason] = new BMessageRunner(BMessenger(this), &message, millis * 1000);
	if(timers[reason]->InitCheck() != B_OK) {
		FineTickerCancel(reason);
	}
}

void ScintillaHaiku::FineTickerCancel(TickReason reason) {
	if(timers[reason] != NULL) {
		delete timers[reason];
		timers[reason] = NULL;
	}
}

void ScintillaHaiku::SetMouseCapture(bool on) {
	capturedMouse = on;
}

bool ScintillaHaiku::HaveMouseCapture() {
	return capturedMouse;
}

sptr_t ScintillaHaiku::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	switch(iMessage) {
	case SCI_SETIMEINTERACTION:
		ScintillaBase::WndProc(iMessage, wParam, lParam);
		if(imeInteraction == imeWindowed)
			SetFlags(Flags() & ~B_INPUT_METHOD_AWARE);
		else if(imeInteraction == imeInline)
			SetFlags(Flags() | B_INPUT_METHOD_AWARE);
	break;
	case SCI_GRABFOCUS:
		MakeFocus(true);
	break;
#ifdef SCI_LEXER
	case SCI_LOADLEXERLIBRARY:
		LexerManager::GetInstance()->Load(reinterpret_cast<const char*>(lParam));
	break;
#endif
	default:
		return ScintillaBase::WndProc(iMessage, wParam, lParam);
	}
	
	return 0;
}

sptr_t ScintillaHaiku::DefWndProc(unsigned int, uptr_t, sptr_t) {
	return 0;
}

void ScintillaHaiku::CreateCallTipWindow(PRectangle rect) {
	if (!ct.wCallTip.Created()) {
		CallTipView *callTip = new CallTipView(BRect(rect.left, rect.top, rect.right, rect.bottom), this, &ct);
		ct.wCallTip = callTip;
		AddChild(callTip);
	}
}

void ScintillaHaiku::AddToPopUp(const char *label, int cmd, bool enabled) {
	BPopUpMenu *menu = static_cast<BPopUpMenu *>(popup.GetID());
	
	if(label[0] == 0)
		menu->AddSeparatorItem();
	else {
		BMessage *message = new BMessage(cmd);
		BMenuItem *item = new BMenuItem(label, message);
		item->SetTarget(this);
		item->SetEnabled(enabled);
		menu->AddItem(item);
	}
}

void ScintillaHaiku::StartDrag() {
	inDragDrop = ddDragging;
	dropWentOutside = true;
	if(drag.Length()) {
		BMessage* dragMessage = new BMessage(B_SIMPLE_DATA);
		dragMessage->AddPointer("be:originator", this);
		dragMessage->AddInt32("be:actions", B_COPY_TARGET | B_MOVE_TARGET);
		dragMessage->AddData("text/plain", B_MIME_TYPE, drag.Data(), drag.Length());
		if(drag.rectangular) {
			dragMessage->AddData(sMimeRectangularMarker, B_MIME_TYPE, drag.Data(), drag.Length());
		}
		BHandler* dragHandler = NULL;
		Point s = LocationFromPosition(sel.LimitsForRectangularElseMain().start);
		Point e = LocationFromPosition(sel.LimitsForRectangularElseMain().end);
		BRect dragRect(s.x, s.y, e.x, e.y + vs.lineHeight);
		if(s.y != e.y && !sel.IsRectangular()) {
			dragRect.left = vs.textStart;
			dragRect.right = GetTextRectangle().right;
		}
		DragMessage(dragMessage, dragRect, dragHandler);
	}
}

bool ScintillaHaiku::Drop(BMessage* message, const BPoint& point, const BPoint& offset, bool move) {
	ssize_t textLen = 0;
	const char* text = NULL;
	bool isRectangular = false;
	
	isRectangular = (message->FindData(sMimeRectangularMarker,
				B_MIME_TYPE, (const void **) &text, &textLen) == B_OK);
	if(isRectangular == false) {
		if(message->FindData("text/plain", B_MIME_TYPE, (const void**) &text, &textLen) != B_OK) {
			return false;
		}
	}
	
	Point p(static_cast<int>(point.x - offset.x), static_cast<int>(point.y - offset.y + vs.lineHeight / 2));
	SelectionPosition movePos = SPositionFromLocation(p, false, false, UserVirtualSpace());
	
	DropAt(movePos, text, textLen, move, isRectangular);
	
	return true;
}

void ScintillaHaiku::Draw(BRect rect) {
	PRectangle rcText(rect.left, rect.top, rect.right, rect.bottom + 1);

	Surface *sw = Surface::Allocate(SC_TECHNOLOGY_DEFAULT);

	if (sw) {
		sw->Init(static_cast<BView*>(this), static_cast<BView*>(wMain.GetID()));
		Paint(sw, rcText);
		sw->Release();
		delete sw;
	}
}
 
void ScintillaHaiku::KeyDown(const char* bytes, int32 len) {
	bool shift = (modifiers() & B_SHIFT_KEY);
	bool ctrl = (modifiers() & B_CONTROL_KEY);
	bool alt = (modifiers() & B_COMMAND_KEY);

	int key = ConvertSingleKey(bytes[0]);
	if(key == -1) return;
	bool consumed;
	bool added = Editor::KeyDown(key, shift, alt, ctrl, &consumed);
	if(!consumed)
		consumed = added;
	if(!consumed) {
		AddCharUTF(bytes, len);
	}
}

void ScintillaHaiku::AttachedToWindow() {
	// TODO: buffered draw does not render properly
	// it does not fix flickering problem anyway
	WndProc(SCI_SETBUFFEREDDRAW, 0, 0);
	WndProc(SCI_SETCODEPAGE, SC_CP_UTF8, 0);
	wMain = (WindowID)Parent();
}

void ScintillaHaiku::MakeFocus(bool focus) {
	if(focus) _Activate();
	else _Deactivate();

	BView::MakeFocus(focus);
	SetFocusState(focus);
}

void ScintillaHaiku::MouseDown(BPoint point) {
	_IMCancel();
	MakeFocus(true);
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	BMessage* msg = Window()->CurrentMessage();
	if (!msg)
		return; //??

	if(displayPopupMenu) {
		BPoint screen = ConvertToScreen(point);
		int32 buttons = msg->GetInt32("buttons", 0);
		if(buttons & B_SECONDARY_MOUSE_BUTTON) {
			ContextMenu(Point(screen.x, screen.y));
			return;
		}
	}

	int64 when = msg->GetInt64("when", 0) / 1000;
	ButtonDown( Point( static_cast<int>( point.x ), static_cast<int>( point.y ) ),
		when,
		(modifiers() & B_SHIFT_KEY),
		(modifiers() & B_COMMAND_KEY),
		(modifiers() & B_CONTROL_KEY) );
}

void ScintillaHaiku::MouseUp(BPoint point) {
	BMessage* msg = Window()->CurrentMessage();
	if (!msg)
		return; //??

	int64 when = msg->GetInt64("when", 0) / 1000;
	ButtonUp( Point( static_cast<int>( point.x ), static_cast<int>( point.y ) ),
		when,
		(modifiers() & B_COMMAND_KEY));
}

void ScintillaHaiku::MouseMoved(BPoint point, uint32 transit, const BMessage* message) {
	switch(transit) {
		case B_ENTERED_VIEW:
		case B_INSIDE_VIEW:
			ButtonMove( Point( static_cast<int>( point.x ), static_cast<int>( point.y ) ));
		break;
		case B_EXITED_VIEW:
		case B_OUTSIDE_VIEW:
		break;
	};
}

void ScintillaHaiku::FrameResized(float width, float height) {
	ChangeSize();
}

void ScintillaHaiku::WindowActivated(bool active) {
	MakeFocus(active);
}

void ScintillaHaiku::ScrollTo(BPoint p) {
	if(p.x == 0) {
		Editor::ScrollTo(p.y - 1, false);
 		ScrollBar(B_VERTICAL)->SetValue(p.y);
	} else if (p.y == 0) {
		Editor::HorizontalScrollTo(p.x - 1);
	}
}

void ScintillaHaiku::_Activate() {
	if(Window() != NULL) {
		BMessage* message;

		if(!Window()->HasShortcut(B_LEFT_ARROW, B_COMMAND_KEY)
			&& !Window()->HasShortcut(B_RIGHT_ARROW, B_COMMAND_KEY)) {
			message = new BMessage('nava');
			message->AddInt32("key", B_LEFT_ARROW);
			message->AddInt32("modifiers", B_COMMAND_KEY);
			Window()->AddShortcut(B_LEFT_ARROW, B_COMMAND_KEY, message, this);

			message = new BMessage('nava');
			message->AddInt32("key", B_RIGHT_ARROW);
			message->AddInt32("modifiers", B_COMMAND_KEY);
			Window()->AddShortcut(B_RIGHT_ARROW, B_COMMAND_KEY, message, this);

			wordwiseNavigationShortcuts = true;
		}

		if(!Window()->HasShortcut(B_LEFT_ARROW, B_COMMAND_KEY | B_SHIFT_KEY)
			&& !Window()->HasShortcut(B_RIGHT_ARROW, B_COMMAND_KEY | B_SHIFT_KEY)) {
			message = new BMessage('nava');
			message->AddInt32("key", B_LEFT_ARROW);
			message->AddInt32("modifiers", B_COMMAND_KEY | B_SHIFT_KEY);
			Window()->AddShortcut(B_LEFT_ARROW, B_COMMAND_KEY | B_SHIFT_KEY, message, this);

			message = new BMessage('nava');
			message->AddInt32("key", B_RIGHT_ARROW);
			message->AddInt32("modifiers", B_COMMAND_KEY | B_SHIFT_KEY);
			Window()->AddShortcut(B_RIGHT_ARROW, B_COMMAND_KEY | B_SHIFT_KEY, message, this);

			wordwiseSelectionShortcuts = true;
		}
	}
}

void ScintillaHaiku::_Deactivate() {
	if(Window() != NULL) {
		if(wordwiseNavigationShortcuts) {
			Window()->RemoveShortcut(B_LEFT_ARROW, B_COMMAND_KEY);
			Window()->RemoveShortcut(B_RIGHT_ARROW, B_COMMAND_KEY);
			wordwiseNavigationShortcuts = false;
		}

		if(wordwiseSelectionShortcuts) {
			Window()->RemoveShortcut(B_LEFT_ARROW, B_COMMAND_KEY | B_SHIFT_KEY);
			Window()->RemoveShortcut(B_RIGHT_ARROW, B_COMMAND_KEY | B_SHIFT_KEY);
			wordwiseSelectionShortcuts = false;
		}
	}
}

#ifdef SCI_NAMESPACE
}
#endif

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

struct BScintillaPrivate {
	ScintillaHaiku* sciControl;
};

/* BScrollView doesn't update scroll bars properly when resizing.
   Since layouts work fine and it is preferred method of creating
   new apps, we can leave BRect constructor unimplemented.
*/
#if 0
BScintillaView::BScintillaView(BRect rect, const char* name, uint32 resizingMode, uint32 flags, border_style border):
	BScrollView(name, NULL, resizingMode, flags, true, true, border) {
	sciControl = new ScintillaHaiku(rect);
	ResizeTo(rect.Width(), rect.Height());
	SetTarget(sciControl);
	sciControl->ResizeTo(rect.Width() - B_V_SCROLL_BAR_WIDTH, rect.Height() - B_H_SCROLL_BAR_HEIGHT);
}
#endif

BScintillaView::BScintillaView(const char* name, uint32 flags, bool horizontal, bool vertical, border_style border):
	BScrollView(name, NULL, flags, horizontal, vertical, border) {
	p = new BScintillaPrivate;
	p->sciControl = new ScintillaHaiku();
	SetTarget(p->sciControl);
}

BScintillaView::~BScintillaView() {
	SetTarget(NULL);
	delete p->sciControl;
	delete p;
}

sptr_t BScintillaView::SendMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	return p->sciControl->WndProc(iMessage, wParam, lParam);
}

void BScintillaView::MessageReceived(BMessage* msg) {
	switch(msg->what) {
 		case B_SCINTILLA_NOTIFICATION: {
 			SCNotification* n = NULL;
 			ssize_t size;
 			if (msg->FindData("notification", B_ANY_TYPE, 0, (const void**)&n, &size) == B_OK) {
 				NotificationReceived(n);
 			}
 		}
 		break;
 		default:
 			BScrollView::MessageReceived(msg);
 		break;
 	};
}

void BScintillaView::NotificationReceived(SCNotification* notification) {
}

int32 BScintillaView::TextLength()
{
	return SendMessage(SCI_GETLENGTH, 0, 0);
}

void BScintillaView::GetText(int32 offset, int32 length, char* buffer)
{
	if(offset == 0) {
		SendMessage(SCI_GETTEXT, length, (sptr_t) buffer);
	} else {
		Sci_TextRange tr;
		tr.chrg.cpMin = offset;
		tr.chrg.cpMax = offset + length;
		tr.lpstrText = buffer;
		SendMessage(SCI_GETTEXTRANGE, 0, (sptr_t) &tr);
	}
}

void BScintillaView::SetText(const char* text)
{
	SendMessage(SCI_SETTEXT, 0, (sptr_t) text);
}
