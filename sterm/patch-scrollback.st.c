--- st.c.orig	2022-10-04 20:41:26.000000000 +0300
+++ st.c	2023-12-10 21:42:11.959909000 +0300
@@ -35,6 +35,7 @@
 #define ESC_ARG_SIZ   16
 #define STR_BUF_SIZ   ESC_BUF_SIZ
 #define STR_ARG_SIZ   ESC_ARG_SIZ
+#define HISTSIZE      2000
 
 /* macros */
 #define IS_SET(flag)		((term.mode & (flag)) != 0)
@@ -42,6 +43,9 @@
 #define ISCONTROLC1(c)		(BETWEEN(c, 0x80, 0x9f))
 #define ISCONTROL(c)		(ISCONTROLC0(c) || ISCONTROLC1(c))
 #define ISDELIM(u)		(u && wcschr(worddelimiters, u))
+#define TLINE(y)		((y) < term.scr ? term.hist[((y) + term.histi - \
+				term.scr + HISTSIZE + 1) % HISTSIZE] : \
+				term.line[(y) - term.scr])
 
 enum term_mode {
 	MODE_WRAP        = 1 << 0,
@@ -115,6 +119,9 @@
 	int col;      /* nb col */
 	Line *line;   /* screen */
 	Line *alt;    /* alternate screen */
+	Line hist[HISTSIZE]; /* history buffer */
+	int histi;    /* history index */
+	int scr;      /* scroll back */
 	int *dirty;   /* dirtyness of lines */
 	TCursor c;    /* cursor */
 	int ocx;      /* old cursor col */
@@ -185,8 +192,8 @@
 static void tputtab(int);
 static void tputc(Rune);
 static void treset(void);
-static void tscrollup(int, int);
-static void tscrolldown(int, int);
+static void tscrollup(int, int, int);
+static void tscrolldown(int, int, int);
 static void tsetattr(const int *, int);
 static void tsetchar(Rune, const Glyph *, int, int);
 static void tsetdirt(int, int);
@@ -409,10 +416,10 @@
 {
 	int i = term.col;
 
-	if (term.line[y][i - 1].mode & ATTR_WRAP)
+	if (TLINE(y)[i - 1].mode & ATTR_WRAP)
 		return i;
 
-	while (i > 0 && term.line[y][i - 1].u == ' ')
+	while (i > 0 && TLINE(y)[i - 1].u == ' ')
 		--i;
 
 	return i;
@@ -521,7 +528,7 @@
 		 * Snap around if the word wraps around at the end or
 		 * beginning of a line.
 		 */
-		prevgp = &term.line[*y][*x];
+		prevgp = &TLINE(*y)[*x];
 		prevdelim = ISDELIM(prevgp->u);
 		for (;;) {
 			newx = *x + direction;
@@ -536,14 +543,14 @@
 					yt = *y, xt = *x;
 				else
 					yt = newy, xt = newx;
-				if (!(term.line[yt][xt].mode & ATTR_WRAP))
+				if (!(TLINE(yt)[xt].mode & ATTR_WRAP))
 					break;
 			}
 
 			if (newx >= tlinelen(newy))
 				break;
 
-			gp = &term.line[newy][newx];
+			gp = &TLINE(newy)[newx];
 			delim = ISDELIM(gp->u);
 			if (!(gp->mode & ATTR_WDUMMY) && (delim != prevdelim
 					|| (delim && gp->u != prevgp->u)))
@@ -564,14 +571,14 @@
 		*x = (direction < 0) ? 0 : term.col - 1;
 		if (direction < 0) {
 			for (; *y > 0; *y += direction) {
-				if (!(term.line[*y-1][term.col-1].mode
+				if (!(TLINE(*y-1)[term.col-1].mode
 						& ATTR_WRAP)) {
 					break;
 				}
 			}
 		} else if (direction > 0) {
 			for (; *y < term.row-1; *y += direction) {
-				if (!(term.line[*y][term.col-1].mode
+				if (!(TLINE(*y)[term.col-1].mode
 						& ATTR_WRAP)) {
 					break;
 				}
@@ -602,13 +609,13 @@
 		}
 
 		if (sel.type == SEL_RECTANGULAR) {
-			gp = &term.line[y][sel.nb.x];
+			gp = &TLINE(y)[sel.nb.x];
 			lastx = sel.ne.x;
 		} else {
-			gp = &term.line[y][sel.nb.y == y ? sel.nb.x : 0];
+			gp = &TLINE(y)[sel.nb.y == y ? sel.nb.x : 0];
 			lastx = (sel.ne.y == y) ? sel.ne.x : term.col-1;
 		}
-		last = &term.line[y][MIN(lastx, linelen-1)];
+		last = &TLINE(y)[MIN(lastx, linelen-1)];
 		while (last >= gp && last->u == ' ')
 			--last;
 
@@ -844,7 +851,10 @@
 ttywrite(const char *s, size_t n, int may_echo)
 {
 	const char *next;
+	Arg arg = (Arg) { .i = term.scr };
 
+	kscrolldown(&arg);
+
 	if (may_echo && IS_SET(MODE_ECHO))
 		twrite(s, n, 1);
 
@@ -1055,13 +1065,53 @@
 }
 
 void
-tscrolldown(int orig, int n)
+kscrolldown(const Arg* a)
 {
+	int n = a->i;
+
+	if (n < 0)
+		n = term.row + n;
+
+	if (n > term.scr)
+		n = term.scr;
+
+	if (term.scr > 0) {
+		term.scr -= n;
+		selscroll(0, -n);
+		tfulldirt();
+	}
+}
+
+void
+kscrollup(const Arg* a)
+{
+	int n = a->i;
+
+	if (n < 0)
+		n = term.row + n;
+
+	if (term.scr <= HISTSIZE-n) {
+		term.scr += n;
+		selscroll(0, n);
+		tfulldirt();
+	}
+}
+
+void
+tscrolldown(int orig, int n, int copyhist)
+{
 	int i;
 	Line temp;
 
 	LIMIT(n, 0, term.bot-orig+1);
+	if (copyhist) {
+		term.histi = (term.histi - 1 + HISTSIZE) % HISTSIZE;
+		temp = term.hist[term.histi];
+		term.hist[term.histi] = term.line[term.bot];
+		term.line[term.bot] = temp;
+	}
 
+
 	tsetdirt(orig, term.bot-n);
 	tclearregion(0, term.bot-n+1, term.col-1, term.bot);
 
@@ -1071,17 +1121,28 @@
 		term.line[i-n] = temp;
 	}
 
-	selscroll(orig, n);
+	if (term.scr == 0)
+		selscroll(orig, n);
 }
 
 void
-tscrollup(int orig, int n)
+tscrollup(int orig, int n, int copyhist)
 {
 	int i;
 	Line temp;
 
 	LIMIT(n, 0, term.bot-orig+1);
 
+	if (copyhist) {
+		term.histi = (term.histi + 1) % HISTSIZE;
+		temp = term.hist[term.histi];
+		term.hist[term.histi] = term.line[orig];
+		term.line[orig] = temp;
+	}
+
+	if (term.scr > 0 && term.scr < HISTSIZE)
+		term.scr = MIN(term.scr + n, HISTSIZE-1);
+
 	tclearregion(0, orig, term.col-1, orig+n-1);
 	tsetdirt(orig+n, term.bot);
 
@@ -1091,7 +1152,8 @@
 		term.line[i+n] = temp;
 	}
 
-	selscroll(orig, -n);
+	if (term.scr == 0)
+		selscroll(orig, -n);
 }
 
 void
@@ -1120,7 +1182,7 @@
 	int y = term.c.y;
 
 	if (y == term.bot) {
-		tscrollup(term.top, 1);
+		tscrollup(term.top, 1, 1);
 	} else {
 		y++;
 	}
@@ -1285,14 +1347,14 @@
 tinsertblankline(int n)
 {
 	if (BETWEEN(term.c.y, term.top, term.bot))
-		tscrolldown(term.c.y, n);
+		tscrolldown(term.c.y, n, 0);
 }
 
 void
 tdeleteline(int n)
 {
 	if (BETWEEN(term.c.y, term.top, term.bot))
-		tscrollup(term.c.y, n);
+		tscrollup(term.c.y, n, 0);
 }
 
 int32_t
@@ -1729,11 +1791,11 @@
 		break;
 	case 'S': /* SU -- Scroll <n> line up */
 		DEFAULT(csiescseq.arg[0], 1);
-		tscrollup(term.top, csiescseq.arg[0]);
+		tscrollup(term.top, csiescseq.arg[0], 0);
 		break;
 	case 'T': /* SD -- Scroll <n> line down */
 		DEFAULT(csiescseq.arg[0], 1);
-		tscrolldown(term.top, csiescseq.arg[0]);
+		tscrolldown(term.top, csiescseq.arg[0], 0);
 		break;
 	case 'L': /* IL -- Insert <n> blank lines */
 		DEFAULT(csiescseq.arg[0], 1);
@@ -2296,7 +2358,7 @@
 		return 0;
 	case 'D': /* IND -- Linefeed */
 		if (term.c.y == term.bot) {
-			tscrollup(term.top, 1);
+			tscrollup(term.top, 1, 1);
 		} else {
 			tmoveto(term.c.x, term.c.y+1);
 		}
@@ -2309,7 +2371,7 @@
 		break;
 	case 'M': /* RI -- Reverse index */
 		if (term.c.y == term.top) {
-			tscrolldown(term.top, 1);
+			tscrolldown(term.top, 1, 1);
 		} else {
 			tmoveto(term.c.x, term.c.y-1);
 		}
@@ -2523,7 +2585,7 @@
 void
 tresize(int col, int row)
 {
-	int i;
+	int i, j;
 	int minrow = MIN(row, term.row);
 	int mincol = MIN(col, term.col);
 	int *bp;
@@ -2560,6 +2622,14 @@
 	term.dirty = xrealloc(term.dirty, row * sizeof(*term.dirty));
 	term.tabs = xrealloc(term.tabs, col * sizeof(*term.tabs));
 
+	for (i = 0; i < HISTSIZE; i++) {
+		term.hist[i] = xrealloc(term.hist[i], col * sizeof(Glyph));
+		for (j = mincol; j < col; j++) {
+			term.hist[i][j] = term.c.attr;
+			term.hist[i][j].u = ' ';
+		}
+	}
+
 	/* resize each row to new width, zero-pad if needed */
 	for (i = 0; i < minrow; i++) {
 		term.line[i] = xrealloc(term.line[i], col * sizeof(Glyph));
@@ -2618,7 +2688,7 @@
 			continue;
 
 		term.dirty[y] = 0;
-		xdrawline(term.line[y], x1, y, x2);
+		xdrawline(TLINE(y), x1, y, x2);
 	}
 }
 
@@ -2639,8 +2709,9 @@
 		cx--;
 
 	drawregion(0, 0, term.col, term.row);
-	xdrawcursor(cx, term.c.y, term.line[term.c.y][cx],
-			term.ocx, term.ocy, term.line[term.ocy][term.ocx]);
+	if (term.scr == 0)
+		xdrawcursor(cx, term.c.y, term.line[term.c.y][cx],
+				term.ocx, term.ocy, term.line[term.ocy][term.ocx]);
 	term.ocx = cx;
 	term.ocy = term.c.y;
 	xfinishdraw();
