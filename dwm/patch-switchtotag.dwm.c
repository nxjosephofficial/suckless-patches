--- dwm.c.orig	2022-10-04 20:38:18.000000000 +0300
+++ dwm.c	2023-12-11 15:10:37.878960000 +0300
@@ -92,6 +92,7 @@
 	int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
 	int bw, oldbw;
 	unsigned int tags;
+	unsigned int switchtotag;
 	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
 	Client *next;
 	Client *snext;
@@ -137,6 +138,7 @@
 	const char *instance;
 	const char *title;
 	unsigned int tags;
+	unsigned int switchtotag;
 	int isfloating;
 	int monitor;
 } Rule;
@@ -303,6 +305,11 @@
 			for (m = mons; m && m->num != r->monitor; m = m->next);
 			if (m)
 				c->mon = m;
+			if (r->switchtotag) {
+				Arg a = { .ui = r->tags };
+				c->switchtotag = selmon->tagset[selmon->seltags];
+				view(&a);
+			}
 		}
 	}
 	if (ch.res_class)
@@ -1781,6 +1788,10 @@
 	focus(NULL);
 	updateclientlist();
 	arrange(m);
+	if (c->switchtotag) {
+		Arg a = { .ui = c->switchtotag };
+		view(&a);
+	}
 }
 
 void
