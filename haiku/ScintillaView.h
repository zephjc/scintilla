/* Scintilla source code edit control */
/** @file ScintillaView.h
 ** Definition of Scintilla widget for Haiku.
 ** Only needed by Haiku code but is harmless on other platforms.
 **/
/* Copyright 2014 by Kacper Kasper <kacperkasper@gmail.com>
 * The License.txt file describes the conditions under which this software may be distributed. */

#ifndef SCINTILLAVIEW_H
#define SCINTILLAVIEW_H

#include "Scintilla.h"
#include <ScrollView.h>

#ifdef SCI_NAMESPACE
#define SCNotification Scintilla::SCNotification
#endif

struct BScintillaPrivate;

#define B_SCINTILLA_NOTIFICATION 'BSCN'

class BScintillaView : public BScrollView {
public:
	/*BScintillaView(BRect rect, const char* name,
		uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
		uint32 flags = 0, border_style border = B_FANCY_BORDER);*/
	BScintillaView(const char* name, uint32 flags = 0, bool horizontal = false,
		bool vertical = false, border_style border = B_FANCY_BORDER);
	~BScintillaView();
	
	sptr_t SendMessage(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	virtual void MessageReceived(BMessage* msg);
	virtual void NotificationReceived(SCNotification* notification);
	
	int32		TextLength();
	void		GetText(int32 offset, int32 length, char* buffer);
	void		SetText(const char* text);
private:
	BScintillaPrivate *p;
};

#ifdef SCI_NAMESPACE
#undef SCNotification
#endif

#endif
