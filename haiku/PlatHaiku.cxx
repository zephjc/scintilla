// Scintilla source code edit control
// PlatHaiku.cxx - implementation of platform facilities on Haiku
// Copyright 2011 by Andrea Anzani <andrea.anzani@gmail.com>
// Copyright 2014-2015 by Kacper Kasper <kacperkasper@gmail.com>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <sys/time.h>
#include <assert.h>

#include <map>
#include <vector>

#include <kernel/image.h>

#include <Autolock.h>
#include <Bitmap.h>
#include <ControlLook.h>
#include <Cursor.h>
#include <Font.h>
#include <ListItem.h>
#include <ListView.h>
#include <PopUpMenu.h>
#include <Picture.h>
#include <Region.h>
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>
#include <Window.h>
#include <View.h>

#include "Platform.h"

#include "Scintilla.h"
#include "UniConversion.h"
#include "XPM.h"

#define DB debugger(__FUNCTION__);
#define TRACE printf("Trace:%s:%d - %s\n", __FILE__, __LINE__, __FUNCTION__);
#define TRACEVIEW if (ownerView) printf("Trace:%s:%d - %s -- ownerView %d (%s)\n", __FILE__, __LINE__, __FUNCTION__, (int)ownerView, ownerView->Name()); else printf("Trace:%s:%d - %s -- ownerView %d (NULL?)\n", __FILE__, __LINE__, __FUNCTION__, (int)ownerView);

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class BitmapLock
{
public:
	BitmapLock(BBitmap *b) {
		bit = b;
		if (bit) b->Lock();
	}
	~BitmapLock() {
		if(bit) {
			BView* drawer = bit->ChildAt(0);
			if (drawer)
				drawer->Sync();
			bit->Unlock();
		}
	}

private:
	BBitmap* bit;
};

BRect toBRect(PRectangle r) {
	// Haiku renders rectangles with left == right or top == bottom
	// as they were 1 pixel wide
	return BRect(r.left, r.top, r.right - 1, r.bottom - 1);
}

BRect toBRect2(PRectangle r) {
	return BRect(r.left, r.top, r.right, r.bottom);
}

PRectangle toPRect(BRect r) {
	return PRectangle(r.left, r.top, r.right + 1, r.bottom + 1);
}

Point Point::FromLong(long lpoint) {
	return Point(Platform::LowShortFromLong(lpoint), Platform::HighShortFromLong(lpoint));
}

Font::Font() : fid(0) 
{ }

Font::~Font() { Release(); }

void Font::Create(const FontParameters &fp) {	
	Release();

	BFont* font = new BFont();
	font->SetFamilyAndStyle(fp.faceName, NULL);
	uint16 face = 0;
	if (fp.weight >= SC_WEIGHT_BOLD)
		face |= B_BOLD_FACE;
	if (fp.italic == true)
		face |= B_ITALIC_FACE;
	font->SetFace(face);
	font->SetSize(fp.size);

	fid = font;
}

void Font::Release() {
	if (fid)
		delete reinterpret_cast<BFont*>( fid );

	fid = 0;
}

BFont* bfont(Font& font_) {
	return reinterpret_cast<BFont*>(font_.GetID());
}

rgb_color bcolor(ColourDesired col) {
	rgb_color hcol;
	hcol.red = col.GetRed();
	hcol.green = col.GetGreen();
	hcol.blue = col.GetBlue();
	hcol.alpha = 255;//!
	return hcol;
}

class SurfaceImpl : public Surface {
private:
	BView* 	  ownerView;
	BBitmap*  bitmap;
	int x;
	int y;

public:
	SurfaceImpl();
	~SurfaceImpl();

	BBitmap* GetBitmap() { return bitmap; }

	void Init(WindowID wid);
	void Init(SurfaceID sid, WindowID wid);
	void InitPixMap(int width, int height, Surface *surface_, WindowID wid);

	void Release();
	bool Initialised();
	void PenColour(ColourDesired fore);
	int LogPixelsY();
	int DeviceHeightFont(int points);
	void MoveTo(int x_, int y_);
	void LineTo(int x_, int y_);
	void Polygon(Point *pts, int npts, ColourDesired fore, ColourDesired back);
	void RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back);
	void FillRectangle(PRectangle rc, ColourDesired back);
	void FillRectangle(PRectangle rc, Surface &surfacePattern);
	void RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back);
	void AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
		ColourDesired outline, int alphaOutline, int flags);
	void DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage);
	void Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back);
	void Copy(PRectangle rc, Point from, Surface &surfaceSource);

	void DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore, ColourDesired back);
	void DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore, ColourDesired back);
	void DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore);
	void MeasureWidths(Font &font_, const char *s, int len, XYPOSITION *positions);
	XYPOSITION WidthText(Font &font_, const char *s, int len);
	XYPOSITION WidthChar(Font &font_, char ch);
	XYPOSITION Ascent(Font &font_);
	XYPOSITION Descent(Font &font_);
	XYPOSITION InternalLeading(Font &font_);
	XYPOSITION ExternalLeading(Font &font_);
	XYPOSITION Height(Font &font_);
	XYPOSITION AverageCharWidth(Font &font_);

	void SetClip(PRectangle rc);
	void FlushCachedState();

	void SetUnicodeMode(bool unicodeMode_);
	void SetDBCSMode(int codePage);
};

