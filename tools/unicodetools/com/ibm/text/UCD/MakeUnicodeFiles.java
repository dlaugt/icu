package com.ibm.text.UCD;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.Field;
import java.text.ParseException;
import java.text.ParsePosition;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.ResourceBundle;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.ibm.icu.dev.test.util.BagFormatter;
import com.ibm.icu.dev.test.util.Tabber;
import com.ibm.icu.dev.test.util.UnicodeLabel;
import com.ibm.icu.dev.test.util.UnicodeProperty;
import com.ibm.icu.text.NumberFormat;
import com.ibm.icu.text.SymbolTable;
import com.ibm.icu.text.UTF16;
import com.ibm.icu.text.UnicodeMatcher;
import com.ibm.icu.text.UnicodeSet;
import com.ibm.text.utility.UnicodeDataFile;
import com.ibm.text.utility.Utility;
import com.ibm.icu.text.Collator;

public class MakeUnicodeFiles {
    public static int dVersion = 8; // change to fix the generated file D version. If less than zero, no "d"
    
    /*static String[] hackNameList = {
       "noBreak", "Arabic_Presentation_Forms-A", "Arabic_Presentation_Forms-B", 
       "CJK_Symbols_and_Punctuation", "Combining_Diacritical_Marks_for_Symbols",
       "Enclosed_CJK_Letters_and_Months", "Greek_and_Coptic",
       "Halfwidth_and_Fullwidth_Forms", "Latin-1_Supplement", "Latin_Extended-A",
       "Latin_Extended-B", "Miscellaneous_Mathematical_Symbols-A",
       "Miscellaneous_Mathematical_Symbols-B", "Miscellaneous_Symbols_and_Arrows",
       "Superscripts_and_Subscripts", "Supplemental_Arrows-A", "Supplemental_Arrows-B",
       "Supplementary_Private_Use_Area-A", "Supplementary_Private_Use_Area-B",
       "Canadian-Aboriginal", "Old-Italic"
    };
    static {
        for (int i = 0; i < hackNameList.length; ++i) {
            System.out.println("HackName:\t" + hackNameList[i]);
        }
    }
    */

    static boolean DEBUG = false;
    
    public static void main(String[] args) throws IOException {
        //generateFile();
        testInvariants();
    }

    static class Format {
        public static Format theFormat = new Format(); // singleton

        
        Map printStyleMap = new TreeMap(UnicodeProperty.PROPERTY_COMPARATOR);
        static PrintStyle DEFAULT_PRINT_STYLE = new PrintStyle();        
        Map fileToPropertySet = new TreeMap();
        Map fileToComments = new TreeMap();
        Map fileToDirectory = new TreeMap();
        TreeMap propertyToValueToComments = new TreeMap();        
        Map hackMap = new HashMap();
        UnicodeProperty.MapFilter hackMapFilter;
        String[] filesToDo;
      
        private Format(){
            build();            
        }
        /*
        static String[] FILE_OPTIONS = {
           "Script            nameStyle=none makeUppercase skipUnassigned=Common hackValues",
           "Age               nameStyle=none noLabel skipValue=unassigned",
           "Numeric_Type      nameStyle=none makeFirstLetterLowercase skipValue=None",
           "General_Category  nameStyle=none valueStyle=short noLabel",
           "Line_Break        nameStyle=none valueStyle=short skipUnassigned=Unknown",
           "Joining_Type      nameStyle=none valueStyle=short skipValue=Non_Joining",
           "Joining_Group     nameStyle=none skipValue=No_Joining_Group makeUppercase",
           "East_Asian_Width      nameStyle=none valueStyle=short skipUnassigned=Neutral",
           "Decomposition_Type    nameStyle=none skipValue=None makeFirstLetterLowercase hackValues",
           "Bidi_Class        nameStyle=none valueStyle=short skipUnassigned=Left_To_Right",
           "Block             nameStyle=none noLabel valueList",
           "Canonical_Combining_Class     nameStyle=none valueStyle=short skipUnassigned=Not_Reordered longValueHeading=ccc",
           "Hangul_Syllable_Type  nameStyle=none valueStyle=short skipValue=Not_Applicable",
           "NFD_Quick_Check   nameStyle=short valueStyle=short skipValue=Yes",
           "NFC_Quick_Check   nameStyle=short valueStyle=short skipValue=Yes",
           "NFKC_Quick_Check  nameStyle=short valueStyle=short skipValue=Yes",
           "NFKD_Quick_Check  nameStyle=short valueStyle=short skipValue=Yes",        
           "FC_NFKC_Closure   nameStyle=short"
       };
     */

    void printFileComments(PrintWriter pw, String filename) {
        String fileComments = (String) fileToComments.get(filename);
        if (fileComments != null)
            pw.println(fileComments);
    }

      private void addPrintStyle(String options) {
          PrintStyle result = new PrintStyle();
          printStyleMap.put(result.parse(options), result);
      }
      
      public PrintStyle getPrintStyle(String propname) {
          PrintStyle result = (PrintStyle) printStyleMap.get(propname);
          if (result != null)
              return result;
          if (DEBUG)
              System.out.println("Using default style!");
          return DEFAULT_PRINT_STYLE;
      }

    public static class PrintStyle {
        boolean noLabel = false;
        boolean makeUppercase = false;
        boolean makeFirstLetterLowercase = false;
        boolean orderByRangeStart = false;
        boolean interleaveValues = false;
        boolean hackValues = false;
        String nameStyle = "none";
        String valueStyle = "long";
        String skipValue = null;
        String skipUnassigned = null;
        String longValueHeading = null;
        boolean sortNumeric = false;

        String parse(String options) {
            options = options.replace('\t', ' ');
            String[] pieces = Utility.split(options, ' ');
            for (int i = 1; i < pieces.length; ++i) {
                String piece = pieces[i];
                // binary
                if (piece.equals("noLabel"))
                    noLabel = true;
                else if (piece.equals("makeUppercase"))
                    makeUppercase = true;
                else if (piece.equals("makeFirstLetterLowercase"))
                    makeFirstLetterLowercase = true;
                else if (piece.equals("orderByRangeStart"))
                    orderByRangeStart = true;
                else if (piece.equals("valueList"))
                    interleaveValues = true;
                else if (piece.equals("hackValues"))
                    hackValues = true;
                else if (piece.equals("sortNumeric"))
                    sortNumeric = true;
                // with parameter
                else if (piece.startsWith("valueStyle="))
                    valueStyle = afterEquals(piece);
                else if (piece.startsWith("nameStyle="))
                    nameStyle = afterEquals(piece);
                else if (piece.startsWith("longValueHeading="))
                    longValueHeading = afterEquals(piece);
                else if (piece.startsWith("skipValue=")) {
                    if (skipUnassigned != null)
                        throw new IllegalArgumentException("Can't have both skipUnassigned and skipValue");
                    skipValue = afterEquals(piece);
                } else if (piece.startsWith("skipUnassigned=")) {
                    if (skipValue != null)
                        throw new IllegalArgumentException("Can't have both skipUnassigned and skipValue");
                    skipUnassigned = afterEquals(piece);
                } else if (piece.length() != 0) {
                    throw new IllegalArgumentException(
                        "Illegal PrintStyle Parameter: "
                            + piece
                            + " in "
                            + pieces[0]);
                }
            }
            return pieces[0];
        }
        public String toString() {
            Class myClass = getClass();
            String result = myClass.getName() + "\n";
            Field[] myFields = myClass.getDeclaredFields();
            for (int i = 0; i < myFields.length; ++i) {
                String value = "<private>";
                try {
                    Object obj = myFields[i].get(this);
                    if (obj == null) value = "<null>";
                    else value = obj.toString();
                } catch (Exception e) {}
                result += "\t" + myFields[i].getName() + "=<" + value + ">\n";
            }
            return result;
        }
    }
           /*
           static {
               for (int i = 0; i < FILE_OPTIONS.length; ++i) {
                   PrintStyle.add(FILE_OPTIONS[i]);
               }
           }
           */

