# © 2016 and later: Unicode, Inc. and others.
# License & terms of use: http://www.unicode.org/copyright.html#License
LANG_CLDR_VERSION = 33
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
LANG_SYNTHETIC_ALIAS = ars.txt az_AZ.txt az_Latn_AZ.txt bs_BA.txt\
 bs_Latn_BA.txt en_NH.txt en_RH.txt fil_PH.txt he_IL.txt\
 id_ID.txt in.txt in_ID.txt iw.txt iw_IL.txt\
 ja_JP.txt ja_JP_TRADITIONAL.txt mo.txt nb_NO.txt nn_NO.txt\
 no.txt no_NO.txt no_NO_NY.txt pa_Arab_PK.txt pa_Guru_IN.txt\
 pa_IN.txt pa_PK.txt sh.txt sh_BA.txt sh_CS.txt\
 sh_YU.txt shi_MA.txt shi_Tfng_MA.txt sr_BA.txt sr_CS.txt\
 sr_Cyrl_CS.txt sr_Cyrl_RS.txt sr_Cyrl_YU.txt sr_Latn_CS.txt sr_Latn_RS.txt\
 sr_Latn_YU.txt sr_ME.txt sr_RS.txt sr_XK.txt sr_YU.txt\
 th_TH.txt th_TH_TRADITIONAL.txt tl.txt tl_PH.txt uz_AF.txt\
 uz_Arab_AF.txt uz_Latn_UZ.txt uz_UZ.txt vai_LR.txt vai_Vaii_LR.txt\
 yue_CN.txt yue_HK.txt yue_Hans_CN.txt yue_Hant_HK.txt zh_CN.txt\
 zh_HK.txt zh_Hans_CN.txt zh_Hans_SG.txt zh_Hant_TW.txt zh_MO.txt\
 zh_SG.txt zh_TW.txt


# All aliases (to not be included under 'installed'), but not including root.
LANG_ALIAS_SOURCE = $(LANG_SYNTHETIC_ALIAS)


