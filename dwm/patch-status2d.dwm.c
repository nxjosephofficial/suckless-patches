--- dwm.c.orig	2023-12-10 23:07:55.035849000 +0300
+++ dwm.c	2023-12-10 23:16:09.986387000 +0300
@@ -163,6 +163,7 @@
 static Monitor *dirtomon(int dir);
 static void drawbar(Monitor *m);
 static void drawbars(void);
+static int drawstatusbar(Monitor *m, int bh, char* text);
 static void enternotify(XEvent *e);
 static void expose(XEvent *e);
 static void focus(Client *c);
@@ -237,7 +238,7 @@
 
 /* variables */
 static const char broken[] = "broken";
-static char stext[256];
+static char stext[1024];
 static int screen;
 static int sw, sh;           /* X display screen geometry width, height */
 static int bh;               /* bar height */
@@ -487,7 +488,7 @@
 		cleanupmon(mons);
 	for (i = 0; i < CurLast; i++)
 		drw_cur_free(drw, cursor[i]);
-	for (i = 0; i < LENGTH(colors); i++)
+	for (i = 0; i < LENGTH(colors) + 1; i++)	
 		free(scheme[i]);
 	free(scheme);
 	XDestroyWindow(dpy, wmcheckwin);
@@ -710,9 +711,7 @@
 
 	/* draw status first so it can be overdrawn by tags later */
 	if (m == selmon) { /* status is only drawn on selected monitor */
-		drw_setscheme(drw, scheme[SchemeNorm]);
-		tw = TEXTW(stext) - lrpad + 2; /* 2px right padding */
-		drw_text(drw, m->ww - tw, 0, tw, bh, 0, stext, 0);
+		tw = m->ww - drawstatusbar(m, bh, stext);
 	}
 
 	for (c = m->clients; c; c = c->next) {
@@ -1568,7 +1567,8 @@
 	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
 	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
 	/* init appearance */
-	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
+	scheme = ecalloc(LENGTH(colors) + 1, sizeof(Clr *));
+	scheme[LENGTH(colors)] = drw_scm_create(drw, colors[0], 3);
 	for (i = 0; i < LENGTH(colors); i++)
 		scheme[i] = drw_scm_create(drw, colors[i], 3);
 	/* init bars */
@@ -2145,3 +2145,112 @@
 	XCloseDisplay(dpy);
 	return EXIT_SUCCESS;
 }
+
+int
+drawstatusbar(Monitor *m, int bh, char* stext) {
+	int ret, i, w, x, len;
+	short isCode = 0;
+	char *text;
+	char *p;
+
+	len = strlen(stext) + 1 ;
+	if (!(text = (char*) malloc(sizeof(char)*len)))
+		die("malloc");
+	p = text;
+	memcpy(text, stext, len);
+
+	/* compute width of the status text */
+	w = 0;
+	i = -1;
+	while (text[++i]) {
+		if (text[i] == '^') {
+			if (!isCode) {
+				isCode = 1;
+				text[i] = '\0';
+				w += TEXTW(text) - lrpad;
+				text[i] = '^';
+				if (text[++i] == 'f')
+					w += atoi(text + ++i);
+			} else {
+				isCode = 0;
+				text = text + i + 1;
+				i = -1;
+			}
+		}
+	}
+	if (!isCode)
+		w += TEXTW(text) - lrpad;
+	else
+		isCode = 0;
+	text = p;
+
+	w += 2; /* 1px padding on both sides */
+	ret = x = m->ww - w;
+
+	drw_setscheme(drw, scheme[LENGTH(colors)]);
+	drw->scheme[ColFg] = scheme[SchemeNorm][ColFg];
+	drw->scheme[ColBg] = scheme[SchemeNorm][ColBg];
+	drw_rect(drw, x, 0, w, bh, 1, 1);
+	x++;
+
+	/* process status text */
+	i = -1;
+	while (text[++i]) {
+		if (text[i] == '^' && !isCode) {
+			isCode = 1;
+
+			text[i] = '\0';
+			w = TEXTW(text) - lrpad;
+			drw_text(drw, x, 0, w, bh, 0, text, 0);
+
+			x += w;
+
+			/* process code */
+			while (text[++i] != '^') {
+				if (text[i] == 'c') {
+					char buf[8];
+					memcpy(buf, (char*)text+i+1, 7);
+					buf[7] = '\0';
+					drw_clr_create(drw, &drw->scheme[ColFg], buf);
+					i += 7;
+				} else if (text[i] == 'b') {
+					char buf[8];
+					memcpy(buf, (char*)text+i+1, 7);
+					buf[7] = '\0';
+					drw_clr_create(drw, &drw->scheme[ColBg], buf);
+					i += 7;
+				} else if (text[i] == 'd') {
+					drw->scheme[ColFg] = scheme[SchemeNorm][ColFg];
+					drw->scheme[ColBg] = scheme[SchemeNorm][ColBg];
+				} else if (text[i] == 'r') {
+					int rx = atoi(text + ++i);
+					while (text[++i] != ',');
+					int ry = atoi(text + ++i);
+					while (text[++i] != ',');
+					int rw = atoi(text + ++i);
+					while (text[++i] != ',');
+					int rh = atoi(text + ++i);
+
+					drw_rect(drw, rx + x, ry, rw, rh, 1, 0);
+				} else if (text[i] == 'f') {
+					x += atoi(text + ++i);
+				}
+			}
+
+			text = text + i + 1;
+			i=-1;
+			isCode = 0;
+		}
+	}
+
+	if (!isCode) {
+		w = TEXTW(text) - lrpad;
+		drw_text(drw, x, 0, w, bh, 0, text, 0);
+	}
+
+	drw_setscheme(drw, scheme[SchemeNorm]);
+	free(p);
+
+	return ret;
+}
+