SurfaceImpl::SurfaceImpl() {
	ownerView = NULL;
	bitmap = NULL;
}

SurfaceImpl::~SurfaceImpl() {
	Release();
}

void SurfaceImpl::Init(WindowID wid) {
	Release();
	
	ownerView = static_cast<BView*>( wid );
}

void SurfaceImpl::Init(SurfaceID sid, WindowID wid) {
	Release();

	ownerView = static_cast<BView*>( sid );
}

void SurfaceImpl::InitPixMap(int width, int height, Surface *surface_, WindowID wid) {
	Release();
	
	ownerView = new BView(BRect(0,0, width, height),"pixMap", B_FOLLOW_ALL, B_WILL_DRAW);
	bitmap = new BBitmap(BRect(0,0, width, height), B_RGBA32, true, false);
	bitmap->AddChild(ownerView);
}

void SurfaceImpl::Release() {
	if (bitmap) {
		BitmapLock l(bitmap);
		bitmap->RemoveChild(ownerView);
		delete bitmap;
		delete ownerView;
		bitmap = NULL;
		ownerView = NULL;
	}
}

bool SurfaceImpl::Initialised() {
	return (ownerView != NULL);
}

void SurfaceImpl::PenColour(ColourDesired fore) {
	BitmapLock l(bitmap);
	ownerView->SetHighColor(bcolor(fore));
}

int SurfaceImpl::LogPixelsY() {
	return 72; //?
}

int SurfaceImpl::DeviceHeightFont(int points) {
	int logPix = LogPixelsY();
	return (points * logPix + logPix / 2) / 72;
}

void SurfaceImpl::MoveTo(int x_, int y_) {
	x = x_; y = y_;
}

void SurfaceImpl::LineTo(int x_, int y_) {
	BitmapLock l(bitmap);
	ownerView->StrokeLine(BPoint(x,y), BPoint(x_, y_));
	x = x_; y = y_;
}

void SurfaceImpl::Polygon(Point *pts, int npts, ColourDesired fore, ColourDesired back) {
	BitmapLock l(bitmap);	
	BPoint* points = new BPoint[npts];
	for (int i = 0; i < npts; i++)
	{
		points[i].x = pts[i].x;
		points[i].y = pts[i].y;
	}
	ownerView->SetHighColor(bcolor(back));
	ownerView->FillPolygon(points, npts);
	ownerView->SetHighColor(bcolor(fore));
	ownerView->StrokePolygon(points, npts);
}

void SurfaceImpl::RectangleDraw(PRectangle rc, ColourDesired fore, ColourDesired back) {
	BitmapLock l(bitmap);

	BRect rect(toBRect(rc));
	
	ownerView->SetHighColor(bcolor(back));
	ownerView->FillRect(rect);
	ownerView->SetHighColor(bcolor(fore));
	ownerView->StrokeRect(rect);
}

void SurfaceImpl::FillRectangle(PRectangle rc, ColourDesired back) {
	BitmapLock l(bitmap);
	ownerView->SetHighColor(bcolor(back));
	ownerView->FillRect(toBRect(rc));
}

