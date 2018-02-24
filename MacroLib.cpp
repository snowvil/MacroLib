#include "stdafx.h"
#include "MacroLib.h"


#define SET_COLOR_RANGE \
{\
	red_low = (aVariation > search_red) ? 0 : search_red - aVariation;\
	green_low = (aVariation > search_green) ? 0 : search_green - aVariation;\
	blue_low = (aVariation > search_blue) ? 0 : search_blue - aVariation;\
	red_high = (aVariation > 0xFF - search_red) ? 0xFF : search_red + aVariation;\
	green_high = (aVariation > 0xFF - search_green) ? 0xFF : search_green + aVariation;\
	blue_high = (aVariation > 0xFF - search_blue) ? 0xFF : search_blue + aVariation;\
}

CMacroLib::CMacroLib(void)
{

	sModifiersLR_persistent = 0;
	sSendMode = SM_EVENT;
}


CMacroLib::~CMacroLib(void)
{
}

ResultType CMacroLib::ImageSearch( int aLeft, int aTop, int aRight, int aBottom, LPTSTR aImageFile, int &nRetPosX, int &nRetPosY )
{

	POINT origin = {0};
	CoordToScreen(origin, COORD_MODE_PIXEL);
	aLeft   += origin.x;
	aTop    += origin.y;
	aRight  += origin.x;
	aBottom += origin.y;

	// Options are done as asterisk+option to permit future expansion.
	// Set defaults to be possibly overridden by any specified options:
	int aVariation = 0;  // This is named aVariation vs. variation for use with the SET_COLOR_RANGE macro.
	COLORREF trans_color = CLR_NONE; // The default must be a value that can't occur naturally in an image.
	int icon_number = 0; // Zero means "load icon or bitmap (doesn't matter)".
	int width = 0, height = 0;
	// For icons, override the default to be 16x16 because that is what is sought 99% of the time.
	// This new default can be overridden by explicitly specifying w0 h0:
	LPTSTR cp = _tcsrchr(aImageFile, '.');
	if (cp)
	{
		++cp;
		if (!(_tcsicmp(cp, _T("ico")) && _tcsicmp(cp, _T("exe")) && _tcsicmp(cp, _T("dll"))))
			width = GetSystemMetrics(SM_CXSMICON), height = GetSystemMetrics(SM_CYSMICON);
	}

	TCHAR color_name[32], *dp;
	cp = omit_leading_whitespace(aImageFile); // But don't alter aImageFile yet in case it contains literal whitespace we want to retain.
	while (*cp == '*')
	{
		++cp;
		switch (_totupper(*cp))
		{
		case 'W': width = ATOI(cp + 1); break;
		case 'H': height = ATOI(cp + 1); break;
		default:
			if (!_tcsnicmp(cp, _T("Icon"), 4))
			{
				cp += 4;  // Now it's the character after the word.
				icon_number = ATOI(cp); // LoadPicture() correctly handles any negative value.
			}
			else if (!_tcsnicmp(cp, _T("Trans"), 5))
			{
				cp += 5;  // Now it's the character after the word.
				// Isolate the color name/number for ColorNameToBGR():
				tcslcpy(color_name, cp, _countof(color_name));
				if (dp = StrChrAny(color_name, _T(" \t"))) // Find space or tab, if any.
					*dp = '\0';
				// Fix for v1.0.44.10: Treat trans_color as containing an RGB value (not BGR) so that it matches
				// the documented behavior.  In older versions, a specified color like "TransYellow" was wrong in
				// every way (inverted) and a specified numeric color like "Trans0xFFFFAA" was treated as BGR vs. RGB.
				trans_color = ColorNameToBGR(color_name);
				if (trans_color == CLR_NONE) // A matching color name was not found, so assume it's in hex format.
					// It seems _tcstol() automatically handles the optional leading "0x" if present:
						trans_color = _tcstol(color_name, NULL, 16);
				// if color_name did not contain something hex-numeric, black (0x00) will be assumed,
				// which seems okay given how rare such a problem would be.
				else
					trans_color = bgr_to_rgb(trans_color); // v1.0.44.10: See fix/comment above.

			}
			else // Assume it's a number since that's the only other asterisk-option.
			{
				aVariation = ATOI(cp); // Seems okay to support hex via ATOI because the space after the number is documented as being mandatory.
				if (aVariation < 0)
					aVariation = 0;
				if (aVariation > 255)
					aVariation = 255;
				// Note: because it's possible for filenames to start with a space (even though Explorer itself
				// won't let you create them that way), allow exactly one space between end of option and the
				// filename itself:
			}
		} // switch()
		if (   !(cp = StrChrAny(cp, _T(" \t")))   ) // Find the first space or tab after the option.
			goto error; // Bad option/format.
		// Now it's the space or tab (if there is one) after the option letter.  Advance by exactly one character
		// because only one space or tab is considered the delimiter.  Any others are considered to be part of the
		// filename (though some or all OSes might simply ignore them or tolerate them as first-try match criteria).
		aImageFile = ++cp; // This should now point to another asterisk or the filename itself.
		// Above also serves to reset the filename to omit the option string whenever at least one asterisk-option is present.
		cp = omit_leading_whitespace(cp); // This is done to make it more tolerant of having more than one space/tab between options.
	}

	// Update: Transparency is now supported in icons by using the icon's mask.  In addition, an attempt
	// is made to support transparency in GIF, PNG, and possibly TIF files via the *Trans option, which
	// assumes that one color in the image is transparent.  In GIFs not loaded via GDIPlus, the transparent
	// color might always been seen as pure white, but when GDIPlus is used, it's probably always black
	// like it is in PNG -- however, this will not relied upon, at least not until confirmed.
	// OLDER/OBSOLETE comment kept for background:
	// For now, images that can't be loaded as bitmaps (icons and cursors) are not supported because most
	// icons have a transparent background or color present, which the image search routine here is
	// probably not equipped to handle (since the transparent color, when shown, typically reveals the
	// color of whatever is behind it; thus screen pixel color won't match image's pixel color).
	// So currently, only BMP and GIF seem to work reliably, though some of the other GDIPlus-supported
	// formats might work too.
	int image_type;
	bool no_delete_bitmap;
	HBITMAP hbitmap_image = LoadPicture(aImageFile, width, height, image_type, icon_number, false, &no_delete_bitmap);
	// The comment marked OBSOLETE below is no longer true because the elimination of the high-byte via
	// 0x00FFFFFF seems to have fixed it.  But "true" is still not passed because that should increase
	// consistency when GIF/BMP/ICO files are used by a script on both Win9x and other OSs (since the
	// same loading method would be used via "false" for these formats across all OSes).
	// OBSOLETE: Must not pass "true" with the above because that causes bitmaps and gifs to be not found
	// by the search.  In other words, nothing works.  Obsolete comment: Pass "true" so that an attempt
	// will be made to load icons as bitmaps if GDIPlus is available.
	if (!hbitmap_image)
		goto error;

	HDC hdc = GetDC(NULL);
	if (!hdc)
	{
		if (!no_delete_bitmap)
		{
			if (image_type == IMAGE_ICON)
				DestroyIcon((HICON)hbitmap_image);
			else
				DeleteObject(hbitmap_image);
		}
		goto error;
	}

	// From this point on, "goto end" will assume hdc and hbitmap_image are non-NULL, but that the below
	// might still be NULL.  Therefore, all of the following must be initialized so that the "end"
	// label can detect them:
	HDC sdc = NULL;
	HBITMAP hbitmap_screen = NULL;
	LPCOLORREF image_pixel = NULL, screen_pixel = NULL, image_mask = NULL;
	HGDIOBJ sdc_orig_select = NULL;
	bool found = false; // Must init here for use by "goto end".

	bool image_is_16bit;
	LONG image_width, image_height;

	if (image_type == IMAGE_ICON)
	{
		// Must be done prior to IconToBitmap() since it deletes (HICON)hbitmap_image:
		ICONINFO ii;
		if (GetIconInfo((HICON)hbitmap_image, &ii))
		{
			// If the icon is monochrome (black and white), ii.hbmMask will contain twice as many pixels as
			// are actually in the icon.  But since the top half of the pixels are the AND-mask, it seems
			// okay to get all the pixels given the rarity of monochrome icons.  This scenario should be
			// handled properly because: 1) the variables image_height and image_width will be overridden
			// further below with the correct icon dimensions; 2) Only the first half of the pixels within
			// the image_mask array will actually be referenced by the transparency checker in the loops,
			// and that first half is the AND-mask, which is the transparency part that is needed.  The
			// second half, the XOR part, is not needed and thus ignored.  Also note that if width/height
			// required the icon to be scaled, LoadPicture() has already done that directly to the icon,
			// so ii.hbmMask should already be scaled to match the size of the bitmap created later below.
			image_mask = getbits(ii.hbmMask, hdc, image_width, image_height, image_is_16bit, 1);
			DeleteObject(ii.hbmColor); // DeleteObject() probably handles NULL okay since few MSDN/other examples ever check for NULL.
			DeleteObject(ii.hbmMask);
		}
		if (   !(hbitmap_image = IconToBitmap((HICON)hbitmap_image, true))   )
			goto error;
	}

	if (   !(image_pixel = getbits(hbitmap_image, hdc, image_width, image_height, image_is_16bit))   )
		goto end;

	// Create an empty bitmap to hold all the pixels currently visible on the screen that lie within the search area:
	int search_width = aRight - aLeft + 1;
	int search_height = aBottom - aTop + 1;
	if (   !(sdc = CreateCompatibleDC(hdc)) || !(hbitmap_screen = CreateCompatibleBitmap(hdc, search_width, search_height))   )
		goto end;

	if (   !(sdc_orig_select = SelectObject(sdc, hbitmap_screen))   )
		goto end;

	// Copy the pixels in the search-area of the screen into the DC to be searched:
	if (   !(BitBlt(sdc, 0, 0, search_width, search_height, hdc, aLeft, aTop, SRCCOPY))   )
		goto end;

	LONG screen_width, screen_height;
	bool screen_is_16bit;
	if (   !(screen_pixel = getbits(hbitmap_screen, sdc, screen_width, screen_height, screen_is_16bit))   )
		goto end;

	LONG image_pixel_count = image_width * image_height;
	LONG screen_pixel_count = screen_width * screen_height;
	int i, j, k, x, y; // Declaring as "register" makes no performance difference with current compiler, so let the compiler choose which should be registers.

	// If either is 16-bit, convert *both* to the 16-bit-compatible 32-bit format:
	if (image_is_16bit || screen_is_16bit)
	{
		if (trans_color != CLR_NONE)
			trans_color &= 0x00F8F8F8; // Convert indicated trans-color to be compatible with the conversion below.
		for (i = 0; i < screen_pixel_count; ++i)
			screen_pixel[i] &= 0x00F8F8F8; // Highest order byte must be masked to zero for consistency with use of 0x00FFFFFF below.
		for (i = 0; i < image_pixel_count; ++i)
			image_pixel[i] &= 0x00F8F8F8;  // Same.
	}

	// v1.0.44.03: The below is now done even for variation>0 mode so its results are consistent with those of
	// non-variation mode.  This is relied upon by variation=0 mode but now also by the following line in the
	// variation>0 section:
	//     || image_pixel[j] == trans_color
	// Without this change, there are cases where variation=0 would find a match but a higher variation
	// (for the same search) wouldn't. 
	for (i = 0; i < image_pixel_count; ++i)
		image_pixel[i] &= 0x00FFFFFF;

	// Search the specified region for the first occurrence of the image:
	if (aVariation < 1) // Caller wants an exact match.
	{
		// Concerning the following use of 0x00FFFFFF, the use of 0x00F8F8F8 above is related (both have high order byte 00).
		// The following needs to be done only when shades-of-variation mode isn't in effect because
		// shades-of-variation mode ignores the high-order byte due to its use of macros such as GetRValue().
		// This transformation incurs about a 15% performance decrease (percentage is fairly constant since
		// it is proportional to the search-region size, which tends to be much larger than the search-image and
		// is therefore the primary determination of how long the loops take). But it definitely helps find images
		// more successfully in some cases.  For example, if a PNG file is displayed in a GUI window, this
		// transformation allows certain bitmap search-images to be found via variation==0 when they otherwise
		// would require variation==1 (possibly the variation==1 success is just a side-effect of it
		// ignoring the high-order byte -- maybe a much higher variation would be needed if the high
		// order byte were also subject to the same shades-of-variation analysis as the other three bytes [RGB]).
		for (i = 0; i < screen_pixel_count; ++i)
			screen_pixel[i] &= 0x00FFFFFF;

		for (i = 0; i < screen_pixel_count; ++i)
		{
			// Unlike the variation-loop, the following one uses a first-pixel optimization to boost performance
			// by about 10% because it's only 3 extra comparisons and exact-match mode is probably used more often.
			// Before even checking whether the other adjacent pixels in the region match the image, ensure
			// the image does not extend past the right or bottom edges of the current part of the search region.
			// This is done for performance but more importantly to prevent partial matches at the edges of the
			// search region from being considered complete matches.
			// The following check is ordered for short-circuit performance.  In addition, image_mask, if
			// non-NULL, is used to determine which pixels are transparent within the image and thus should
			// match any color on the screen.
			if ((screen_pixel[i] == image_pixel[0] // A screen pixel has been found that matches the image's first pixel.
			|| image_mask && image_mask[0]     // Or: It's an icon's transparent pixel, which matches any color.
			|| image_pixel[0] == trans_color)  // This should be okay even if trans_color==CLR_NONE, since CLR_NONE should never occur naturally in the image.
				&& image_height <= screen_height - i/screen_width // Image is short enough to fit in the remaining rows of the search region.
				&& image_width <= screen_width - i%screen_width)  // Image is narrow enough not to exceed the right-side boundary of the search region.
			{
				// Check if this candidate region -- which is a subset of the search region whose height and width
				// matches that of the image -- is a pixel-for-pixel match of the image.
				for (found = true, x = 0, y = 0, j = 0, k = i; j < image_pixel_count; ++j)
				{
					if (!(found = (screen_pixel[k] == image_pixel[j] // At least one pixel doesn't match, so this candidate is discarded.
					|| image_mask && image_mask[j]      // Or: It's an icon's transparent pixel, which matches any color.
					|| image_pixel[j] == trans_color))) // This should be okay even if trans_color==CLR_NONE, since CLR none should never occur naturally in the image.
						break;
					if (++x < image_width) // We're still within the same row of the image, so just move on to the next screen pixel.
						++k;
					else // We're starting a new row of the image.
					{
						x = 0; // Return to the leftmost column of the image.
						++y;   // Move one row downward in the image.
						// Move to the next row within the current-candidate region (not the entire search region).
						// This is done by moving vertically downward from "i" (which is the upper-left pixel of the
						// current-candidate region) by "y" rows.
						k = i + y*screen_width; // Verified correct.
					}
				}
				if (found) // Complete match found.
					break;
			}
		}
	}
	else // Allow colors to vary by aVariation shades; i.e. approximate match is okay.
	{
		// The following section is part of the first-pixel-check optimization that improves performance by
		// 15% or more depending on where and whether a match is found.  This section and one the follows
		// later is commented out to reduce code size.
		// Set high/low range for the first pixel of the image since it is the pixel most often checked
		// (i.e. for performance).
		//BYTE search_red1 = GetBValue(image_pixel[0]);  // Because it's RGB vs. BGR, the B value is fetched, not R (though it doesn't matter as long as everything is internally consistent here).
		//BYTE search_green1 = GetGValue(image_pixel[0]);
		//BYTE search_blue1 = GetRValue(image_pixel[0]); // Same comment as above.
		//BYTE red_low1 = (aVariation > search_red1) ? 0 : search_red1 - aVariation;
		//BYTE green_low1 = (aVariation > search_green1) ? 0 : search_green1 - aVariation;
		//BYTE blue_low1 = (aVariation > search_blue1) ? 0 : search_blue1 - aVariation;
		//BYTE red_high1 = (aVariation > 0xFF - search_red1) ? 0xFF : search_red1 + aVariation;
		//BYTE green_high1 = (aVariation > 0xFF - search_green1) ? 0xFF : search_green1 + aVariation;
		//BYTE blue_high1 = (aVariation > 0xFF - search_blue1) ? 0xFF : search_blue1 + aVariation;
		// Above relies on the fact that the 16-bit conversion higher above was already done because like
		// in PixelSearch, it seems more appropriate to do the 16-bit conversion prior to setting the range
		// of high and low colors (vs. than applying 0xF8 to each of the high/low values individually).

		BYTE red, green, blue;
		BYTE search_red, search_green, search_blue;
		BYTE red_low, green_low, blue_low, red_high, green_high, blue_high;

		// The following loop is very similar to its counterpart above that finds an exact match, so maintain
		// them together and see above for more detailed comments about it.
		for (i = 0; i < screen_pixel_count; ++i)
		{
			// The following is commented out to trade code size reduction for performance (see comment above).
			//red = GetBValue(screen_pixel[i]);   // Because it's RGB vs. BGR, the B value is fetched, not R (though it doesn't matter as long as everything is internally consistent here).
			//green = GetGValue(screen_pixel[i]);
			//blue = GetRValue(screen_pixel[i]);
			//if ((red >= red_low1 && red <= red_high1
			//	&& green >= green_low1 && green <= green_high1
			//	&& blue >= blue_low1 && blue <= blue_high1 // All three color components are a match, so this screen pixel matches the image's first pixel.
			//		|| image_mask && image_mask[0]         // Or: It's an icon's transparent pixel, which matches any color.
			//		|| image_pixel[0] == trans_color)      // This should be okay even if trans_color==CLR_NONE, since CLR none should never occur naturally in the image.
			//	&& image_height <= screen_height - i/screen_width // Image is short enough to fit in the remaining rows of the search region.
			//	&& image_width <= screen_width - i%screen_width)  // Image is narrow enough not to exceed the right-side boundary of the search region.

			// Instead of the above, only this abbreviated check is done:
			if (image_height <= screen_height - i/screen_width    // Image is short enough to fit in the remaining rows of the search region.
				&& image_width <= screen_width - i%screen_width)  // Image is narrow enough not to exceed the right-side boundary of the search region.
			{
				// Since the first pixel is a match, check the other pixels.
				for (found = true, x = 0, y = 0, j = 0, k = i; j < image_pixel_count; ++j)
				{
					search_red = GetBValue(image_pixel[j]);
					search_green = GetGValue(image_pixel[j]);
					search_blue = GetRValue(image_pixel[j]);
					SET_COLOR_RANGE
					red = GetBValue(screen_pixel[k]);
					green = GetGValue(screen_pixel[k]);
					blue = GetRValue(screen_pixel[k]);

					if (!(found = red >= red_low && red <= red_high
						&& green >= green_low && green <= green_high
						&& blue >= blue_low && blue <= blue_high
						|| image_mask && image_mask[j]     // Or: It's an icon's transparent pixel, which matches any color.
					|| image_pixel[j] == trans_color)) // This should be okay even if trans_color==CLR_NONE, since CLR_NONE should never occur naturally in the image.
						break; // At least one pixel doesn't match, so this candidate is discarded.
					if (++x < image_width) // We're still within the same row of the image, so just move on to the next screen pixel.
						++k;
					else // We're starting a new row of the image.
					{
						x = 0; // Return to the leftmost column of the image.
						++y;   // Move one row downward in the image.
						k = i + y*screen_width; // Verified correct.
					}
				}
				if (found) // Complete match found.
					break;
			}
		}
	}

end:
	// If found==false when execution reaches here, ErrorLevel is already set to the right value, so just
	// clean up then return.
	ReleaseDC(NULL, hdc);
	if (!no_delete_bitmap)
		DeleteObject(hbitmap_image);
	if (sdc)
	{
		if (sdc_orig_select) // i.e. the original call to SelectObject() didn't fail.
			SelectObject(sdc, sdc_orig_select); // Probably necessary to prevent memory leak.
		DeleteDC(sdc);
	}
	if (hbitmap_screen)
		DeleteObject(hbitmap_screen);
	if (image_pixel)
		free(image_pixel);
	if (image_mask)
		free(image_mask);
	if (screen_pixel)
		free(screen_pixel);
	else // One of the GDI calls failed.
		goto error;

	if (!found) // Let ErrorLevel, which is either "1" or "2" as set earlier, tell the story.
		return FAIL;

	// Otherwise, success.  Calculate xpos and ypos of where the match was found and adjust
	// coords to make them relative to the position of the target window (rect will contain
	// zeroes if this doesn't need to be done):
	nRetPosX = (aLeft + i%screen_width) - origin.x;
	nRetPosY = (aTop + i/screen_width) - origin.y;

	return OK;

error:
	return CRITICAL_ERROR;
}