        void addValueComments(String property, String value, String comments) {
            if (DEBUG)
                showPVC(property, value, comments);
            TreeMap valueToComments =
                (TreeMap) propertyToValueToComments.get(property);
            if (valueToComments == null) {
                valueToComments = new TreeMap();
                propertyToValueToComments.put(property, valueToComments);
            }
            valueToComments.put(value, comments);
            if (DEBUG && property.equals("BidiClass")) {
                getValueComments(property, value);
            }
        }

        private void showPVC(String property, String value, String comments) {
            System.out.println(
                "Putting Property: <"
                    + property
                    + ">, Value: <"
                    + value
                    + ">, Comments: <"
                    + comments + ">");
        }
        
        String getValueComments(String property, String value) {
            TreeMap valueToComments =
                (TreeMap) propertyToValueToComments.get(property);
            String result = null;
            if (valueToComments != null)
                result = (String) valueToComments.get(value);
            if (DEBUG) System.out.println("Getting Property: <" + property + ">, Value: <"
                + value + ">, Comment: <" + result + ">");
            return result;
        }
        
        Map getValue2CommentsMap(String property) {
            return (Map) propertyToValueToComments.get(property);
        }

        static String afterEquals(String source) {
            return source.substring(source.indexOf('=') + 1);
        }

        static String afterWhitespace(String source) {
            // Note: don't need to be international
            for (int i = 0; i < source.length(); ++i) {
                char ch = source.charAt(i);
                if (Character.isWhitespace(source.charAt(i))) {
                    return source.substring(i).trim();
                }
            }
            return "";
        }
         
        /*private void add(String name, String[] properties) {
             fileToPropertySet.put(name, properties);
        }*/      
         
        private void build() {
            /*
            for (int i = 0; i < hackNameList.length; ++i) {
                   String item = hackNameList[i];
                   String regularItem = UnicodeProperty.regularize(item,true);
                   hackMap.put(regularItem, item);
               }
               */

            /*
            for (int i = 0; i < UCD_Names.UNIFIED_PROPERTIES.length; ++i) {
                String name = Utility.getUnskeleton(UCD_Names.UNIFIED_PROPERTIES[i], false);
                valueComments.add(name, "*", "# " + UCD_Names.UNIFIED_PROPERTY_HEADERS[i]);
                System.out.println();
                System.out.println(name);
                System.out.println("# " + UCD_Names.UNIFIED_PROPERTY_HEADERS[i]);
            }
            // HACK
            valueComments.add("Bidi_Mirroring", "*", "# " + UCD_Names.UNIFIED_PROPERTY_HEADERS[9]);
            */
            try {
                BufferedReader br =
                    Utility.openReadFile("MakeUnicodeFiles.txt", Utility.UTF8);
                String key = null;
                String file = null, property = null, value = "", comments = "";
                while (true) {
                    String line = br.readLine();
                    if (line == null)
                        break;
                    line = line.trim();
                    if (line.length() == 0)
                        continue;
                    if (DEBUG)
                        System.out.println("\t" + line);
                    String lineValue = afterWhitespace(line);
                    if (line.startsWith("Format:")) {
                        addPrintStyle(property + " " + lineValue); // fix later
                    } else if (line.startsWith("#")) {
                        if (comments.length() != 0) comments += "\n";
                        comments += line;
                    } else {
                        // end of comments, roll up
                        if (property != null)
                            addValueComments(property, value, comments);
                        comments = "";
                        if (line.startsWith("Generate:")) {
                            filesToDo = Utility.split(lineValue, ' ');
                            if (filesToDo.length == 0) {
                                filesToDo = new String[] {""};
                            }
                        } else if (line.startsWith("DeltaVersion:")) {
                            dVersion = Integer.parseInt(lineValue);
                        } else if (line.startsWith("File:")) {
                            int p2 = lineValue.lastIndexOf('/');
                            file = lineValue.substring(p2+1);
                            if (p2 >= 0) {
                                fileToDirectory.put(file, lineValue.substring(0,p2+1));
                            }
                            property = null;
                        } else if (line.startsWith("Property:")) {
                            property = lineValue;
                            addPropertyToFile(file, property);
                            value = "";
                        } else if (line.startsWith("Value:")) {
                            value = lineValue;
                        } else if (line.startsWith("HackName:")) {
                            String regularItem =
                                UnicodeProperty.regularize(lineValue, true);
                            hackMap.put(regularItem, lineValue);
                        } else if (line.startsWith("FinalComments")) {
                            break;
                        } else {
                            throw new IllegalArgumentException("Unknown command: " + line);
                        }
                    }
                }
                br.close();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                throw new IllegalArgumentException("File missing");
            }
            hackMapFilter = new UnicodeProperty.MapFilter(hackMap);
            /*
               add("PropertyValueAliases", null);
               add("PropertyAliases", null);
               add("SpecialCasing", null);
               add("NormalizationTest", null);
               add("StandardizedVariants", null);
               add("CaseFolding", null);
               add("DerivedAge", new String[] {"Age"});
               add("Scripts", new String[] {"Script"});
               add("HangulSyllableType", new String[] {"HangulSyllableType"});
               add("DerivedBidiClass", new String[] {"BidiClass"});
               add("DerivedBinaryProperties", new String[] {"BidiMirrored"});
               add("DerivedCombiningClass", new String[] {"CanonicalCombiningClass"});
               add("DerivedDecompositionType", new String[] {"DecompositionType"});
               add("DerivedEastAsianWidth", new String[] {"EastAsianWidth"});
               add("DerivedGeneralCategory", new String[] {"GeneralCategory"});
               add("DerivedJoiningGroup", new String[] {"JoiningGroup"});
               add("DerivedJoiningType", new String[] {"JoiningType"});
               add("DerivedLineBreak", new String[] {"LineBreak"});
               add("DerivedNumericType", new String[] {"NumericType"});
               add("DerivedNumericValues", new String[] {"NumericValue"});
               add("PropList", new String[] {
                   "White_Space", "Bidi_Control", "Join_Control",
                   "Dash", "Hyphen", "Quotation_Mark",
                   "Terminal_Punctuation", "Other_Math", 
                   "Hex_Digit", "ASCII_Hex_Digit",
                   "Other_Alphabetic",
                   "Ideographic",
                   "Diacritic", "Extender", 
                   "Other_Lowercase", "Other_Uppercase",
                   "Noncharacter_Code_Point",
                   "Other_Grapheme_Extend",
                   "Grapheme_Link",
                   "IDS_Binary_Operator", "IDS_Trinary_Operator",
                   "Radical", "Unified_Ideograph", 
                   "Other_Default_Ignorable_Code_Point",
                   "Deprecated", "Soft_Dotted",
                   "Logical_Order_Exception",
                   "Other_ID_Start"
               });
               add("DerivedCoreProperties", new String[] {
                   "Math", "Alphabetic", "Lowercase", "Uppercase",
                   "ID_Start", "ID_Continue", 
                   "XID_Start", "XID_Continue", 
                   "Default_Ignorable_Code_Point",
                   "Grapheme_Extend", "Grapheme_Base"
               });
               add("DerivedNormalizationProps", new String[] {
                   "FC_NFKC_Closure", 
                   "Full_Composition_Exclusion", 
                   "NFD_QuickCheck", "NFC_QuickCheck", "NFKD_QuickCheck", "NFKC_QuickCheck", 
                   "Expands_On_NFD", "Expands_On_NFC", "Expands_On_NFKD", "Expands_On_NFKC"
                  });
            */
            write();
        }
        