void SurfaceImpl::FillRectangle(PRectangle rc, Surface &surfacePattern) {
	SurfaceImpl& pattern = static_cast<SurfaceImpl &>(surfacePattern);

	BBitmap* image = pattern.GetBitmap();
	// If we could not get an image reference, fill the rectangle black
	if (image == NULL) {
		FillRectangle(rc, ColourDesired(0));
		return;
	}
	
	XYPOSITION imageW = image->Bounds().Width();
	XYPOSITION imageH = image->Bounds().Height();
	BRect destRect = toBRect2(rc);
	uint32 toEvenLeft = (uint32) imageW - (uint32) destRect.left % (uint32) imageW;
	uint32 toEvenTop = (uint32) imageH - (uint32) destRect.top % (uint32) imageH;
	uint32 timesHor = (destRect.Width() - toEvenLeft) / imageW;
	uint32 timesVert = (destRect.Height() - toEvenTop) / imageH;
	uint32 toEvenRight = (uint32) (destRect.Width() - toEvenLeft) % (uint32) imageW;
	uint32 toEvenBottom = (uint32) (destRect.Height() - toEvenTop) % (uint32) imageH;
	// This code ensures drawn pattern will ALWAYS match,
	// regardless of destination rectangle
	BitmapLock l(bitmap);
	uint32 i, j;
	// top left
	ownerView->DrawBitmapAsync(image,
		BRect(imageW - toEvenLeft, imageH - toEvenTop, imageW, imageH),
		BRect(0, 0, toEvenLeft, toEvenTop)
			.OffsetBySelf(destRect.left, destRect.top)
		);
	// top
	for(i = 0; i < timesHor; i++) {
		ownerView->DrawBitmapAsync(image, 
			BRect(0, imageH - toEvenTop, imageW, imageH),
			BRect(i * imageW, 0, (i + 1) * imageW, toEvenTop)
				.OffsetBySelf(destRect.left, destRect.top)
				.OffsetBySelf(toEvenLeft, 0)
			);
	}
	// top right
	ownerView->DrawBitmapAsync(image,
		BRect(0, imageH - toEvenTop, toEvenRight, imageH),
		BRect(timesHor * imageW, 0, timesHor * imageW + toEvenRight, toEvenTop)
			.OffsetBySelf(destRect.left, destRect.top)
			.OffsetBySelf(toEvenLeft, 0)
		);
	// left
	for(j = 0; j < timesVert; j++) {
		ownerView->DrawBitmapAsync(image,
			BRect(imageW - toEvenLeft, 0, imageW, imageH),
			BRect(0, j * imageH, toEvenLeft, (j + 1) * imageH)
				.OffsetBySelf(destRect.left, destRect.top)
				.OffsetBySelf(0, toEvenTop)
			);
	}
	// center
	for(i = 0; i < timesHor; i++) {
		for(j = 0; j < timesVert; j++) {
			ownerView->DrawBitmapAsync(image,
				BRect(0, 0, imageW, imageH), 
				BRect(i * imageW, j * imageH, (i + 1) * imageW, (j + 1) * imageH)
					.OffsetBySelf(destRect.left, destRect.top)
					.OffsetBySelf(toEvenLeft, toEvenTop)
				);
		}
	}
	// right
	for(j = 0; j < timesVert; j++) {
		ownerView->DrawBitmapAsync(image,
			BRect(0, 0, toEvenRight, imageH),
			BRect(timesHor * imageW, j * imageH, timesHor * imageW + toEvenRight, (j + 1) * imageH)
				.OffsetBySelf(destRect.left, destRect.top)
				.OffsetBySelf(toEvenLeft, toEvenTop)
			);
	}
	// bottom left
	ownerView->DrawBitmapAsync(image,
		BRect(imageW - toEvenLeft, 0, imageW, toEvenBottom),
		BRect(0, timesVert * imageH, toEvenLeft, timesVert * imageH + toEvenBottom)
			.OffsetBySelf(destRect.left, destRect.top)
			.OffsetBySelf(0, toEvenTop)
		);
	// bottom
	for(i = 0; i < timesHor; i++) {
		ownerView->DrawBitmapAsync(image,
			BRect(0, 0, imageW, toEvenBottom),
			BRect(i * imageW, timesVert * imageH, (i + 1) * imageW, timesVert * imageH + toEvenBottom)
				.OffsetBySelf(destRect.left, destRect.top)
				.OffsetBySelf(toEvenLeft, toEvenTop)
			);
	}
	// bottom right
	ownerView->DrawBitmapAsync(image,
		BRect(0, 0, toEvenRight, toEvenBottom),
		BRect(timesHor * imageW, timesVert * imageH, timesHor * imageW + toEvenRight, timesVert * imageH + toEvenBottom)
			.OffsetBySelf(destRect.left, destRect.top)
			.OffsetBySelf(toEvenLeft, toEvenTop)
		);
}

void SurfaceImpl::RoundedRectangle(PRectangle rc, ColourDesired fore, ColourDesired back) {
	BitmapLock l(bitmap);
	
	BRect rect(toBRect(rc));
	const int radius = 3;
	
	ownerView->SetHighColor(bcolor(back));
	ownerView->FillRoundRect(rect, radius, radius);
	ownerView->SetHighColor(bcolor(fore));
	ownerView->StrokeRoundRect(rect, radius, radius);
}

void SurfaceImpl::AlphaRectangle(PRectangle rc, int cornerSize, ColourDesired fill, int alphaFill,
		ColourDesired outline, int alphaOutline, int flags) {
	BitmapLock l(bitmap);
	
	BRect rect(toBRect(rc));
	rgb_color back, fore;
	back = bcolor(fill);
	back.alpha = alphaFill;
	fore = bcolor(outline);
	fore.alpha = alphaOutline;

	ownerView->PushState();
	ownerView->SetDrawingMode(B_OP_ALPHA);
	ownerView->SetHighColor(back);
	ownerView->FillRect(rect);
	ownerView->SetHighColor(fore);
	ownerView->StrokeRect(rect);
	ownerView->PopState();
}

void SurfaceImpl::DrawRGBAImage(PRectangle rc, int width, int height, const unsigned char *pixelsImage) {
	BRect dest(toBRect(rc));
	BRect src(0, 0, width, height);
	
	BBitmap* image = new BBitmap(src, B_RGBA32);
	image->SetBits((void*) pixelsImage, width * height, 0, B_RGBA32);
	
	{
		BitmapLock l(bitmap);
		ownerView->DrawBitmap(image, src, dest);
	}
	
	delete image;
}