LPCOLORREF CMacroLib::getbits(HBITMAP ahImage, HDC hdc, LONG &aWidth, LONG &aHeight, bool &aIs16Bit, int aMinColorDepth)
{
	HDC tdc = CreateCompatibleDC(hdc);
	if (!tdc)
		return NULL;

	// From this point on, "goto end" will assume tdc is non-NULL, but that the below
	// might still be NULL.  Therefore, all of the following must be initialized so that the "end"
	// label can detect them:
	HGDIOBJ tdc_orig_select = NULL;
	LPCOLORREF image_pixel = NULL;
	bool success = false;

	// Confirmed:
	// Needs extra memory to prevent buffer overflow due to: "A bottom-up DIB is specified by setting
	// the height to a positive number, while a top-down DIB is specified by setting the height to a
	// negative number. THE BITMAP COLOR TABLE WILL BE APPENDED to the BITMAPINFO structure."
	// Maybe this applies only to negative height, in which case the second call to GetDIBits()
	// below uses one.
	struct BITMAPINFO3
	{
		BITMAPINFOHEADER    bmiHeader;
		RGBQUAD             bmiColors[260];  // v1.0.40.10: 260 vs. 3 to allow room for color table when color depth is 8-bit or less.
	} bmi;

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biBitCount = 0; // i.e. "query bitmap attributes" only.
	if (!GetDIBits(tdc, ahImage, 0, 0, NULL, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS)
		|| bmi.bmiHeader.biBitCount < aMinColorDepth) // Relies on short-circuit boolean order.
		goto end;

	// Set output parameters for caller:
	aIs16Bit = (bmi.bmiHeader.biBitCount == 16);
	aWidth = bmi.bmiHeader.biWidth;
	aHeight = bmi.bmiHeader.biHeight;

	int image_pixel_count = aWidth * aHeight;
	if (   !(image_pixel = (LPCOLORREF)malloc(image_pixel_count * sizeof(COLORREF)))   )
		goto end;

	// v1.0.40.10: To preserve compatibility with callers who check for transparency in icons, don't do any
	// of the extra color table handling for 1-bpp images.  Update: For code simplification, support only
	// 8-bpp images.  If ever support lower color depths, use something like "bmi.bmiHeader.biBitCount > 1
	// && bmi.bmiHeader.biBitCount < 9";
	bool is_8bit = (bmi.bmiHeader.biBitCount == 8);
	if (!is_8bit)
		bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biHeight = -bmi.bmiHeader.biHeight; // Storing a negative inside the bmiHeader struct is a signal for GetDIBits().

	// Must be done only after GetDIBits() because: "The bitmap identified by the hbmp parameter
	// must not be selected into a device context when the application calls GetDIBits()."
	// (Although testing shows it works anyway, perhaps because GetDIBits() above is being
	// called in its informational mode only).
	// Note that this seems to return NULL sometimes even though everything still works.
	// Perhaps that is normal.
	tdc_orig_select = SelectObject(tdc, ahImage); // Returns NULL when we're called the second time?

	// Apparently there is no need to specify DIB_PAL_COLORS below when color depth is 8-bit because
	// DIB_RGB_COLORS also retrieves the color indices.
	if (   !(GetDIBits(tdc, ahImage, 0, aHeight, image_pixel, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS))   )
		goto end;

	if (is_8bit) // This section added in v1.0.40.10.
	{
		// Convert the color indices to RGB colors by going through the array in reverse order.
		// Reverse order allows an in-place conversion of each 8-bit color index to its corresponding
		// 32-bit RGB color.
		LPDWORD palette = (LPDWORD)_alloca(256 * sizeof(PALETTEENTRY));
		GetSystemPaletteEntries(tdc, 0, 256, (LPPALETTEENTRY)palette); // Even if failure can realistically happen, consequences of using uninitialized palette seem acceptable.
		// Above: GetSystemPaletteEntries() is the only approach that provided the correct palette.
		// The following other approaches didn't give the right one:
		// GetDIBits(): The palette it stores in bmi.bmiColors seems completely wrong.
		// GetPaletteEntries()+GetCurrentObject(hdc, OBJ_PAL): Returned only 20 entries rather than the expected 256.
		// GetDIBColorTable(): I think same as above or maybe it returns 0.

		// The following section is necessary because apparently each new row in the region starts on
		// a DWORD boundary.  So if the number of pixels in each row isn't an exact multiple of 4, there
		// are between 1 and 3 zero-bytes at the end of each row.
		int remainder = aWidth % 4;
		int empty_bytes_at_end_of_each_row = remainder ? (4 - remainder) : 0;

		// Start at the last RGB slot and the last color index slot:
		BYTE *byte = (BYTE *)image_pixel + image_pixel_count - 1 + (aHeight * empty_bytes_at_end_of_each_row); // Pointer to 8-bit color indices.
		DWORD *pixel = image_pixel + image_pixel_count - 1; // Pointer to 32-bit RGB entries.

		int row, col;
		for (row = 0; row < aHeight; ++row) // For each row.
		{
			byte -= empty_bytes_at_end_of_each_row;
			for (col = 0; col < aWidth; ++col) // For each column.
				*pixel-- = rgb_to_bgr(palette[*byte--]); // Caller always wants RGB vs. BGR format.
		}
	}

	// Since above didn't "goto end", indicate success:
	success = true;

end:
	if (tdc_orig_select) // i.e. the original call to SelectObject() didn't fail.
		SelectObject(tdc, tdc_orig_select); // Probably necessary to prevent memory leak.
	DeleteDC(tdc);
	if (!success && image_pixel)
	{
		free(image_pixel);
		image_pixel = NULL;
	}
	return image_pixel;
}

ResultType CMacroLib::ControlSend( CWnd* pWnd, LPTSTR aKeysToSend, LPTSTR aTitle, LPTSTR aText, LPTSTR aExcludeTitle, LPTSTR aExcludeText, bool aSendRaw )
{
	ASSERT(pWnd);
	CString strTmp = aKeysToSend;
	m_strKeysToSend = (LPTSTR)(LPCTSTR)strTmp;

	SendKeys(m_strKeysToSend, aSendRaw, SM_EVENT, pWnd->GetSafeHwnd());

	return OK;
}

