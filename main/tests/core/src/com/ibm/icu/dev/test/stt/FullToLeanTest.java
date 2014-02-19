/*
 *******************************************************************************
 *   Copyright (C) 2001-2014, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *******************************************************************************
 */

package com.ibm.icu.dev.test.stt;

import com.ibm.icu.impl.stt.Environment;
import com.ibm.icu.impl.stt.Expert;
import com.ibm.icu.impl.stt.ExpertFactory;
import com.ibm.icu.text.BidiStructuredProcessor;

/**
 * Tests fullToLean method
 */
public class FullToLeanTest extends TestBase {

    private static final Environment envLTR = new Environment(null, false,
            BidiStructuredProcessor.Orientation.LTR);
    private static final Environment envRTL = new Environment(null, false,
            BidiStructuredProcessor.Orientation.RTL);

    private BidiStructuredProcessor.StructuredTypes type;
    int cntError;

    private void doTest1(String msg, String data, String leanLTR,
            String fullLTR, int[] l2fMapLTR, int[] f2lMapLTR, String leanRTL,
            String fullRTL, int[] l2fMapRTL, int[] f2lMapRTL) {
        String text, full, lean, label;
        int[] map;

        text = toUT16(data);
        Expert expertLTR = ExpertFactory.getExpert(type, envLTR);
        lean = expertLTR.fullToLeanText(text);

        cntError += assertEquals(msg + "LTR lean", leanLTR, toPseudo(lean));
        full = expertLTR.leanToFullText(lean);

        cntError += assertEquals(msg + "LTR full", fullLTR, toPseudo(full));
        map = expertLTR.leanToFullMap(lean);

        label = msg + "leanToFullMap() LTR";
        cntError += assertEquals(label, array_display(l2fMapLTR),
                array_display(map));
        map = expertLTR.fullToLeanMap(text);

        label = msg + "fullToLeanMap() LTR";
        cntError += assertEquals(label, array_display(f2lMapLTR),
                array_display(map));

        Expert expertRTL = ExpertFactory.getExpert(type, envRTL);
        lean = expertRTL.fullToLeanText(text);
        cntError += assertEquals(msg + "RTL lean", leanRTL, toPseudo(lean));
        full = expertRTL.leanToFullText(lean);

        cntError += assertEquals(msg + "RTL full", fullRTL, toPseudo(full));
        map = expertRTL.leanToFullMap(lean);

        label = msg + "leanToFullMap() RTL";
        cntError += assertEquals(label, array_display(l2fMapRTL),
                array_display(map));
        map = expertRTL.fullToLeanMap(text);

        label = msg + "fullToLeanMap() RTL";
        cntError += assertEquals(label, array_display(f2lMapRTL),
                array_display(map));
    }

    private void doTest2(String msg) {
/*        
        String text, data, full, lean, model;
        Object state1, state2, state3;

        data = "update \"AB_CDE\" set \"COL1\"@='01', \"COL2\"@='02' /* GH IJK";
        text = toUT16(data);
        Expert expertLTR = ExpertFactory.getStatefulExpert(type, envLTR);
        expertLTR.clearState();
        lean = expertLTR.fullToLeanText(text);

        state1 = expertLTR.getState();
        model = "update \"AB_CDE\" set \"COL1\"='01', \"COL2\"='02' /* GH IJK";
        cntError += assertEquals(msg + "LTR lean", model, toPseudo(lean));
        Expert expertLTR2 = ExpertFactory.getStatefulExpert(type, envLTR);
        expertLTR2.clearState();
        full = expertLTR2.leanToFullText(lean);

        cntError += assertEquals(msg + "LTR full", data, toPseudo(full));
        cntError += assertEquals(msg + "state from leanToFullText", state1,
                expertLTR2.getState());
        data = "THIS IS A COMMENT LINE";
        text = toUT16(data);
        expertLTR.setState(state1);
        lean = expertLTR.fullToLeanText(text);

        state2 = expertLTR.getState();
        model = "THIS IS A COMMENT LINE";
        cntError += assertEquals(msg + "LTR lean2", model, toPseudo(lean));
        expertLTR2.setState(state1);
        full = expertLTR2.leanToFullText(lean);

        cntError += assertEquals(msg + "LTR full2", data, toPseudo(full));
        cntError += assertEquals(msg + "state from leanToFullText2", state2,
                expertLTR2.getState());
        */
        // data = "SOME MORE */ where \"COL3\"@=123";
        /*
        text = toUT16(data);

        expertLTR.setState(state2);
        lean = expertLTR.fullToLeanText(text);

        state3 = expertLTR.getState();
        */
        //model = "SOME MORE */ where \"COL3\"=123";
        /*
        cntError += assertEquals(msg + "LTR lean3", model, toPseudo(lean));

        expertLTR.setState(state2);
        full = expertLTR.leanToFullText(lean);
        cntError += assertEquals(msg + "LTR full3", data, toPseudo(full));
        cntError += assertEquals(msg + "state from leanToFullText3", state3,
                expertLTR.getState());
        */
    }