void SurfaceImpl::Ellipse(PRectangle rc, ColourDesired fore, ColourDesired back) {
	BitmapLock l(bitmap);
	BRect rect(toBRect(rc));
	ownerView->SetHighColor(bcolor(back));
	ownerView->FillEllipse(rect);
	ownerView->SetHighColor(bcolor(fore));
	ownerView->StrokeEllipse(rect);
}

void SurfaceImpl::Copy(PRectangle rc, Point from, Surface &surfaceSource) {
	SurfaceImpl& source = static_cast<SurfaceImpl &>(surfaceSource);

	BBitmap* image = source.GetBitmap();
	// If we could not get an image reference, fill the rectangle black
	if (image == NULL) {
		FillRectangle(rc, ColourDesired(0));
		return;
	}
	
	XYPOSITION imageW = image->Bounds().Width();
	XYPOSITION imageH = image->Bounds().Height();
	BRect srcRect = BRect(from.x, from.y, imageW, imageH);
	BRect destRect = toBRect2(rc);
	BitmapLock l(bitmap);
	ownerView->DrawBitmap(image, srcRect, destRect);
}

void SurfaceImpl::DrawTextNoClip(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore, ColourDesired back) {
	BitmapLock l(bitmap);
	if (ownerView) {
		FillRectangle(rc, back);
		ownerView->SetFont(bfont(font_));
		PenColour(fore);
		ownerView->SetLowColor(bcolor(back));
		BPoint where(rc.left, ybase);
		ownerView->DrawString(s, len, where);
	}
}

void SurfaceImpl::DrawTextClipped(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore, ColourDesired back) {
	BitmapLock l(bitmap);
	if (ownerView) {
		FillRectangle(rc, back);
		ownerView->SetFont(bfont(font_));
		PenColour(fore);
		ownerView->SetLowColor(bcolor(back));
		BPoint where(rc.left, ybase);
		ownerView->DrawString(s, len, where);
	}
}

void SurfaceImpl::DrawTextTransparent(PRectangle rc, Font &font_, XYPOSITION ybase, const char *s, int len, ColourDesired fore) {
	BitmapLock l(bitmap);
	if (ownerView) {
		ownerView->PushState();
		PenColour(fore);
		ownerView->SetDrawingMode(B_OP_ALPHA);
		ownerView->SetFont(bfont(font_));
		ownerView->DrawString(s, len, BPoint(rc.left, ybase));
		ownerView->PopState();
	}
}

static size_t utf8LengthFromLead(unsigned char uch)
{
	if (uch >= (0x80 + 0x40 + 0x20 + 0x10)) {
		return 4;
	} else if (uch >= (0x80 + 0x40 + 0x20)) {
		return 3;
	} else if (uch >= (0x80)) {
		return 2;
	} else {
		return 1;
	}
}

int MultibyteToUTF8Length(const unsigned char* us, int len) {
	int length = 0;
	for(int i = 0; i < len;) {
		if(us[i] >= (0x80 + 0x40 + 0x20 + 0x10)) {
			i += 4;
		} else if(us[i] >= (0x80 + 0x40 + 0x20)) {
			i += 3;
		} else if(us[i] >= (0x80)) {
			i += 2;
		} else {
			i++;
		}
		length++;
	}
	return length;
}

// borrowed from Qt port
void SurfaceImpl::MeasureWidths(Font &font_, const char *s, int len, XYPOSITION *positions) {
	BFont* font = bfont(font_);
	if (font) {
		const unsigned char *us = reinterpret_cast<const unsigned char *>(s);
		int fit = MultibyteToUTF8Length(us, len);
		int ui = 0;
		int i = 0;
		float currentPos = 0.0f;
		float* escp = new float[len];
		font->GetEscapements(s, len, escp);
		while(ui < fit) {
			size_t lenChar = utf8LengthFromLead(us[i]);
			int codeUnits = (lenChar < 4) ? 1 : 2;
			float pos = currentPos + escp[ui] * font->Size();
			for(unsigned int bytePos = 0; (bytePos < lenChar) && (i < len); bytePos++) {
				positions[i++] = pos;
			}
			ui += codeUnits;
			currentPos = pos;
		}
		float lastPos = 0;
		if(i > 0)
			lastPos = positions[i - 1];
		while(i < len)
			positions[i++] = lastPos;
		delete []escp;
	}
}

XYPOSITION SurfaceImpl::WidthText(Font &font_, const char *s, int len) {
	if (!font_.GetID())
		return 1;

	XYPOSITION width = round(bfont(font_)->StringWidth(s, len));
	return width;
}

XYPOSITION SurfaceImpl::WidthChar(Font &font_, char ch) {
	return WidthText(font_, &ch, 1);
}