void CMacroLib::SendKeys( LPTSTR aKeys, bool aSendRaw, SendModes aSendModeOrig, HWND aTargetWindow /*= NULL*/ )
{
	if (!*aKeys)
		return;

	// Might be better to do this prior to changing capslock state.  UPDATE: In v1.0.44.03, the following section
	// has been moved to the top of the function because:
	// 1) For ControlSend, GetModifierLRState() might be more accurate if the threads are attached beforehand.
	// 2) Determines sTargetKeybdLayout and sTargetLayoutHasAltGr early (for maintainability).
	bool threads_are_attached = false; // Set default.
	DWORD keybd_layout_thread = 0;     //
	DWORD target_thread; // Doesn't need init.
	DWORD g_MainThreadID = GetCurrentThreadId();
	if (aTargetWindow) // Caller has ensured this is NULL for SendInput and SendPlay modes.
	{
		if ((target_thread = GetWindowThreadProcessId(aTargetWindow, NULL)) // Assign.
			&& target_thread != g_MainThreadID && !IsHungAppWindow(aTargetWindow))
		{
			threads_are_attached = AttachThreadInput(g_MainThreadID, target_thread, TRUE) != 0;
			keybd_layout_thread = target_thread; // Testing shows that ControlSend benefits from the adapt-to-layout technique too.
		}
		//else no target thread, or it's our thread, or it's hung; so keep keybd_layout_thread at its default.
	}
	else
	{
		
		// v1.0.44.03: The following change is meaningful only to people who use more than one keyboard layout.
		// It seems that the vast majority of them would want the Send command (as well as other features like
		// Hotstrings and the Input command) to adapt to the keyboard layout of the active window (or target window
		// in the case of ControlSend) rather than sticking with the script's own keyboard layout.  In addition,
		// testing shows that this adapt-to-layout method costs almost nothing in performance, especially since
		// the active window, its thread, and its layout are retrieved only once for each Send rather than once
		// for each keystroke.
		HWND active_window;
		if (active_window = GetForegroundWindow())
			keybd_layout_thread = GetWindowThreadProcessId(active_window, NULL);
		//else no foreground window, so keep keybd_layout_thread at default.
	}

	// Below is now called with "true" so that the hook's modifier state will be corrected (if necessary)
	// prior to every send.
	modLR_type mods_current = GetModifierLRState(true); // Current "logical" modifier state.

	// Make a best guess of what the physical state of the keys is prior to starting (there's no way
	// to be certain without the keyboard hook). Note: We only want those physical
	// keys that are also logically down (it's possible for a key to be down physically
	// but not logically such as when R-control, for example, is a suffix hotkey and the
	// user is physically holding it down):
	modLR_type mods_down_physically_orig, mods_down_physically_and_logically
		, mods_down_physically_but_not_logically_orig;

	mods_down_physically_orig = 0;

	mods_down_physically_and_logically = mods_down_physically_orig;
	mods_down_physically_but_not_logically_orig = 0; // There's no way of knowing, so assume none.

	sModifiersLR_persistent &= mods_current & ~mods_down_physically_and_logically;
	modLR_type persistent_modifiers_for_this_SendKeys, extra_persistent_modifiers_for_blind_mode;

	extra_persistent_modifiers_for_blind_mode = 0;
	persistent_modifiers_for_this_SendKeys = sModifiersLR_persistent;


	sSendMode = aSendModeOrig;

	vk_type vk;
	sc_type sc;
	modLR_type key_as_modifiersLR = 0;
	modLR_type mods_for_next_key = 0;
	// Above: For v1.0.35, it was changed to modLR vs. mod so that AltGr keys such as backslash and '{'
	// are supported on layouts such as German when sending to apps such as Putty that are fussy about
	// which ALT key is held down to produce the character.
	vk_type this_event_modifier_down;
	size_t key_text_length, key_name_length;
	TCHAR *end_pos, *space_pos, *next_word, old_char, single_char_string[2];
	KeyEventTypes event_type;
	int repeat_count, click_x, click_y;
	bool move_offset, key_down_is_persistent;
	DWORD placeholder;
	single_char_string[1] = '\0'; // Terminate in advance.

	static vk_type sPrevEventModifierDown = 0;

	LONG_OPERATION_INIT  // Needed even for SendInput/Play.

	for (; *aKeys; ++aKeys, sPrevEventModifierDown = this_event_modifier_down)
	{
		this_event_modifier_down = 0; // Set default for this iteration, overridden selectively below.
		if (!sSendMode)
			LONG_OPERATION_UPDATE_FOR_SENDKEYS // This does not measurably affect the performance of SendPlay/Event.

		if (!aSendRaw && _tcschr(_T("^+!#{}"), *aKeys))
		{
			switch (*aKeys)
			{
			case '^':
				if (!(persistent_modifiers_for_this_SendKeys & (MOD_LCONTROL|MOD_RCONTROL)))
					mods_for_next_key |= MOD_LCONTROL;
				// else don't add it, because the value of mods_for_next_key may also used to determine
				// which keys to release after the key to which this modifier applies is sent.
				// We don't want persistent modifiers to ever be released because that's how
				// AutoIt2 behaves and it seems like a reasonable standard.
				continue;
			case '+':
				if (!(persistent_modifiers_for_this_SendKeys & (MOD_LSHIFT|MOD_RSHIFT)))
					mods_for_next_key |= MOD_LSHIFT;
				continue;
			case '!':
				if (!(persistent_modifiers_for_this_SendKeys & (MOD_LALT|MOD_RALT)))
					mods_for_next_key |= MOD_LALT;
				continue;
			case '#':
				if (!(persistent_modifiers_for_this_SendKeys & (MOD_LWIN|MOD_RWIN)))
					mods_for_next_key |= MOD_LWIN;
				continue;
			case '}': continue;  // Important that these be ignored.  Be very careful about changing this, see below.
			case '{':
			{
				if (   !(end_pos = _tcschr(aKeys + 1, '}'))   ) // Ignore it and due to rarity, don't reset mods_for_next_key.
					continue; // This check is relied upon by some things below that assume a '}' is present prior to the terminator.
				aKeys = omit_leading_whitespace(aKeys + 1); // v1.0.43: Skip leading whitespace inside the braces to be more flexible.
				if (   !(key_text_length = end_pos - aKeys)   )
				{
					if (end_pos[1] == '}')
					{
						// The literal string "{}}" has been encountered, which is interpreted as a single "}".
						++end_pos;
						key_text_length = 1;
					}
					else if (IS_SPACE_OR_TAB(end_pos[1])) // v1.0.48: Support "{} down}", "{} downtemp}" and "{} up}".
					{
						next_word = omit_leading_whitespace(end_pos + 1);
						if (   !_tcsnicmp(next_word, _T("Down"), 4) // "Down" or "DownTemp" (or likely enough).
							|| !_tcsnicmp(next_word, _T("Up"), 2)   )
						{
							if (   !(end_pos = _tcschr(next_word, '}'))   ) // See comments at similar section above.
								continue;
							key_text_length = end_pos - aKeys; // This result must be non-zero due to the checks above.
						}
						else
							goto brace_case_end;  // The loop's ++aKeys will now skip over the '}', ignoring it.
					}
					else // Empty braces {} were encountered (or all whitespace, but literal whitespace isn't sent).
						goto brace_case_end;  // The loop's ++aKeys will now skip over the '}', ignoring it.
				}

				if (!_tcsnicmp(aKeys, _T("Click"), 5))
				{
					*end_pos = '\0';  // Temporarily terminate the string here to omit the closing brace from consideration below.
					ParseClickOptions(omit_leading_whitespace(aKeys + 5), click_x, click_y, vk
						, event_type, repeat_count, move_offset);
					*end_pos = '}';  // Undo temp termination.
					if (repeat_count < 1) // Allow {Click 100, 100, 0} to do a mouse-move vs. click (but modifiers like ^{Click..} aren't supported in this case.
						MouseMove(click_x, click_y, placeholder, 10, move_offset);
					else // Use SendKey because it supports modifiers (e.g. ^{Click}) SendKey requires repeat_count>=1.
						SendKey(vk, 0, mods_for_next_key, persistent_modifiers_for_this_SendKeys
							, repeat_count, event_type, 0, aTargetWindow, click_x, click_y, move_offset);
					goto brace_case_end; // This {} item completely handled, so move on to next.
				}
				else if (!_tcsnicmp(aKeys, _T("Raw"), 3)) // This is used by auto-replace hotstrings too.
				{
					// As documented, there's no way to switch back to non-raw mode afterward since there's no
					// correct way to support special (non-literal) strings such as {Raw Off} while in raw mode.
					aSendRaw = true;
					goto brace_case_end; // This {} item completely handled, so move on to next.
				}

				// Since above didn't "goto", this item isn't {Click}.
				event_type = KEYDOWNANDUP;         // Set defaults.
				repeat_count = 1;                  //
				key_name_length = key_text_length; //
				
				*end_pos = '\0';  // Temporarily terminate the string here to omit the closing brace from consideration below.

				if (space_pos = StrChrAny(aKeys, _T(" \t"))) // Assign. Also, it relies on the fact that {} key names contain no spaces.
				{
					old_char = *space_pos;
					*space_pos = '\0';  // Temporarily terminate here so that TextToVK() can properly resolve a single char.
					key_name_length = space_pos - aKeys; // Override the default value set above.
					next_word = omit_leading_whitespace(space_pos + 1);
					UINT next_word_length = (UINT)(end_pos - next_word);
					if (next_word_length > 0)
					{
						if (!_tcsnicmp(next_word, _T("Down"), 4))
						{
							event_type = KEYDOWN;
							// v1.0.44.05: Added key_down_is_persistent (which is not initialized except here because
							// it's only applicable when event_type==KEYDOWN).  It avoids the following problem:
							// When a key is remapped to become a modifier (such as F1::Control), launching one of
							// the script's own hotkeys via F1 would lead to bad side-effects if that hotkey uses
							// the Send command. This is because the Send command assumes that any modifiers pressed
							// down by the script itself (such as Control) are intended to stay down during all
							// keystrokes generated by that script. To work around this, something like KeyWait F1
							// would otherwise be needed. within any hotkey triggered by the F1 key.
							key_down_is_persistent = _tcsnicmp(next_word + 4, _T("Temp"), 4); // "DownTemp" means non-persistent.
						}
						else if (!_tcsicmp(next_word, _T("Up")))
							event_type = KEYUP;
						else
							repeat_count = ATOI(next_word);
							// Above: If negative or zero, that is handled further below.
							// There is no complaint for values <1 to support scripts that want to conditionally send
							// zero keystrokes, e.g. Send {a %Count%}
					}
				}

				vk = TextToVK(aKeys, &mods_for_next_key, true, false); // false must be passed due to below.
				sc = vk ? 0 : TextToSC(aKeys);  // If sc is 0, it will be resolved by KeyEvent() later.
				if (!vk && !sc && ctoupper(aKeys[0]) == 'V' && ctoupper(aKeys[1]) == 'K')
				{
					LPTSTR sc_string = StrChrAny(aKeys + 2, _T("Ss")); // Look for the "SC" that demarks the scan code.
					if (sc_string && ctoupper(sc_string[1]) == 'C')
						sc = (sc_type)_tcstol(sc_string + 2, NULL, 16);  // Convert from hex.
					// else leave sc set to zero and just get the specified VK.  This supports Send {VKnn}.
					vk = (vk_type)_tcstol(aKeys + 2, NULL, 16);  // Convert from hex.
				}

				if (space_pos)  // undo the temporary termination
					*space_pos = old_char;
				*end_pos = '}';  // undo the temporary termination
				if (repeat_count < 1)
					goto brace_case_end; // Gets rid of one level of indentation. Well worth it.

				if (vk || sc)
				{
					if (key_as_modifiersLR = KeyToModifiersLR(vk, sc)) // Assign
					{
						if (!aTargetWindow)
						{
							if (event_type == KEYDOWN) // i.e. make {Shift down} have the same effect {ShiftDown}
							{
								this_event_modifier_down = vk;
								if (key_down_is_persistent) // v1.0.44.05.
									sModifiersLR_persistent |= key_as_modifiersLR;
								persistent_modifiers_for_this_SendKeys |= key_as_modifiersLR; // v1.0.44.06: Added this line to fix the fact that "DownTemp" should keep the key pressed down after the send.
							}
							else if (event_type == KEYUP) // *not* KEYDOWNANDUP, since that would be an intentional activation of the Start Menu or menu bar.
							{
								
								sModifiersLR_persistent &= ~key_as_modifiersLR;
								// By contrast with KEYDOWN, KEYUP should also remove this modifier
								// from extra_persistent_modifiers_for_blind_mode if it happens to be
								// in there.  For example, if "#i::Send {LWin Up}" is a hotkey,
								// LWin should become persistently up in every respect.
								extra_persistent_modifiers_for_blind_mode &= ~key_as_modifiersLR;
								// Fix for v1.0.43: Also remove LControl if this key happens to be AltGr.
								// Since key_as_modifiersLR isn't 0, update to reflect any changes made above:
								persistent_modifiers_for_this_SendKeys = sModifiersLR_persistent | extra_persistent_modifiers_for_blind_mode;
							}
							// else must never change sModifiersLR_persistent in response to KEYDOWNANDUP
							// because that would break existing scripts.  This is because that same
							// modifier key may have been pushed down via {ShiftDown} rather than "{Shift Down}".
							// In other words, {Shift} should never undo the effects of a prior {ShiftDown}
							// or {Shift down}.
						}
						//else don't add this event to sModifiersLR_persistent because it will not be
						// manifest via keybd_event.  Instead, it will done via less intrusively
						// (less interference with foreground window) via SetKeyboardState() and
						// PostMessage().  This change is for ControlSend in v1.0.21 and has been
						// documented.
					}
					// Below: sModifiersLR_persistent stays in effect (pressed down) even if the key
					// being sent includes that same modifier.  Surprisingly, this is how AutoIt2
					// behaves also, which is good.  Example: Send, {AltDown}!f  ; this will cause
					// Alt to still be down after the command is over, even though F is modified
					// by Alt.
					SendKey(vk, sc, mods_for_next_key, persistent_modifiers_for_this_SendKeys
						, repeat_count, event_type, key_as_modifiersLR, aTargetWindow);
				}

				else if (key_name_length == 1) // No vk/sc means a char of length one is sent via special method.
				{
					// v1.0.40: SendKeySpecial sends only keybd_event keystrokes, not ControlSend style
					// keystrokes.
					// v1.0.43.07: Added check of event_type!=KEYUP, which causes something like Send {ð up} to
					// do nothing if the curr. keyboard layout lacks such a key.  This is relied upon by remappings
					// such as F1::ð (i.e. a destination key that doesn't have a VK, at least in English).
					if (event_type != KEYUP) // In this mode, mods_for_next_key and event_type are ignored due to being unsupported.
					{
						if (aTargetWindow)
							// Although MSDN says WM_CHAR uses UTF-16, it seems to really do automatic
							// translation between ANSI and UTF-16; we rely on this for correct results:
							PostMessage(aTargetWindow, WM_CHAR, aKeys[0], 0);
						else
							SendKeySpecial(aKeys[0], repeat_count);
					}
				}

				// See comment "else must never change sModifiersLR_persistent" above about why
				// !aTargetWindow is used below:
				else if (vk = TextToSpecial(aKeys, key_text_length, event_type
					, persistent_modifiers_for_this_SendKeys, !aTargetWindow)) // Assign.
				{
					if (!aTargetWindow)
					{
						if (event_type == KEYDOWN)
							this_event_modifier_down = vk;
						//else // It must be KEYUP because TextToSpecial() never returns KEYDOWNANDUP.
							//DisguiseWinAltIfNeeded(vk);
					}
					// Since we're here, repeat_count > 0.
					// v1.0.42.04: A previous call to SendKey() or SendKeySpecial() might have left modifiers
					// in the wrong state (e.g. Send +{F1}{ControlDown}).  Since modifiers can sometimes affect
					// each other, make sure they're in the state intended by the user before beginning:
					SetModifierLRState(persistent_modifiers_for_this_SendKeys
						, sSendMode ? sEventModifiersLR : GetModifierLRState()
						, aTargetWindow, false, false); // It also does DoKeyDelay(g->PressDuration).
					for (int i = 0; i < repeat_count; ++i)
					{
						// Don't tell it to save & restore modifiers because special keys like this one
						// should have maximum flexibility (i.e. nothing extra should be done so that the
						// user can have more control):
						KeyEvent(event_type, vk, 0, aTargetWindow, true);
						if (!sSendMode)
							LONG_OPERATION_UPDATE_FOR_SENDKEYS
					}
				}

				else if (key_text_length > 4 && !_tcsnicmp(aKeys, _T("ASC "), 4) && !aTargetWindow) // {ASC nnnnn}
				{
					// Include the trailing space in "ASC " to increase uniqueness (selectivity).
					// Also, sending the ASC sequence to window doesn't work, so don't even try:
					SendASC(omit_leading_whitespace(aKeys + 3));
					// Do this only once at the end of the sequence:
					DoKeyDelay(); // It knows not to do the delay for SM_INPUT.
				}

				else if (key_text_length > 2 && !_tcsnicmp(aKeys, _T("U+"), 2))
				{
					// L24: Send a unicode value as shown by Character Map.
					wchar_t u_code = (wchar_t) _tcstol(aKeys + 2, NULL, 16);

					if (aTargetWindow)
					{
						// Although MSDN says WM_CHAR uses UTF-16, PostMessageA appears to truncate it to 8-bit.
						// This probably means it does automatic translation between ANSI and UTF-16.  Since we
						// specifically want to send a Unicode character value, use PostMessageW:
						PostMessageW(aTargetWindow, WM_CHAR, u_code, 0);
					}
					else
					{
						// Use SendInput in unicode mode if available, otherwise fall back to SendASC.
						// To know why the following requires sSendMode != SM_PLAY, see SendUnicodeChar.
						if (sSendMode != SM_PLAY && g_os.IsWin2000orLater())
						{
							SendUnicodeChar(u_code, mods_for_next_key | persistent_modifiers_for_this_SendKeys);
						}
						else // Note that this method generally won't work with Unicode characters except
						{	 // with specific controls which support it, such as RichEdit (tested on WordPad).
							TCHAR asc[8];
							*asc = '0';
							_itot(u_code, asc + 1, 10);
							SendASC(asc);
						}
					}
					DoKeyDelay();
				}

				//else do nothing since it isn't recognized as any of the above "else if" cases (see below).

				// If what's between {} is unrecognized, such as {Bogus}, it's safest not to send
				// the contents literally since that's almost certainly not what the user intended.
				// In addition, reset the modifiers, since they were intended to apply only to
				// the key inside {}.  Also, the below is done even if repeat-count is zero.

brace_case_end: // This label is used to simplify the code without sacrificing performance.
				aKeys = end_pos;  // In prep for aKeys++ done by the loop.
				mods_for_next_key = 0;
				continue;
			} // case '{'
			} // switch()
		} // if (!aSendRaw && strchr("^+!#{}", *aKeys))

		else // Encountered a character other than ^+!#{} ... or we're in raw mode.
		{
			// Best to call this separately, rather than as first arg in SendKey, since it changes the
			// value of modifiers and the updated value is *not* guaranteed to be passed.
			// In other words, SendKey(TextToVK(...), modifiers, ...) would often send the old
			// value for modifiers.
			single_char_string[0] = *aKeys; // String was pre-terminated earlier.
			if (vk = TextToVK(single_char_string, &mods_for_next_key, true, true))
				// TextToVK() takes no measurable time compared to the amount of time SendKey takes.
				SendKey(vk, 0, mods_for_next_key, persistent_modifiers_for_this_SendKeys, 1, KEYDOWNANDUP
					, 0, aTargetWindow);
			else // Try to send it by alternate means.
			{
				// In this mode, mods_for_next_key is ignored due to being unsupported.
				if (aTargetWindow) 
					// Although MSDN says WM_CHAR uses UTF-16, it seems to really do automatic
					// translation between ANSI and UTF-16; we rely on this for correct results:
					PostMessage(aTargetWindow, WM_CHAR, *aKeys, 0);
				else
					SendKeySpecial(*aKeys, 1);
			}
			mods_for_next_key = 0;  // Safest to reset this regardless of whether a key was sent.
		}
	} // for()

	modLR_type mods_to_set;
	if (sSendMode)
	{

		int final_key_delay = -1;  // Set default.
		
		/*
		if (!sAbortArraySend && sEventCount > 0) // Check for zero events for performance, but more importantly because playback hook will not operate correctly with zero.
		{
			// Add more events to the array (prior to sending) to support the following:
			// Restore the modifiers to match those the user is physically holding down, but do it as *part*
			// of the single SendInput/Play call.  The reasons it's done here as part of the array are:
			// 1) It avoids the need for #HotkeyModifierTimeout (and it's superior to it) for both SendInput
			//    and SendPlay.
			// 2) The hook will not be present during the SendInput, nor can it be reinstalled in time to
			//    catch any physical events generated by the user during the Send. Consequently, there is no
			//    known way to reliably detect physical keystate changes.
			// 3) Changes made to modifier state by SendPlay are seen only by the active window's thread.
			//    Thus, it would be inconsistent and possibly incorrect to adjust global modifier state
			//    after (or during) a SendPlay.
			// So rather than resorting to #HotkeyModifierTimeout, we can restore the modifiers within the
			// protection of SendInput/Play's uninterruptibility, allowing the user's buffered keystrokes
			// (if any) to hit against the correct modifier state when the SendInput/Play completes.
			// For example, if #c:: is a hotkey and the user releases Win during the SendInput/Play, that
			// release would hit after SendInput/Play restores Win to the down position, and thus Win would
			// not be stuck down.  Furthermore, if the user didn't release Win, Win would be in the
			// correct/intended position.
			// This approach has a few weaknesses (but the strengths appear to outweigh them):
			// 1) Hitting SendInput's 5000 char limit would omit the tail-end keystrokes, which would mess up
			//    all the assumptions here.  But hitting that limit should be very rare, especially since it's
			//    documented and thus scripts will avoid it.
			// 2) SendInput's assumed uninterruptibility is false if any other app or script has an LL hook
			//    installed.  This too is documented, so scripts should generally avoid using SendInput when
			//    they know there are other LL hooks in the system.  In any case, there's no known solution
			//    for it, so nothing can be done.
			mods_to_set = persistent_modifiers_for_this_SendKeys
				| (sInBlindMode ? 0 : (mods_down_physically_orig & ~mods_down_physically_but_not_logically_orig)); // The last item is usually 0.
			// Above: When in blind mode, don't restore physical modifiers.  This is done to allow a hotkey
			// such as the following to release Shift:
			//    +space::SendInput/Play {Blind}{Shift up}
			// Note that SendPlay can make such a change only from the POV of the target window; i.e. it can
			// release shift as seen by the target window, but not by any other thread; so the shift key would
			// still be considered to be down for the purpose of firing hotkeys (it can't change global key state
			// as seen by GetAsyncKeyState).
			// For more explanation of above, see a similar section for the non-array/old Send below.
			SetModifierLRState(mods_to_set, sEventModifiersLR, NULL, true, true); // Disguise in case user released or pressed Win/Alt during the Send (seems best to do it even for SendPlay, though it probably needs only Alt, not Win).
			// mods_to_set is used further below as the set of modifiers that were explicitly put into effect at the tail end of SendInput.
			SendEventArray(final_key_delay, mods_to_set);
		}
		CleanupEventArray(final_key_delay);
		*/
	}
	else // A non-array send is in effect, so a more elaborate adjustment to logical modifiers is called for.
	{
		// Determine (or use best-guess, if necessary) which modifiers are down physically now as opposed
		// to right before the Send began.
		modLR_type mods_down_physically = 0; // As compared to mods_down_physically_orig.

		// Restore the state of the modifiers to be those the user is physically holding down right now.
		// Any modifiers that are logically "persistent", as detected upon entrance to this function
		// (e.g. due to something such as a prior "Send, {LWinDown}"), are also pushed down if they're not already.
		// Don't press back down the modifiers that were used to trigger this hotkey if there's
		// any doubt that they're still down, since doing so when they're not physically down
		// would cause them to be stuck down, which might cause unwanted behavior when the unsuspecting
		// user resumes typing.
		// v1.0.42.04: Now that SendKey() is lazy about releasing Ctrl and/or Shift (but not Win/Alt),
		// the section below also releases Ctrl/Shift if appropriate.  See SendKey() for more details.
		mods_to_set = persistent_modifiers_for_this_SendKeys; // Set default.
		if (sInBlindMode) // This section is not needed for the array-sending modes because they exploit uninterruptibility to perform a more reliable restoration.
		{
			// At the end of a blind-mode send, modifiers are restored differently than normal. One
			// reason for this is to support the explicit ability for a Send to turn off a hotkey's
			// modifiers even if the user is still physically holding them down.  For example:
			//   #space::Send {LWin up}  ; Fails to release it, by design and for backward compatibility.
			//   #space::Send {Blind}{LWin up}  ; Succeeds, allowing LWin to be logically up even though it's physically down.
			modLR_type mods_changed_physically_during_send = mods_down_physically_orig ^ mods_down_physically;
			// Fix for v1.0.42.04: To prevent keys from getting stuck down, compensate for any modifiers
			// the user physically pressed or released during the Send (especially those released).
			// Remove any modifiers physically released during the send so that they don't get pushed back down:
			mods_to_set &= ~(mods_changed_physically_during_send & mods_down_physically_orig); // Remove those that changed from down to up.
			// Conversely, add any modifiers newly, physically pressed down during the Send, because in
			// most cases the user would want such modifiers to be logically down after the Send.
			// Obsolete comment from v1.0.40: For maximum flexibility and minimum interference while
			// in blind mode, never restore modifiers to the down position then.
			mods_to_set |= mods_changed_physically_during_send & mods_down_physically; // Add those that changed from up to down.
		}
		else // Regardless of whether the keyboard hook is present, the following formula applies.
			mods_to_set |= mods_down_physically & ~mods_down_physically_but_not_logically_orig; // The second item is usually 0.
			// Above takes into account the fact that the user may have pressed and/or released some modifiers
			// during the Send.
			// So it includes all keys that are physically down except those that were down physically but not
			// logically at the *start* of the send operation (since the send operation may have changed the
			// logical state).  In other words, we want to restore the keys to their former logical-down
			// position to match the fact that the user is still holding them down physically.  The
			// previously-down keys we don't do this for are those that were physically but not logically down,
			// such as a naked Control key that's used as a suffix without being a prefix.  More details:
			// mods_down_physically_but_not_logically_orig is used to distinguish between the following two cases,
			// allowing modifiers to be properly restored to the down position when the hook is installed:
			// 1) A naked modifier key used only as suffix: when the user phys. presses it, it isn't
			//    logically down because the hook suppressed it.
			// 2) A modifier that is a prefix, that triggers a hotkey via a suffix, and that hotkey sends
			//    that modifier.  The modifier will go back up after the SEND, so the key will be physically
			//    down but not logically.

		// Use KEY_IGNORE_ALL_EXCEPT_MODIFIER to tell the hook to adjust g_modifiersLR_logical_non_ignored
		// because these keys being put back down match the physical pressing of those same keys by the
		// user, and we want such modifiers to be taken into account for the purpose of deciding whether
		// other hotkeys should fire (or the same one again if auto-repeating):
		// v1.0.42.04: A previous call to SendKey() might have left Shift/Ctrl in the down position
		// because by procrastinating, extraneous keystrokes in examples such as "Send ABCD" are
		// eliminated (previously, such that example released the shift key after sending each key,
		// only to have to press it down again for the next one.  For this reason, some modifiers
		// might get released here in addition to any that need to get pressed down.  That's why
		// SetModifierLRState() is called rather than the old method of pushing keys down only,
		// never releasing them.
		// Put the modifiers in mods_to_set into effect.  Although "true" is passed to disguise up-events,
		// there generally shouldn't be any up-events for Alt or Win because SendKey() would have already
		// released them.  One possible exception to this is when the user physically released Alt or Win
		// during the send (perhaps only during specific sensitive/vulnerable moments).
		SetModifierLRState(mods_to_set, GetModifierLRState(), aTargetWindow, true, true); // It also does DoKeyDelay(g->PressDuration).
	} // End of non-array Send.

	// For peace of mind and because that's how it was tested originally, the following is done
	// only after adjusting the modifier state above (since that adjustment might be able to
	// affect the global variables used below in a meaningful way).


	// Might be better to do this after changing capslock state, since having the threads attached
	// tends to help with updating the global state of keys (perhaps only under Win9x in this case):
	if (threads_are_attached)
		AttachThreadInput(g_MainThreadID, target_thread, FALSE);


	// v1.0.43.03: Someone reported that when a non-autoreplace hotstring calls us to do its backspacing, the
	// hotstring's subroutine can execute a command that activates another window owned by the script before
	// the original window finished receiving its backspaces.  Although I can't reproduce it, this behavior
	// fits with expectations since our thread won't necessarily have a chance to process the incoming
	// keystrokes before executing the command that comes after SendInput.  If those command(s) activate
	// another of this thread's windows, that window will most likely intercept the keystrokes (assuming
	// that the message pump dispatches buffered keystrokes to whichever window is active at the time the
	// message is processed).
	// This fix does not apply to the SendPlay or SendEvent modes, the former due to the fact that it sleeps
	// a lot while the playback is running, and the latter due to key-delay and because testing has never shown
	// a need for it.
	if (aSendModeOrig == SM_INPUT && GetWindowThreadProcessId(GetForegroundWindow(), NULL) == g_MainThreadID) // GetWindowThreadProcessId() tolerates a NULL hwnd.
		Sleep(0);

}

