/*
 *******************************************************************************
 * Copyright (C) 2001, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 * $Source: /xsrl/Nsvn/icu/icu4j/src/com/ibm/icu/dev/test/format/NumberFormatSerialTestData.java,v $ 
 * $Date: 2002/02/19 04:10:24 $ 
 * $Revision: 1.3 $
 *
 *****************************************************************************************
 */

package com.ibm.icu.dev.test.format;

public class NumberFormatSerialTestData {
    //get Content
    public static byte[][] getContent() {
        return content;
    }
   
	//NumberFormat.getInstance(Locale.US)
	static byte[] generalInstance = new byte[]{ 
		-84,-19,0,5,115,114,0,30,99,111,109,46,105,98,109,46,105,99,117,46,
		116,101,120,116,46,68,101,99,105,109,97,108,70,111,114,109,97,116,11,-1,
		3,98,-40,114,48,58,2,0,22,90,0,27,100,101,99,105,109,97,108,83,
		101,112,97,114,97,116,111,114,65,108,119,97,121,115,83,104,111,119,110,90,
		0,23,101,120,112,111,110,101,110,116,83,105,103,110,65,108,119,97,121,115,
		83,104,111,119,110,73,0,11,102,111,114,109,97,116,87,105,100,116,104,66,
		0,12,103,114,111,117,112,105,110,103,83,105,122,101,66,0,13,103,114,111,
		117,112,105,110,103,83,105,122,101,50,66,0,17,109,105,110,69,120,112,111,
		110,101,110,116,68,105,103,105,116,115,73,0,10,109,117,108,116,105,112,108,
		105,101,114,67,0,3,112,97,100,73,0,11,112,97,100,80,111,115,105,116,
		105,111,110,73,0,12,114,111,117,110,100,105,110,103,77,111,100,101,73,0,
		21,115,101,114,105,97,108,86,101,114,115,105,111,110,79,110,83,116,114,101,
		97,109,90,0,22,117,115,101,69,120,112,111,110,101,110,116,105,97,108,78,
		111,116,97,116,105,111,110,76,0,16,110,101,103,80,114,101,102,105,120,80,
		97,116,116,101,114,110,116,0,18,76,106,97,118,97,47,108,97,110,103,47,
		83,116,114,105,110,103,59,76,0,16,110,101,103,83,117,102,102,105,120,80,
		97,116,116,101,114,110,113,0,126,0,1,76,0,14,110,101,103,97,116,105,
		118,101,80,114,101,102,105,120,113,0,126,0,1,76,0,14,110,101,103,97,
		116,105,118,101,83,117,102,102,105,120,113,0,126,0,1,76,0,16,112,111,
		115,80,114,101,102,105,120,80,97,116,116,101,114,110,113,0,126,0,1,76,
		0,16,112,111,115,83,117,102,102,105,120,80,97,116,116,101,114,110,113,0,
		126,0,1,76,0,14,112,111,115,105,116,105,118,101,80,114,101,102,105,120,
		113,0,126,0,1,76,0,14,112,111,115,105,116,105,118,101,83,117,102,102,
		105,120,113,0,126,0,1,76,0,17,114,111,117,110,100,105,110,103,73,110,
		99,114,101,109,101,110,116,116,0,22,76,106,97,118,97,47,109,97,116,104,
		47,66,105,103,68,101,99,105,109,97,108,59,76,0,7,115,121,109,98,111,
		108,115,116,0,39,76,99,111,109,47,105,98,109,47,105,99,117,47,116,101,
		120,116,47,68,101,99,105,109,97,108,70,111,114,109,97,116,83,121,109,98,
		111,108,115,59,120,114,0,29,99,111,109,46,105,98,109,46,105,99,117,46,
		116,101,120,116,46,78,117,109,98,101,114,70,111,114,109,97,116,-33,-10,-77,
		-65,19,125,7,-24,3,0,11,90,0,12,103,114,111,117,112,105,110,103,85,
		115,101,100,66,0,17,109,97,120,70,114,97,99,116,105,111,110,68,105,103,
		105,116,115,66,0,16,109,97,120,73,110,116,101,103,101,114,68,105,103,105,
		116,115,73,0,21,109,97,120,105,109,117,109,70,114,97,99,116,105,111,110,
		68,105,103,105,116,115,73,0,20,109,97,120,105,109,117,109,73,110,116,101,
		103,101,114,68,105,103,105,116,115,66,0,17,109,105,110,70,114,97,99,116,
		105,111,110,68,105,103,105,116,115,66,0,16,109,105,110,73,110,116,101,103,
		101,114,68,105,103,105,116,115,73,0,21,109,105,110,105,109,117,109,70,114,
		97,99,116,105,111,110,68,105,103,105,116,115,73,0,20,109,105,110,105,109,
		117,109,73,110,116,101,103,101,114,68,105,103,105,116,115,90,0,16,112,97,
		114,115,101,73,110,116,101,103,101,114,79,110,108,121,73,0,21,115,101,114,
		105,97,108,86,101,114,115,105,111,110,79,110,83,116,114,101,97,109,120,114,
		0,16,106,97,118,97,46,116,101,120,116,46,70,111,114,109,97,116,-5,-40,
		-68,18,-23,15,24,67,2,0,0,120,112,1,3,127,0,0,0,3,0,0,
		1,53,0,1,0,0,0,0,0,0,0,1,0,0,0,0,1,120,0,0,
		0,0,0,0,3,0,0,0,0,0,1,0,32,0,0,0,0,0,0,0,
		6,0,0,0,2,0,116,0,1,45,116,0,0,116,0,1,45,116,0,0,
		116,0,0,116,0,0,116,0,0,116,0,0,112,115,114,0,37,99,111,109,
		46,105,98,109,46,105,99,117,46,116,101,120,116,46,68,101,99,105,109,97,
		108,70,111,114,109,97,116,83,121,109,98,111,108,115,80,29,23,-103,8,104,
		-109,-100,2,0,18,67,0,16,100,101,99,105,109,97,108,83,101,112,97,114,
		97,116,111,114,67,0,5,100,105,103,105,116,67,0,11,101,120,112,111,110,
		101,110,116,105,97,108,67,0,17,103,114,111,117,112,105,110,103,83,101,112,
		97,114,97,116,111,114,67,0,9,109,105,110,117,115,83,105,103,110,67,0,
		17,109,111,110,101,116,97,114,121,83,101,112,97,114,97,116,111,114,67,0,
		9,112,97,100,69,115,99,97,112,101,67,0,16,112,97,116,116,101,114,110,
		83,101,112,97,114,97,116,111,114,67,0,7,112,101,114,77,105,108,108,67,
		0,7,112,101,114,99,101,110,116,67,0,8,112,108,117,115,83,105,103,110,
		73,0,21,115,101,114,105,97,108,86,101,114,115,105,111,110,79,110,83,116,
		114,101,97,109,67,0,9,122,101,114,111,68,105,103,105,116,76,0,3,78,
		97,78,113,0,126,0,1,76,0,14,99,117,114,114,101,110,99,121,83,121,
		109,98,111,108,113,0,126,0,1,76,0,17,101,120,112,111,110,101,110,116,
		83,101,112,97,114,97,116,111,114,113,0,126,0,1,76,0,8,105,110,102,
		105,110,105,116,121,113,0,126,0,1,76,0,18,105,110,116,108,67,117,114,
		114,101,110,99,121,83,121,109,98,111,108,113,0,126,0,1,120,112,0,46,
		0,35,0,0,0,44,0,45,0,46,0,42,0,59,32,48,0,37,0,43,
		0,0,0,2,0,48,116,0,3,-17,-65,-67,116,0,1,36,116,0,1,69,
		116,0,3,-30,-120,-98,116,0,3,85,83,68,
	};
	//NumberFormat.getCurrencyInstance(Locale.US)
	static byte[] currencyInstance = new byte[]{ 
		-84,-19,0,5,115,114,0,30,99,111,109,46,105,98,109,46,105,99,117,46,
		116,101,120,116,46,68,101,99,105,109,97,108,70,111,114,109,97,116,11,-1,
		3,98,-40,114,48,58,2,0,22,90,0,27,100,101,99,105,109,97,108,83,
		101,112,97,114,97,116,111,114,65,108,119,97,121,115,83,104,111,119,110,90,
		0,23,101,120,112,111,110,101,110,116,83,105,103,110,65,108,119,97,121,115,
		83,104,111,119,110,73,0,11,102,111,114,109,97,116,87,105,100,116,104,66,
		0,12,103,114,111,117,112,105,110,103,83,105,122,101,66,0,13,103,114,111,
		117,112,105,110,103,83,105,122,101,50,66,0,17,109,105,110,69,120,112,111,
		110,101,110,116,68,105,103,105,116,115,73,0,10,109,117,108,116,105,112,108,
		105,101,114,67,0,3,112,97,100,73,0,11,112,97,100,80,111,115,105,116,
		105,111,110,73,0,12,114,111,117,110,100,105,110,103,77,111,100,101,73,0,
		21,115,101,114,105,97,108,86,101,114,115,105,111,110,79,110,83,116,114,101,
		97,109,90,0,22,117,115,101,69,120,112,111,110,101,110,116,105,97,108,78,
		111,116,97,116,105,111,110,76,0,16,110,101,103,80,114,101,102,105,120,80,
		97,116,116,101,114,110,116,0,18,76,106,97,118,97,47,108,97,110,103,47,
		83,116,114,105,110,103,59,76,0,16,110,101,103,83,117,102,102,105,120,80,
		97,116,116,101,114,110,113,0,126,0,1,76,0,14,110,101,103,97,116,105,
		118,101,80,114,101,102,105,120,113,0,126,0,1,76,0,14,110,101,103,97,
		116,105,118,101,83,117,102,102,105,120,113,0,126,0,1,76,0,16,112,111,
		115,80,114,101,102,105,120,80,97,116,116,101,114,110,113,0,126,0,1,76,
		0,16,112,111,115,83,117,102,102,105,120,80,97,116,116,101,114,110,113,0,
		126,0,1,76,0,14,112,111,115,105,116,105,118,101,80,114,101,102,105,120,
		113,0,126,0,1,76,0,14,112,111,115,105,116,105,118,101,83,117,102,102,
		105,120,113,0,126,0,1,76,0,17,114,111,117,110,100,105,110,103,73,110,
		99,114,101,109,101,110,116,116,0,22,76,106,97,118,97,47,109,97,116,104,
		47,66,105,103,68,101,99,105,109,97,108,59,76,0,7,115,121,109,98,111,
		108,115,116,0,39,76,99,111,109,47,105,98,109,47,105,99,117,47,116,101,
		120,116,47,68,101,99,105,109,97,108,70,111,114,109,97,116,83,121,109,98,
		111,108,115,59,120,114,0,29,99,111,109,46,105,98,109,46,105,99,117,46,
		116,101,120,116,46,78,117,109,98,101,114,70,111,114,109,97,116,-33,-10,-77,
		-65,19,125,7,-24,3,0,11,90,0,12,103,114,111,117,112,105,110,103,85,
		115,101,100,66,0,17,109,97,120,70,114,97,99,116,105,111,110,68,105,103,
		105,116,115,66,0,16,109,97,120,73,110,116,101,103,101,114,68,105,103,105,
		116,115,73,0,21,109,97,120,105,109,117,109,70,114,97,99,116,105,111,110,
		68,105,103,105,116,115,73,0,20,109,97,120,105,109,117,109,73,110,116,101,
		103,101,114,68,105,103,105,116,115,66,0,17,109,105,110,70,114,97,99,116,
		105,111,110,68,105,103,105,116,115,66,0,16,109,105,110,73,110,116,101,103,
		101,114,68,105,103,105,116,115,73,0,21,109,105,110,105,109,117,109,70,114,
		97,99,116,105,111,110,68,105,103,105,116,115,73,0,20,109,105,110,105,109,
		117,109,73,110,116,101,103,101,114,68,105,103,105,116,115,90,0,16,112,97,
		114,115,101,73,110,116,101,103,101,114,79,110,108,121,73,0,21,115,101,114,
		105,97,108,86,101,114,115,105,111,110,79,110,83,116,114,101,97,109,120,114,
		0,16,106,97,118,97,46,116,101,120,116,46,70,111,114,109,97,116,-5,-40,
		-68,18,-23,15,24,67,2,0,0,120,112,1,2,127,0,0,0,2,0,0,
		1,53,2,1,0,0,0,2,0,0,0,1,0,0,0,0,1,120,0,0,
		0,0,0,0,3,0,0,0,0,0,1,0,32,0,0,0,0,0,0,0,
		6,0,0,0,2,0,116,0,3,40,-62,-92,116,0,1,41,116,0,2,40,
		36,116,0,1,41,116,0,2,-62,-92,116,0,0,116,0,1,36,116,0,0,
		112,115,114,0,37,99,111,109,46,105,98,109,46,105,99,117,46,116,101,120,
		116,46,68,101,99,105,109,97,108,70,111,114,109,97,116,83,121,109,98,111,
		108,115,80,29,23,-103,8,104,-109,-100,2,0,18,67,0,16,100,101,99,105,
		109,97,108,83,101,112,97,114,97,116,111,114,67,0,5,100,105,103,105,116,
		67,0,11,101,120,112,111,110,101,110,116,105,97,108,67,0,17,103,114,111,
		117,112,105,110,103,83,101,112,97,114,97,116,111,114,67,0,9,109,105,110,
		117,115,83,105,103,110,67,0,17,109,111,110,101,116,97,114,121,83,101,112,
		97,114,97,116,111,114,67,0,9,112,97,100,69,115,99,97,112,101,67,0,
		16,112,97,116,116,101,114,110,83,101,112,97,114,97,116,111,114,67,0,7,
		112,101,114,77,105,108,108,67,0,7,112,101,114,99,101,110,116,67,0,8,
		112,108,117,115,83,105,103,110,73,0,21,115,101,114,105,97,108,86,101,114,
		115,105,111,110,79,110,83,116,114,101,97,109,67,0,9,122,101,114,111,68,
		105,103,105,116,76,0,3,78,97,78,113,0,126,0,1,76,0,14,99,117,
		114,114,101,110,99,121,83,121,109,98,111,108,113,0,126,0,1,76,0,17,
		101,120,112,111,110,101,110,116,83,101,112,97,114,97,116,111,114,113,0,126,
		0,1,76,0,8,105,110,102,105,110,105,116,121,113,0,126,0,1,76,0,
		18,105,110,116,108,67,117,114,114,101,110,99,121,83,121,109,98,111,108,113,
		0,126,0,1,120,112,0,46,0,35,0,0,0,44,0,45,0,46,0,42,
		0,59,32,48,0,37,0,43,0,0,0,2,0,48,116,0,3,-17,-65,-67,
		116,0,1,36,116,0,1,69,116,0,3,-30,-120,-98,116,0,3,85,83,68,
		
	};
	//NumberFormat.getPercentInstance(Locale.US)
	static byte[] percentInstance = new byte[]{ 
		-84,-19,0,5,115,114,0,30,99,111,109,46,105,98,109,46,105,99,117,46,
		116,101,120,116,46,68,101,99,105,109,97,108,70,111,114,109,97,116,11,-1,
		3,98,-40,114,48,58,2,0,22,90,0,27,100,101,99,105,109,97,108,83,
		101,112,97,114,97,116,111,114,65,108,119,97,121,115,83,104,111,119,110,90,
		0,23,101,120,112,111,110,101,110,116,83,105,103,110,65,108,119,97,121,115,
		83,104,111,119,110,73,0,11,102,111,114,109,97,116,87,105,100,116,104,66,
		0,12,103,114,111,117,112,105,110,103,83,105,122,101,66,0,13,103,114,111,
		117,112,105,110,103,83,105,122,101,50,66,0,17,109,105,110,69,120,112,111,
		110,101,110,116,68,105,103,105,116,115,73,0,10,109,117,108,116,105,112,108,
		105,101,114,67,0,3,112,97,100,73,0,11,112,97,100,80,111,115,105,116,
		105,111,110,73,0,12,114,111,117,110,100,105,110,103,77,111,100,101,73,0,
		21,115,101,114,105,97,108,86,101,114,115,105,111,110,79,110,83,116,114,101,
		97,109,90,0,22,117,115,101,69,120,112,111,110,101,110,116,105,97,108,78,
		111,116,97,116,105,111,110,76,0,16,110,101,103,80,114,101,102,105,120,80,
		97,116,116,101,114,110,116,0,18,76,106,97,118,97,47,108,97,110,103,47,
		83,116,114,105,110,103,59,76,0,16,110,101,103,83,117,102,102,105,120,80,
		97,116,116,101,114,110,113,0,126,0,1,76,0,14,110,101,103,97,116,105,
		118,101,80,114,101,102,105,120,113,0,126,0,1,76,0,14,110,101,103,97,
		116,105,118,101,83,117,102,102,105,120,113,0,126,0,1,76,0,16,112,111,
		115,80,114,101,102,105,120,80,97,116,116,101,114,110,113,0,126,0,1,76,
		0,16,112,111,115,83,117,102,102,105,120,80,97,116,116,101,114,110,113,0,
		126,0,1,76,0,14,112,111,115,105,116,105,118,101,80,114,101,102,105,120,
		113,0,126,0,1,76,0,14,112,111,115,105,116,105,118,101,83,117,102,102,
		105,120,113,0,126,0,1,76,0,17,114,111,117,110,100,105,110,103,73,110,
		99,114,101,109,101,110,116,116,0,22,76,106,97,118,97,47,109,97,116,104,
		47,66,105,103,68,101,99,105,109,97,108,59,76,0,7,115,121,109,98,111,
		108,115,116,0,39,76,99,111,109,47,105,98,109,47,105,99,117,47,116,101,
		120,116,47,68,101,99,105,109,97,108,70,111,114,109,97,116,83,121,109,98,
		111,108,115,59,120,114,0,29,99,111,109,46,105,98,109,46,105,99,117,46,
		116,101,120,116,46,78,117,109,98,101,114,70,111,114,109,97,116,-33,-10,-77,
		-65,19,125,7,-24,3,0,11,90,0,12,103,114,111,117,112,105,110,103,85,
		115,101,100,66,0,17,109,97,120,70,114,97,99,116,105,111,110,68,105,103,
		105,116,115,66,0,16,109,97,120,73,110,116,101,103,101,114,68,105,103,105,
		116,115,73,0,21,109,97,120,105,109,117,109,70,114,97,99,116,105,111,110,
		68,105,103,105,116,115,73,0,20,109,97,120,105,109,117,109,73,110,116,101,
		103,101,114,68,105,103,105,116,115,66,0,17,109,105,110,70,114,97,99,116,
		105,111,110,68,105,103,105,116,115,66,0,16,109,105,110,73,110,116,101,103,
		101,114,68,105,103,105,116,115,73,0,21,109,105,110,105,109,117,109,70,114,
		97,99,116,105,111,110,68,105,103,105,116,115,73,0,20,109,105,110,105,109,
		117,109,73,110,116,101,103,101,114,68,105,103,105,116,115,90,0,16,112,97,
		114,115,101,73,110,116,101,103,101,114,79,110,108,121,73,0,21,115,101,114,
		105,97,108,86,101,114,115,105,111,110,79,110,83,116,114,101,97,109,120,114,
		0,16,106,97,118,97,46,116,101,120,116,46,70,111,114,109,97,116,-5,-40,
		-68,18,-23,15,24,67,2,0,0,120,112,1,0,127,0,0,0,0,0,0,
		1,53,0,1,0,0,0,0,0,0,0,1,0,0,0,0,1,120,0,0,
		0,0,0,0,3,0,0,0,0,0,100,0,32,0,0,0,0,0,0,0,
		6,0,0,0,2,0,116,0,1,45,116,0,1,37,116,0,1,45,116,0,
		1,37,116,0,0,113,0,126,0,8,116,0,0,116,0,1,37,112,115,114,
		0,37,99,111,109,46,105,98,109,46,105,99,117,46,116,101,120,116,46,68,
		101,99,105,109,97,108,70,111,114,109,97,116,83,121,109,98,111,108,115,80,
		29,23,-103,8,104,-109,-100,2,0,18,67,0,16,100,101,99,105,109,97,108,
		83,101,112,97,114,97,116,111,114,67,0,5,100,105,103,105,116,67,0,11,
		101,120,112,111,110,101,110,116,105,97,108,67,0,17,103,114,111,117,112,105,
		110,103,83,101,112,97,114,97,116,111,114,67,0,9,109,105,110,117,115,83,
		105,103,110,67,0,17,109,111,110,101,116,97,114,121,83,101,112,97,114,97,
		116,111,114,67,0,9,112,97,100,69,115,99,97,112,101,67,0,16,112,97,
		116,116,101,114,110,83,101,112,97,114,97,116,111,114,67,0,7,112,101,114,
		77,105,108,108,67,0,7,112,101,114,99,101,110,116,67,0,8,112,108,117,
		115,83,105,103,110,73,0,21,115,101,114,105,97,108,86,101,114,115,105,111,
		110,79,110,83,116,114,101,97,109,67,0,9,122,101,114,111,68,105,103,105,
		116,76,0,3,78,97,78,113,0,126,0,1,76,0,14,99,117,114,114,101,
		110,99,121,83,121,109,98,111,108,113,0,126,0,1,76,0,17,101,120,112,
		111,110,101,110,116,83,101,112,97,114,97,116,111,114,113,0,126,0,1,76,
		0,8,105,110,102,105,110,105,116,121,113,0,126,0,1,76,0,18,105,110,
		116,108,67,117,114,114,101,110,99,121,83,121,109,98,111,108,113,0,126,0,
		1,120,112,0,46,0,35,0,0,0,44,0,45,0,46,0,42,0,59,32,
		48,0,37,0,43,0,0,0,2,0,48,116,0,3,-17,-65,-67,116,0,1,
		36,116,0,1,69,116,0,3,-30,-120,-98,116,0,3,85,83,68,
	};
	//NumberFormat.getScientificInstance(Locale.US)
	static byte[] scientificInstance = new byte[]{ 
		-84,-19,0,5,115,114,0,30,99,111,109,46,105,98,109,46,105,99,117,46,
		116,101,120,116,46,68,101,99,105,109,97,108,70,111,114,109,97,116,11,-1,
		3,98,-40,114,48,58,2,0,22,90,0,27,100,101,99,105,109,97,108,83,
		101,112,97,114,97,116,111,114,65,108,119,97,121,115,83,104,111,119,110,90,
		0,23,101,120,112,111,110,101,110,116,83,105,103,110,65,108,119,97,121,115,
		83,104,111,119,110,73,0,11,102,111,114,109,97,116,87,105,100,116,104,66,
		0,12,103,114,111,117,112,105,110,103,83,105,122,101,66,0,13,103,114,111,
		117,112,105,110,103,83,105,122,101,50,66,0,17,109,105,110,69,120,112,111,
		110,101,110,116,68,105,103,105,116,115,73,0,10,109,117,108,116,105,112,108,
		105,101,114,67,0,3,112,97,100,73,0,11,112,97,100,80,111,115,105,116,
		105,111,110,73,0,12,114,111,117,110,100,105,110,103,77,111,100,101,73,0,
		21,115,101,114,105,97,108,86,101,114,115,105,111,110,79,110,83,116,114,101,
		97,109,90,0,22,117,115,101,69,120,112,111,110,101,110,116,105,97,108,78,
		111,116,97,116,105,111,110,76,0,16,110,101,103,80,114,101,102,105,120,80,
		97,116,116,101,114,110,116,0,18,76,106,97,118,97,47,108,97,110,103,47,
		83,116,114,105,110,103,59,76,0,16,110,101,103,83,117,102,102,105,120,80,
		97,116,116,101,114,110,113,0,126,0,1,76,0,14,110,101,103,97,116,105,
		118,101,80,114,101,102,105,120,113,0,126,0,1,76,0,14,110,101,103,97,
		116,105,118,101,83,117,102,102,105,120,113,0,126,0,1,76,0,16,112,111,
		115,80,114,101,102,105,120,80,97,116,116,101,114,110,113,0,126,0,1,76,
		0,16,112,111,115,83,117,102,102,105,120,80,97,116,116,101,114,110,113,0,
		126,0,1,76,0,14,112,111,115,105,116,105,118,101,80,114,101,102,105,120,
		113,0,126,0,1,76,0,14,112,111,115,105,116,105,118,101,83,117,102,102,
		105,120,113,0,126,0,1,76,0,17,114,111,117,110,100,105,110,103,73,110,
		99,114,101,109,101,110,116,116,0,22,76,106,97,118,97,47,109,97,116,104,
		47,66,105,103,68,101,99,105,109,97,108,59,76,0,7,115,121,109,98,111,
		108,115,116,0,39,76,99,111,109,47,105,98,109,47,105,99,117,47,116,101,
		120,116,47,68,101,99,105,109,97,108,70,111,114,109,97,116,83,121,109,98,
		111,108,115,59,120,114,0,29,99,111,109,46,105,98,109,46,105,99,117,46,
		116,101,120,116,46,78,117,109,98,101,114,70,111,114,109,97,116,-33,-10,-77,
		-65,19,125,7,-24,3,0,11,90,0,12,103,114,111,117,112,105,110,103,85,
		115,101,100,66,0,17,109,97,120,70,114,97,99,116,105,111,110,68,105,103,
		105,116,115,66,0,16,109,97,120,73,110,116,101,103,101,114,68,105,103,105,
		116,115,73,0,21,109,97,120,105,109,117,109,70,114,97,99,116,105,111,110,
		68,105,103,105,116,115,73,0,20,109,97,120,105,109,117,109,73,110,116,101,
		103,101,114,68,105,103,105,116,115,66,0,17,109,105,110,70,114,97,99,116,
		105,111,110,68,105,103,105,116,115,66,0,16,109,105,110,73,110,116,101,103,
		101,114,68,105,103,105,116,115,73,0,21,109,105,110,105,109,117,109,70,114,
		97,99,116,105,111,110,68,105,103,105,116,115,73,0,20,109,105,110,105,109,
		117,109,73,110,116,101,103,101,114,68,105,103,105,116,115,90,0,16,112,97,
		114,115,101,73,110,116,101,103,101,114,79,110,108,121,73,0,21,115,101,114,
		105,97,108,86,101,114,115,105,111,110,79,110,83,116,114,101,97,109,120,114,
		0,16,106,97,118,97,46,116,101,120,116,46,70,111,114,109,97,116,-5,-40,
		-68,18,-23,15,24,67,2,0,0,120,112,0,0,1,0,0,0,0,0,0,
		0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,120,0,0,
		0,0,0,0,0,0,1,0,0,0,1,0,32,0,0,0,0,0,0,0,
		6,0,0,0,2,1,116,0,1,45,116,0,0,116,0,1,45,116,0,0,
		116,0,0,113,0,126,0,8,116,0,0,116,0,0,112,115,114,0,37,99,
		111,109,46,105,98,109,46,105,99,117,46,116,101,120,116,46,68,101,99,105,
		109,97,108,70,111,114,109,97,116,83,121,109,98,111,108,115,80,29,23,-103,
		8,104,-109,-100,2,0,18,67,0,16,100,101,99,105,109,97,108,83,101,112,
		97,114,97,116,111,114,67,0,5,100,105,103,105,116,67,0,11,101,120,112,
		111,110,101,110,116,105,97,108,67,0,17,103,114,111,117,112,105,110,103,83,
		101,112,97,114,97,116,111,114,67,0,9,109,105,110,117,115,83,105,103,110,
		67,0,17,109,111,110,101,116,97,114,121,83,101,112,97,114,97,116,111,114,
		67,0,9,112,97,100,69,115,99,97,112,101,67,0,16,112,97,116,116,101,
		114,110,83,101,112,97,114,97,116,111,114,67,0,7,112,101,114,77,105,108,
		108,67,0,7,112,101,114,99,101,110,116,67,0,8,112,108,117,115,83,105,
		103,110,73,0,21,115,101,114,105,97,108,86,101,114,115,105,111,110,79,110,
		83,116,114,101,97,109,67,0,9,122,101,114,111,68,105,103,105,116,76,0,
		3,78,97,78,113,0,126,0,1,76,0,14,99,117,114,114,101,110,99,121,
		83,121,109,98,111,108,113,0,126,0,1,76,0,17,101,120,112,111,110,101,
		110,116,83,101,112,97,114,97,116,111,114,113,0,126,0,1,76,0,8,105,
		110,102,105,110,105,116,121,113,0,126,0,1,76,0,18,105,110,116,108,67,
		117,114,114,101,110,99,121,83,121,109,98,111,108,113,0,126,0,1,120,112,
		0,46,0,35,0,0,0,44,0,45,0,46,0,42,0,59,32,48,0,37,
		0,43,0,0,0,2,0,48,116,0,3,-17,-65,-67,116,0,1,36,116,0,
		1,69,116,0,3,-30,-120,-98,116,0,3,85,83,68,
	};

    
    //content
    final static byte[][] content = {generalInstance, currencyInstance, percentInstance, scientificInstance};
}