XYPOSITION SurfaceImpl::Ascent(Font &font_) {
	BFont* font = bfont(font_);
	if (font) {
		font_height fh;
		font->GetHeight(&fh);
		return ceil(fh.ascent);
	}
	return 0;
}

XYPOSITION SurfaceImpl::Descent(Font &font_) {
	BFont* font = bfont(font_);
	if (font) {
		font_height fh;
		font->GetHeight(&fh);
		return ceil(fh.descent);
	}
	return 0;
}

XYPOSITION SurfaceImpl::InternalLeading(Font &font_) {
	BFont* font = bfont(font_);
	if (font) {
		font_height fh;
		font->GetHeight(&fh);
		return ceil(fh.leading); //TODO FIX: Internal VS External?
	}
	return 0;
}

XYPOSITION SurfaceImpl::ExternalLeading(Font &font_) {	 
	//TODO FIX: Internal VS External?
	return InternalLeading(font_);
}

XYPOSITION SurfaceImpl::Height(Font &font_) {
	BFont* font = bfont(font_);
	if (font) {
		font_height fh;
		font->GetHeight(&fh);
		int h = (ceil(fh.descent) + ceil(fh.ascent));
		return h;
	}
	return 0;
}

// This string contains a good range of characters to test for size.
const char sizeString[] = "`~!@#$%^&*()-_=+\\|[]{};:\"\'<,>.?/1234567890"
						  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

XYPOSITION SurfaceImpl::AverageCharWidth(Font &font_) {
	if (!font_.GetID())
		return 1;

	const int sizeStringLength = (sizeof( sizeString ) / sizeof( sizeString[0] ) - 1);
	int width = WidthText( font_, sizeString, sizeStringLength	);

	return (int) ((width / (float) sizeStringLength) + 0.5);
}

void SurfaceImpl::SetClip(PRectangle rc) {
	BRegion r(toBRect2(rc));
	ownerView->ConstrainClippingRegion(&r);
}

void SurfaceImpl::FlushCachedState() {
	// Not sure about this:
	ownerView->Flush();
}

void SurfaceImpl::SetUnicodeMode(bool unicodeMode_) {
	// In Haiku we are always in UTF8
}

void SurfaceImpl::SetDBCSMode(int codePage) {
	// Doesn't matter for us
}

Surface* Surface::Allocate(int technology) {
	return new SurfaceImpl();
}

BView* bwin(WindowID id) { // in Haiku a Window is just the base View.
	return reinterpret_cast<BView*>(id);
}

Window::~Window() {
}

void Window::Destroy() {
	BView* view = bwin(wid);
	if(view) {
		BLooper* looper = view->Looper();
		if(looper->Lock())
		{
			view->RemoveSelf();
			looper->Unlock();
			delete view;
			wid = 0;
		}
	}
}

bool Window::HasFocus() {
	return wid ? bwin(wid)->IsFocus() : false;
}

PRectangle Window::GetPosition() {
	// Before any size allocated pretend its 1000 wide so not scrolled
	PRectangle rc(0, 0, 1000, 1000);

	// The frame rectangle gives the position of this view inside the parent view
	if (wid) {
		BView *window = bwin(wid);
		BAutolock a(window->Looper());
		if(a.IsLocked()) {
			rc = toPRect(window->Frame());
		}
	}

	return rc;
}

void Window::SetPosition(PRectangle rc) {
	DB
	// Moves this view inside the parent view
	if ( wid )
	{
		// Set the frame on the view, the function handles the rest
		//TODO
	}
}

void Window::SetPositionRelative(PRectangle rc, Window window) {
	BView *view = bwin(wid);
	BAutolock a(view->Looper());
	if(a.IsLocked()) {
		view->MoveTo(BPoint(rc.left, rc.top));
		view->ResizeTo(rc.Width(), rc.Height());
	}
}

PRectangle Window::GetClientPosition() {
	PRectangle rc;
	if (wid) {
		BView *window = bwin(wid);
		BAutolock a(window->Looper());
		if(a.IsLocked()) {
			rc = toPRect(window->Bounds());
		}
		// This can't be done other way
		// Calltip window's name is the only one we can be sure of
		if(strcmp(bwin(wid)->Name(), "callTipView") != 0) {
			rc.right -= B_V_SCROLL_BAR_WIDTH;
			rc.bottom -= B_H_SCROLL_BAR_HEIGHT;
		}
	}
	return rc;
}

void Window::Show(bool show) {
	if (wid) {
		if (show)
			bwin(wid)->Show();
		else {
			bwin(wid)->Hide();
		}
	}
}

void Window::InvalidateAll() {
	if ( wid ) {
		BView *window = bwin(wid);
		BAutolock a(window->Looper());
		if(a.IsLocked()) {
			int count = window->CountChildren();
			for (int i = 0; i < count; i++)
				window->ChildAt(i)->Invalidate();
		}
	}
}

void Window::InvalidateRectangle(PRectangle rc) {
	if (wid) {
		BView *window = bwin(wid);
		BAutolock a(window->Looper());
		if(a.IsLocked()) {
			int count = window->CountChildren();
			for (int i = 0; i < count; i++)
				window->ChildAt(i)->Invalidate(toBRect(rc));
		}
	}
}