        private void write() {
            TreeMap fileoptions = new TreeMap();
            /*for (int i = 0; i < FILE_OPTIONS.length; ++i) {
                String s = FILE_OPTIONS[i];
                int pos = s.indexOf(' ');
                String name = s.substring(0,pos);
                String options = s.substring(pos).trim();
                fileoptions.put(name, options);
            }
            */
            for (Iterator it = fileToPropertySet.keySet().iterator(); it.hasNext();) {
                String key = (String) it.next();
                if (DEBUG) {
                    System.out.println();
                    System.out.println("File:\t" + key);
                }
                List propList2 = (List) fileToPropertySet.get(key);
                if (propList2 == null) {
                    System.out.println("SPECIAL");
                    continue;
                } 
                for (Iterator pIt = propList2.iterator(); pIt.hasNext();) {
                    String prop = (String) pIt.next();
                    String options = (String)fileoptions.get(prop);
                    if (DEBUG) {
                        System.out.println();
                        System.out.println("Property:\t" + prop);
                        if (options != null) {
                            System.out.println("Format:\t" + options);
                        }
                    }
                    Map vc = getValue2CommentsMap(prop);
                    if (vc == null) continue;
                    for (Iterator it2 = vc.keySet().iterator(); it2.hasNext();) {
                        String value = (String) it2.next();
                        String comment = (String) vc.get(value);
                        if (DEBUG) {
                            if (!value.equals("")) {
                                System.out.println("Value:\t" + value);
                            }
                            System.out.println(comment);
                        }
                    }
                }
            }
        }
        
        private void addCommentToFile(String filename, String comment) {
            fileToComments.put(filename, comment);
        }
        
        private void addPropertyToFile(String filename, String property) {
            List properties = (List) fileToPropertySet.get(filename);
            if (properties == null) {
                properties = new ArrayList(1);
                fileToPropertySet.put(filename, properties);
            }
            properties.add(property);
        }
        public List getPropertiesFromFile(String filename) {
            return (List) fileToPropertySet.get(filename);
        }
        public Set getFiles() {
            return fileToPropertySet.keySet();
        }
    }
    
    public static void generateFile() throws IOException {
        for (int i = 0; i < Format.theFormat.filesToDo.length; ++i) {
            String fileName =
                Format.theFormat.filesToDo[i].trim().toLowerCase(
                    Locale.ENGLISH);
            Iterator it = Format.theFormat.getFiles().iterator();
            boolean gotOne = false;
            while (it.hasNext()) {
                String propname = (String) it.next();
                if (!propname.toLowerCase(Locale.ENGLISH).startsWith(fileName)) continue;
                generateFile(propname);
                gotOne = true;
            }
            if (!gotOne) {
                throw new IllegalArgumentException(
                    "Non-matching file name: " + fileName);
            }
        }
    }
     
    public static void generateFile(String filename) throws IOException {
        if (filename.endsWith("Aliases")) {
            if (filename.endsWith("ValueAliases")) generateValueAliasFile(filename);
            else generateAliasFile(filename);
        } else if (filename.equals("NormalizationTest")) {
            GenerateData.writeNormalizerTestSuite("DerivedData/", "NormalizationTest");
        } else if (filename.equals("CaseFolding")) {
            GenerateCaseFolding.makeCaseFold(false);
        } else if (filename.equals("SpecialCasing")) {
            GenerateCaseFolding.generateSpecialCasing(false);
        } else if (filename.equals("StandardizedVariants")) {
            GenerateStandardizedVariants.generate();
        } else {
            generatePropertyFile(filename);
        }
    }
    
    static final String SEPARATOR = "# ================================================";
    
