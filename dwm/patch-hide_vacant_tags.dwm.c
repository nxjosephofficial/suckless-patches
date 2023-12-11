--- dwm.c.orig	2022-10-04 20:38:18.000000000 +0300
+++ dwm.c	2023-12-10 21:33:39.562336000 +0300
@@ -434,9 +434,15 @@
 	}
 	if (ev->window == selmon->barwin) {
 		i = x = 0;
-		do
+		unsigned int occ = 0;
+		for(c = m->clients; c; c=c->next)
+			occ |= c->tags;
+		do {
+			/* Do not reserve space for vacant tags */
+			if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
+				continue;
 			x += TEXTW(tags[i]);
-		while (ev->x >= x && ++i < LENGTH(tags));
+		} while (ev->x >= x && ++i < LENGTH(tags));
 		if (i < LENGTH(tags)) {
 			click = ClkTagBar;
 			arg.ui = 1 << i;
@@ -722,13 +728,12 @@
 	}
 	x = 0;
 	for (i = 0; i < LENGTH(tags); i++) {
+		/* Do not draw vacant tags */
+		if(!(occ & 1 << i || m->tagset[m->seltags] & 1 << i))
+			continue;
 		w = TEXTW(tags[i]);
 		drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel : SchemeNorm]);
 		drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
-		if (occ & 1 << i)
-			drw_rect(drw, x + boxs, boxs, boxw, boxw,
-				m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
-				urg & 1 << i);
 		x += w;
 	}
 	w = TEXTW(m->ltsymbol);