void Window::SetFont(Font &) {
	DB
	// TODO: Do I need to implement this? MSDN: specifies the font that a control is to use when drawing text.
}

void Window::SetCursor(Cursor curs) {
	static BCursor upArrow(B_CURSOR_ID_RESIZE_NORTH);
	static BCursor horizArrow(B_CURSOR_ID_RESIZE_EAST_WEST);
	static BCursor vertArrow(B_CURSOR_ID_RESIZE_NORTH_SOUTH);
	static BCursor wait(B_CURSOR_ID_PROGRESS);
	static BCursor hand(B_CURSOR_ID_FOLLOW_LINK);
	if (wid) {
		const BCursor* cursor = B_CURSOR_SYSTEM_DEFAULT;
		switch ( curs ) {
			case cursorText:
				cursor = B_CURSOR_I_BEAM;
				break;
			case cursorWait:
				cursor = &wait;
				break;
			case cursorHoriz:
				cursor = &horizArrow;
				break;
			case cursorVert:
				cursor = &vertArrow;
				break;
			case cursorUp:
				cursor = &upArrow;
				break;
			case cursorArrow:
			case cursorReverseArrow:
			case cursorInvalid:
			default:
				cursor = B_CURSOR_SYSTEM_DEFAULT;
				break;
		}
		bwin(wid)->ChildAt(0)->SetViewCursor(cursor, true);
		cursorLast = curs;
	}
}

void Window::SetTitle(const char *s) {
	// On Haiku BView doesn't have title
}

PRectangle Window::GetMonitorRect(Point) {
	// TODO Haiku currently doesn't support multiple screens so we
	//      just return first screen rect
	BScreen screen(0);
	BRect rect = screen.Frame();
	return PRectangle(rect.left, rect.top, rect.right, rect.bottom);
}

Menu::Menu() : mid(0) {}

void Menu::CreatePopUp() {
	Destroy();
	mid = new BPopUpMenu("popupMenu", false, false);
}

void Menu::Destroy() {
	if (mid != 0) {
		BPopUpMenu *menu = static_cast<BPopUpMenu *>(mid);
		delete menu;
	}
	mid = 0;
}

void Menu::Show(Point pt, Window &window) {
	BPopUpMenu *menu = static_cast<BPopUpMenu *>(mid);
	menu->Go(BPoint(pt.x, pt.y), true);
	Destroy();
}

#define BORDER_WIDTH 2

class ListBoxImpl : public ListBox {
public:
	ListBoxImpl();
	~ListBoxImpl();

	void SetFont(Font &font);
	void Create(Window &parent, int ctrlID, Point location, int lineHeight_, bool unicodeMode_, int technology_);
	void SetAverageCharWidth(int width);
	void SetVisibleRows(int rows);
	int GetVisibleRows() const;
	PRectangle GetDesiredRect();
	int CaretFromEdge();
	void Clear();
	void Append(char *s, int type = -1);
	int Length();
	void Select(int n);
	int GetSelection();
	int Find(const char *prefix);
	void GetValue(int n, char *value, int len);
	void RegisterImage(int type, const char *xpm_data);
	void RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage);
	void ClearRegisteredImages();
	void SetDoubleClickAction(CallBackAction, void *);
	void SetList(const char* list, char separator, char typesep);
	
private:
	BView* listWindow;
	int visibleRows;
	int maxTextWidth;
};

class ListView : public BListView {
public:
	ListView(BRect rect, const char* name, list_view_type type = B_SINGLE_SELECTION_LIST);

	void Draw(BRect updateRect);
};

ListView::ListView(BRect rect, const char* name, list_view_type type)
	:
	BListView(rect, name, type, B_FOLLOW_ALL)
{
}

void
ListView::Draw(BRect updateRect)
{
	SetViewColor(ui_color(B_LIST_BACKGROUND_COLOR));
	SetHighColor(ViewColor());
	SetLowColor(ViewColor());
	if(Bounds().Intersects(updateRect))
		FillRect(updateRect);
	BListView::Draw(updateRect);
}

class ListBoxView : public BView {
public:
	ListBoxView(BRect rect);
	
	void Draw(BRect updateRect);
	
	BListView* GetListView() { return listView; }
private:
	BScrollView* scrollView;
	BListView* listView;
};

ListBoxView::ListBoxView(BRect rect)
	:
	BView(rect, "ListBoxView", 0, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	listView = new ListView(BRect(BORDER_WIDTH, BORDER_WIDTH, rect.Width() - BORDER_WIDTH - B_V_SCROLL_BAR_WIDTH, rect.Height() - BORDER_WIDTH), "ListView", B_SINGLE_SELECTION_LIST);
	scrollView = new BScrollView("ListBoxScrollView", listView, B_FOLLOW_ALL, 0, false, true, B_FANCY_BORDER);
	AddChild(scrollView);
}