modLR_type CMacroLib::GetModifierLRState( bool aExplicitlyGet /*= false*/ )
{
	modLR_type modifiersLR = 0;  // Allows all to default to up/off to simplify the below.
	if (g_os.IsWin9x() || g_os.IsWinNT4())
	{
		if (IsKeyDown9xNT(VK_SHIFT))   modifiersLR |= MOD_LSHIFT;
		if (IsKeyDown9xNT(VK_CONTROL)) modifiersLR |= MOD_LCONTROL;
		if (IsKeyDown9xNT(VK_MENU))    modifiersLR |= MOD_LALT;
		if (IsKeyDown9xNT(VK_LWIN))    modifiersLR |= MOD_LWIN;
		if (IsKeyDown9xNT(VK_RWIN))    modifiersLR |= MOD_RWIN;
	}
	else
	{
		if (IsKeyDownAsync(VK_LSHIFT))   modifiersLR |= MOD_LSHIFT;
		if (IsKeyDownAsync(VK_RSHIFT))   modifiersLR |= MOD_RSHIFT;
		if (IsKeyDownAsync(VK_LCONTROL)) modifiersLR |= MOD_LCONTROL;
		if (IsKeyDownAsync(VK_RCONTROL)) modifiersLR |= MOD_RCONTROL;
		if (IsKeyDownAsync(VK_LMENU))    modifiersLR |= MOD_LALT;
		if (IsKeyDownAsync(VK_RMENU))    modifiersLR |= MOD_RALT;
		if (IsKeyDownAsync(VK_LWIN))     modifiersLR |= MOD_LWIN;
		if (IsKeyDownAsync(VK_RWIN))     modifiersLR |= MOD_RWIN;
	}

	return modifiersLR;

}


void CMacroLib::ParseClickOptions( LPTSTR aOptions, int &aX, int &aY, vk_type &aVK, KeyEventTypes &aEventType, int &aRepeatCount, bool &aMoveOffset )
{
	// Set defaults for all output parameters for caller.
	aX = COORD_UNSPECIFIED;
	aY = COORD_UNSPECIFIED;
	aVK = VK_LBUTTON_LOGICAL; // v1.0.43: Logical vs. physical for {Click} and Click-cmd, in case user has buttons swapped via control panel.
	aEventType = KEYDOWNANDUP;
	aRepeatCount = 1;
	aMoveOffset = false;

	TCHAR *next_option, *option_end, orig_char;
	vk_type temp_vk;

	for (next_option = aOptions; *next_option; next_option = omit_leading_whitespace(option_end))
	{
		// Allow optional commas to make scripts more readable.  Don't support g_delimiter because StrChrAny
		// below doesn't.
		while (*next_option == ',') // Ignore all commas.
			if (!*(next_option = omit_leading_whitespace(next_option + 1)))
				goto break_both; // Entire option string ends in a comma.
		// Find the end of this option item:
		if (   !(option_end = StrChrAny(next_option, _T(" \t,")))   )  // Space, tab, comma.
			option_end = next_option + _tcslen(next_option); // Set to position of zero terminator instead.

		// Temp termination for IsPureNumeric(), ConvertMouseButton(), and peace-of-mind.
		orig_char = *option_end;
		*option_end = '\0';

		// Parameters can occur in almost any order to enhance usability (at the cost of
		// slightly diminishing the ability unambiguously add more parameters in the future).
		// Seems okay to support floats because ATOI() will just omit the decimal portion.
		if (IsPureNumeric(next_option, true, false, true))
		{
			// Any numbers present must appear in the order: X, Y, RepeatCount
			// (optionally with other options between them).
			if (aX == COORD_UNSPECIFIED) // This will be converted into repeat-count if it is later discovered there's no Y coordinate.
				aX = ATOI(next_option);
			else if (aY == COORD_UNSPECIFIED)
				aY = ATOI(next_option);
			else // Third number is the repeat-count (but if there's only one number total, that's repeat count too, see further below).
				aRepeatCount = ATOI(next_option); // If negative or zero, caller knows to handle it as a MouseMove vs. Click.
		}
		else // Mouse button/name and/or Down/Up/Repeat-count is present.
		{
			if (temp_vk = ConvertMouseButton(next_option, true, true))
				aVK = temp_vk;
			else
			{
				switch (ctoupper(*next_option))
				{
				case 'D': aEventType = KEYDOWN; break;
				case 'U': aEventType = KEYUP; break;
				case 'R': aMoveOffset = true; break; // Since it wasn't recognized as the right mouse button, it must have other letters after it, e.g. Rel/Relative.
					// default: Ignore anything else to reserve them for future use.
				}
			}
		}

		// If the item was not handled by the above, ignore it because it is unknown.
		*option_end = orig_char; // Undo the temporary termination because the caller needs aOptions to be unaltered.
	} // for() each item in option list

break_both:
	if (aX != COORD_UNSPECIFIED && aY == COORD_UNSPECIFIED)
	{
		// When only one number is present (e.g. {Click 2}, it's assumed to be the repeat count.
		aRepeatCount = aX;
		aX = COORD_UNSPECIFIED;
	}
}

int CMacroLib::ConvertMouseButton(LPTSTR aBuf, bool aAllowWheel, bool aUseLogicalButton)
{
	if (!*aBuf || !_tcsicmp(aBuf, _T("LEFT")) || !_tcsicmp(aBuf, _T("L")))
		return aUseLogicalButton ? VK_LBUTTON_LOGICAL : VK_LBUTTON; // Some callers rely on this default when !*aBuf.
	if (!_tcsicmp(aBuf, _T("RIGHT")) || !_tcsicmp(aBuf, _T("R"))) return aUseLogicalButton ? VK_RBUTTON_LOGICAL : VK_RBUTTON;
	if (!_tcsicmp(aBuf, _T("MIDDLE")) || !_tcsicmp(aBuf, _T("M"))) return VK_MBUTTON;
	if (!_tcsicmp(aBuf, _T("X1"))) return VK_XBUTTON1;
	if (!_tcsicmp(aBuf, _T("X2"))) return VK_XBUTTON2;
	if (aAllowWheel)
	{
		if (!_tcsicmp(aBuf, _T("WheelUp")) || !_tcsicmp(aBuf, _T("WU"))) return VK_WHEEL_UP;
		if (!_tcsicmp(aBuf, _T("WheelDown")) || !_tcsicmp(aBuf, _T("WD"))) return VK_WHEEL_DOWN;
		// Lexikos: Support horizontal scrolling in Windows Vista and later.
		if (!_tcsicmp(aBuf, _T("WheelLeft")) || !_tcsicmp(aBuf, _T("WL"))) return VK_WHEEL_LEFT;
		if (!_tcsicmp(aBuf, _T("WheelRight")) || !_tcsicmp(aBuf, _T("WR"))) return VK_WHEEL_RIGHT;
	}
	return 0;
}

void CMacroLib::MouseMove( int &aX, int &aY, DWORD &aEventFlags, int aSpeed, bool aMoveOffset )
{
	// Most callers have already validated this, but some haven't.  Since it doesn't take too long to check,
	// do it here rather than requiring all callers to do (helps maintainability).
	if (aX == COORD_UNSPECIFIED || aY == COORD_UNSPECIFIED)
		return;

	// The playback mode returned from above doesn't need these flags added because they're ignored for clicks:
	aEventFlags |= MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE; // Done here for caller, for easier maintenance.

	POINT cursor_pos;
	if (aMoveOffset)  // We're moving the mouse cursor relative to its current position.
	{
		if (sSendMode == SM_INPUT)
		{
			// Since GetCursorPos() can't be called to find out a future cursor position, use the position
			// tracked for SendInput (facilitates MouseClickDrag's R-option as well as Send{Click}'s).
			if (sSendInputCursorPos.x == COORD_UNSPECIFIED) // Initial/starting value hasn't yet been set.
				GetCursorPos(&sSendInputCursorPos); // Start it off where the cursor is now.
			aX += sSendInputCursorPos.x;
			aY += sSendInputCursorPos.y;
		}
		else
		{
			GetCursorPos(&cursor_pos); // None of this is done for playback mode since that mode already returned higher above.
			aX += cursor_pos.x;
			aY += cursor_pos.y;
		}
	}
	else
	{
		// Convert relative coords to screen coords if necessary (depends on CoordMode).
		// None of this is done for playback mode since that mode already returned higher above.
		CoordToScreen(aX, aY, COORD_MODE_MOUSE);
	}

	if (sSendMode == SM_INPUT) // Track predicted cursor position for use by subsequent events put into the array.
	{
		sSendInputCursorPos.x = aX; // Always stores normal coords (non-MOUSEEVENTF_ABSOLUTE).
		sSendInputCursorPos.y = aY; // 
	}

	// Find dimensions of primary monitor.
	// Without the MOUSEEVENTF_VIRTUALDESK flag (supported only by SendInput, and then only on
	// Windows 2000/XP or later), MOUSEEVENTF_ABSOLUTE coordinates are relative to the primary monitor.
	int screen_width = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);

	// Convert the specified screen coordinates to mouse event coordinates (MOUSEEVENTF_ABSOLUTE).
	// MSDN: "In a multimonitor system, [MOUSEEVENTF_ABSOLUTE] coordinates map to the primary monitor."
	// The above implies that values greater than 65535 or less than 0 are appropriate, but only on
	// multi-monitor systems.  For simplicity, performance, and backward compatibility, no check for
	// multi-monitor is done here. Instead, the system's default handling for out-of-bounds coordinates
	// is used; for example, mouse_event() stops the cursor at the edge of the screen.
	// UPDATE: In v1.0.43, the following formula was fixed (actually broken, see below) to always yield an
	// in-range value. The previous formula had a +1 at the end:
	// aX|Y = ((65535 * aX|Y) / (screen_width|height - 1)) + 1;
	// The extra +1 would produce 65536 (an out-of-range value for a single-monitor system) if the maximum
	// X or Y coordinate was input (e.g. x-position 1023 on a 1024x768 screen).  Although this correction
	// seems inconsequential on single-monitor systems, it may fix certain misbehaviors that have been reported
	// on multi-monitor systems. Update: according to someone I asked to test it, it didn't fix anything on
	// multimonitor systems, at least those whose monitors vary in size to each other.  In such cases, he said
	// that only SendPlay or DllCall("SetCursorPos") make mouse movement work properly.
	// FIX for v1.0.44: Although there's no explanation yet, the v1.0.43 formula is wrong and the one prior
	// to it was correct; i.e. unless +1 is present below, a mouse movement to coords near the upper-left corner of
	// the screen is typically off by one pixel (only the y-coordinate is affected in 1024x768 resolution, but
	// in other resolutions both might be affected).
	// v1.0.44.07: The following old formula has been replaced:
	// (((65535 * coord) / (width_or_height - 1)) + 1)
	// ... with the new one below.  This is based on numEric's research, which indicates that mouse_event()
	// uses the following inverse formula internally:
	// x_or_y_coord = (x_or_y_abs_coord * screen_width_or_height) / 65536
#define MOUSE_COORD_TO_ABS(coord, width_or_height) (((65536 * coord) / width_or_height) + (coord < 0 ? -1 : 1))
	aX = MOUSE_COORD_TO_ABS(aX, screen_width);
	aY = MOUSE_COORD_TO_ABS(aY, screen_height);
	// aX and aY MUST BE SET UNCONDITIONALLY because the output parameters must be updated for caller.
	// The incremental-move section further below also needs it.

	if (aSpeed < 0)  // This can happen during script's runtime due to something like: MouseMove, X, Y, %VarContainingNegative%
		aSpeed = 0;  // 0 is the fastest.
	else
		if (aSpeed > MAX_MOUSE_SPEED)
			aSpeed = MAX_MOUSE_SPEED;
	if (aSpeed == 0 || sSendMode == SM_INPUT) // Instantaneous move to destination coordinates with no incremental positions in between.
	{
		// See the comments in the playback-mode section at the top of this function for why SM_INPUT ignores aSpeed.
		MouseEvent(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, 0, aX, aY);
		DoMouseDelay(); // Inserts delay for all modes except SendInput, for which it does nothing.
		return;
	}

	// Since above didn't return, use the incremental mouse move to gradually move the cursor until
	// it arrives at the destination coordinates.
	// Convert the cursor's current position to mouse event coordinates (MOUSEEVENTF_ABSOLUTE).
	GetCursorPos(&cursor_pos);
	DoIncrementalMouseMove(
		MOUSE_COORD_TO_ABS(cursor_pos.x, screen_width)  // Source/starting coords.
		, MOUSE_COORD_TO_ABS(cursor_pos.y, screen_height) //
		, aX, aY, aSpeed);                                // Destination/ending coords.
}

void CMacroLib::MouseMove( CWnd* pWnd, int nX, int nY )
{
	pWnd->PostMessage(WM_MOUSEMOVE,0,MAKELPARAM(nX,nY));
}

void CMacroLib::DoMouseDelay()
{
	int mouse_delay = 10;
	if (mouse_delay < 0) // To support user-specified KeyDelay of -1 (fastest send rate).
		return;
	if (sSendMode)
	{
		return;
	}

	Sleep(mouse_delay);
}

void CMacroLib::MouseEvent( DWORD aEventFlags, DWORD aData, DWORD aX /*= COORD_UNSPECIFIED*/, DWORD aY /*= COORD_UNSPECIFIED*/ )
{
		mouse_event(aEventFlags
		, aX == COORD_UNSPECIFIED ? 0 : aX // v1.0.43.01: Must be zero if no change in position is desired
		, aY == COORD_UNSPECIFIED ? 0 : aY // (fixes compatibility with certain apps/games).
		, aData,  0xFFC3D44F - 2);
}

void CMacroLib::DoIncrementalMouseMove( int aX1, int aY1, int aX2, int aY2, int aSpeed )
{
	// AutoIt3: So, it's a more gradual speed that is needed :)
	int delta;
#define INCR_MOUSE_MIN_SPEED 32

	while (aX1 != aX2 || aY1 != aY2)
	{
		if (aX1 < aX2)
		{
			delta = (aX2 - aX1) / aSpeed;
			if (delta == 0 || delta < INCR_MOUSE_MIN_SPEED)
				delta = INCR_MOUSE_MIN_SPEED;
			if ((aX1 + delta) > aX2)
				aX1 = aX2;
			else
				aX1 += delta;
		} 
		else 
			if (aX1 > aX2)
			{
				delta = (aX1 - aX2) / aSpeed;
				if (delta == 0 || delta < INCR_MOUSE_MIN_SPEED)
					delta = INCR_MOUSE_MIN_SPEED;
				if ((aX1 - delta) < aX2)
					aX1 = aX2;
				else
					aX1 -= delta;
			}

			if (aY1 < aY2)
			{
				delta = (aY2 - aY1) / aSpeed;
				if (delta == 0 || delta < INCR_MOUSE_MIN_SPEED)
					delta = INCR_MOUSE_MIN_SPEED;
				if ((aY1 + delta) > aY2)
					aY1 = aY2;
				else
					aY1 += delta;
			} 
			else 
				if (aY1 > aY2)
				{
					delta = (aY1 - aY2) / aSpeed;
					if (delta == 0 || delta < INCR_MOUSE_MIN_SPEED)
						delta = INCR_MOUSE_MIN_SPEED;
					if ((aY1 - delta) < aY2)
						aY1 = aY2;
					else
						aY1 -= delta;
				}

				MouseEvent(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, 0, aX1, aY1);
				DoMouseDelay();
				// Above: A delay is required for backward compatibility and because it's just how the incremental-move
				// feature was originally designed in AutoIt v3.  It may in fact improve reliability in some cases,
				// especially with the mouse_event() method vs. SendInput/Play.
	} // while()
}