    public static void generateAliasFile(String filename) throws IOException {
        UnicodeDataFile udf = UnicodeDataFile.openAndWriteHeader("DerivedData/", filename);
        PrintWriter pw = udf.out;
        UnicodeProperty.Factory ups 
                  = ToolUnicodePropertySource.make(Default.ucdVersion());
        TreeSet sortedSet = new TreeSet(CASELESS_COMPARATOR);
        BagFormatter bf = new BagFormatter();
        Tabber.MonoTabber mt = new Tabber.MonoTabber()
        .add(10,Tabber.LEFT);
        int count = 0;
        
        for (int i = UnicodeProperty.LIMIT_TYPE - 1; i >= UnicodeProperty.BINARY; --i) {
            if ((i & UnicodeProperty.EXTENDED_MASK) != 0) continue;
            List list = ups.getAvailableNames(1<<i);
            //if (list.size() == 0) continue;
            sortedSet.clear();
            StringBuffer buffer = new StringBuffer();
            for (Iterator it = list.iterator(); it.hasNext();) {
                String propAlias = (String)it.next();
                
                UnicodeProperty up = ups.getProperty(propAlias);
                List aliases = up.getNameAliases();
                if (aliases.size() == 1) {
                    sortedSet.add(mt.process(aliases.get(0) + "\t; " + aliases.get(0)));
                } else {
                    buffer.setLength(0);
                    boolean isFirst = true;
                    for (Iterator it2 = aliases.iterator(); it2.hasNext();) {
                        if (isFirst) isFirst = false;
                        else buffer.append("\t; ");
                        buffer.append(it2.next());
                    }
                    if (aliases.size() == 1) {
                        // repeat 
                        buffer.append("\t; ").append(aliases.get(0));
                    }
                    sortedSet.add(mt.process(buffer.toString()));
                }
            }
            if (i == UnicodeProperty.STRING) {
                for (int j = 0; j < specialString.length; ++j) {
                    sortedSet.add(mt.process(specialString[j]));
                }
            } else if (i == UnicodeProperty.MISC) {
                for (int j = 0; j < specialMisc.length; ++j) {
                    sortedSet.add(mt.process(specialMisc[j]));
                }
            }
            pw.println();
            pw.println(SEPARATOR);
            pw.println("# " + UnicodeProperty.getTypeName(i) + " Properties");
            pw.println(SEPARATOR);
            for (Iterator it = sortedSet.iterator(); it.hasNext();) {
                pw.println(it.next());
                count++;
            }
        }
        pw.println();
        pw.println(SEPARATOR);
        pw.println("# Total:    " + count);
        pw.println();
        udf.close();       
    }
    
    static String[] specialMisc = {
        "isc\t; ISO_Comment",
        "na1\t; Unicode_1_Name",
        "URS\t; Unicode_Radical_Stroke"};
        
    static String[] specialString = {
        "dm\t; Decomposition_Mapping",
        "lc\t; Lowercase_Mapping",
        "scc\t; Special_Case_Condition",
        "sfc\t; Simple_Case_Folding",
        "slc\t; Simple_Lowercase_Mapping",
        "stc\t; Simple_Titlecase_Mapping",
        "suc\t; Simple_Uppercase_Mapping",
        "tc\t; Titlecase_Mapping",
        "uc\t; Uppercase_Mapping"};

    static String[] specialGC = {
        "gc\t;\tC\t;\tOther\t# Cc | Cf | Cn | Co | Cs",
        "gc\t;\tL\t;\tLetter\t# Ll | Lm | Lo | Lt | Lu",
        "gc\t;\tLC\t;\tCased_Letter\t# Ll | Lt | Lu",
        "gc\t;\tM\t;\tMark\t# Mc | Me | Mn",
        "gc\t;\tN\t;\tNumber\t# Nd | Nl | No",
        "gc\t;\tP\t;\tPunctuation\t# Pc | Pd | Pe | Pf | Pi | Po | Ps",
        "gc\t;\tS\t;\tSymbol\t# Sc | Sk | Sm | So",
        "gc\t;\tZ\t;\tSeparator\t# Zl | Zp | Zs"};
        
    public static void generateValueAliasFile(String filename) throws IOException {
        UnicodeDataFile udf = UnicodeDataFile.openAndWriteHeader("DerivedData/", filename);
        PrintWriter pw = udf.out;
        Format.theFormat.printFileComments(pw, filename);
        UnicodeProperty.Factory toolFactory 
          = ToolUnicodePropertySource.make(Default.ucdVersion());
        BagFormatter bf = new BagFormatter(toolFactory);
        StringBuffer buffer = new StringBuffer();
        Set sortedSet = new TreeSet(CASELESS_COMPARATOR);
        
        //gc ; C         ; Other                            # Cc | Cf | Cn | Co | Cs
                        // 123456789012345678901234567890123

        // sc ; Arab      ; Arabic
        Tabber.MonoTabber mt2 = new Tabber.MonoTabber()
        .add(3,Tabber.LEFT)
        .add(2,Tabber.LEFT) // ;
        .add(10,Tabber.LEFT)
        .add(2,Tabber.LEFT) // ;
        .add(33,Tabber.LEFT)
        .add(2,Tabber.LEFT) // ;
        .add(33,Tabber.LEFT);
        
        // ccc; 216; ATAR ; Attached_Above_Right
        Tabber.MonoTabber mt3 = new Tabber.MonoTabber()
        .add(3,Tabber.LEFT)
        .add(2,Tabber.LEFT) // ;
        .add(3,Tabber.RIGHT)
        .add(2,Tabber.LEFT) // ;
        .add(5,Tabber.LEFT)
        .add(2,Tabber.LEFT) // ;
        .add(33,Tabber.LEFT)
        .add(2,Tabber.LEFT) // ;
        .add(33,Tabber.LEFT);

        for (Iterator it = toolFactory.getAvailableNames(UnicodeProperty.ENUMERATED_OR_CATALOG_MASK).iterator(); it.hasNext();) {
            String propName = (String) it.next();
            UnicodeProperty up = toolFactory.getProperty(propName);
            String shortProp = up.getFirstNameAlias();
            sortedSet.clear();
            
            for (Iterator it2 = up.getAvailableValues().iterator(); it2.hasNext();) {
                String value = (String) it2.next();
                List l = up.getValueAliases(value);
                if (DEBUG) System.out.println(value + "\t" + bf.join(l));
                
                // HACK
                Tabber mt = mt2;
                if (l.size() == 1) {
                    if (propName.equals("Canonical_Combining_Class")) continue;
                    if (propName.equals("Block") 
                      || propName.equals("Joining_Group")
                      //|| propName.equals("Numeric_Type")
                      || propName.equals("Age")) {
                        l.add(0, "n/a");
                    } else {
                        l.add(0, l.get(0)); // double up
                    } 
                } else if (propName.equals("Canonical_Combining_Class")) {
                    mt = mt3;
                }
                if (UnicodeProperty.equalNames(value,"Cyrillic_Supplement")) {
                    l.add("Cyrillic_Supplementary");
                }

                buffer.setLength(0);                
                buffer.append(shortProp);
                for (Iterator it3 = l.iterator(); it3.hasNext();) {
                    buffer.append("\t; \t" + it3.next());
                }
                
                sortedSet.add(mt.process(buffer.toString()));
            }
            // HACK
            if (propName.equals("General_Category")) {
                for (int i = 0; i < specialGC.length; ++i) {
                    sortedSet.add(mt2.process(specialGC[i]));
                }
            }
            pw.println();
            pw.println("# " + propName + " (" + shortProp + ")");
            pw.println();
            for (Iterator it4 = sortedSet.iterator(); it4.hasNext();) {
                String line = (String) it4.next();
                pw.println(line);
            }
        }
        udf.close();
    }
    