void
ListBoxView::Draw(BRect updateRect) {
	SetHighColor(ViewColor());
	SetLowColor(ViewColor());
	if(Bounds().Intersects(updateRect))
		FillRect(updateRect);
	
	BView::Draw(updateRect);
}

BListView* blist(WindowID wid) {
	return static_cast<BListView*>(static_cast<ListBoxView*>(wid)->GetListView());
}

ListBox::ListBox(){}
ListBox::~ListBox(){}

ListBox *ListBox::Allocate() {
	return new ListBoxImpl();
}

ListBoxImpl::ListBoxImpl()
	:
	visibleRows(5), maxTextWidth(0)
{
}
ListBoxImpl::~ListBoxImpl(){}

void 
ListBoxImpl::SetFont(Font &font)
{
	BListView* list = blist(wid);
	BAutolock a(list->Looper());
	if(a.IsLocked()) {
		list->SetFont(bfont(font));
	}
}

void 
ListBoxImpl::Create(Window &parent, int ctrlID, Point location, int lineHeight_, bool unicodeMode_, int technology_)
{
	listWindow = new ListBoxView(BRect(location.x, location.y, location.x + 50, location.y + 50));
	BView* bparent = static_cast<BView*>(parent.GetID());
	bparent->AddChild(listWindow);
	wid = listWindow;
}

void 
ListBoxImpl::SetAverageCharWidth(int width)
{
	
}

void ListBoxImpl::SetVisibleRows(int rows)
{
	visibleRows = rows;
}

int ListBoxImpl::GetVisibleRows() const 
{
	return visibleRows;
}

PRectangle 
ListBoxImpl::GetDesiredRect() {
	BListView* list = blist(wid);
	
	int rows = Length();
	if (rows == 0 || rows > visibleRows) {
		rows = visibleRows;
	}
	int rowHeight = list->ItemAt(0)->Height();
	int height = (rows * rowHeight) + (2 * BORDER_WIDTH) - 1;
	int width = maxTextWidth + (2 * be_control_look->DefaultLabelSpacing())
		+ (2 * BORDER_WIDTH);
	width += B_V_SCROLL_BAR_WIDTH;
	return PRectangle(0, 0, width, height);
}

int 
ListBoxImpl::CaretFromEdge() {
	return be_control_look->DefaultLabelSpacing();
}

void 
ListBoxImpl::Clear() {
	BListView* list = blist(wid);
	BListItem* item;
	BAutolock a(list->Looper());
	if(a.IsLocked()) {
		while((item = list->RemoveItem((int32)0)) != NULL) {
			delete item;
		}
	}
}

void 
ListBoxImpl::Append(char *s, int type) {
	// TODO icons
	BListView* list = blist(wid);
	BStringItem* item = new BStringItem(s);
	int sWidth = list->StringWidth(s);
	if(sWidth > maxTextWidth) maxTextWidth = sWidth;
	
	BAutolock a(list->Looper());
	if(a.IsLocked()) {
		list->AddItem(item);
	}
}

int 
ListBoxImpl::Length(){
	BListView* list = blist(wid);
	return list->CountItems();
}

void 
ListBoxImpl::Select(int n){
	BListView* list = blist(wid);
	list->Select(n);
	BAutolock a(list->Looper());
	if(a.IsLocked()) {
		list->ScrollToSelection();
	}
}

int 
ListBoxImpl::GetSelection(){
	BListView* list = blist(wid);
	return list->CurrentSelection();
}

int 
ListBoxImpl::Find(const char *prefix){
	DB
	return 0;
}

void 
ListBoxImpl::GetValue(int n, char *value, int len) {
	BListView* list = blist(wid);
	BStringItem* item = static_cast<BStringItem*>(list->ItemAt(n));
	strncpy(value, item->Text(), len);
	value[len - 1] = '\0';
}

void 
ListBoxImpl::RegisterImage(int type, const char *xpm_data){
	DB
	// TODO: convert to RGBA32
}

void
ListBoxImpl::RegisterRGBAImage(int type, int width, int height, const unsigned char *pixelsImage){
	DB
	// TODO: register in std::map
}

void 
ListBoxImpl::ClearRegisteredImages(){
	DB
	// TODO: clear std::map
}

void 
ListBoxImpl::SetDoubleClickAction(CallBackAction, void *){
	
}

void 
ListBoxImpl::SetList(const char* list, char separator, char typesep){
	// This method is *not* platform dependent.
	// It is borrowed from the GTK implementation.
	Clear();
	size_t count = strlen(list) + 1;
	std::vector<char> words(list, list+count);
	char *startword = &words[0];
	char *numword = NULL;
	int i = 0;
	for (; words[i]; i++) {
		if (words[i] == separator) {
			words[i] = '\0';
			if (numword)
				*numword = '\0';
			Append(startword, numword?atoi(numword + 1):-1);
			startword = &words[0] + i + 1;
			numword = NULL;
		} else if (words[i] == typesep) {
			numword = &words[0] + i;
		}
	}
	if (startword) {
		if (numword)
			*numword = '\0';
		Append(startword, numword?atoi(numword + 1):-1);
	}
	
}

