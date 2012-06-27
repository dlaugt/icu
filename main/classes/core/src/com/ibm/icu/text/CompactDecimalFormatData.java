/*
 *******************************************************************************
 * Copyright (C) 1996-2012, Google, International Business Machines Corporation and
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */
package com.ibm.icu.text;

/**
 * Only until we access the data from CLDR.
 */
class CompactDecimalFormatData {
	static void load() {
        CompactDecimalFormat.add("af", 
                new long[]{1L, 1L, 1L, 1L, 1L, 1L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "", "", " mil", " mil", " mil", " mjd", " mjd", " mjd", " bil", " bil", " bil"});
            CompactDecimalFormat.add("am", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "ኪባ", " ሜትር", " ሜትር", " ሜትር", "G", "G", "G", "T", "T", "T"});
            CompactDecimalFormat.add("ar", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("bg", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " Х", " Х", " Х", " М", " М", " М", " Б", " Б", " Б", " Т", " Т", " Т"});
            CompactDecimalFormat.add("bn", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("ca", 
                new long[]{1L, 1L, 1L, 1L, 1L, 1L, 1000000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "", "", " M", " M", " M", " M", "k M", "k M", " B", " B", " B"});
            CompactDecimalFormat.add("cs", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " tis.", " tis.", " tis.", " mil.", " mil.", " mil.", " mld.", " mld.", " mld.", " bil.", " bil.", " bil."});
            CompactDecimalFormat.add("da", 
                new long[]{1L, 1L, 1L, 1L, 1L, 1L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "", "", " mio", " mio", " mio", " mia", " mia", " mia", " bill", " bill", " bill"});
            CompactDecimalFormat.add("de", 
                new long[]{1L, 1L, 1L, 1L, 1L, 1L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "", "", " Mio", " Mio", " Mio", " Mrd", " Mrd", " Mrd", " B", " B", " B"});
            CompactDecimalFormat.add("el", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " χιλ.", " χιλ.", " χιλ.", " εκ.", " εκ.", " εκ.", " δις", " δις", " δις", " τρις", " τρις", " τρις"});
            CompactDecimalFormat.add("en", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("es", 
                new long[]{1L, 1L, 1L, 1L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "k", "k", " M", " M", " M", " M", "k M", "k M", " B", " B", " B"});
            CompactDecimalFormat.add("et", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " tuh", " tuh", " tuh", " mln", " mln", " mln", " mld", " mld", " mld", " trl", " trl", " trl"});
            CompactDecimalFormat.add("eu", 
                new long[]{1L, 1L, 1L, 1L, 1L, 1L, 1000000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "", "", " M", " M", " M", " M", "k M", "k M", " B", " B", " B"});
            CompactDecimalFormat.add("fa", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("fi", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " t", " t", " t", " ml", " ml", " ml", " mr", " mr", " mr", " b", " b", " b"});
            CompactDecimalFormat.add("fil", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("fr", 
                new long[]{1L, 1L, 1L, 1L, 1L, 1L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "", "", " M", " M", " M", " Md", " Mds", " Mds", " Bn", " Bn", " Bn"});
            CompactDecimalFormat.add("gl", 
                new long[]{1L, 1L, 1L, 1L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "k", "k", " M", " M", " M", " M", "k M", "k M", " B", " B", " B"});
            CompactDecimalFormat.add("gu", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("he", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("hi", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("hr", 
                new long[]{1L, 1L, 1L, 1L, 1L, 1L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "", "", " mil.", " mil.", " mil.", " mlr", " mlr", " mlr", " bil.", " bil.", " bil."});
            CompactDecimalFormat.add("hu", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " e", " e", " e", " m", " m", " m", " mrd", " mrd", " mrd", " b", " b", " b"});
            CompactDecimalFormat.add("id", 
                new long[]{1L, 1L, 1L, 1L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", " rb", " rb", " jt", " jt", " jt", " M", " M", " M", " T", " T", " T"});
            CompactDecimalFormat.add("is", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " þ.", " þ.", " þ.", " m.", " m.", " m.", " ma.", " ma.", " ma.", " tr.", " tr.", " tr."});
            CompactDecimalFormat.add("it", 
                new long[]{1L, 1L, 1L, 1L, 1L, 1L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "", "", " M", " M", " M", " Md", " Md", " Md", " B", " B", " B"});
            CompactDecimalFormat.add("ja", 
                new long[]{1L, 1L, 1L, 1L, 10000L, 10000L, 10000L, 10000L, 100000000L, 100000000L, 100000000L, 100000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "万", "万", "万", "万", "億", "億", "億", "億", "兆", "兆", "兆"});
            CompactDecimalFormat.add("kn", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("ko", 
                new long[]{1L, 1L, 1L, 1L, 10000L, 10000L, 10000L, 10000L, 100000000L, 100000000L, 100000000L, 100000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "만", "만", "만", "만", "억", "억", "억", "억", "조", "조", "조"});
            CompactDecimalFormat.add("lt", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " K", " K", " K", " M", " M", " M", " G", " G", " G", " T", " T", " T"});
            CompactDecimalFormat.add("lv", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " K", " K", " K", " M", " M", " M", " G", " G", " G", " T", " T", " T"});
            CompactDecimalFormat.add("ml", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("mr", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("ms", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("nb", 
                new long[]{1L, 1L, 1L, 1L, 1L, 1L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "", "", " mill", " mill", " mill", " mrd", " mrd", " mrd", " bill", " bill", " bill"});
            CompactDecimalFormat.add("nl", 
                new long[]{1L, 1L, 1L, 1L, 1L, 1L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "", "", " mln", " mln", " mln", " mld", " mld", " mld", " bilj", " bilj", " bilj"});
            CompactDecimalFormat.add("pl", 
                new long[]{1L, 1L, 1L, 1L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", " tys.", " tys.", " mln", " mln", " mln", " mld", " mld", " mld", " bln", " bln", " bln"});
            CompactDecimalFormat.add("pt", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " mil", " mil", " mil", " mi", " mi", " mi", " bi", " bi", " bi", " tri", " tri", " tri"});
            CompactDecimalFormat.add("ro", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " mii", " mii", " mii", " mil.", " mil.", " mil.", " mld.", " mld.", " mld.", " bln.", " bln.", " bln."});
            CompactDecimalFormat.add("ru", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " тыс.", " тыс.", " тыс.", " млн", " млн", " млн", " млрд", " млрд", " млрд", " трлн", " трлн", " трлн"});
            CompactDecimalFormat.add("sk", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " tis.", " tis.", " tis.", " mil.", " mil.", " mil.", " mld.", " mld.", " mld.", " bil.", " bil.", " bil."});
            CompactDecimalFormat.add("sl", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " tis.", " tis.", " tis.", " mio.", " mio.", " mio.", " mrd.", " mrd.", " mrd.", " bil.", " bil.", " bil."});
            CompactDecimalFormat.add("sr", 
                new long[]{1L, 1L, 1L, 1L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", " хиљ", " хиљ", " мил", " мил", " мил", " млрд", " млрд", " млрд", " бил", " бил", " бил"});
            CompactDecimalFormat.add("sv", 
                new long[]{1L, 1L, 1L, 1L, 1L, 1L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "", "", " mjn", " mjn", " mjn", " mjd", " mjd", " mjd", " trl", " trl", " trl"});
            CompactDecimalFormat.add("sw", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 100000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "elfu ", "elfu ", "laki", "M", "M", "M", "B", "B", "B", "T", "T", "T"},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""});
            CompactDecimalFormat.add("ta", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("te", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("th", 
                new long[]{1L, 1L, 1L, 1000L, 10000L, 100000L, 1000000L, 1000000L, 1000000L, 1000000000L, 10000000000L, 100000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " พัน", "หมื่น", "แสน", " ล้าน", " ล้าน", " ล.", " พ.ล.", " ม.ล.", " ส.ล.", " ล.ล.", " ล.ล", " ล.ล."});
            CompactDecimalFormat.add("tr", 
                new long[]{1L, 1L, 1L, 1L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", " B", " B", " Mn", " Mn", " Mn", " Mr", " Mr", " Mr", " T", " T", " T"});
            CompactDecimalFormat.add("uk", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " тис", " тис", " тис", " млн", " млн", " млн", " млрд", " млрд", " млрд", " трлн", " трлн", " трлн"});
            CompactDecimalFormat.add("ur", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "K", "K", "K", "M", "M", "M", "B", "B", "B", "T", "T", "T"});
            CompactDecimalFormat.add("vi", 
                new long[]{1L, 1L, 1L, 1000L, 1000L, 1000L, 1000000L, 1000000L, 1000000L, 1000000000L, 1000000000L, 1000000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", " N", " N", " N", " Tr", " Tr", " Tr", " Tỷ", " Tỷ", " Tỷ", " NT", " NT", " NT"});
            CompactDecimalFormat.add("zh", 
                new long[]{1L, 1L, 1L, 1L, 10000L, 10000L, 10000L, 10000L, 100000000L, 100000000L, 100000000L, 100000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "万", "万", "万", "万", "亿", "亿", "亿", "亿", "兆", "兆", "兆"});
            CompactDecimalFormat.add("zh_Hant", 
                new long[]{1L, 1L, 1L, 1L, 10000L, 10000L, 10000L, 10000L, 100000000L, 100000000L, 100000000L, 100000000L, 1000000000000L, 1000000000000L, 1000000000000L},
                new String[]{"", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
                new String[]{"", "", "", "", "萬", "萬", "萬", "萬", "億", "億", "億", "億", "兆", "兆", "兆"});
}}