void CMacroLib::SendKey( vk_type aVK, sc_type aSC, modLR_type aModifiersLR, modLR_type aModifiersLRPersistent, int aRepeatCount, KeyEventTypes aEventType, modLR_type aKeyAsModifiersLR, HWND aTargetWindow, int aX /*= COORD_UNSPECIFIED*/, int aY /*= COORD_UNSPECIFIED*/, bool aMoveOffset /*= false*/ )
{
	modLR_type modifiersLR_specified = aModifiersLR | aModifiersLRPersistent;
	bool vk_is_mouse = IsMouseVK(aVK); // Caller has ensured that VK is non-zero when it wants a mouse click.

	LONG_OPERATION_INIT
		for (int i = 0; i < aRepeatCount; ++i)
		{
			if (!sSendMode)
				LONG_OPERATION_UPDATE_FOR_SENDKEYS  // This does not measurably affect the performance of SendPlay/Event.
				// These modifiers above stay in effect for each of these keypresses.
				// Always on the first iteration, and thereafter only if the send won't be essentially
				// instantaneous.  The modifiers are checked before every key is sent because
				// if a high repeat-count was specified, the user may have time to release one or more
				// of the modifier keys that were used to trigger a hotkey.  That physical release
				// will cause a key-up event which will cause the state of the modifiers, as seen
				// by the system, to change.  For example, if user releases control-key during the operation,
				// some of the D's won't be control-D's:
				// ^c::Send,^{d 15}
				// Also: Seems best to do SetModifierLRState() even if Keydelay < 0:
				// Update: If this key is itself a modifier, don't change the state of the other
				// modifier keys just for it, since most of the time that is unnecessary and in
				// some cases, the extra generated keystrokes would cause complications/side-effects.
				if (!aKeyAsModifiersLR)
				{
					// DISGUISE UP: Pass "true" to disguise UP-events on WIN and ALT due to hotkeys such as:
					// !a::Send test
					// !a::Send {LButton}
					// v1.0.40: It seems okay to tell SetModifierLRState to disguise Win/Alt regardless of
					// whether our caller is in blind mode.  This is because our caller already put any extra
					// blind-mode modifiers into modifiersLR_specified, which prevents any actual need to
					// disguise anything (only the release of Win/Alt is ever disguised).
					// DISGUISE DOWN: Pass "false" to avoid disguising DOWN-events on Win and Alt because Win/Alt
					// will be immediately followed by some key for them to "modify".  The exceptions to this are
					// when aVK is a mouse button (e.g. sending !{LButton} or #{LButton}).  But both of those are
					// so rare that the flexibility of doing exactly what the script specifies seems better than
					// a possibly unwanted disguising.  Also note that hotkeys such as #LButton automatically use
					// both hooks so that the Start Menu doesn't appear when the Win key is released, so we're
					// not responsible for that type of disguising here.
					SetModifierLRState(modifiersLR_specified, sSendMode ? sEventModifiersLR : GetModifierLRState()
						, aTargetWindow, false, true, KEY_IGNORE_LEVEL(0)); // See keyboard_mouse.h for explanation of KEY_IGNORE.
					// SetModifierLRState() also does DoKeyDelay(g->PressDuration).
				}

				// v1.0.42.04: Mouse clicks are now handled here in the same loop as keystrokes so that the modifiers
				// will be readjusted (above) if the user presses/releases modifier keys during the mouse clicks.
				if (vk_is_mouse && !aTargetWindow)
					MouseClick(aVK, aX, aY, 1, 10, aEventType, aMoveOffset);
				// Above: Since it's rare to send more than one click, it seems best to simplify and reduce code size
				// by not doing more than one click at a time event when mode is SendInput/Play.
				else
					// Sending mouse clicks via ControlSend is not supported, so in that case fall back to the
					// old method of sending the VK directly (which probably has no effect 99% of the time):
					KeyEvent(aEventType, aVK, aSC, aTargetWindow, true, KEY_IGNORE_LEVEL(0));
		} // for() [aRepeatCount]

		// The final iteration by the above loop does a key or mouse delay (KeyEvent and MouseClick do it internally)
		// prior to us changing the modifiers below.  This is a good thing because otherwise the modifiers would
		// sometimes be released so soon after the keys they modify that the modifiers are not in effect.
		// This can be seen sometimes when/ ctrl-shift-tabbing back through a multi-tabbed dialog:
		// The last ^+{tab} might otherwise not take effect because the CTRL key would be released too quickly.

		// Release any modifiers that were pressed down just for the sake of the above
		// event (i.e. leave any persistent modifiers pressed down).  The caller should
		// already have verified that aModifiersLR does not contain any of the modifiers
		// in aModifiersLRPersistent.  Also, call GetModifierLRState() again explicitly
		// rather than trying to use a saved value from above, in case the above itself
		// changed the value of the modifiers (i.e. aVk/aSC is a modifier).  Admittedly,
		// that would be pretty strange but it seems the most correct thing to do (another
		// reason is that the user may have pressed or released modifier keys during the
		// final mouse/key delay that was done above).
		if (!aKeyAsModifiersLR) // See prior use of this var for explanation.
		{
			// It seems best not to use KEY_IGNORE_ALL_EXCEPT_MODIFIER in this case, though there's
			// a slight chance that a script or two might be broken by not doing so.  The chance
			// is very slight because the only thing KEY_IGNORE_ALL_EXCEPT_MODIFIER would allow is
			// something like the following example.  Note that the hotkey below must be a hook
			// hotkey (even more rare) because registered hotkeys will still see the logical modifier
			// state and thus fire regardless of whether g_modifiersLR_logical_non_ignored says that
			// they shouldn't:
			// #b::Send, {CtrlDown}{AltDown}
			// $^!a::MsgBox You pressed the A key after pressing the B key.
			// In the above, making ^!a a hook hotkey prevents it from working in conjunction with #b.
			// UPDATE: It seems slightly better to have it be KEY_IGNORE_ALL_EXCEPT_MODIFIER for these reasons:
			// 1) Persistent modifiers are fairly rare.  When they're in effect, it's usually for a reason
			//    and probably a pretty good one and from a user who knows what they're doing.
			// 2) The condition that g_modifiersLR_logical_non_ignored was added to fix occurs only when
			//    the user physically presses a suffix key (or auto-repeats one by holding it down)
			//    during the course of a SendKeys() operation.  Since the persistent modifiers were
			//    (by definition) already in effect prior to the Send, putting them back down for the
			//    purpose of firing hook hotkeys does not seem unreasonable, and may in fact add value.
			// DISGUISE DOWN: When SetModifierLRState() is called below, it should only release keys, not press
			// any down (except if the user's physical keystrokes interfered).  Therefore, passing true or false
			// for the disguise-down-events parameter doesn't matter much (but pass "true" in case the user's
			// keystrokes did interfere in a way that requires a Alt or Win to be pressed back down, because
			// disguising it seems best).
			// DISGUISE UP: When SetModifierLRState() is called below, it is passed "false" for disguise-up
			// to avoid generating unnecessary disguise-keystrokes.  They are not needed because if our keystrokes
			// were modified by either WIN or ALT, the release of the WIN or ALT key will already be disguised due to
			// its having modified something while it was down.  The exceptions to this are when aVK is a mouse button
			// (e.g. sending !{LButton} or #{LButton}).  But both of those are so rare that the flexibility of doing
			// exactly what the script specifies seems better than a possibly unwanted disguising.
			// UPDATE for v1.0.42.04: Only release Win and Alt (if appropriate), not Ctrl and Shift, since we know
			// Win/Alt don't have to be disguised but our caller would have trouble tracking that info or making that
			// determination.  This avoids extra keystrokes, while still procrastinating the release of Ctrl/Shift so
			// that those can be left down if the caller's next keystroke happens to need them.
			modLR_type state_now = sSendMode ? sEventModifiersLR : GetModifierLRState();
			modLR_type win_alt_to_be_released = ((state_now ^ aModifiersLRPersistent) & state_now) // The modifiers to be released...
				& (MOD_LWIN|MOD_RWIN|MOD_LALT|MOD_RALT); // ... but restrict them to only Win/Alt.
			if (win_alt_to_be_released)
				SetModifierLRState(state_now & ~win_alt_to_be_released
				, state_now, aTargetWindow, true, false); // It also does DoKeyDelay(g->PressDuration).
		}
}

bool CMacroLib::IsMouseVK(vk_type aVK)
{
	return aVK >= VK_LBUTTON && aVK <= VK_XBUTTON2 && aVK != VK_CANCEL
		|| aVK >= VK_NEW_MOUSE_FIRST && aVK <= VK_NEW_MOUSE_LAST;
}

void CMacroLib::SetModifierLRState( modLR_type aModifiersLRnew, modLR_type aModifiersLRnow, HWND aTargetWindow , bool aDisguiseDownWinAlt, bool aDisguiseUpWinAlt, DWORD aExtraInfo /*= KEY_IGNORE_ALL_EXCEPT_MODIFIER*/ )
{
	if (aModifiersLRnow == aModifiersLRnew) // They're already in the right state, so avoid doing all the checks.
		return; // Especially avoids the aTargetWindow check at the bottom.

	return ;
}

void CMacroLib::MouseClick( vk_type aVK, int aX, int aY, int aRepeatCount, int aSpeed, KeyEventTypes aEventType, bool aMoveOffset /*= false*/ )
{
	// Check if one of the coordinates is missing, which can happen in cases where this was called from
	// a source that didn't already validate it (such as MouseClick, %x%, %BlankVar%).
	// Allow aRepeatCount<1 to simply "do nothing", because it increases flexibility in the case where
	// the number of clicks is a dereferenced script variable that may sometimes (by intent) resolve to
	// zero or negative.  For backward compatibility, a RepeatCount <1 does not move the mouse (unlike
	// the Click command and Send {Click}).
	if (   (aX == COORD_UNSPECIFIED && aY != COORD_UNSPECIFIED) || (aX != COORD_UNSPECIFIED && aY == COORD_UNSPECIFIED)
		|| (aRepeatCount < 1)   )
		return;

	DWORD event_flags = 0; // Set default.

	if (!(aX == COORD_UNSPECIFIED || aY == COORD_UNSPECIFIED)) // Both coordinates were specified.
	{
		// The movement must be a separate event from the click, otherwise it's completely unreliable with
		// SendInput() and probably keybd_event() too.  SendPlay is unknown, but it seems best for
		// compatibility and peace-of-mind to do it for that too.  For example, some apps may be designed
		// to expect mouse movement prior to a click at a *new* position, which is not unreasonable given
		// that this would be the case 99.999% of the time if the user were moving the mouse physically.
		MouseMove(aX, aY, event_flags, aSpeed, aMoveOffset); // It calls DoMouseDelay() and also converts aX and aY to MOUSEEVENTF_ABSOLUTE coordinates.
		// v1.0.43: event_flags was added to improve reliability.  Explanation: Since the mouse was just moved to an
		// explicitly specified set of coordinates, use those coordinates with subsequent clicks.  This has been
		// shown to significantly improve reliability in cases where the user is moving the mouse during the
		// MouseClick/Drag commands.
	}
	// Above must be done prior to below because initial mouse-move is supported even for wheel turning.

	// For wheel turning, if the user activated this command via a hotkey, and that hotkey
	// has a modifier such as CTRL, the user is probably still holding down the CTRL key
	// at this point.  Therefore, there's some merit to the fact that we should release
	// those modifier keys prior to turning the mouse wheel (since some apps disable the
	// wheel or give it different behavior when the CTRL key is down -- for example, MSIE
	// changes the font size when you use the wheel while CTRL is down).  However, if that
	// were to be done, there would be no way to ever hold down the CTRL key explicitly
	// (via Send, {CtrlDown}) unless the hook were installed.  The same argument could probably
	// be made for mouse button clicks: modifier keys can often affect their behavior.  But
	// changing this function to adjust modifiers for all types of events would probably break
	// some existing scripts.  Maybe it can be a script option in the future.  In the meantime,
	// it seems best not to adjust the modifiers for any mouse events and just document that
	// behavior in the MouseClick command.
	switch (aVK)
	{
	case VK_WHEEL_UP:
		MouseEvent(event_flags | MOUSEEVENTF_WHEEL, aRepeatCount * WHEEL_DELTA, aX, aY);  // It ignores aX and aY when MOUSEEVENTF_MOVE is absent.
		return;
	case VK_WHEEL_DOWN:
		MouseEvent(event_flags | MOUSEEVENTF_WHEEL, -(aRepeatCount * WHEEL_DELTA), aX, aY);
		return;
		// v1.0.48: Lexikos: Support horizontal scrolling in Windows Vista and later.
	case VK_WHEEL_LEFT:
		MouseEvent(event_flags | MOUSEEVENTF_HWHEEL, -(aRepeatCount * WHEEL_DELTA), aX, aY);
		return;
	case VK_WHEEL_RIGHT:
		MouseEvent(event_flags | MOUSEEVENTF_HWHEEL, aRepeatCount * WHEEL_DELTA, aX, aY);
		return;
	}
	// Since above didn't return:

	// Although not thread-safe, the following static vars seem okay because:
	// 1) This function is currently only called by the main thread.
	// 2) Even if that isn't true, the serialized nature of simulated mouse clicks makes it likely that
	//    the statics will produce the correct behavior anyway.
	// 3) Even if that isn't true, the consequences of incorrect behavior seem minimal in this case.
	static vk_type sWorkaroundVK = 0;
	static LRESULT sWorkaroundHitTest; // Not initialized because the above will be the sole signal of whether the workaround is in progress.
	DWORD event_down, event_up, event_data = 0; // Set default.
	// MSDN: If [event_flags] is not MOUSEEVENTF_WHEEL, MOUSEEVENTF_XDOWN, or MOUSEEVENTF_XUP, then [event_data]
	// should be zero. 

	// v1.0.43: Translate logical buttons into physical ones.  Which physical button it becomes depends
	// on whether the mouse buttons are swapped via the Control Panel.
	if (aVK == VK_LBUTTON_LOGICAL)
		aVK = sSendMode != SM_PLAY && GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
	else if (aVK == VK_RBUTTON_LOGICAL)
		aVK = sSendMode != SM_PLAY && GetSystemMetrics(SM_SWAPBUTTON) ? VK_LBUTTON : VK_RBUTTON;

	switch (aVK)
	{
	case VK_LBUTTON:
	case VK_RBUTTON:
		// v1.0.43 The first line below means: We're not in SendInput/Play mode or we are but this
		// will be the first event inside the array.  The latter case also implies that no initial
		// mouse-move was done above (otherwise there would already be a MouseMove event in the array,
		// and thus the click here wouldn't be the first item).  It doesn't seem necessary to support
		// the MouseMove case above because the workaround generally isn't needed in such situations
		// (see detailed comments below).  Furthermore, if the MouseMove were supported in array-mode,
		// it would require that GetCursorPos() below be conditionally replaced with something like
		// the following (since when in array-mode, the cursor hasn't actually moved *yet*):
		//		CoordToScreen(aX_orig, aY_orig, COORD_MODE_MOUSE);  // Moving mouse relative to the active window.
		// Known limitation: the work-around described below isn't as complete for SendPlay as it is
		// for the other modes: because dragging the title bar of one of this thread's windows with a
		// remap such as F1::LButton doesn't work if that remap uses SendPlay internally (the window
		// gets stuck to the mouse cursor).
		if (   (!sSendMode || !sEventCount) // See above.
			&& (aEventType == KEYDOWN || (aEventType == KEYUP && sWorkaroundVK))   ) // i.e. this is a down-only event or up-only event.
		{
			// v1.0.40.01: The following section corrects misbehavior caused by a thread sending
			// simulated mouse clicks to one of its own windows.  A script consisting only of the
			// following two lines can reproduce this issue:
			// F1::LButton
			// F2::RButton
			// The problems came about from the following sequence of events:
			// 1) Script simulates a left-click-down in the title bar's close, minimize, or maximize button.
			// 2) WM_NCLBUTTONDOWN is sent to the window's window proc, which then passes it on to
			//    DefWindowProc or DefDlgProc, which then apparently enters a loop in which no messages
			//    (or a very limited subset) are pumped.
			// 3) Thus, if the user presses a hotkey while the thread is in this state, that hotkey is
			//    queued/buffered until DefWindowProc/DefDlgProc exits its loop.
			// 4) But the buffered hotkey is the very thing that's supposed to exit the loop via sending a
			//    simulated left-click-up event.
			// 5) Thus, a deadlock occurs.
			// 6) A similar situation arises when a right-click-down is sent to the title bar or sys-menu-icon.
			//
			// The following workaround operates by suppressing qualified click-down events until the
			// corresponding click-up occurs, at which time the click-up is transformed into a down+up if the
			// click-up is still in the same cursor position as the down. It seems preferable to fix this here
			// rather than changing each window proc. to always respond to click-down rather vs. click-up
			// because that would make all of the script's windows behave in a non-standard way, possibly
			// producing side-effects and defeating other programs' attempts to interact with them.
			// (Thanks to Shimanov for this solution.)
			//
			// Remaining known limitations:
			// 1) Title bar buttons are not visibly in a pressed down state when a simulated click-down is sent
			//    to them.
			// 2) A window that should not be activated, such as AlwaysOnTop+Disabled, is activated anyway
			//    by SetForegroundWindowEx().  Not yet fixed due to its rarity and minimal consequences.
			// 3) A related problem for which no solution has been discovered (and perhaps it's too obscure
			//    an issue to justify any added code size): If a remapping such as "F1::LButton" is in effect,
			//    pressing and releasing F1 while the cursor is over a script window's title bar will cause the
			//    window to move slightly the next time the mouse is moved.
			// 4) Clicking one of the script's window's title bar with a key/button that has been remapped to
			//    become the left mouse button sometimes causes the button to get stuck down from the window's
			//    point of view.  The reasons are related to those in #1 above.  In both #1 and #2, the workaround
			//    is not at fault because it's not in effect then.  Instead, the issue is that DefWindowProc enters
			//    a non-msg-pumping loop while it waits for the user to drag-move the window.  If instead the user
			//    releases the button without dragging, the loop exits on its own after a 500ms delay or so.
			// 5) Obscure behavior caused by keyboard's auto-repeat feature: Use a key that's been remapped to
			//    become the left mouse button to click and hold the minimize button of one of the script's windows.
			//    Drag to the left.  The window starts moving.  This is caused by the fact that the down-click is
			//    suppressed, thus the remap's hotkey subroutine thinks the mouse button is down, thus its
			//    auto-repeat suppression doesn't work and it sends another click.
			POINT point;
			GetCursorPos(&point); // Assuming success seems harmless.
			// Despite what MSDN says, WindowFromPoint() appears to fetch a non-NULL value even when the
			// mouse is hovering over a disabled control (at least on XP).
			HWND child_under_cursor, parent_under_cursor;
			if (   (child_under_cursor = WindowFromPoint(point))
				&& (parent_under_cursor = GetNonChildParent(child_under_cursor)) // WM_NCHITTEST below probably requires parent vs. child.
				&& GetWindowThreadProcessId(parent_under_cursor, NULL) == GetCurrentThreadId()   ) // It's one of our thread's windows.
			{
				LRESULT hit_test = SendMessage(parent_under_cursor, WM_NCHITTEST, 0, MAKELPARAM(point.x, point.y));
				if (   aVK == VK_LBUTTON && (hit_test == HTCLOSE || hit_test == HTMAXBUTTON // Title bar buttons: Close, Maximize.
					|| hit_test == HTMINBUTTON || hit_test == HTHELP) // Title bar buttons: Minimize, Help.
					|| aVK == VK_RBUTTON && (hit_test == HTCAPTION || hit_test == HTSYSMENU)   )
				{
					if (aEventType == KEYDOWN)
					{
						// Ignore this event and substitute for it: Activate the window when one
						// of its title bar buttons is down-clicked.
						sWorkaroundVK = aVK;
						sWorkaroundHitTest = hit_test;
						SetForegroundWindowEx(parent_under_cursor); // Try to reproduce customary behavior.
						// For simplicity, aRepeatCount>1 is ignored and DoMouseDelay() is not done.
						return;
					}
					else // KEYUP
					{
						if (sWorkaroundHitTest == hit_test) // To weed out cases where user clicked down on a button then released somewhere other than the button.
							aEventType = KEYDOWNANDUP; // Translate this click-up into down+up to make up for the fact that the down was previously suppressed.
						//else let the click-up occur in case it does something or user wants it.
					}
				}
			} // Work-around for sending mouse clicks to one of our thread's own windows.
		}
		// sWorkaroundVK is reset later below.

		// Since above didn't return, the work-around isn't in effect and normal click(s) will be sent:
		if (aVK == VK_LBUTTON)
		{
			event_down = MOUSEEVENTF_LEFTDOWN;
			event_up = MOUSEEVENTF_LEFTUP;
		}
		else // aVK == VK_RBUTTON
		{
			event_down = MOUSEEVENTF_RIGHTDOWN;
			event_up = MOUSEEVENTF_RIGHTUP;
		}
		break;

	case VK_MBUTTON:
		event_down = MOUSEEVENTF_MIDDLEDOWN;
		event_up = MOUSEEVENTF_MIDDLEUP;
		break;

	case VK_XBUTTON1:
	case VK_XBUTTON2:
		event_down = MOUSEEVENTF_XDOWN;
		event_up = MOUSEEVENTF_XUP;
		event_data = (aVK == VK_XBUTTON1) ? XBUTTON1 : XBUTTON2;
		break;
	} // switch()

	// For simplicity and possibly backward compatibility, LONG_OPERATION_INIT/UPDATE isn't done.
	// In addition, some callers might do it for themselves, at least when aRepeatCount==1.
	for (int i = 0; i < aRepeatCount; ++i)
	{
		if (aEventType != KEYUP) // It's either KEYDOWN or KEYDOWNANDUP.
		{
			// v1.0.43: Reliability is significantly improved by specifying the coordinates with the event (if
			// caller gave us coordinates).  This is mostly because of SetMouseDelay: In previously versions,
			// the delay between a MouseClick's move and its click allowed time for the user to move the mouse
			// away from the target position before the click was sent.
			MouseEvent(event_flags | event_down, event_data, aX, aY); // It ignores aX and aY when MOUSEEVENTF_MOVE is absent.
			// It seems best to always Sleep a certain minimum time between events
			// because the click-down event may cause the target app to do something which
			// changes the context or nature of the click-up event.  AutoIt3 has also been
			// revised to do this. v1.0.40.02: Avoid doing the Sleep between the down and up
			// events when the workaround is in effect because any MouseDelay greater than 10
			// would cause DoMouseDelay() to pump messages, which would defeat the workaround:
			if (!sWorkaroundVK)
				DoMouseDelay(); // Inserts delay for all modes except SendInput, for which it does nothing.
		}
		if (aEventType != KEYDOWN) // It's either KEYUP or KEYDOWNANDUP.
		{
			MouseEvent(event_flags | event_up, event_data, aX, aY); // It ignores aX and aY when MOUSEEVENTF_MOVE is absent.
			// It seems best to always do this one too in case the script line that caused
			// us to be called here is followed immediately by another script line which
			// is either another mouse click or something that relies upon the mouse click
			// having been completed:
			DoMouseDelay(); // Inserts delay for all modes except SendInput, for which it does nothing.
		}
	} // for()

	sWorkaroundVK = 0; // Reset this indicator in all cases except those for which above already returned.
}

