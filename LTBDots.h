// dots.h

#ifndef _DOTS_h
#define _DOTS_h

#if (ARDUINO >= 100)
#include <Arduino.h>

#else
#include <WProgram.h>
#include <pins_arduino.h>
#endif
#include <SPI.h>

typedef struct  RGB { uint8_t g; uint8_t r; uint8_t b; }RGB;
typedef struct  iRGB { unsigned g; unsigned r; unsigned b; }iRGB;
#define ushort unsigned short

#define CLR(r,g,b) ((RGB){b, g, r})
#define CLRH(h) ((RGB){(h & 0x0000ff)>>8, (h & 0x00ff00)>>16, (h & 0xff0000)})

#define DIMMER 1
#define ROT_LFT 2
#define ROT_RGT 3

#define SCALE 8

RGB		addRGB(RGB v1, RGB v2);
iRGB	toiRGB(RGB c);
iRGB	RGBStepSz(RGB start, RGB end, int nsteps);

/*!
* \class pixPat
*
* \brief hold and manipulate individual light patterns
*
* This class defines a light pattern which consists of a number of RGB values, the number
* of colors in the string and the number of times the sequence will be written
*
* \author Kevin Wilson
* \date
*/
class Pattern;
class pixPat;
class RTPat;

class Action
{
public:
	Action() { nxt = 0; actionComplete = false; };

	virtual ~Action() { nxt = 0; };

	inline Action			*Nxt() { return nxt; };
	inline bool				isLast() { return nxt == NULL; };
	inline bool				isComplete() { return (durTmr == durTime); };
	inline void				Append(Action *p) { nxt = p; return; };
	void					deleteNext() { Action *nnxt = nxt->nxt; delete nxt; nxt = nnxt; return; };
	virtual uint8_t			actionType() = 0;
	virtual void			setDimAct(Pattern *pat, uint8_t tgt, ushort dur) = 0;
	virtual bool			timerTic(unsigned short deltaT) = 0;
	Action					*nxt;
protected:
	Pattern					*pat;				// pattern that is target of action
	short					durTmr;				// timer for durTime
	short					durTime;			//  time length of action
	bool					actionComplete;		// flag if action is done and can be deleted
};

#define MINTIC 32
class actionOnLvl :public Action
{
public:
	actionOnLvl(Pattern *ptr, uint8_t tgt, ushort dur);

	void			setDimAct(Pattern *pat, uint8_t tgt, ushort dur);
	void			calcSlopes(Pattern *ptr, uint8_t *start, uint8_t *end, ushort dur);
	bool			timerTic(unsigned short deltaT);
	inline uint8_t	actionType() { return DIMMER; };

protected:
	uint8_t			startLvl;
	uint8_t			nxtLvl;
	float			dY;
	Pattern			*pat;
};


class Pattern
{
public:
	Pattern() { nxt = 0; acts = 0; };
	Pattern(uint8_t pct);
	virtual ~Pattern() { nxt = 0; };//also need to delete the actions list

	inline Pattern			*Nxt() { return nxt; };
	inline bool				isLast() { return nxt == NULL; };
	inline void				Append(Pattern *p) { p->nxt = 0; nxt = p; return; };
	bool					doActions(uint16_t deltaT);
	void					addAct(Action *a);
	void					dimPat(uint8_t tgt, ushort dur);
	void					deleteAct(Action *ptr);
	void					cleanCompleteActions();

	void					setOnLvl(uint8_t pct);
	uint8_t					getOnLvl();
	void					updateLvl(int8_t deltaPct);

	virtual void			rampPat(int start = -1, int end = -1) {};
	virtual void			fillPat(RGB fill, int start = -1, int end = -1) {};
	void					printPat(char *s);
	virtual	inline void		setNumPix(uint8_t n) {};
	inline void				setNumReps(uint16_t n) { numReps = n; };
	inline void				incNumReps() { numReps++; };
	inline void				decNumReps() { numReps--; };
	virtual	void			rotateLeft(uint8_t num = 1) {};
	virtual	void			rotateRight(uint8_t num = 1) {};
	virtual	void			mergePix(pixPat &p1, uint8_t startPix1, pixPat &p2, uint8_t startPix2) {};

	virtual	void			initFader(RGB *fadeEnd, short fadeSteps) {};	// setup to fade pattern to fadeEnd in fadeSteps
	virtual	void			stepFader() {};								// update the pixels one fade step
	virtual	void			clearFader() {};								// delete buffers... requires initFader to fade again
	virtual	void			resetFader() {};								// restart colors at pre-fade values
	virtual uint8_t			*fillRGB(uint8_t *p) = 0;
	virtual	RGB				*getCol(short indx) { if (indx < 0)indx = 0; return color + indx; };
protected:
	virtual void			reCalc() = 0;

	RGB						*color;			// holds color values to be displayed
	Pattern					*nxt;
	Action					*acts;			// holds list of change events
	uint8_t					onLvl;			// brightness level as %
	uint8_t					numPix;
	uint16_t				numReps;
};