    public static int main(String[] args) {
        FullToLeanTest test = new FullToLeanTest();
        test.type = BidiStructuredProcessor.StructuredTypes.COMMA_DELIMITED;
        test.doTest1("testFullToLean #1 - ", "", "", "", new int[0],
                new int[0], "", "", new int[0], new int[0]);
        int[] map1 = new int[] { 0, 1, 2, 3, 4 };
        int[] map2 = new int[] { 2, 3, 4, 5, 6 };
        int[] map3;
        test.doTest1("testFullToLean #01 - ", "1.abc", "1.abc", "1.abc", map1,
                map1, "1.abc", ">@1.abc@^", map2, map1);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        map2 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        test.doTest1("testFullToLean #02 - ", "2.abc,def", "2.abc,def",
                "2.abc,def", map1, map1, "2.abc,def", ">@2.abc,def@^", map2,
                map1);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        map2 = new int[] { -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        test.doTest1("testFullToLean #03 - ", "@a.3.bc,def", "a.3.bc,def",
                "a.3.bc,def", map1, map2, "a.3.bc,def", ">@a.3.bc,def@^", map3,
                map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        map2 = new int[] { -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        test.doTest1("testFullToLean #04 - ", "@@a.4.bc,def", "a.4.bc,def",
                "a.4.bc,def", map1, map2, "a.4.bc,def", ">@a.4.bc,def@^", map3,
                map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        map2 = new int[] { -1, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        test.doTest1("testFullToLean #05 - ", "@5.abc,def", "5.abc,def",
                "5.abc,def", map1, map2, "5.abc,def", ">@5.abc,def@^", map3,
                map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        map2 = new int[] { -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        test.doTest1("testFullToLean #06 - ", "@@6.abc,def", "6.abc,def",
                "6.abc,def", map1, map2, "6.abc,def", ">@6.abc,def@^", map3,
                map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        map2 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        test.doTest1("testFullToLean #07 - ", "7abc,@def", "7abc,@def",
                "7abc,@def", map1, map1, "7abc,@def", ">@7abc,@def@^", map2,
                map1);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        map2 = new int[] { 0, 1, 2, 3, 4, 5, -1, 6, 7, 8 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        test.doTest1("testFullToLean #08 - ", "8abc,@@def", "8abc,@def",
                "8abc,@def", map1, map2, "8abc,@def", ">@8abc,@def@^", map3,
                map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7 };
        map2 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9 };
        test.doTest1("testFullToLean #09 - ", "9abc,def@", "9abc,def",
                "9abc,def", map1, map2, "9abc,def", ">@9abc,def@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        map2 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        test.doTest1("testFullToLean #10 - ", "10abc,def@@", "10abc,def",
                "10abc,def", map1, map2, "10abc,def", ">@10abc,def@^", map3,
                map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        map2 = new int[] { -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
        test.doTest1("testFullToLean #11 - ", "@a.11.bc,@def@", "a.11.bc,@def",
                "a.11.bc,@def", map1, map2, "a.11.bc,@def", ">@a.11.bc,@def@^",
                map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        map2 = new int[] { -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, 9, 10, 11,
                -1, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
        test.doTest1("testFullToLean #12 - ", "@@a.12.bc,@@def@@",
                "a.12.bc,@def", "a.12.bc,@def", map1, map2, "a.12.bc,@def",
                ">@a.12.bc,@def@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4 };
        map2 = new int[] { 2, 3, 4, 5, 6 };
        test.doTest1("testFullToLean #13 - ", "13ABC", "13ABC", "13ABC", map1,
                map1, "13ABC", ">@13ABC@^", map2, map1);
        map1 = new int[] { 0, 1, 2, 3, 4, 6, 7, 8 };
        map2 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7 };
        map3 = new int[] { 2, 3, 4, 5, 6, 8, 9, 10 };
        test.doTest1("testFullToLean #14 - ", "14ABC,DE", "14ABC,DE",
                "14ABC@,DE", map1, map2, "14ABC,DE", ">@14ABC@,DE@^", map3,
                map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 6, 7, 8 };
        map2 = new int[] { 0, 1, 2, 3, 4, -1, 5, 6, 7 };
        map3 = new int[] { 2, 3, 4, 5, 6, 8, 9, 10 };
        test.doTest1("testFullToLean #15 - ", "15ABC@,DE", "15ABC,DE",
                "15ABC@,DE", map1, map2, "15ABC,DE", ">@15ABC@,DE@^", map3,
                map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 6, 7, 8 };
        map2 = new int[] { 0, 1, 2, 3, 4, -1, -1, 5, 6, 7 };
        map3 = new int[] { 2, 3, 4, 5, 6, 8, 9, 10 };
        test.doTest1("testFullToLean #16 - ", "16ABC@@,DE", "16ABC,DE",
                "16ABC@,DE", map1, map2, "16ABC,DE", ">@16ABC@,DE@^", map3,
                map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        map2 = new int[] { 0, 1, 2, 3, 4, 5, 6, -1, 7, 8 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        test.doTest1("testFullToLean #17 - ", "17ABC,@@DE", "17ABC,@DE",
                "17ABC,@DE", map1, map2, "17ABC,@DE", ">@17ABC,@DE@^", map3,
                map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 6, 7, 8, 10, 11, 12, 13 };
        map2 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        map3 = new int[] { 2, 3, 4, 5, 6, 8, 9, 10, 12, 13, 14, 15 };
        test.doTest1("testFullToLean #18 - ", "18ABC,DE,FGH", "18ABC,DE,FGH",
                "18ABC@,DE@,FGH", map1, map2, "18ABC,DE,FGH",
                ">@18ABC@,DE@,FGH@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 6, 7, 8, 10, 11, 12, 13 };
        map2 = new int[] { 0, 1, 2, 3, 4, -1, 5, 6, 7, -1, 8, 9, 10, 11 };
        map3 = new int[] { 2, 3, 4, 5, 6, 8, 9, 10, 12, 13, 14, 15 };
        test.doTest1("testFullToLean #19 - ", "19ABC@,DE@,FGH", "19ABC,DE,FGH",
                "19ABC@,DE@,FGH", map1, map2, "19ABC,DE,FGH",
                ">@19ABC@,DE@,FGH@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
        map2 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        test.doTest1("testFullToLean #20 - ", "20ABC,@DE,@FGH",
                "20ABC,@DE,@FGH", "20ABC,@DE,@FGH", map1, map1,
                "20ABC,@DE,@FGH", ">@20ABC,@DE,@FGH@^", map2, map1);
        map1 = new int[] { 0, 1, 2, 3, 4, 6, 7, 8, 10, 11, 12, 13 };
        map2 = new int[] { 0, 1, 2, 3, 4, -1, -1, 5, 6, 7, -1, -1, 8, 9, 10, 11 };
        map3 = new int[] { 2, 3, 4, 5, 6, 8, 9, 10, 12, 13, 14, 15 };
        test.doTest1("testFullToLean #21 - ", "21ABC@@,DE@@,FGH",
                "21ABC,DE,FGH", "21ABC@,DE@,FGH", map1, map2, "21ABC,DE,FGH",
                ">@21ABC@,DE@,FGH@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
        map2 = new int[] { 0, 1, 2, 3, 4, 5, 6, -1, 7, 8, 9, 10, -1, 11, 12, 13 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        test.doTest1("testFullToLean #22 - ", "22ABC,@@DE,@@FGH",
                "22ABC,@DE,@FGH", "22ABC,@DE,@FGH", map1, map2,
                "22ABC,@DE,@FGH", ">@22ABC,@DE,@FGH@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4 };
        map2 = new int[] { -1, -1, 0, 1, 2, 3, 4, -1, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6 };
        test.doTest1("testFullToLean #23 - ", ">@23abc@^", "23abc", "23abc",
                map1, map2, "23abc", ">@23abc@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4 };
        map2 = new int[] { 0, 1, 2, 3, 4, -1, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6 };
        test.doTest1("testFullToLean #24 - ", "24abc@^", "24abc", "24abc",
                map1, map2, "24abc", ">@24abc@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4 };
        map2 = new int[] { -1, -1, 0, 1, 2, 3, 4 };
        map3 = new int[] { 2, 3, 4, 5, 6 };
        test.doTest1("testFullToLean #25 - ", ">@25abc", "25abc", "25abc",
                map1, map2, "25abc", ">@25abc@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15 };
        map2 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
        map3 = new int[] { 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17 };
        test.doTest1("testFullToLean #26 - ", "26AB,CD@EF,GHI",
                "26AB,CD@EF,GHI", "26AB@,CD@EF@,GHI", map1, map2,
                "26AB,CD@EF,GHI", ">@26AB@,CD@EF@,GHI@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                16, 17 };
        map2 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                15, 16 };
        map3 = new int[] { 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
                18, 19 };
        test.doTest1("testFullToLean #27 - ", "27AB,CD@123ef,GHI",
                "27AB,CD@123ef,GHI", "27AB@,CD@123ef,GHI", map1, map2,
                "27AB,CD@123ef,GHI", ">@27AB@,CD@123ef,GHI@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 6, 7, 8, 10, 11, 12, 13 };
        map2 = new int[] { -1, 0, 1, 2, 3, 4, -1, 5, 6, 7, -1, 8, 9, 10, 11, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 8, 9, 10, 12, 13, 14, 15 };
        test.doTest1("testFullToLean #28 - ", ">28ABC@,DE@,FGH^",
                "28ABC,DE,FGH", "28ABC@,DE@,FGH", map1, map2, "28ABC,DE,FGH",
                ">@28ABC@,DE@,FGH@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 6, 7, 8, 10, 11, 12, 13 };
        map2 = new int[] { -1, -1, 0, 1, 2, 3, 4, -1, 5, 6, 7, -1, 8, 9, 10,
                11, -1, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 8, 9, 10, 12, 13, 14, 15 };
        test.doTest1("testFullToLean #29 - ", ">>29ABC@,DE@,FGH^^",
                "29ABC,DE,FGH", "29ABC@,DE@,FGH", map1, map2, "29ABC,DE,FGH",
                ">@29ABC@,DE@,FGH@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 13, 14, 15 };
        map2 = new int[] { -1, 0, 1, 2, 3, 4, 5, 6, -1, 7, 8, 9, -1, 10, 11,
                12, 13, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 14, 15, 16, 17 };
        test.doTest1("testFullToLean #30 - ", ">30AB>C^@,DE@,FGH^",
                "30AB>C^,DE,FGH", "30AB>C^@,DE@,FGH", map1, map2,
                "30AB>C^,DE,FGH", ">@30AB>C^@,DE@,FGH@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 7, 8, 9, 11, 12, 13, 14 };
        map2 = new int[] { -1, 0, 1, 2, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11,
                12, -1, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 9, 10, 11, 13, 14, 15, 16 };
        test.doTest1("testFullToLean #31 - ", ">31AB>C@,DE@,FGH^^",
                "31AB>C,DE,FGH", "31AB>C@,DE@,FGH", map1, map2,
                "31AB>C,DE,FGH", ">@31AB>C@,DE@,FGH@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 6, 7, 8, 10, 11, 12, 13 };
        map2 = new int[] { -1, -1, 0, 1, 2, 3, 4, -1, 5, 6, 7, -1, 8, 9, 10,
                11, -1, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 8, 9, 10, 12, 13, 14, 15 };
        test.doTest1("testFullToLean #32 - ", ">@32ABC@,DE@,FGH@^",
                "32ABC,DE,FGH", "32ABC@,DE@,FGH", map1, map2, "32ABC,DE,FGH",
                ">@32ABC@,DE@,FGH@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 6, 7, 8, 10, 11, 12, 13 };
        map2 = new int[] { -1, 0, 1, 2, 3, 4, -1, 5, 6, 7, -1, 8, 9, 10, 11,
                -1, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 8, 9, 10, 12, 13, 14, 15 };
        test.doTest1("testFullToLean #33 - ", "@33ABC@,DE@,FGH@^",
                "33ABC,DE,FGH", "33ABC@,DE@,FGH", map1, map2, "33ABC,DE,FGH",
                ">@33ABC@,DE@,FGH@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 6, 7, 8, 10, 11, 12, 13 };
        map2 = new int[] { -1, -1, 0, 1, 2, 3, 4, -1, 5, 6, 7, -1, 8, 9, 10,
                11, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 8, 9, 10, 12, 13, 14, 15 };
        test.doTest1("testFullToLean #34 - ", ">@34ABC@,DE@,FGH@",
                "34ABC,DE,FGH", "34ABC@,DE@,FGH", map1, map2, "34ABC,DE,FGH",
                ">@34ABC@,DE@,FGH@^", map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        map2 = new int[] { 0, 1, 2, 3, 4, 5, -1, 6, 7, 8, -1, -1, 9, 10, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
        test.doTest1("testFullToLean #35 - ", "35ABC@@DE@@@GH@", "35ABC@DE@GH",
                "35ABC@DE@GH", map1, map2, "35ABC@DE@GH", ">@35ABC@DE@GH@^",
                map3, map2);
        map1 = new int[] { 0, 1, 2, 3, 4, 5, 6, 7 };
        map2 = new int[] { 0, 1, 2, 3, 4, 5, -1, 6, 7, -1, -1, -1, -1, -1, -1 };
        map3 = new int[] { 2, 3, 4, 5, 6, 7, 8, 9 };
        test.doTest1("testFullToLean #36 - ", "36ABC@@DE@@@@@@", "36ABC@DE",
                "36ABC@DE", map1, map2, "36ABC@DE", ">@36ABC@DE@^", map3, map2);
        map1 = new int[0];
        map2 = new int[] { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
        test.doTest1("testFullToLean #37 - ", ">>>@@@@@^^^", "", "", map1,
                map2, "", "", map1, map2);

        // test fullToLeanText with initial state
        test.type = BidiStructuredProcessor.StructuredTypes.SQL;
        test.doTest2("testFullToLean #38 - ");

        return test.cntError;
    }
}
