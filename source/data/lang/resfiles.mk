# *   Copyright (C) 1998-2009, International Business Machines
# *   Corporation and others.  All Rights Reserved.
LANG_CLDR_VERSION = 1.7
# A list of txt's to build
# Note:
#
#   If you are thinking of modifying this file, READ THIS.
#
# Instead of changing this file [unless you want to check it back in],
# you should consider creating a 'reslocal.mk' file in this same directory.
# Then, you can have your local changes remain even if you upgrade or
# reconfigure ICU.
#
# Example 'reslocal.mk' files:
#
#  * To add an additional locale to the list:
#    _____________________________________________________
#    |  LANG_SOURCE_LOCAL =   myLocale.txt ...
#
#  * To REPLACE the default list and only build with a few
#    locales:
#    _____________________________________________________
#    |  LANG_SOURCE = ar.txt ar_AE.txt en.txt de.txt zh.txt
#
#
# Generated by LDML2ICUConverter, from LDML source files.

# Aliases without a corresponding xx.xml file (see icu-config.xml & build.xml)
LANG_SYNTHETIC_ALIAS = en_RH.txt en_ZW.txt he_IL.txt id_ID.txt\
 in_ID.txt iw_IL.txt ja_JP.txt ja_JP_TRADITIONAL.txt nb_NO.txt\
 nn_NO.txt no_NO.txt no_NO_NY.txt th_TH.txt th_TH_TRADITIONAL.txt


# All aliases (to not be included under 'installed'), but not including root.
LANG_ALIAS_SOURCE = $(LANG_SYNTHETIC_ALIAS) az_AZ.txt ha_GH.txt ha_NE.txt ha_NG.txt\
 in.txt iw.txt kk_KZ.txt no.txt pa_IN.txt\
 pa_PK.txt sh.txt sh_BA.txt sh_CS.txt sh_YU.txt\
 sr_BA.txt sr_CS.txt sr_Cyrl_CS.txt sr_Cyrl_YU.txt sr_Latn_CS.txt\
 sr_Latn_YU.txt sr_ME.txt sr_RS.txt sr_YU.txt uz_AF.txt\
 uz_UZ.txt zh_CN.txt zh_HK.txt zh_MO.txt zh_SG.txt\
 zh_TW.txt


# Ordinary resources
LANG_SOURCE = af.txt am.txt ar.txt ar_AE.txt\
 ar_BH.txt ar_DZ.txt ar_IQ.txt ar_JO.txt ar_KW.txt\
 ar_LB.txt ar_LY.txt ar_MA.txt ar_OM.txt ar_QA.txt\
 ar_SA.txt ar_SD.txt ar_SY.txt ar_TN.txt ar_YE.txt\
 as.txt az.txt az_Cyrl.txt az_Latn.txt az_Latn_AZ.txt\
 be.txt bg.txt bn.txt bn_IN.txt bo.txt\
 ca.txt cs.txt cy.txt da.txt de.txt\
 de_CH.txt el.txt en.txt eo.txt es.txt\
 es_AR.txt es_CL.txt et.txt eu.txt fa.txt\
 fa_AF.txt fi.txt fo.txt fr.txt ga.txt\
 gl.txt gsw.txt gu.txt gv.txt ha.txt\
 ha_Latn.txt ha_Latn_GH.txt ha_Latn_NE.txt ha_Latn_NG.txt haw.txt\
 he.txt hi.txt hr.txt hu.txt hy.txt\
 id.txt ii.txt is.txt it.txt ja.txt\
 ka.txt kk.txt kk_Cyrl.txt kk_Cyrl_KZ.txt kl.txt\
 km.txt kn.txt ko.txt kok.txt kw.txt\
 lt.txt lv.txt mk.txt ml.txt mr.txt\
 ms.txt mt.txt nb.txt ne.txt nl.txt\
 nl_BE.txt nn.txt om.txt or.txt pa.txt\
 pa_Arab.txt pa_Arab_PK.txt pa_Guru.txt pa_Guru_IN.txt pl.txt\
 ps.txt pt.txt pt_PT.txt ro.txt ru.txt\
 ru_UA.txt si.txt sk.txt sl.txt so.txt\
 sq.txt sr.txt sr_Cyrl.txt sr_Cyrl_BA.txt sr_Cyrl_RS.txt\
 sr_Latn.txt sr_Latn_BA.txt sr_Latn_ME.txt sr_Latn_RS.txt sv.txt\
 sv_FI.txt sw.txt ta.txt te.txt th.txt\
 ti.txt tr.txt uk.txt ur.txt uz.txt\
 uz_Arab.txt uz_Arab_AF.txt uz_Cyrl.txt uz_Cyrl_UZ.txt uz_Latn.txt\
 vi.txt zh.txt zh_Hans.txt zh_Hans_CN.txt zh_Hans_SG.txt\
 zh_Hant.txt zh_Hant_HK.txt zh_Hant_MO.txt zh_Hant_TW.txt zu.txt