class pixPat :public Pattern
{
public:
	pixPat();
	pixPat(RGB *leds, uint8_t nleds, uint16_t nreps, uint8_t onlvl);
	//		pixPat	operator=(const pixPat &p);	
	~pixPat();

	void			reCalc() {};

	void			fillPat(RGB fill, int start = -1, int end = -1);
	void			rotateLeft(uint8_t num = 1);
	void			rotateRight(uint8_t num = 1);

	inline void		setNumPix(uint8_t n) { numPix = n; };

	uint8_t 		*fillRGB(uint8_t *p);
	void			mergePix(pixPat &p1, uint8_t startPix1, pixPat &p2, uint8_t startPix2);
	void			initFader(RGB *fadeEnd, short fadeSteps);	// setup to fade pattern to fadeEnd in fadeSteps 
	void			stepFader();								// update the pixels one fade step
	void			clearFader();								// delete buffers... requires initFader to fade again
	void			resetFader();								// restart colors at pre-fade values
	void			fadeNeighbors(RGB prev);					// fades pattern from prev pixPat last pix to next pat first pix

protected:
	void	leftRot(uint8_t *p, short n);
	pixPat(const pixPat &p);


	//fader stuff
	short *current;
	short *initPix;
	short *delta;
	iRGB	xstart;
	iRGB	xdelta;

};


class RTPat :public Pattern
{
public:
	RTPat() { numReps = 0; };
	RTPat(RGB *c, uint16_t nReps, uint8_t onlvl);
	~RTPat() { numReps = 0; };

	uint8_t		*fillRGB(uint8_t *p);

protected:
	void		reCalc();
	RTPat(const RTPat &p);

	iRGB	start;
	iRGB	delta;
};





/*!
* \class LTBDots
*
* \brief Manages a set of pixPat elements to define a pixel sequence for a neopixel string
*
* This class contains a set of pixPats to fill an entire rgb tape. Calls are made to this
* class to define and add pixPats and then to right the RGB values to the led tape
*
* \author Kevin Wilson
* \date
*/
class LTBDots
{
public:
	LTBDots() {};
	/*!
	* \brief [brief description]
	*
	* [detailed description]
	*
	* \param[in] RGB *pix RGB color array of the pattern to be added
	* \param[in] np number of elements in pix
	* \param[in] nr number of reps of pix make up the pattern
	* \return pointer to newly created pixPat instance... should be saved for later
	* \sa [see also section]
	* \note [any note about the function you might have]
	* \warning [any warning if necessary]
	*/
	LTBDots(short n);
	Pattern		*addPat(RGB *pix, uint8_t np, uint8_t nr, uint8_t onlvl = 100);
	Pattern		*addTrans(RGB *pix, short nr, uint8_t onlvl = 100);
	void		printStrip(char *title, bool dotsOnly=false);
	void		setOnLvl(uint8_t pct);
	void		showLights(bool force = false);
	void		clearPats();
	void		clearLights(RGB fill);
	void		sendTrailer();
	void		sendLeader();

	//	~LTBDots();
	uint8_t *curStrip;
	uint8_t *dp;

protected:
	void	addPat(Pattern *pat);

	short	nPix;			// total number of leds in chain
	Pattern *pats;
	uint8_t	*dots;
	unsigned long lastMsec;

private:
	LTBDots(const LTBDots &c);
	LTBDots& operator=(const LTBDots &c);

};

