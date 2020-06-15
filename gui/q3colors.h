#ifndef _Q3_COLORS_H_
#define _Q3_COLORS_H_

#define COLOR_BITS	31
#define ColorIndex(c)	( ( (c) - '0' ) & COLOR_BITS )
#define Q_COLOR_ESCAPE	'^'
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) != Q_COLOR_ESCAPE )

static const char *q3colors[] = {
	"#000000",
	"#ff0000",
	"#00ff00",
	"#ffff00",
	"#0000ff",
	"#00ffff",
	"#ff00ff",
	"#ffffff",
	"#ff7f00",
	"#7f7f7f",
	"#bfbfbf",
	"#bfbfbf",
	"#007f00",
	"#7f7f00",
	"#00007f",
	"#7f0000",
	"#7f3f00",
	"#ff9919",
	"#007f7f",
	"#7f007f",
	"#007fff",
	"#7f00ff",
	"#3399cc",
	"#ccffcc",
	"#006633",
	"#ff0033",
	"#b21919",
	"#993300",
	"#cc9933",
	"#999933",
	"#ffffbf",
	"#ffff7f",
};

#endif