class DynamicLibraryImpl : public DynamicLibrary {
protected:
	image_id lib;
public:
	DynamicLibraryImpl(const char* modulePath) {
		lib = load_add_on(modulePath);
	}
	
	virtual ~DynamicLibraryImpl() {
		if(lib > 0) {
			unload_add_on(lib);
		}
		lib = 0;
	}
	
	virtual Function FindFunction(const char* name) {
		if(lib > 0) {
			void* fp;
			if(get_image_symbol(lib, name, B_SYMBOL_TYPE_TEXT, &fp) == B_OK) {
				return static_cast<Function>(fp);
			}
		}
		return NULL;
	}
	
	virtual bool IsValid() {
		return lib > 0;
	}
};

DynamicLibrary* DynamicLibrary::Load(const char* name) {
	return static_cast<DynamicLibrary*>(new DynamicLibraryImpl(name));
}



// TODO: Consider if I should be using GetCurrentEventTime instead of gettimeoday
ElapsedTime::ElapsedTime() {
	struct timeval curTime;gettimeofday( &curTime, NULL );

	bigBit = curTime.tv_sec;
	littleBit = curTime.tv_usec;
}

double ElapsedTime::Duration(bool reset) {
	struct timeval curTime;
	gettimeofday( &curTime, NULL );
	long endBigBit = curTime.tv_sec;
	long endLittleBit = curTime.tv_usec;
	double result = 1000000.0 * (endBigBit - bigBit);
	result += endLittleBit - littleBit;
	result /= 1000000.0;
	if (reset) {
		bigBit = endBigBit;
		littleBit = endLittleBit;
	}
	return result;
}

ColourDesired Platform::Chrome() {
	rgb_color c = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
	return ColourDesired(c.red, c.green, c.blue);
}

ColourDesired Platform::ChromeHighlight() {
	rgb_color c = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	return ColourDesired(c.red, c.green, c.blue);
}

char* default_font_family = NULL;

const char *Platform::DefaultFont() {
	if(default_font_family == NULL) {
		if(be_plain_font) {
			font_family ff;
			font_style fs;
			be_plain_font->GetFamilyAndStyle(&ff, &fs);
			default_font_family = strdup(ff);
		} else {
			default_font_family = strdup("Sans");
		}
	}
	return default_font_family;
}

int Platform::DefaultFontSize() {
	return be_plain_font->Size();
}

unsigned int Platform::DoubleClickTime() {
	bigtime_t interval;
	if(get_click_speed(&interval) == B_OK)
		return interval / 1000;
	return 500;
}

bool Platform::MouseButtonBounce() {	
	return false;
}

bool Platform::IsKeyDown(int keyCode) {
	return false;
}

long Platform::SendScintilla(WindowID w, unsigned int msg, unsigned long wParam, long lParam) {
	return 0;
}

bool Platform::IsDBCSLeadByte(int /*codePage*/, char /*ch*/) {
	// No need to implement this
	return false;
}

int Platform::DBCSCharLength(int /*codePage*/, const char* /*s*/) {
	// No need to implement this
	return 1;
}

int Platform::DBCSCharMaxLength() {
	// No need to implement this
	return 2;
}

// These are utility functions not really tied to a platform
int Platform::Minimum(int a, int b) {
	if (a < b)
		return a;
	else
		return b;
}

int Platform::Maximum(int a, int b) {
	if (a > b)
		return a;
	else
		return b;
}

#ifdef TRACE

void Platform::DebugDisplay(const char *s) {
	fprintf( stderr, s );
}

void Platform::DebugPrintf(const char *format, ...) {
	const int BUF_SIZE = 2000;
	char buffer[BUF_SIZE];

	va_list pArguments;
	va_start(pArguments, format);
	vsnprintf(buffer, BUF_SIZE, format, pArguments);
	va_end(pArguments);
	Platform::DebugDisplay(buffer);
}

#else

void Platform::DebugDisplay(const char *) {}

void Platform::DebugPrintf(const char *, ...) {}

#endif

// Not supported for GTK+
static bool assertionPopUps = true;

bool Platform::ShowAssertionPopUps(bool assertionPopUps_) {
	bool ret = assertionPopUps;
	assertionPopUps = assertionPopUps_;
	return ret;
}

void Platform::Assert(const char *c, const char *file, int line) {
	char buffer[2000];
	sprintf(buffer, "Assertion [%s] failed at %s %d", c, file, line);
	strcat(buffer, "\r\n");
	Platform::DebugDisplay(buffer);
	
}

int Platform::Clamp(int val, int minVal, int maxVal) {
	if (val > maxVal)
		val = maxVal;
	if (val < minVal)
		val = minVal;
	return val;
}

#ifdef SCI_NAMESPACE
}
#endif