    public static void generatePropertyFile(String filename) throws IOException {
        String dir = (String) Format.theFormat.fileToDirectory.get(filename);
        if (dir == null) dir = "";
        UnicodeDataFile udf =
            UnicodeDataFile.openAndWriteHeader("DerivedData/" + dir, filename);
        PrintWriter pw = udf.out;
        // bf2.openUTF8Writer(UCD_Types.GEN_DIR, "Test" + filename + ".txt");
        Format.theFormat.printFileComments(pw, filename);
        UnicodeProperty.Factory toolFactory =
            ToolUnicodePropertySource.make(Default.ucdVersion());
        UnicodeSet unassigned =
            toolFactory.getSet("gc=cn").addAll(toolFactory.getSet("gc=cs"));
        //System.out.println(unassigned.toPattern(true));
        // .removeAll(toolFactory.getSet("noncharactercodepoint=true"));

        List propList = Format.theFormat.getPropertiesFromFile(filename);
        for (Iterator propIt = propList.iterator(); propIt.hasNext();) {
             BagFormatter bf = new BagFormatter(toolFactory);
             UnicodeProperty prop = toolFactory.getProperty((String) propIt.next());
             String name = prop.getName();
             System.out.println("Property: " + name + "; " + prop.getTypeName(prop.getType()));
            pw.println();
            pw.println(SEPARATOR);
             String propComment = Format.theFormat.getValueComments(name, "");
             if (propComment != null && propComment.length() != 0) {
                 pw.println();
                 pw.println(propComment);
             } else if (!prop.isType(UnicodeProperty.BINARY_MASK)) {
                 pw.println();
                 pw.println("# Property:\t" + name);
             }
             
            Format.PrintStyle ps = Format.theFormat.getPrintStyle(name);
            if (DEBUG) System.out.println(ps.toString());

            if (!prop.isType(UnicodeProperty.BINARY_MASK) &&
                (ps.skipUnassigned != null || ps.skipValue != null)) {
                 String v = ps.skipValue;
                if (v == null) {
                    v = ps.skipUnassigned;
                }
                String v2 = prop.getFirstValueAlias(v);
                if (UnicodeProperty.compareNames(v,v2) != 0) v = v + " (" + v2 + ")";
                pw.println();
                pw.println("#  All code points not explicitly listed for " + prop.getName());
                pw.println("#  have the value " + v + ".");
            }
             
             if (!ps.interleaveValues && prop.isType(UnicodeProperty.BINARY_MASK)) {
                 if (DEBUG) System.out.println("Resetting Binary Values");
                 ps.skipValue = "False";
                 if (ps.nameStyle.equals("none")) ps.nameStyle = "long";
                 ps.valueStyle = "none";
             }

             if (ps.noLabel) bf.setLabelSource(null);
             if (ps.nameStyle.equals("none")) bf.setPropName(null);
             else if (ps.nameStyle.equals("short")) bf.setPropName(prop.getFirstNameAlias());
             else bf.setPropName(name);
            
             if (ps.interleaveValues) {
                writeInterleavedValues(pw, bf, prop);
             } else if (prop.isType(UnicodeProperty.STRING_OR_MISC_MASK)) {
                writeStringValues(pw, bf, prop);
             //} else if (prop.isType(UnicodeProperty.BINARY_MASK)) {
             //   writeBinaryValues(pw, bf, prop);
             } else {
                 writeEnumeratedValues(pw, bf, unassigned, prop, ps);
             }
         }
         udf.close();
     }
     
    private static void writeEnumeratedValues(
            PrintWriter pw,
            BagFormatter bf,
            UnicodeSet unassigned,
            UnicodeProperty prop,
            Format.PrintStyle ps) {
        if (DEBUG) System.out.println("Writing Enumerated Values: " + prop.getName());
        
         bf.setValueSource(new UnicodeProperty.FilteredProperty(prop, Format.theFormat.hackMapFilter));
         Collection aliases = prop.getAvailableValues();
         if (ps.orderByRangeStart) {
             if (DEBUG) System.out.println("Reordering");
             TreeSet temp2 = new TreeSet(new RangeStartComparator(prop));
             temp2.addAll(aliases);
             aliases = temp2;
         }
         if (ps.sortNumeric) {
             if (DEBUG) System.out.println("Reordering");
             TreeSet temp2 = new TreeSet(NUMERIC_STRING_COMPARATOR);
             temp2.addAll(aliases);
             aliases = temp2;
         }
         for (Iterator it = aliases.iterator(); it.hasNext();) {
             String value = (String)it.next();
             if (DEBUG) System.out.println("Getting value " + value);
             UnicodeSet s = prop.getSet(value);
             String valueComment = Format.theFormat.getValueComments(prop.getName(), value);
        
            if (DEBUG) {
                System.out.println("Value:\t" + value + "\t" + prop.getFirstValueAlias(value) + "\tskip:" + ps.skipValue); 
                System.out.println("Value Comment\t" + valueComment); 
                System.out.println(s.toPattern(true));
            }
            
            int totalSize = s.size();
            if (s.size() == 0) {
                System.out.println("\tSkipping Empty: " + prop.getName() + "=" + value);
                continue;
            } 
            
            if (UnicodeProperty.compareNames(value, ps.skipValue) == 0) {
                if (DEBUG) System.out.println("Skipping: " + value);
                continue;
            } 
             
            if (UnicodeProperty.compareNames(value, ps.skipUnassigned) == 0) {
                bf.setFullTotal(s.size());
                if (DEBUG) System.out.println("Removing Unassigneds: " + value);
                s.removeAll(unassigned);
            }
             
            //if (s.size() == 0) continue;
             //if (unassigned.containsAll(s)) continue; // skip if all unassigned
             //if (s.contains(0xD0000)) continue; // skip unassigned
             
             boolean nonLongValue = false;
             String displayValue = value;
             if (ps.valueStyle.equals("none")) {
                 displayValue = null;
                 nonLongValue = true;
             } else if (ps.valueStyle.equals("short")) {
                 displayValue = prop.getFirstValueAlias(displayValue);
                 nonLongValue = true;
                 if (DEBUG) System.out.println("Changing value " + displayValue);
             } 
             if (ps.makeUppercase && displayValue != null) {
                 displayValue = displayValue.toUpperCase(Locale.ENGLISH);
                 if (DEBUG) System.out.println("Changing value2 " + displayValue);
             } 
             if (ps.makeFirstLetterLowercase && displayValue != null) {
                 // NOTE: this is ok since we are only working in ASCII
                 displayValue = displayValue.substring(0,1).toLowerCase(Locale.ENGLISH)
                     + displayValue.substring(1);
                 if (DEBUG) System.out.println("Changing value2 " + displayValue);
             }
             if (DEBUG) System.out.println("Setting value " + displayValue);
             bf.setValueSource(displayValue);
             
             if (!prop.isType(UnicodeProperty.BINARY_MASK)) {
                 pw.println();
                 pw.println(SEPARATOR);
                 if (nonLongValue) {
                     pw.println();
                     pw.println("# " + prop.getName() + "=" + value);
                 }
             }

             if (valueComment != null) {
                 pw.println();
                 pw.println(valueComment);
             }
             if (false && ps.longValueHeading != null) {
                 String headingValue = value;
                 if (ps.longValueHeading == "ccc") {
                     headingValue = Utility.replace(value, "_", "");
                     char c = headingValue.charAt(0);
                     if ('0' <= c && c <= '9') headingValue = "Other Combining Class";
                 }
                 pw.println();
                 pw.println("# " + headingValue);
             }
             pw.println();
             //if (s.size() != 0) 
             bf.showSetNames(pw, s);
        }
        
    }
    //static NumberFormat nf = NumberFormat.getInstance();
    static Comparator NUMERIC_STRING_COMPARATOR = new Comparator() {
        public int compare(Object o1, Object o2) {
            if (o1 == o2) return 0;
            if (o1 == null) return -1;
            if (o2 == null) return 1;
            return Double.compare(
                Double.parseDouble((String) o1),
                Double.parseDouble((String) o2));
        }
        
    };
    /*
    private static void writeBinaryValues(
        PrintWriter pw,
        BagFormatter bf,
        UnicodeProperty prop) {
        if (DEBUG) System.out.println("Writing Binary Values: " + prop.getName());
         UnicodeSet s = prop.getSet("True");
         bf.setValueSource(prop.getName());
         bf.showSetNames(pw, s);
    }
    */
    
