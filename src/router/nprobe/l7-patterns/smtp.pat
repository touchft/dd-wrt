# SMTP - Simple Mail Transfer Protocol - RFC 2821 (See also RFC 1869)
# Pattern quality: great fast
# usually runs on port 25
# 
# This pattern has been tested and is believed to work well.  If it does not
# work for you, or you believe it could be improved, please post to 
# l7-filter-developers@lists.sf.net .  This list may be subscribed to at
# http://lists.sourceforge.net/lists/listinfo/l7-filter-developers

smtp
# As usual, no text is required after "220", but all known servers have some
# there.  It (almost?) always has string "smtp" in it.  The RFC examples
# does not, so we match those too, just in case anyone has copied them 
# literally.
MAIL FROM|RCPT TO
#^220.* (e?smtp|simple mail)

# Some examples:
# 220 mail.stalker.com ESMTP CommuniGate Pro 4.1.3
# 220 mail.vieodata.com ESMTP Merak 6.1.0; Mon, 15 Sep 2003 13:48:11 -0400
# 220 mail.ut.caldera.com ESMTP
# 220 persephone.pmail.gen.nz ESMTP server ready.
# 220 smtp1.superb.net ESMTP
# 220 mail.kerio.com Kerio MailServer 5.6.7 ESMTP ready
# 220-mail.deerfield.com ESMTP VisNetic.MailServer.v6.0.9.0; Mon, 15 Sep 2003 13:4
# 220 altn.com ESMTP MDaemon 6.8.5; Mon, 15 Sep 2003 12:46:42 -0500
# 220 X1 NT-ESMTP Server ipsmin0165atl2.interland.net (IMail 6.06 73062-3)
# 220 mail.icewarp.com ESMTP Merak 6.1.1; Mon, 15 Sep 2003 19:43:23 +0200
# 220-mail.email-scan.com ESMTP
# 220 smaug.dreamhost.com ESMTP
# 220 kona.carleton.edu -- Server ESMTP (PMDF V6.2#30648)
# 220 letra.reed.edu ESMTP Sendmail 8.12.9/8.12.9; Mon, 15 Sep 2003 10:35:57 -0700 (PDT)
# 220-swan.mail.pas.earthlink.net ESMTP Exim 3.33 #1 Mon, 15 Sep 2003 10:32:15 -0700
# 
# RFC examples:
# 220 xyz.com Simple Mail Transfer Service Ready (RFC example)
# 220 dbc.mtview.ca.us SMTP service ready