# Ordinary resources
LANG_SOURCE = af.txt agq.txt ak.txt am.txt\
 ar.txt ar_EG.txt ar_LY.txt ar_SA.txt as.txt\
 asa.txt ast.txt az.txt az_Cyrl.txt az_Latn.txt\
 bas.txt be.txt bem.txt bez.txt bg.txt\
 bm.txt bn.txt bn_IN.txt bo.txt br.txt\
 brx.txt bs.txt bs_Cyrl.txt bs_Latn.txt ca.txt\
 ccp.txt ce.txt cgg.txt chr.txt ckb.txt\
 cs.txt cy.txt da.txt dav.txt de.txt\
 de_AT.txt de_CH.txt de_LU.txt dje.txt dsb.txt\
 dua.txt dyo.txt dz.txt ebu.txt ee.txt\
 el.txt en.txt en_001.txt en_150.txt en_AG.txt\
 en_AI.txt en_AT.txt en_AU.txt en_BB.txt en_BE.txt\
 en_BM.txt en_BS.txt en_BW.txt en_BZ.txt en_CA.txt\
 en_CC.txt en_CH.txt en_CK.txt en_CM.txt en_CX.txt\
 en_CY.txt en_DE.txt en_DG.txt en_DK.txt en_DM.txt\
 en_ER.txt en_FI.txt en_FJ.txt en_FK.txt en_FM.txt\
 en_GB.txt en_GD.txt en_GG.txt en_GH.txt en_GI.txt\
 en_GM.txt en_GY.txt en_HK.txt en_IE.txt en_IL.txt\
 en_IM.txt en_IN.txt en_IO.txt en_JE.txt en_JM.txt\
 en_KE.txt en_KI.txt en_KN.txt en_KY.txt en_LC.txt\
 en_LR.txt en_LS.txt en_MG.txt en_MO.txt en_MS.txt\
 en_MT.txt en_MU.txt en_MW.txt en_MY.txt en_NA.txt\
 en_NF.txt en_NG.txt en_NL.txt en_NR.txt en_NU.txt\
 en_NZ.txt en_PG.txt en_PH.txt en_PK.txt en_PN.txt\
 en_PW.txt en_RW.txt en_SB.txt en_SC.txt en_SD.txt\
 en_SE.txt en_SG.txt en_SH.txt en_SI.txt en_SL.txt\
 en_SS.txt en_SX.txt en_SZ.txt en_TC.txt en_TK.txt\
 en_TO.txt en_TT.txt en_TV.txt en_TZ.txt en_UG.txt\
 en_VC.txt en_VG.txt en_VU.txt en_WS.txt en_ZA.txt\
 en_ZM.txt en_ZW.txt eo.txt es.txt es_419.txt\
 es_AR.txt es_BO.txt es_BR.txt es_BZ.txt es_CL.txt\
 es_CO.txt es_CR.txt es_CU.txt es_DO.txt es_EC.txt\
 es_GT.txt es_HN.txt es_MX.txt es_NI.txt es_PA.txt\
 es_PE.txt es_PR.txt es_PY.txt es_SV.txt es_US.txt\
 es_UY.txt es_VE.txt et.txt eu.txt ewo.txt\
 fa.txt fa_AF.txt ff.txt fi.txt fil.txt\
 fo.txt fr.txt fr_BE.txt fr_CA.txt fr_CH.txt\
 fur.txt fy.txt ga.txt gd.txt gl.txt\
 gsw.txt gu.txt guz.txt gv.txt ha.txt\
 haw.txt he.txt hi.txt hr.txt hsb.txt\
 hu.txt hy.txt id.txt ig.txt ii.txt\
 is.txt it.txt ja.txt jgo.txt jmc.txt\
 ka.txt kab.txt kam.txt kde.txt kea.txt\
 khq.txt ki.txt kk.txt kkj.txt kl.txt\
 kln.txt km.txt kn.txt ko.txt kok.txt\
 ks.txt ksb.txt ksf.txt ksh.txt kw.txt\
 ky.txt lag.txt lb.txt lg.txt lkt.txt\
 ln.txt lo.txt lrc.txt lt.txt lu.txt\
 luo.txt luy.txt lv.txt mas.txt mer.txt\
 mfe.txt mg.txt mgh.txt mgo.txt mk.txt\
 ml.txt mn.txt mr.txt ms.txt mt.txt\
 mua.txt my.txt mzn.txt naq.txt nb.txt\
 nd.txt nds.txt ne.txt nl.txt nmg.txt\
 nn.txt nnh.txt nus.txt nyn.txt om.txt\
 or.txt os.txt pa.txt pa_Arab.txt pa_Guru.txt\
 pl.txt ps.txt pt.txt pt_AO.txt pt_CH.txt\
 pt_CV.txt pt_GQ.txt pt_GW.txt pt_LU.txt pt_MO.txt\
 pt_MZ.txt pt_PT.txt pt_ST.txt pt_TL.txt qu.txt\
 rm.txt rn.txt ro.txt ro_MD.txt rof.txt\
 ru.txt rw.txt rwk.txt sah.txt saq.txt\
 sbp.txt se.txt se_FI.txt seh.txt ses.txt\
 sg.txt shi.txt shi_Latn.txt shi_Tfng.txt si.txt\
 sk.txt sl.txt smn.txt sn.txt so.txt\
 sq.txt sr.txt sr_Cyrl.txt sr_Cyrl_BA.txt sr_Cyrl_ME.txt\
 sr_Cyrl_XK.txt sr_Latn.txt sr_Latn_BA.txt sr_Latn_ME.txt sr_Latn_XK.txt\
 sv.txt sv_FI.txt sw.txt sw_CD.txt sw_KE.txt\
 ta.txt te.txt teo.txt tg.txt th.txt\
 ti.txt to.txt tr.txt tt.txt twq.txt\
 tzm.txt ug.txt uk.txt ur.txt ur_IN.txt\
 uz.txt uz_Arab.txt uz_Cyrl.txt uz_Latn.txt vai.txt\
 vai_Latn.txt vai_Vaii.txt vi.txt vun.txt wae.txt\
 wo.txt xog.txt yav.txt yi.txt yo.txt\
 yo_BJ.txt yue.txt yue_Hans.txt yue_Hant.txt zgh.txt\
 zh.txt zh_Hans.txt zh_Hant.txt zh_Hant_HK.txt zh_Hant_MO.txt\
 zu.txt