    private static void writeInterleavedValues(
        PrintWriter pw,
        BagFormatter bf,
        UnicodeProperty prop) {
        if (DEBUG) System.out.println("Writing Interleaved Values: " + prop.getName());
        pw.println();   
         bf.setValueSource(new UnicodeProperty.FilteredProperty(prop, new RestoreSpacesFilter()))
         .setNameSource(null)
         .setLabelSource(null)
         .setRangeBreakSource(null)
         .setShowCount(false)
         .showSetNames(pw,new UnicodeSet(0,0x10FFFF));                 
    }
     
    private static void writeStringValues(
            PrintWriter pw,
            BagFormatter bf,
            UnicodeProperty prop) {
        if (DEBUG) System.out.println("Writing String Values: " + prop.getName());
        pw.println();   
        bf.setValueSource(prop)
        .setHexValue(true)
        .setMergeRanges(false)
        .showSetNames(pw,new UnicodeSet(0,0x10FFFF));                 
    }
     
    static class RangeStartComparator implements Comparator {
        UnicodeProperty prop;
        CompareProperties.UnicodeSetComparator comp = new CompareProperties.UnicodeSetComparator();
        RangeStartComparator(UnicodeProperty prop) {
            this.prop = prop;
        }
        public int compare(Object o1, Object o2) {
            UnicodeSet s1 = prop.getSet((String)o1);
            UnicodeSet s2 = prop.getSet((String)o2);
            if (true) System.out.println("comparing " + o1 + ", " + o2
                + s1.toPattern(true) + "?" + s2.toPattern(true)
                + ", " + comp.compare(s1, s2));
            return comp.compare(s1, s2);
        }
        
    }
    
    static class RestoreSpacesFilter extends UnicodeProperty.StringFilter {
        public String remap(String original) {
            // ok, because doesn't change length
            String mod = (String) Format.theFormat.hackMap.get(original);
            if (mod != null) original = mod;
            return original.replace('_',' ');
        }
    }
    
    static Comparator CASELESS_COMPARATOR = new Comparator() {
        public int compare(Object o1, Object o2) {
            String s = o1.toString();
            String t = o2.toString();
            return s.compareToIgnoreCase(t);
        }
    };
    
    public static void showDiff() throws IOException {
        PrintWriter out = BagFormatter.openUTF8Writer(UCD_Types.GEN_DIR, "propertyDifference.txt");
        try {
            showDifferences(out, "4.0.1", "LB", "GC");
            showDifferences(out, "4.0.1", "East Asian Width", "LB");
            showDifferences(out, "4.0.1", "East Asian Width", "GC");
        } finally {
            out.close();
        }
    }
    
    public static void showMatches() throws IOException {
        PrintWriter out = BagFormatter.openUTF8Writer(UCD_Types.GEN_DIR, "propertyDifference.txt");
        try {
            showDifferences(out, "4.0.1", "LB", "GC");
            showDifferences(out, "4.0.1", "East Asian Width", "LB");
            showDifferences(out, "4.0.1", "East Asian Width", "GC");
        } finally {
            out.close();
        }
    }

    static NumberFormat nf = NumberFormat.getIntegerInstance(Locale.ENGLISH);
    
    static void showDifferences(PrintWriter out, String version, String prop1, String prop2) throws IOException {
        UnicodeProperty p1 = ToolUnicodePropertySource.make(version).getProperty(prop1);
        UnicodeProperty p2 = ToolUnicodePropertySource.make(version).getProperty(prop2);
        BagFormatter bf = new BagFormatter();
        out.println("Comparing " + p1.getName() + " and " + p2.getName());
        System.out.println("Comparing " + p1.getName() + " and " + p2.getName());
        UnicodeSet intersection = new UnicodeSet();
        UnicodeSet disjoint = new UnicodeSet();
            main:
            for (Iterator it1 = p1.getAvailableValues().iterator(); it1.hasNext();) {
                String v1 = (String)it1.next();
                UnicodeSet s1 = p1.getSet(v1);
                v1 += " (" + p1.getFirstValueAlias(v1) + ")";
                System.out.println(v1);
                out.println();
                out.println(v1 + " [" + nf.format(s1.size()) + "]");

                // create some containers so that the output is organized reasonably
                String contains = "";
                String overlaps = "";
                UnicodeSet containsSet = new UnicodeSet();
                Set overlapsSet = new TreeSet();
                for (Iterator it2 = p2.getAvailableValues().iterator(); it2.hasNext();) {
                    String v2 = (String)it2.next();
                    UnicodeSet s2 = p2.getSet(v2);
                    // v2 += "(" + p2.getFirstValueAlias(v2) + ")";
                    v2 = p2.getFirstValueAlias(v2);
                    if (s1.containsNone(s2)) continue;
                    if (s1.equals(s2)) {
                        out.println("\t= " + v2);
                        continue main; // since they are partitions, we can stop here
                    } else if (s2.containsAll(s1)) {
                        out.println("\t\u2282 " + v2 + " [" + nf.format(s2.size()) + "]");
                        continue main; // partition, stop
                    } else if (s1.containsAll(s2)) {
                        if (contains.length() != 0) contains += " \u222a ";
                        contains += v2 + " [" + nf.format(s2.size()) + "]";
                        containsSet.addAll(s2);
                        if (containsSet.size() == s1.size()) break;
                    } else { // doesn't contain, isn't contained
                        if (overlaps.length() != 0) overlaps += "\r\n\t";
                        intersection.clear().addAll(s2).retainAll(s1);
                        disjoint.clear().addAll(s1).removeAll(s2);
                        overlaps += "\u2283 " + v2 + " [" + nf.format(intersection.size()) + "]"
                            + " \u2285 " + v2 + " [" + nf.format(disjoint.size()) + "]";
                        overlapsSet.add(v2);
                    } 
                }
                if (contains.length() != 0) {
                    out.println((containsSet.size() == s1.size() ? "\t= " : "\t\u2283 ") + contains);
                } 
                if (overlaps.length() != 0) out.println("\t" + overlaps);
                if (false && overlapsSet.size() != 0) {
                    out.println("\t\u2260\u2284\u2285");
                    for (Iterator it3 = overlapsSet.iterator(); it3.hasNext();) {
                        String v3 = (String) it3.next();
                        UnicodeSet s3 = p2.getSet(v3);
                        out.println("\t" + v3);
                        bf.showSetDifferences(out,v1,s1,v3,s3);
                    }
                }
            }
    }
    