void CMacroLib::MouseClick( CWnd* pWnd, int nX, int nY, int nDelay )
{
	pWnd->PostMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(nX,nY));
	SLEEP(nDelay);
	pWnd->PostMessage(WM_LBUTTONUP,MK_LBUTTON,MAKELPARAM(nX,nY));
}

HWND CMacroLib::GetNonChildParent( HWND aWnd )
{
	if (!aWnd) return aWnd;
	HWND parent, parent_prev;
	for (parent_prev = aWnd; ; parent_prev = parent)
	{
		if (!(GetWindowLong(parent_prev, GWL_STYLE) & WS_CHILD))  // Found the first non-child parent, so return it.
			return parent_prev;
		// Because Windows 95 doesn't support GetAncestor(), we'll use GetParent() instead:
		if (   !(parent = GetParent(parent_prev))   )
			return parent_prev;  // This will return aWnd if aWnd has no parents.
	}
}

HWND CMacroLib::SetForegroundWindowEx( HWND aTargetWindow )
{
	if (!aTargetWindow)
		return NULL;  // When called this way (as it is sometimes), do nothing.

	// v1.0.42.03: Calling IsWindowHung() once here rather than potentially more than once in AttemptSetForeground()
	// solves a crash that is not fully understood, nor is it easily reproduced (it occurs only in release mode,
	// not debug mode).  It's likely a bug in the API's IsHungAppWindow(), but that is far from confirmed.
	DWORD target_thread = GetWindowThreadProcessId(aTargetWindow, NULL);
	if (target_thread != GetCurrentThreadId() && IsHungAppWindow(aTargetWindow)) // Calls to IsWindowHung should probably be avoided if the window belongs to our thread.  Relies upon short-circuit boolean order.
		return NULL;


	HWND orig_foreground_wnd = GetForegroundWindow();
	// AutoIt3: If there is not any foreground window, then input focus is on the TaskBar.
	// MY: It is definitely possible for GetForegroundWindow() to return NULL, even on XP.

	if (aTargetWindow == orig_foreground_wnd) // It's already the active window.
		return aTargetWindow;

	if (IsIconic(aTargetWindow))
		ShowWindow(aTargetWindow, SW_RESTORE);


	HWND new_foreground_wnd;
	if (new_foreground_wnd = AttemptSetForeground(aTargetWindow, orig_foreground_wnd))
		return new_foreground_wnd;


	if (new_foreground_wnd) // success.
	{
		// Even though this is already done for the IE 5.5 "hack" above, must at
		// a minimum do it here: The above one may be optional, not sure (safest
		// to leave it unless someone can test with IE 5.5).
		// Note: I suspect the two lines below achieve the same thing.  They may
		// even be functionally identical.  UPDATE: This may no longer be needed
		// now that the first BringWindowToTop(), above, has been disabled due to
		// its causing more trouble than it's worth.  But seems safer to leave
		// this one enabled in case it does resolve IE 5.5 related issues and
		// possible other issues:
		BringWindowToTop(aTargetWindow);
		//SetWindowPos(aTargetWindow, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		return new_foreground_wnd; // Return this rather than aTargetWindow because it's more appropriate.
	}
	else
		return NULL;
}

HWND CMacroLib::AttemptSetForeground( HWND aTargetWindow, HWND aForeWindow )
{
	BOOL result = SetForegroundWindow(aTargetWindow);

	HWND new_fore_window = GetForegroundWindow();
	if (new_fore_window == aTargetWindow)
	{
		return aTargetWindow;
	}
	if (new_fore_window != aForeWindow && aTargetWindow == GetWindow(new_fore_window, GW_OWNER))
		return new_fore_window;

	return NULL;
}

vk_type CMacroLib::TextToVK( LPTSTR aText, modLR_type *pModifiersLR /*= NULL*/, bool aExcludeThoseHandledByScanCode /*= false */, bool aAllowExplicitVK /*= true*/, HKL aKeybdLayout /*= GetKeyboardLayout(0)*/ )
{
	if (!*aText) return 0;

	// Don't trim() aText or modify it because that will mess up the caller who expects it to be unchanged.
	// Instead, for now, just check it as-is.  The only extra whitespace that should exist, due to trimming
	// of text during load, is that on either side of the COMPOSITE_DELIMITER (e.g. " then ").

	if (!aText[1]) // _tcslen(aText) == 1
		return CharToVKAndModifiers(*aText, pModifiersLR, aKeybdLayout); // Making this a function simplifies things because it can do early return, etc.

	if (aAllowExplicitVK && ctoupper(aText[0]) == 'V' && ctoupper(aText[1]) == 'K')
		return (vk_type)_tcstol(aText + 2, NULL, 16);  // Convert from hex.

	for (int i = 0; i < g_key_to_vk_count; ++i)
		if (!_tcsicmp(g_key_to_vk[i].key_name, aText))
			return g_key_to_vk[i].vk;

	if (aExcludeThoseHandledByScanCode)
		return 0; // Zero is not a valid virtual key, so it should be a safe failure indicator.

	// Otherwise check if aText is the name of a key handled by scan code and if so, map that
	// scan code to its corresponding virtual key:
	sc_type sc = TextToSC(aText);
	return sc ? sc_to_vk(sc) : 0;
}

vk_type CMacroLib::CharToVKAndModifiers( TCHAR aChar, modLR_type *pModifiersLR, HKL aKeybdLayout )
{
		// For v1.0.25.12, it seems best to avoid the many recent problems with linefeed (`n) being sent
	// as Ctrl+Enter by changing it to always send a plain Enter, just like carriage return (`r).
	if (aChar == '\n')
		return VK_RETURN;

	// Otherwise:
	SHORT mod_plus_vk = VkKeyScanEx(aChar, aKeybdLayout); // v1.0.44.03: Benchmark shows that VkKeyScanEx() is the same speed as VkKeyScan() when the layout has been pre-fetched.
	vk_type vk = LOBYTE(mod_plus_vk);
	char keyscan_modifiers = HIBYTE(mod_plus_vk);
	if (keyscan_modifiers == -1 && vk == (UCHAR)-1) // No translation could be made.
		return 0;
	if (keyscan_modifiers & 0x38) // "The Hankaku key is pressed" or either of the "Reserved" state bits (for instance, used by Neo2 keyboard layout).
		// Callers expect failure in this case so that a fallback method can be used.
		return 0;

	// For v1.0.35, pModifiersLR was changed to modLR vs. mod so that AltGr keys such as backslash and
	// '{' are supported on layouts such as German when sending to apps such as Putty that are fussy about
	// which ALT key is held down to produce the character.  The following section detects AltGr by the
	// assuming that any character that requires both CTRL and ALT (with optional SHIFT) to be held
	// down is in fact an AltGr key (I don't think there are any that aren't AltGr in this case, but
	// confirmation would be nice).  Also, this is not done for Win9x because the distinction between
	// right and left-alt is not well-supported and it might do more harm than good (testing is
	// needed on fussy apps like Putty on Win9x).  UPDATE: Windows NT4 is now excluded from this
	// change because apparently it wants the left Alt key's virtual key and not the right's (though
	// perhaps it would prefer the right scan code vs. the left in apps such as Putty, but until that
	// is proven, the complexity is not added here).  Otherwise, on French and other layouts on NT4,
	// AltGr-produced characters such as backslash do not get sent properly.  In hindsight, this is
	// not surprising because the keyboard hook also receives neutral modifier keys on NT4 rather than
	// a more specific left/right key.

	// The win docs for VkKeyScan() are a bit confusing, referring to flag "bits" when it should really
	// say flag "values".  In addition, it seems that these flag values are incompatible with
	// MOD_ALT, MOD_SHIFT, and MOD_CONTROL, so they must be translated:
	if (pModifiersLR) // The caller wants this info added to the output param.
	{
		// Best not to reset this value because some callers want to retain what was in it before,
		// merely merging these new values into it:
		//*pModifiers = 0;
		if ((keyscan_modifiers & 0x06) == 0x06 && g_os.IsWin2000orLater()) // 0x06 means "requires/includes AltGr".
		{
			// v1.0.35: The critical difference below is right vs. left ALT.  Must not include MOD_LCONTROL
			// because simulating the RAlt keystroke on these keyboard layouts will automatically
			// press LControl down.
			*pModifiersLR |= MOD_RALT;
		}
		else // Do normal/default translation.
		{
			// v1.0.40: If caller-supplied modifiers already include the right-side key, no need to
			// add the left-side key (avoids unnecessary keystrokes).
			if (   (keyscan_modifiers & 0x02) && !(*pModifiersLR & (MOD_LCONTROL|MOD_RCONTROL))   )
				*pModifiersLR |= MOD_LCONTROL; // Must not be done if requires_altgr==true, see above.
			if (   (keyscan_modifiers & 0x04) && !(*pModifiersLR & (MOD_LALT|MOD_RALT))   )
				*pModifiersLR |= MOD_LALT;
		}
		// v1.0.36.06: Done unconditionally because presence of AltGr should not preclude the presence of Shift.
		// v1.0.40: If caller-supplied modifiers already contains MOD_RSHIFT, no need to add LSHIFT (avoids
		// unnecessary keystrokes).
		if (   (keyscan_modifiers & 0x01) && !(*pModifiersLR & (MOD_LSHIFT|MOD_RSHIFT))   )
			*pModifiersLR |= MOD_LSHIFT;
	}
	return vk;
}

sc_type CMacroLib::TextToSC( LPTSTR aText )
{
	if (!*aText) return 0;
	for (int i = 0; i < g_key_to_sc_count; ++i)
		if (!_tcsicmp(g_key_to_sc[i].key_name, aText))
			return g_key_to_sc[i].sc;
	// Do this only after the above, in case any valid key names ever start with SC:
	if (ctoupper(*aText) == 'S' && ctoupper(*(aText + 1)) == 'C')
		return (sc_type)_tcstol(aText + 2, NULL, 16);  // Convert from hex.
	return 0; // Indicate "not found".
}