enum
{
	AliceBlue = 0xF0F8FF, Amethyst = 0x9966CC, AntiqueWhite = 0xFAEBD7, Aqua = 0x00FFFF, Aquamarine = 0x7FFFD4, Azure = 0xF0FFFF, Beige = 0xF5F5DC,
	Bisque = 0xFFE4C4, Black = 0x000000, BlanchedAlmond = 0xFFEBCD, Blue = 0x0000FF, BlueViolet = 0x8A2BE2, Brown = 0xA52A2A, BurlyWood = 0xDEB887,
	CadetBlue = 0x5F9EA0, Chartreuse = 0x7FFF00, Chocolate = 0xD2691E, Coral = 0xFF7F50, CornflowerBlue = 0x6495ED, Cornsilk = 0xFFF8DC, Crimson = 0xDC143C,
	Cyan = 0x00FFFF,
	DarkBlue = 0x00008B,
	DarkCyan = 0x008B8B,
	DarkGoldenrod = 0xB8860B,
	DarkGray = 0xA9A9A9,
	DarkGrey = 0xA9A9A9,
	DarkGreen = 0x006400,
	DarkKhaki = 0xBDB76B,
	DarkMagenta = 0x8B008B,
	DarkOliveGreen = 0x556B2F,
	DarkOrange = 0xFF8C00,
	DarkOrchid = 0x9932CC,
	DarkRed = 0x8B0000,
	DarkSalmon = 0xE9967A,
	DarkSeaGreen = 0x8FBC8F,
	DarkSlateBlue = 0x483D8B,
	DarkSlateGray = 0x2F4F4F,
	DarkSlateGrey = 0x2F4F4F,
	DarkTurquoise = 0x00CED1,
	DarkViolet = 0x9400D3,
	DeepPink = 0xFF1493,
	DeepSkyBlue = 0x00BFFF,
	DimGray = 0x696969,
	DimGrey = 0x696969,
	DodgerBlue = 0x1E90FF,
	FireBrick = 0xB22222,
	FloralWhite = 0xFFFAF0,
	ForestGreen = 0x228B22,
	Fuchsia = 0xFF00FF,
	Gainsboro = 0xDCDCDC,
	GhostWhite = 0xF8F8FF,
	Gold = 0xFFD700,
	Goldenrod = 0xDAA520,
	Gray = 0x808080,
	Grey = 0x808080,
	Green = 0x008000,
	GreenYellow = 0xADFF2F,
	Honeydew = 0xF0FFF0,
	HotPink = 0xFF69B4,
	IndianRed = 0xCD5C5C,
	Indigo = 0x4B0082,
	Ivory = 0xFFFFF0,
	Khaki = 0xF0E68C,
	Lavender = 0xE6E6FA,
	LavenderBlush = 0xFFF0F5,
	LawnGreen = 0x7CFC00,
	LemonChiffon = 0xFFFACD,
	LightBlue = 0xADD8E6,
	LightCoral = 0xF08080,
	LightCyan = 0xE0FFFF,
	LightGoldenrodYellow = 0xFAFAD2,
	LightGreen = 0x90EE90,
	LightGrey = 0xD3D3D3,
	LightPink = 0xFFB6C1,
	LightSalmon = 0xFFA07A,
	LightSeaGreen = 0x20B2AA,
	LightSkyBlue = 0x87CEFA,
	LightSlateGray = 0x778899,
	LightSlateGrey = 0x778899,
	LightSteelBlue = 0xB0C4DE,
	LightYellow = 0xFFFFE0,
	Lime = 0x00FF00,
	LimeGreen = 0x32CD32,
	Linen = 0xFAF0E6,
	Magenta = 0xFF00FF,
	Maroon = 0x800000,
	MediumAquamarine = 0x66CDAA,
	MediumBlue = 0x0000CD,
	MediumOrchid = 0xBA55D3,
	MediumPurple = 0x9370DB,
	MediumSeaGreen = 0x3CB371,
	MediumSlateBlue = 0x7B68EE,
	MediumSpringGreen = 0x00FA9A,
	MediumTurquoise = 0x48D1CC,
	MediumVioletRed = 0xC71585,
	MidnightBlue = 0x191970,
	MintCream = 0xF5FFFA,
	MistyRose = 0xFFE4E1,
	Moccasin = 0xFFE4B5,
	NavajoWhite = 0xFFDEAD,
	Navy = 0x000080,
	OldLace = 0xFDF5E6,
	Olive = 0x808000,
	OliveDrab = 0x6B8E23,
	Orange = 0xFFA500,
	OrangeRed = 0xFF4500,
	Orchid = 0xDA70D6,
	PaleGoldenrod = 0xEEE8AA,
	PaleGreen = 0x98FB98,
	PaleTurquoise = 0xAFEEEE,
	PaleVioletRed = 0xDB7093,
	PapayaWhip = 0xFFEFD5,
	PeachPuff = 0xFFDAB9,
	Peru = 0xCD853F,
	Pink = 0xFFC0CB,
	Plaid = 0xCC5533,
	Plum = 0xDDA0DD,
	PowderBlue = 0xB0E0E6,
	Purple = 0x800080,
	Red = 0xFF0000,
	RosyBrown = 0xBC8F8F,
	RoyalBlue = 0x4169E1,
	SaddleBrown = 0x8B4513,
	Salmon = 0xFA8072,
	SandyBrown = 0xF4A460,
	SeaGreen = 0x2E8B57,
	Seashell = 0xFFF5EE,
	Sienna = 0xA0522D,
	Silver = 0xC0C0C0,
	SkyBlue = 0x87CEEB,
	SlateBlue = 0x6A5ACD,
	SlateGray = 0x708090,
	SlateGrey = 0x708090,
	Snow = 0xFFFAFA,
	SpringGreen = 0x00FF7F,
	SteelBlue = 0x4682B4,
	Tan = 0xD2B48C,
	Teal = 0x008080,
	Thistle = 0xD8BFD8,
	Tomato = 0xFF6347,
	Turquoise = 0x40E0D0,
	Violet = 0xEE82EE,
	Wheat = 0xF5DEB3,
	White = 0xFFFFFF,
	WhiteSmoke = 0xF5F5F5,
	Yellow = 0xFFFF00,
	YellowGreen = 0x9ACD32
} ColorDefs;

#endif