    static class UnicodeDataHack extends UnicodeLabel {
        private UnicodeProperty.Factory factory;
        private UnicodeProperty name;
        private UnicodeProperty bidiMirrored;
        private UnicodeProperty numericValue;
        private UnicodeProperty numericType;
        private UnicodeProperty decompositionValue;
        private UnicodeProperty decompositionType;
        private UnicodeProperty bidiClass;
        private UnicodeProperty combiningClass;
        private UnicodeProperty category;
        UnicodeDataHack(UnicodeProperty.Factory factory) {
            this.factory = factory;
            name = factory.getProperty("Name");
            category = factory.getProperty("General_Category");
            combiningClass = factory.getProperty("Canonical_Combining_Class");
            bidiClass = factory.getProperty("Bidi_Class");
            decompositionType = factory.getProperty("Decomposition_Type");
            decompositionValue = factory.getProperty("Decomposition_Value");
            numericType = factory.getProperty("Numeric_Type");
            numericValue = factory.getProperty("Numeric_Value");
            bidiMirrored = factory.getProperty("Bidi_Mirrored");
            //name10
            //isoComment
        }
        public String getValue(int codepoint, boolean isShort) {
            String nameStr = name.getName();
            if (nameStr.startsWith("<reserved")) return null;
            String code = Utility.hex(codepoint);
            int pos = nameStr.indexOf(code);
            if (pos > 0) {
                nameStr = nameStr.substring(0,pos) + "%" + nameStr.substring(pos + code.length());
            }
            nameStr += ";"
            + category.getValue(codepoint, true) + ";"
            + combiningClass.getValue(codepoint, true) + ";"
            + bidiClass.getValue(codepoint, true) + ";"
            ;
            String temp = decompositionType.getValue(codepoint, true);
            if (!temp.equals("None")) {
                nameStr += "<" + temp + "> " + Utility.hex(decompositionValue.getValue(codepoint));
            }
            nameStr += ";";
            temp = numericType.getValue(codepoint, true);
            if (temp.equals("Decimal")) {
                nameStr += temp + ";" + temp + ";" + temp + ";";
            } else if (temp.equals("Digit")) {
                nameStr += ";" + temp + ";" + temp + ";";
            } else if (temp.equals("Numeric")) {
                nameStr += ";;" + temp + ";";
            } else if (temp.equals("Digit")) {
                nameStr += ";;;";
            }
            if (bidiMirrored.getValue(codepoint, true).equals("True")) {
                nameStr += "Y" + ";";
            }
            nameStr += ";";
            return nameStr;
        }       
    }
    
    /**
     * Chain together several SymbolTables. 
     * @author Davis
     */
    static class ChainedSymbolTable implements SymbolTable {
        // TODO: add accessors?
        private List symbolTables;
        /**
         * Each SymbolTable is each accessed in order by the other methods,
         * so the first in the list is accessed first, etc.
         * @param symbolTables
         */
        ChainedSymbolTable(SymbolTable[] symbolTables) {
            this.symbolTables = Arrays.asList(symbolTables);
        }
        public char[] lookup(String s) {
            for (Iterator it = symbolTables.iterator(); it.hasNext();) {
                SymbolTable st = (SymbolTable) it.next();
                char[] result = st.lookup(s);
                if (result != null) return result;
            }
            return null;
        }

        public UnicodeMatcher lookupMatcher(int ch) {
            for (Iterator it = symbolTables.iterator(); it.hasNext();) {
                SymbolTable st = (SymbolTable) it.next();
                UnicodeMatcher result = st.lookupMatcher(ch);
                if (result != null) return result;
            }
            return null;
        }
        
        // Warning: this depends on pos being left alone unless a string is returned!!
        public String parseReference(String text, ParsePosition pos, int limit) {
            for (Iterator it = symbolTables.iterator(); it.hasNext();) {
                SymbolTable st = (SymbolTable) it.next();
                String result = st.parseReference(text, pos, limit);
                if (result != null) return result;
            }
            return null;
        }
    }
    
    static final UnicodeSet INVARIANT_RELATIONS = new UnicodeSet("[\\= \\! \\? \\< \\> \u2264 \u2265 \u2282 \u2286 \u2283 \u2287]");
    