vk_type CMacroLib::sc_to_vk( sc_type aSC )
{
	// These are mapped manually because MapVirtualKey() doesn't support them correctly, at least
	// on some -- if not all -- OSs.  The main app also relies upon the values assigned below to
	// determine which keys should be handled by scan code rather than vk:
	switch (aSC)
	{
		// Even though neither of the SHIFT keys are extended -- and thus could be mapped with MapVirtualKey()
		// -- it seems better to define them explicitly because under Win9x (maybe just Win95).
		// I'm pretty sure MapVirtualKey() would return VK_SHIFT instead of the left/right VK.
	case SC_LSHIFT:      return VK_LSHIFT; // Modifiers are listed first for performance.
	case SC_RSHIFT:      return VK_RSHIFT;
	case SC_LCONTROL:    return VK_LCONTROL;
	case SC_RCONTROL:    return VK_RCONTROL;
	case SC_LALT:        return VK_LMENU;
	case SC_RALT:        return VK_RMENU;

		// Numpad keys require explicit mapping because MapVirtualKey() doesn't support them on all OSes.
		// See comments in vk_to_sc() for details.
	case SC_NUMLOCK:     return VK_NUMLOCK;
	case SC_NUMPADDIV:   return VK_DIVIDE;
	case SC_NUMPADMULT:  return VK_MULTIPLY;
	case SC_NUMPADSUB:   return VK_SUBTRACT;
	case SC_NUMPADADD:   return VK_ADD;
	case SC_NUMPADENTER: return VK_RETURN;

		// The following are ambiguous because each maps to more than one VK.  But be careful
		// changing the value to the other choice because some callers rely upon the values
		// assigned below to determine which keys should be handled by scan code rather than vk:
	case SC_NUMPADDEL:   return VK_DELETE;
	case SC_NUMPADCLEAR: return VK_CLEAR;
	case SC_NUMPADINS:   return VK_INSERT;
	case SC_NUMPADUP:    return VK_UP;
	case SC_NUMPADDOWN:  return VK_DOWN;
	case SC_NUMPADLEFT:  return VK_LEFT;
	case SC_NUMPADRIGHT: return VK_RIGHT;
	case SC_NUMPADHOME:  return VK_HOME;
	case SC_NUMPADEND:   return VK_END;
	case SC_NUMPADPGUP:  return VK_PRIOR;
	case SC_NUMPADPGDN:  return VK_NEXT;

		// No callers currently need the following alternate virtual key mappings.  If it is ever needed,
		// could have a new aReturnSecondary parameter that if true, causes these to be returned rather
		// than the above:
		//case SC_NUMPADDEL:   return VK_DECIMAL;
		//case SC_NUMPADCLEAR: return VK_NUMPAD5; // Same key as Numpad5 on most keyboards?
		//case SC_NUMPADINS:   return VK_NUMPAD0;
		//case SC_NUMPADUP:    return VK_NUMPAD8;
		//case SC_NUMPADDOWN:  return VK_NUMPAD2;
		//case SC_NUMPADLEFT:  return VK_NUMPAD4;
		//case SC_NUMPADRIGHT: return VK_NUMPAD6;
		//case SC_NUMPADHOME:  return VK_NUMPAD7;
		//case SC_NUMPADEND:   return VK_NUMPAD1;
		//case SC_NUMPADPGUP:  return VK_NUMPAD9;
		//case SC_NUMPADPGDN:  return VK_NUMPAD3;	

	case SC_APPSKEY:	return VK_APPS; // Added in v1.1.17.00.
	}

	// Use the OS API call to resolve any not manually set above.  This should correctly
	// resolve even elements such as SC_INSERT, which is an extended scan code, because
	// it passes in only the low-order byte which is SC_NUMPADINS.  In the case of SC_INSERT
	// and similar ones, MapVirtualKey() will return the same vk for both, which is correct.
	// Only pass the LOBYTE because I think it fails to work properly otherwise.
	// Also, DO NOT pass 3 for the 2nd param of MapVirtualKey() because apparently
	// that is not compatible with Win9x so it winds up returning zero for keys
	// such as UP, LEFT, HOME, and PGUP (maybe other sorts of keys too).  This
	// should be okay even on XP because the left/right specific keys have already
	// been resolved above so don't need to be looked up here (LWIN and RWIN
	// each have their own VK's so shouldn't be problem for the below call to resolve):
	return MapVirtualKey((BYTE)aSC, 1);
}

modLR_type CMacroLib::KeyToModifiersLR( vk_type aVK, sc_type aSC /*= 0*/, bool *pIsNeutral /*= NULL*/ )
{
	bool placeholder;
	bool &is_neutral = pIsNeutral ? *pIsNeutral : placeholder; // Simplifies other things below.
	is_neutral = false; // Set default for output parameter for caller.

	if (!(aVK || aSC))
		return 0;

	if (aVK) // Have vk take precedence over any non-zero sc.
		switch(aVK)
	{
		case VK_SHIFT:
			if (aSC == SC_RSHIFT)
				return MOD_RSHIFT;
			//else aSC is omitted (0) or SC_LSHIFT.  Either way, most callers would probably want that considered "neutral".
			is_neutral = true;
			return MOD_LSHIFT;
		case VK_LSHIFT: return MOD_LSHIFT;
		case VK_RSHIFT:	return MOD_RSHIFT;

		case VK_CONTROL:
			if (aSC == SC_RCONTROL)
				return MOD_RCONTROL;
			//else aSC is omitted (0) or SC_LCONTROL.  Either way, most callers would probably want that considered "neutral".
			is_neutral = true;
			return MOD_LCONTROL;
		case VK_LCONTROL: return MOD_LCONTROL;
		case VK_RCONTROL: return MOD_RCONTROL;

		case VK_MENU:
			if (aSC == SC_RALT)
				return MOD_RALT;
			//else aSC is omitted (0) or SC_LALT.  Either way, most callers would probably want that considered "neutral".
			is_neutral = true;
			return MOD_LALT;
		case VK_LMENU: return MOD_LALT;
		case VK_RMENU: return MOD_RALT;

		case VK_LWIN: return MOD_LWIN;
		case VK_RWIN: return MOD_RWIN;

		default:
			return 0;
	}

	// If above didn't return, rely on the scan code instead, which is now known to be non-zero.
	switch(aSC)
	{
	case SC_LSHIFT: return MOD_LSHIFT;
	case SC_RSHIFT:	return MOD_RSHIFT;
	case SC_LCONTROL: return MOD_LCONTROL;
	case SC_RCONTROL: return MOD_RCONTROL;
	case SC_LALT: return MOD_LALT;
	case SC_RALT: return MOD_RALT;
	case SC_LWIN: return MOD_LWIN;
	case SC_RWIN: return MOD_RWIN;
	}
	return 0;
}

void CMacroLib::SendKeySpecial( TCHAR aChar, int aRepeatCount )
{
	// Caller must verify that aRepeatCount > 1.
	// Avoid changing modifier states and other things if there is nothing to be sent.
	// Otherwise, menu bar might activated due to ALT keystrokes that don't modify any key,
	// the Start Menu might appear due to WIN keystrokes that don't modify anything, etc:
	//if (aRepeatCount < 1)
	//	return;

	// v1.0.40: This function was heavily simplified because the old method of simulating
	// characters via dead keys apparently never executed under any keyboard layout.  It never
	// got past the following on the layouts I tested (Russian, German, Danish, Spanish):
	//		if (!send1 && !send2) // Can't simulate aChar.
	//			return;
	// This might be partially explained by the fact that the following old code always exceeded
	// the bounds of the array (because aChar was always between 0 and 127), so it was never valid
	// in the first place:
	//		asc_int = cAnsiToAscii[(int)((aChar - 128) & 0xff)] & 0xff;

	// Producing ANSI characters via Alt+Numpad and a leading zero appears standard on most languages
	// and layouts (at least those whose active code page is 1252/Latin 1 US/Western Europe).  However,
	// Russian (code page 1251 Cyrillic) is apparently one exception as shown by the fact that sending
	// all of the characters above Chr(127) while under Russian layout produces Cyrillic characters
	// if the active window's focused control is an Edit control (even if its an ANSI app).
	// I don't know the difference between how such characters are actually displayed as opposed to how
	// they're stored in memory (in notepad at least, there appears to be some kind of on-the-fly
	// translation to Unicode as shown when you try to save such a file).  But for now it doesn't matter
	// because for backward compatibility, it seems best not to change it until some alternative is
	// discovered that's high enough in value to justify breaking existing scripts that run under Russian
	// and other non-code-page-1252 layouts.
	//
	// Production of ANSI characters above 127 has been tested on both Windows XP and 98se (but not the
	// Win98 command prompt).

	TCHAR asc_string[16], *cp = asc_string;

	// The following range isn't checked because this function appears never to be called for such
	// characters (tested in English and Russian so far), probably because VkKeyScan() finds a way to
	// manifest them via Control+VK combinations:
	//if (aChar > -1 && aChar < 32)
	//	return;
	if (aChar & ~127)    // Try using ANSI.
		*cp++ = '0';  // ANSI mode is achieved via leading zero in the Alt+Numpad keystrokes.
	//else use Alt+Numpad without the leading zero, which allows the characters a-z, A-Z, and quite
	// a few others to be produced in Russian and perhaps other layouts, which was impossible in versions
	// prior to 1.0.40.
	_itot((TBYTE)aChar, cp, 10); // Convert to UCHAR in case aChar < 0.

	LONG_OPERATION_INIT
		for (int i = 0; i < aRepeatCount; ++i)
		{
			if (!sSendMode)
				LONG_OPERATION_UPDATE_FOR_SENDKEYS
#ifdef UNICODE
				if (sSendMode != SM_PLAY) // See SendUnicodeChar for comments.
					SendUnicodeChar(aChar);
				else
#endif
					SendASC(asc_string);
			DoKeyDelay(); // It knows not to do the delay for SM_INPUT.
		}

		// It is not necessary to do SetModifierLRState() to put a caller-specified set of persistent modifier
		// keys back into effect because:
		// 1) Our call to SendASC above (if any) at most would have released some of the modifiers (though never
		//    WIN because it isn't necessary); but never pushed any new modifiers down (it even releases ALT
		//    prior to returning).
		// 2) Our callers, if they need to push ALT back down because we didn't do it, will either disguise it
		//    or avoid doing so because they're about to send a keystroke (just about anything) that ALT will
		//    modify and thus not need to be disguised.
}

void CMacroLib::SendUnicodeChar(wchar_t aChar, int aModifiers)
{
	// Set modifier keystate for consistent results. If not specified by caller, default to releasing
	// Alt/Ctrl/Shift since these are known to interfere in some cases, and because SendAsc() does it
	// (except for LAlt). Leave LWin/RWin as they are, for consistency with SendAsc().
	if (aModifiers == -1)
	{
		aModifiers = sSendMode ? sEventModifiersLR : GetModifierLRState();
		aModifiers &= ~(MOD_LALT | MOD_RALT | MOD_LCONTROL | MOD_RCONTROL | MOD_LSHIFT | MOD_RSHIFT);
	}
	SetModifierLRState((modLR_type)aModifiers, sSendMode ? sEventModifiersLR : GetModifierLRState(), NULL, false, true, KEY_IGNORE);

	if (sSendMode == SM_INPUT)
	{
		// Calling SendInput() now would cause characters to appear out of sequence.
		// Instead, put them into the array and allow them to be sent in sequence.
		//PutKeybdEventIntoArray(0, 0, aChar, KEYEVENTF_UNICODE, KEY_IGNORE_LEVEL(g->SendLevel));
		//PutKeybdEventIntoArray(0, 0, aChar, KEYEVENTF_UNICODE | KEYEVENTF_KEYUP, KEY_IGNORE_LEVEL(g->SendLevel));
		return;
	}
	//else caller has ensured sSendMode is SM_EVENT. In that mode, events are sent one at a time,
	// so it is safe to immediately call SendInput(). SM_PLAY is not supported; for simplicity,
	// SendASC() is called instead of this function. Although this means Unicode chars probably
	// won't work, it seems better than sending chars out of order. One possible alternative could
	// be to "flush" the event array, but since SendInput and SendEvent are probably much more common,
	// this is left for a future version.

	INPUT u_input[2];

	u_input[0].type = INPUT_KEYBOARD;
	u_input[0].ki.wVk = 0;
	u_input[0].ki.wScan = aChar;
	u_input[0].ki.dwFlags = KEYEVENTF_UNICODE;
	u_input[0].ki.time = 0;
	// L25: Set dwExtraInfo to ensure AutoHotkey ignores the event; otherwise it may trigger a SCxxx hotkey (where xxx is u_code).
	u_input[0].ki.dwExtraInfo = KEY_IGNORE_LEVEL(0);

	u_input[1].type = INPUT_KEYBOARD;
	u_input[1].ki.wVk = 0;
	u_input[1].ki.wScan = aChar;
	u_input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
	u_input[1].ki.time = 0;
	u_input[1].ki.dwExtraInfo = KEY_IGNORE_LEVEL(0);

	SendInput(2, u_input, sizeof(INPUT));
}

vk_type CMacroLib::TextToSpecial( LPTSTR aText, size_t aTextLength, KeyEventTypes &aEventType, modLR_type &aModifiersLR, bool aUpdatePersistent )
{
	if (!tcslicmp(aText, _T("ALTDOWN"), aTextLength))
	{
		if (aUpdatePersistent)
			if (!(aModifiersLR & (MOD_LALT | MOD_RALT))) // i.e. do nothing if either left or right is already present.
				aModifiersLR |= MOD_LALT; // If neither is down, use the left one because it's more compatible.
		aEventType = KEYDOWN;
		return VK_MENU;
	}
	if (!tcslicmp(aText, _T("ALTUP"), aTextLength))
	{
		// Unlike for Lwin/Rwin, it seems best to have these neutral keys (e.g. ALT vs. LALT or RALT)
		// restore either or both of the ALT keys into the up position.  The user can use {LAlt Up}
		// to be more specific and avoid this behavior:
		if (aUpdatePersistent)
			aModifiersLR &= ~(MOD_LALT | MOD_RALT);
		aEventType = KEYUP;
		return VK_MENU;
	}
	if (!tcslicmp(aText, _T("SHIFTDOWN"), aTextLength))
	{
		if (aUpdatePersistent)
			if (!(aModifiersLR & (MOD_LSHIFT | MOD_RSHIFT))) // i.e. do nothing if either left or right is already present.
				aModifiersLR |= MOD_LSHIFT; // If neither is down, use the left one because it's more compatible.
		aEventType = KEYDOWN;
		return VK_SHIFT;
	}
	if (!tcslicmp(aText, _T("SHIFTUP"), aTextLength))
	{
		if (aUpdatePersistent)
			aModifiersLR &= ~(MOD_LSHIFT | MOD_RSHIFT); // See "ALTUP" for explanation.
		aEventType = KEYUP;
		return VK_SHIFT;
	}
	if (!tcslicmp(aText, _T("CTRLDOWN"), aTextLength) || !tcslicmp(aText, _T("CONTROLDOWN"), aTextLength))
	{
		if (aUpdatePersistent)
			if (!(aModifiersLR & (MOD_LCONTROL | MOD_RCONTROL))) // i.e. do nothing if either left or right is already present.
				aModifiersLR |= MOD_LCONTROL; // If neither is down, use the left one because it's more compatible.
		aEventType = KEYDOWN;
		return VK_CONTROL;
	}
	if (!tcslicmp(aText, _T("CTRLUP"), aTextLength) || !tcslicmp(aText, _T("CONTROLUP"), aTextLength))
	{
		if (aUpdatePersistent)
			aModifiersLR &= ~(MOD_LCONTROL | MOD_RCONTROL); // See "ALTUP" for explanation.
		aEventType = KEYUP;
		return VK_CONTROL;
	}
	if (!tcslicmp(aText, _T("LWINDOWN"), aTextLength))
	{
		if (aUpdatePersistent)
			aModifiersLR |= MOD_LWIN;
		aEventType = KEYDOWN;
		return VK_LWIN;
	}
	if (!tcslicmp(aText, _T("LWINUP"), aTextLength))
	{
		if (aUpdatePersistent)
			aModifiersLR &= ~MOD_LWIN;
		aEventType = KEYUP;
		return VK_LWIN;
	}
	if (!tcslicmp(aText, _T("RWINDOWN"), aTextLength))
	{
		if (aUpdatePersistent)
			aModifiersLR |= MOD_RWIN;
		aEventType = KEYDOWN;
		return VK_RWIN;
	}
	if (!tcslicmp(aText, _T("RWINUP"), aTextLength))
	{
		if (aUpdatePersistent)
			aModifiersLR &= ~MOD_RWIN;
		aEventType = KEYUP;
		return VK_RWIN;
	}
	// Otherwise, leave aEventType unchanged and return zero to indicate failure:
	return 0;
}

