package com.ibm.text.UCD;
import com.ibm.text.utility.*;
import java.util.Date;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.TimeZone;


public final class Default implements UCD_Types {
    
    private static String ucdVersion = UCD.latestVersion;
    private static UCD ucd;
    private static Normalizer nfc;
    private static Normalizer nfd;
    private static Normalizer nfkc;
    private static Normalizer nfkd;
    private static Normalizer[] nf = new Normalizer[4];
    
    public static void setUCD(String version) {
        ucdVersion = version;
    	setUCD();
    }
    
    private static void setUCD() {
        ucd = UCD.make(ucdVersion());
        nfd = nf[NFD] = new Normalizer(Normalizer.NFD, ucdVersion());
        nfc = nf[NFC] = new Normalizer(Normalizer.NFC, ucdVersion());
        nfkd = nf[NFKD] = new Normalizer(Normalizer.NFKD, ucdVersion());
        nfkc = nf[NFKC] = new Normalizer(Normalizer.NFKC, ucdVersion());
        System.out.println("Loaded UCD" + ucd().getVersion() + " " + (new Date(ucd().getDate())));
    }
    
    static DateFormat myDateFormat = new SimpleDateFormat("yyyy-MM-dd', 'HH:mm:ss' GMT'");
    static {
        myDateFormat.setTimeZone(TimeZone.getTimeZone("GMT"));
    }
    
    public static String getDate() {
        return myDateFormat.format(new Date());
    }

    public static String ucdVersion() {
        if (ucd() == null) setUCD();
        return ucdVersion;
    }

    public static UCD ucd() {
        if (ucd() == null) setUCD();
        return ucd;
    }
    public static Normalizer nfc() {
        if (ucd() == null) setUCD();
        return nfc;
    }
    public static Normalizer nfd() {
        if (ucd() == null) setUCD();
        return nfd;
    }
    public static Normalizer nfkc() {
        if (ucd() == null) setUCD();
        return nfkc;
    }
    public static Normalizer nfkd() {
        if (ucd() == null) setUCD();
        return nfkd;
    }
    public static Normalizer nf(int index) {
        if (ucd() == null) setUCD();
        return nf[index];
    }

}