    static void testInvariants() throws IOException {
        PrintWriter out = BagFormatter.openUTF8Writer(UCD_Types.GEN_DIR, "UnicodeInvariantResults.txt");
        out.write('\uFEFF'); // BOM
        BufferedReader in = BagFormatter.openUTF8Reader("", "UnicodeInvariants.txt");
        BagFormatter bf = new BagFormatter();
        ChainedSymbolTable st = new ChainedSymbolTable(new SymbolTable[] {
            ToolUnicodePropertySource.make("4.0.0").getSymbolTable("\u00D7"),
            ToolUnicodePropertySource.make(Default.ucdVersion()).getSymbolTable("")});
        ParsePosition pp = new ParsePosition(0);
        int parseErrorCount = 0;
        int testFailureCount = 0;
        while (true) {
            String rightSide = null;
            String leftSide = null;
            String line = in.readLine();
            if (line == null) break;
            if (line.startsWith("\uFEFF")) line = line.substring(1);
            line = line.trim();
            int pos = line.indexOf('#');
            if (pos >= 0) line = line.substring(0,pos).trim();
            if (line.length() == 0) continue;

            char relation = 0;
            UnicodeSet leftSet = null;
            UnicodeSet rightSet = null;
            try {
                pp.setIndex(0);
                leftSet = new UnicodeSet(line, pp, st);
                leftSide = line.substring(0,pp.getIndex());
                eatWhitespace(line, pp);
                relation = line.charAt(pp.getIndex());
                if (!INVARIANT_RELATIONS.contains(relation)) {
                    throw new ParseException("Invalid relation, must be one of " + INVARIANT_RELATIONS.toPattern(false),
                        pp.getIndex());
                }
                pp.setIndex(pp.getIndex()+1); // skip char
                eatWhitespace(line, pp);
                int start = pp.getIndex();
                rightSet = new UnicodeSet(line, pp, st);
                rightSide = line.substring(start,pp.getIndex());
                eatWhitespace(line, pp);
                if (line.length() != pp.getIndex()) {
                    throw new ParseException("Extra characters at end", pp.getIndex());
                }
            } catch (ParseException e) {
                out.println("PARSE ERROR:\t" + line.substring(0,e.getErrorOffset())
                    + "<@>" + line.substring(e.getErrorOffset()));
                out.println();
                out.println("**** START Error Info ****");
                out.println(e.getMessage());
                out.println("**** END Error Info ****");
                out.println();
                parseErrorCount++;
                continue;
            } catch (IllegalArgumentException e) {
                out.println("PARSE ERROR:\t" + line);
                out.println();
                out.println("**** START Error Info ****");
                out.println(e.getMessage());
                out.println("**** END Error Info ****");
                out.println();
                parseErrorCount++;
                continue;
            }
            
            boolean ok = true;
            switch(relation) {
                case '=': ok = leftSet.equals(rightSet); break;
                case '<': case '\u2282': ok = rightSet.containsAll(leftSet) && !leftSet.equals(rightSet); break;
                case '>': case '\u2283': ok = leftSet.containsAll(rightSet) && !leftSet.equals(rightSet); break;
                case '\u2264': case '\u2286': ok = rightSet.containsAll(leftSet); break;
                case '\u2265': case '\u2287': ok = leftSet.containsAll(rightSet); break;
                case '!': ok = leftSet.containsNone(rightSet); break;
                case '?': ok = !leftSet.equals(rightSet) 
                        && !leftSet.containsAll(rightSet) 
                        && !rightSet.containsAll(leftSet)
                        && !leftSet.containsNone(rightSet); 
                    break;
                default: throw new IllegalArgumentException("Internal Error");
            }
            out.println(String.valueOf(ok).toUpperCase(Locale.ENGLISH) + ":\t" + line);
            if (ok) continue;
            out.println();
            out.println("**** START Error Info ****");
            bf.showSetDifferences(out, rightSide, rightSet, leftSide, leftSet);
            out.println("**** END Error Info ****");
            out.println();
            testFailureCount++;      
        }
        out.println();
        out.println("**** SUMMARY ****");
        out.println();
        out.println("ParseErrorCount=" + parseErrorCount);
        out.println("TestFailureCount=" + testFailureCount);
        out.close();
        System.out.println("ParseErrorCount=" + parseErrorCount);
        System.out.println("TestFailureCount=" + testFailureCount);
    }
    
    /**
     * @param line
     * @param pp
     */
    private static void eatWhitespace(String line, ParsePosition pp) {
        int cp = 0;
        int i;
        for (i = pp.getIndex(); i < line.length(); i += UTF16.getCharCount(cp)) {
            cp = UTF16.charAt(line, i);
            if (!com.ibm.icu.lang.UCharacter.isUWhiteSpace(cp)) {
                break;
            }
        }
        pp.setIndex(i);
    }

    /*
    static class PropertySymbolTable implements SymbolTable {
        static boolean DEBUG = false;
        UnicodeProperty.Factory factory;
        //static Matcher identifier = Pattern.compile("([:letter:] [\\_\\-[:letter:][:number:]]*)").matcher("");
        
        PropertySymbolTable (UnicodeProperty.Factory factory) {
            this.factory = factory;
        }
        
        public char[] lookup(String s) {
            if (DEBUG) System.out.println("\tLooking up " + s);
            int pos = s.indexOf('=');
            if (pos < 0) return null; // should never happen
            UnicodeProperty prop = factory.getProperty(s.substring(0,pos));
            if (prop == null) {
                throw new IllegalArgumentException("Invalid Property: " + s + "\r\nUse "
                 + showSet(factory.getAvailableNames())); 
            }
            String value = s.substring(pos+1);
            UnicodeSet set = prop.getSet(value);
            if (set.size() == 0) {
                throw new IllegalArgumentException("Empty Property-Value: " + s + "\r\nUse "
                    + showSet(prop.getAvailableValues()));
            }
            if (DEBUG) System.out.println("\tReturning " + set.toPattern(true));
            return set.toPattern(true).toCharArray(); // really ugly
        }

        private String showSet(List list) {
            StringBuffer result = new StringBuffer("[");
            boolean first = true;
            for (Iterator it = list.iterator(); it.hasNext();) {
                if (!first) result.append(", ");
                else first = false;
                result.append(it.next().toString());
            }
            result.append("]");
            return result.toString();
        }

        public UnicodeMatcher lookupMatcher(int ch) {
            return null;
        }

        public String parseReference(String text, ParsePosition pos, int limit) {
            if (DEBUG) System.out.println("\tParsing <" + text.substring(pos.getIndex(),limit) + ">");
            int start = pos.getIndex();
            int i = getIdentifier(text, start, limit);
            if (i == start) return null;
            String prop = text.substring(start, i);
            String value = "true";
            if (i < limit) {
                int cp = text.charAt(i);
                if (cp == ':' || cp == '=') {
                    int j = getIdentifier(text, i+1, limit);
                    value = text.substring(i+1, j);
                    i = j;
                }
            }
            pos.setIndex(i);
            if (DEBUG) System.out.println("\tParsed <" + prop + ">=<" + value + ">");
            return prop + '=' + value;
        }

        private int getIdentifier(String text, int start, int limit) {
            if (DEBUG) System.out.println("\tGetID <" + text.substring(start,limit) + ">");
            int cp = 0;
            int i;
            for (i = start; i < limit; i += UTF16.getCharCount(cp)) {
                cp = UTF16.charAt(text, i);
                if (!com.ibm.icu.lang.UCharacter.isUnicodeIdentifierPart(cp)) {
                    break;
                }
            }
            if (DEBUG) System.out.println("\tGotID <" + text.substring(start,i) + ">");
            return i;
        }
       
    };
    
    /* getCombo(UnicodeProperty.Factory factory, String line) {
        UnicodeSet result = new UnicodeSet();
        String[] pieces = Utility.split(line, '+');
        for (int i = 0; i < pieces.length; ++i) {
            String[] parts = Utility.split(pieces[i],':');
            String prop = parts[0].trim();
            String value = "true";
            if (parts.length > 1) value = parts[1].trim();
            UnicodeProperty p = factory.getProperty(prop);
            result.addAll(p.getSet(value));
        }
        return result;
    }
    */
}
    
/*
static class OrderedMap {
     HashMap map = new HashMap();
     ArrayList keys = new ArrayList();
     void put(Object o, Object t) {
         map.put(o,t);
         keys.add(o);
     }
     List keyset() {
         return keys;
     }
 }
    */