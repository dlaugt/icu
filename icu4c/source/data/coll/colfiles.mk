# *   Copyright (C) 1997-2003, International Business Machines
# *   Corporation and others.  All Rights Reserved.
# A list of txt's to build
# Note: 
#
#   If you are thinking of modifying this file, READ THIS. 
#
# Instead of changing this file [unless you want to check it back in],
# you should consider creating a 'reslocal.mk' file in this same directory.
# Then, you can have your local changes remain even if you upgrade or re-
# configure ICU.
#
# Example 'reslocal.mk' files:
#
#  * To add an additional locale to the list: 
#    _____________________________________________________
#    |  GENRB_SOURCE_LOCAL =   myLocale.txt ...
#
#  * To REPLACE the default list and only build with a few
#     locale:
#    _____________________________________________________
#    |  GENRB_SOURCE = ar.txt ar_AE.txt en.txt de.txt zh.txt
#
#


# This is the list of locales that are built, but not considered installed in ICU.
# These are usually aliased locales or the root locale.
COLLATION_ALIAS_SOURCE = de__PHONEBOOK.txt  es__TRADITIONAL.txt zh_TW_STROKE.txt zh__PINYIN.txt hi__DIRECT.txt

# Please try to keep this list in alphabetical order
COLLATION_SOURCE =  ar.txt be.txt bg.txt ca.txt cs.txt da.txt de.txt  el.txt en.txt en_BE.txt eo.txt es.txt et.txt fa.txt fa_AF.txt fi.txt fo.txt fr.txt gu.txt he.txt hi.txt  hr.txt hu.txt is.txt it.txt ja.txt kk.txt kl.txt kn.txt ko.txt lt.txt lv.txt mk.txt mr.txt mt.txt nb.txt nn.txt pa.txt pl.txt ps.txt ro.txt ru.txt sh.txt sk.txt sl.txt sq.txt sr.txt sv.txt ta.txt te.txt th.txt tr.txt uk.txt vi.txt zh.txt zh_HK.txt zh_MO.txt zh_TW.txt 