void CMacroLib::KeyEvent( KeyEventTypes aEventType, vk_type aVK, sc_type aSC /*= 0*/, HWND aTargetWindow /*= NULL*/, bool aDoKeyDelay /*= false*/, DWORD aExtraInfo /*= KEY_IGNORE_ALL_EXCEPT_MODIFIER*/ )
{
	if (!(aVK | aSC)) // MUST USE BITWISE-OR (see comment below).
		return;
	// The above implements the rule "if neither VK nor SC was specified, return".  But they must be done as
	// bitwise-OR rather than logical-AND/OR, otherwise MSVC++ 7.1 generates 16KB of extra OBJ size for some reason.
	// That results in an 2 KB increase in compressed EXE size, and I think about 6 KB uncompressed.
	// I tried all kids of variations and reconfigurations, but the above is the only simple one that works.
	// Strangely, the same logic above (but with the preferred logical-AND/OR operator) appears elsewhere in the
	// code but doesn't bloat there.  Examples:
	//   !aVK && !aSC
	//   !vk && !sc
	//   !(aVK || aSC)

	if (!aExtraInfo) // Shouldn't be called this way because 0 is considered false in some places below (search on " = aExtraInfo" to find them).
		aExtraInfo = KEY_IGNORE_ALL_EXCEPT_MODIFIER; // Seems slightly better to use a standard default rather than something arbitrary like 1.

	// Since calls from the hook thread could come in even while the SendInput array is being constructed,
	// don't let those events get interspersed with the script's explicit use of SendInput.
	bool caller_is_keybd_hook = false;
	bool put_event_into_array = sSendMode && !caller_is_keybd_hook;
	if (sSendMode == SM_INPUT || caller_is_keybd_hook) // First check is necessary but second is just for maintainability.
		aDoKeyDelay = false;

	// Even if the sc_to_vk() mapping results in a zero-value vk, don't return.
	// I think it may be valid to send keybd_events	that have a zero vk.
	// In any case, it's unlikely to hurt anything:
	if (!aVK)
		aVK = sc_to_vk(aSC);
	else
		if (!aSC)
			// In spite of what the MSDN docs say, the scan code parameter *is* used, as evidenced by
				// the fact that the hook receives the proper scan code as sent by the below, rather than
					// zero like it normally would.  Even though the hook would try to use MapVirtualKey() to
						// convert zero-value scan codes, it's much better to send it here also for full compatibility
							// with any apps that may rely on scan code (and such would be the case if the hook isn't
								// active because the user doesn't need it; also for some games maybe).  In addition, if the
									// current OS is Win9x, we must map it here manually (above) because otherwise the hook
										// wouldn't be able to differentiate left/right on keys such as RControl, which is detected
											// via its scan code.
												aSC = vk_to_sc(aVK);

	BYTE aSC_lobyte = LOBYTE(aSC);
	DWORD event_flags = HIBYTE(aSC) ? KEYEVENTF_EXTENDEDKEY : 0;

	// Do this only after the above, so that the SC is left/right specific if the VK was such,
	// even on Win9x (though it's probably never called that way for Win9x; it's probably always
	// called with either just the proper left/right SC or that plus the neutral VK).
	// Under WinNT/2k/XP, sending VK_LCONTROL and such result in the high-level (but not low-level
	// I think) hook receiving VK_CONTROL.  So somewhere internally it's being translated (probably
	// by keybd_event itself).  In light of this, translate the keys here manually to ensure full
	// support under Win9x (which might not support this internal translation).  The scan code
	// looked up above should still be correct for left-right centric keys even under Win9x.
	// v1.0.43: Apparently, the journal playback hook also requires neutral modifier keystrokes
	// rather than left/right ones.  Otherwise, the Shift key can't capitalize letters, etc.
	if (sSendMode == SM_PLAY || g_os.IsWin9x())
	{
		// Convert any non-neutral VK's to neutral for these OSes, since apps and the OS itself
		// can't be expected to support left/right specific VKs while running under Win9x:
		switch(aVK)
		{
		case VK_LCONTROL:
		case VK_RCONTROL: aVK = VK_CONTROL; break; // But leave scan code set to a left/right specific value rather than converting it to "left" unconditionally.
		case VK_LSHIFT:
		case VK_RSHIFT: aVK = VK_SHIFT; break;
		case VK_LMENU:
		case VK_RMENU: aVK = VK_MENU; break;
		}
	}

	// aTargetWindow is almost always passed in as NULL by our caller, even if the overall command
	// being executed is ControlSend.  This is because of the following reasons:
	// 1) Modifiers need to be logically changed via keybd_event() when using ControlSend against
	//    a cmd-prompt, console, or possibly other types of windows.
	// 2) If a hotkey triggered the ControlSend that got us here and that hotkey is a naked modifier
	//    such as RAlt:: or modified modifier such as ^#LShift, that modifier would otherwise auto-repeat
	//    an possibly interfere with the send operation.  This auto-repeat occurs because unlike a normal
	//    send, there are no calls to keybd_event() (keybd_event() stop the auto-repeat as a side-effect).
	// One exception to this is something like "ControlSend, Edit1, {Control down}", which explicitly
	// calls us with a target window.  This exception is by design and has been bug-fixed and documented
	// in ControlSend for v1.0.21:
	if (aTargetWindow) // This block shouldn't affect overall thread-safety because hook thread never calls it in this mode.
	{
		if (KeyToModifiersLR(aVK, aSC))
		{
			// When sending modifier keystrokes directly to a window, use the AutoIt3 SetKeyboardState()
			// technique to improve the reliability of changes to modifier states.  If this is not done,
			// sometimes the state of the SHIFT key (and perhaps other modifiers) will get out-of-sync
			// with what's intended, resulting in uppercase vs. lowercase problems (and that's probably
			// just the tip of the iceberg).  For this to be helpful, our caller must have ensured that
			// our thread is attached to aTargetWindow's (but it seems harmless to do the below even if
			// that wasn't done for any reason).  Doing this here in this function rather than at a
			// higher level probably isn't best in terms of performance (e.g. in the case where more
			// than one modifier is being changed, the multiple calls to Get/SetKeyboardState() could
			// be consolidated into one call), but it is much easier to code and maintain this way
			// since many different functions might call us to change the modifier state:
			BYTE state[256];
			GetKeyboardState((PBYTE)&state);
			if (aEventType == KEYDOWN)
				state[aVK] |= 0x80;
			else if (aEventType == KEYUP)
				state[aVK] &= ~0x80;
			// else KEYDOWNANDUP, in which case it seems best (for now) not to change the state at all.
			// It's rarely if ever called that way anyway.

			// If aVK is a left/right specific key, be sure to also update the state of the neutral key:
			switch(aVK)
			{
			case VK_LCONTROL: 
			case VK_RCONTROL:
				if ((state[VK_LCONTROL] & 0x80) || (state[VK_RCONTROL] & 0x80))
					state[VK_CONTROL] |= 0x80;
				else
					state[VK_CONTROL] &= ~0x80;
				break;
			case VK_LSHIFT:
			case VK_RSHIFT:
				if ((state[VK_LSHIFT] & 0x80) || (state[VK_RSHIFT] & 0x80))
					state[VK_SHIFT] |= 0x80;
				else
					state[VK_SHIFT] &= ~0x80;
				break;
			case VK_LMENU:
			case VK_RMENU:
				if ((state[VK_LMENU] & 0x80) || (state[VK_RMENU] & 0x80))
					state[VK_MENU] |= 0x80;
				else
					state[VK_MENU] &= ~0x80;
				break;
			}

			SetKeyboardState((PBYTE)&state);
			// Even after doing the above, we still continue on to send the keystrokes
			// themselves to the window, for greater reliability (same as AutoIt3).
		}

		// lowest 16 bits: repeat count: always 1 for up events, probably 1 for down in our case.
		// highest order bits: 11000000 (0xC0) for keyup, usually 00000000 (0x00) for keydown.
		LPARAM lParam = (LPARAM)(aSC << 16);
		if (aEventType != KEYUP)  // i.e. always do it for KEYDOWNANDUP
			PostMessage(aTargetWindow, WM_KEYDOWN, aVK, lParam | 0x00000001);
		// The press-duration delay is done only when this is a down-and-up because otherwise,
		// the normal g->KeyDelay will be in effect.  In other words, it seems undesirable in
		// most cases to do both delays for only "one half" of a keystroke:
		if (aDoKeyDelay && aEventType == KEYDOWNANDUP)
			DoKeyDelay(-1); // Since aTargetWindow!=NULL, sSendMode!=SM_PLAY, so no need for to ever use the SendPlay press-duration.
		if (aEventType != KEYDOWN)  // i.e. always do it for KEYDOWNANDUP
			PostMessage(aTargetWindow, WM_KEYUP, aVK, lParam | 0xC0000001);
	}


	if (aDoKeyDelay) // SM_PLAY also uses DoKeyDelay(): it stores the delay item in the event array.
		DoKeyDelay(); // Thread-safe because only called by main thread in this mode.  See notes above.
}

sc_type CMacroLib::vk_to_sc( vk_type aVK, bool aReturnSecondary /*= false*/ )
{
	// Try to minimize the number mappings done manually because MapVirtualKey is a more reliable
	// way to get the mapping if user has non-standard or custom keyboard layout.

	sc_type sc = 0;

	switch (aVK)
	{
		// Yield a manually translation for virtual keys that MapVirtualKey() doesn't support or for which it
		// doesn't yield consistent result (such as Win9x supporting only SHIFT rather than VK_LSHIFT/VK_RSHIFT).
	case VK_LSHIFT:   sc = SC_LSHIFT; break; // Modifiers are listed first for performance.
	case VK_RSHIFT:   sc = SC_RSHIFT; break;
	case VK_LCONTROL: sc = SC_LCONTROL; break;
	case VK_RCONTROL: sc = SC_RCONTROL; break;
	case VK_LMENU:    sc = SC_LALT; break;
	case VK_RMENU:    sc = SC_RALT; break;
	case VK_LWIN:     sc = SC_LWIN; break; // Earliest versions of Win95/NT might not support these, so map them manually.
	case VK_RWIN:     sc = SC_RWIN; break; //

		// According to http://support.microsoft.com/default.aspx?scid=kb;en-us;72583
		// most or all numeric keypad keys cannot be mapped reliably under any OS. The article is
		// a little unclear about which direction, if any, that MapVirtualKey() does work in for
		// the numpad keys, so for peace-of-mind map them all manually for now:
	case VK_NUMPAD0:  sc = SC_NUMPAD0; break;
	case VK_NUMPAD1:  sc = SC_NUMPAD1; break;
	case VK_NUMPAD2:  sc = SC_NUMPAD2; break;
	case VK_NUMPAD3:  sc = SC_NUMPAD3; break;
	case VK_NUMPAD4:  sc = SC_NUMPAD4; break;
	case VK_NUMPAD5:  sc = SC_NUMPAD5; break;
	case VK_NUMPAD6:  sc = SC_NUMPAD6; break;
	case VK_NUMPAD7:  sc = SC_NUMPAD7; break;
	case VK_NUMPAD8:  sc = SC_NUMPAD8; break;
	case VK_NUMPAD9:  sc = SC_NUMPAD9; break;
	case VK_DECIMAL:  sc = SC_NUMPADDOT; break;
	case VK_NUMLOCK:  sc = SC_NUMLOCK; break;
	case VK_DIVIDE:   sc = SC_NUMPADDIV; break;
	case VK_MULTIPLY: sc = SC_NUMPADMULT; break;
	case VK_SUBTRACT: sc = SC_NUMPADSUB; break;
	case VK_ADD:      sc = SC_NUMPADADD; break;
	}

	if (sc) // Above found a match.
		return aReturnSecondary ? 0 : sc; // Callers rely on zero being returned for VKs that don't have secondary SCs.

	// Use the OS API's MapVirtualKey() to resolve any not manually done above:
	if (   !(sc = MapVirtualKey(aVK, 0))   )
		return 0; // Indicate "no mapping".

	// Turn on the extended flag for those that need it.
	// Because MapVirtualKey can only accept (and return) naked scan codes (the low-order byte),
	// handle extended scan codes that have a non-extended counterpart manually.
	// Older comment: MapVirtualKey() should include 0xE0 in HIBYTE if key is extended, BUT IT DOESN'T.
	// There doesn't appear to be any built-in function to determine whether a vk's scan code
	// is extended or not.  See MSDN topic "keyboard input" for the below list.
	// Note: NumpadEnter is probably the only extended key that doesn't have a unique VK of its own.
	// So in that case, probably safest not to set the extended flag.  To send a true NumpadEnter,
	// as well as a true NumPadDown and any other key that shares the same VK with another, the
	// caller should specify the sc param to circumvent the need for KeyEvent() to use the below:
	switch (aVK)
	{
	case VK_APPS:     // Application key on keyboards with LWIN/RWIN/Apps.  Not listed in MSDN as "extended"?
	case VK_CANCEL:   // Ctrl-break
	case VK_SNAPSHOT: // PrintScreen
	case VK_DIVIDE:   // NumpadDivide (slash)
	case VK_NUMLOCK:
		// Below are extended but were already handled and returned from higher above:
		//case VK_LWIN:
		//case VK_RWIN:
		//case VK_RMENU:
		//case VK_RCONTROL:
		//case VK_RSHIFT: // WinXP needs this to be extended for keybd_event() to work properly.
		sc |= 0x0100;
		break;

		// The following virtual keys have more than one physical key, and thus more than one scan code.
		// If the caller passed true for aReturnSecondary, the extended version of the scan code will be
		// returned (all of the following VKs have two SCs):
	case VK_RETURN:
	case VK_INSERT:
	case VK_DELETE:
	case VK_PRIOR: // PgUp
	case VK_NEXT:  // PgDn
	case VK_HOME:
	case VK_END:
	case VK_UP:
	case VK_DOWN:
	case VK_LEFT:
	case VK_RIGHT:
		return aReturnSecondary ? (sc | 0x0100) : sc; // Below relies on the fact that these cases return early.
	}

	// Since above didn't return, if aReturnSecondary==true, return 0 to indicate "no secondary SC for this VK".
	return aReturnSecondary ? 0 : sc; // Callers rely on zero being returned for VKs that don't have secondary SCs.
}

void CMacroLib::DoKeyDelay( int aDelay /*= (sSendMode == SM_PLAY) ? g->KeyDelayPlay : g->KeyDelay*/ )
{
	if (aDelay < 0) // To support user-specified KeyDelay of -1 (fastest send rate).
		return;
	if (sSendMode)
	{
		//if (sSendMode == SM_PLAY && aDelay > 0) // Zero itself isn't supported by playback hook, so no point in inserting such delays into the array.
			//PutKeybdEventIntoArray(0, 0, 0, 0, aDelay); // Passing zero for vk and sc signals it that aExtraInfo contains the delay.
		//else for other types of arrays, never insert a delay or do one now.
		return;
	}
	if (g_os.IsWin9x())
	{
		// Do a true sleep on Win9x because the MsgSleep() method is very inaccurate on Win9x
		// for some reason (a MsgSleep(1) causes a sleep between 10 and 55ms, for example):
		Sleep(aDelay);
		return;
	}
	Sleep(aDelay);
	//SLEEP_WITHOUT_INTERRUPTION(aDelay);

}

void CMacroLib::SendASC( LPCTSTR aAscii )
{
	// UPDATE: In v1.0.42.04, the left Alt key is always used below because:
	// 1) It might be required on Win95/NT (though testing shows that RALT works okay on Windows 98se).
	// 2) It improves maintainability because if the keyboard layout has AltGr, and the Control portion
	//    of AltGr is released without releasing the RAlt portion, anything that expects LControl to
	//    be down whenever RControl is down would be broken.
	// The following test demonstrates that on previous versions under German layout, the right-Alt key
	// portion of AltGr could be used to manifest Alt+Numpad combinations:
	//   Send {RAlt down}{Asc 67}{RAlt up}  ; Should create a C character even when both the active window an AHK are set to German layout.
	//   KeyHistory  ; Shows that the right-Alt key was successfully used rather than the left.
	// Changing the modifier state via SetModifierLRState() (rather than some more error-prone multi-step method)
	// also ensures that the ALT key is pressed down only after releasing any shift key that needed it above.
	// Otherwise, the OS's switch-keyboard-layout hotkey would be triggered accidentally; e.g. the following
	// in English layout: Send ~~âÂ{^}.
	//
	// Make sure modifier state is correct: ALT pressed down and other modifiers UP
	// because CTRL and SHIFT seem to interfere with this technique if they are down,
	// at least under WinXP (though the Windows key doesn't seem to be a problem).
	// Specify KEY_IGNORE so that this action does not affect the modifiers that the
	// hook uses to determine which hotkey should be triggered for a suffix key that
	// has more than one set of triggering modifiers (for when the user is holding down
	// that suffix to auto-repeat it -- see keyboard_mouse.h for details).
	modLR_type modifiersLR_now = sSendMode ? sEventModifiersLR : GetModifierLRState();
	SetModifierLRState((modifiersLR_now | MOD_LALT) & ~(MOD_RALT | MOD_LCONTROL | MOD_RCONTROL | MOD_LSHIFT | MOD_RSHIFT)
		, modifiersLR_now, NULL, false // Pass false because there's no need to disguise the down-event of LALT.
		, true, KEY_IGNORE); // Pass true so that any release of RALT is disguised (Win is never released here).
	// Note: It seems best never to press back down any key released above because the
	// act of doing so may do more harm than good (i.e. the keystrokes may caused
	// unexpected side-effects.

	// Known limitation (but obscure): There appears to be some OS limitation that prevents the following
	// AltGr hotkey from working more than once in a row:
	// <^>!i::Send {ASC 97}
	// Key history indicates it's doing what it should, but it doesn't actually work.  You have to press the
	// left-Alt key (not RAlt) once to get the hotkey working again.

	// This is not correct because it is possible to generate unicode characters by typing
	// Alt+256 and beyond:
	// int value = ATOI(aAscii);
	// if (value < 0 || value > 255) return 0; // Sanity check.

	// Known issue: If the hotkey that triggers this Send command is CONTROL-ALT
	// (and maybe either CTRL or ALT separately, as well), the {ASC nnnn} method
	// might not work reliably due to strangeness with that OS feature, at least on
	// WinXP.  I already tried adding delays between the keystrokes and it didn't help.

	// Caller relies upon us to stop upon reaching the first non-digit character:
	for (LPCTSTR cp = aAscii; *cp >= '0' && *cp <= '9'; ++cp)
		// A comment from AutoIt3: ASCII 0 is 48, NUMPAD0 is 96, add on 48 to the ASCII.
			// Also, don't do WinDelay after each keypress in this case because it would make
				// such keys take up to 3 or 4 times as long to send (AutoIt3 avoids doing the
					// delay also).  Note that strings longer than 4 digits are allowed because
						// some or all OSes support Unicode characters 0 through 65535.
							KeyEvent(KEYDOWNANDUP, *cp + 48);

	// Must release the key regardless of whether it was already down, so that the sequence will take effect
	// immediately.  Otherwise, our caller might not release the Alt key (since it might need to stay down for
	// other purposes), in which case Alt+Numpad character would never appear and the caller's subsequent
	// keystrokes might get absorbed by the OS's special state of "waiting for Alt+Numpad sequence to complete".
	// Another reason is that the user may be physically holding down Alt, in which case the caller might never
	// release it.  In that case, we want the Alt+Numpad character to appear immediately rather than waiting for
	// the user to release Alt (in the meantime, the caller will likely press Alt back down to match the physical
	// state).
	KeyEvent(KEYUP, VK_MENU);
}

void CMacroLib::SetClipBoard( CString strText )
{
	COleDataSource* pobjDataSource = new COleDataSource;

	HGLOBAL hGlobal = GlobalAlloc(GPTR, (strText.GetLength() + 1) * 2);

	LPCTSTR wszSourceText = (LPCTSTR)strText;
	wchar_t* wszTargetText = (wchar_t*)GlobalLock(hGlobal);
	wcscpy(wszTargetText, wszSourceText);
	GlobalUnlock(hGlobal);

	pobjDataSource->CacheGlobalData(CF_UNICODETEXT, hGlobal);

	OleInitialize(NULL);
	pobjDataSource->SetClipboard();
}
