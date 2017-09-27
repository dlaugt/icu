// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

void testToPatternSimple() {
    const char16_t *cases[][2] = {{u"#", u"0"},
                                  {u"0", u"0"},
                                  {u"#0", u"0"},
                                  {u"###", u"0"},
                                  {u"0.##", u"0.##"},
                                  {u"0.00", u"0.00"},
                                  {u"0.00#", u"0.00#"},
                                  {u"#E0", u"#E0"},
                                  {u"0E0", u"0E0"},
                                  {u"#00E00", u"#00E00"},
                                  {u"#,##0", u"#,##0"},
                                  {u"#;#", u"0;0"},
            // ignore a negative prefix pattern of '-' since that is the default:
                                  {u"#;-#", u"0"},
                                  {u"**##0", u"**##0"},
                                  {u"*'x'##0", u"*x##0"},
                                  {u"a''b0", u"a''b0"},
                                  {u"*''##0", u"*''##0"},
                                  {u"*📺##0", u"*'📺'##0"},
                                  {u"*'நி'##0", u"*'நி'##0"},};

    UErrorCode status = U_ZERO_ERROR;
    for (const char16_t **cas : cases) {
        UnicodeString input(cas[0]);
        UnicodeString output(cas[1]);

        DecimalFormatProperties properties = PatternParser::parseToProperties(
                input, PatternParser::IGNORE_ROUNDING_NEVER, status);
        assertSuccess(input, status);
        UnicodeString actual = PatternStringUtils::propertiesToPatternString(properties, status);
        assertEquals(input, output, actual);
    }
}

void testExceptionOnInvalid() {
    static const char16_t *invalidPatterns[] = {
            u"#.#.#",
            u"0#",
            u"0#.",
            u".#0",
            u"0#.#0",
            u"@0",
            u"0@",
            u"0,",
            u"0,,",
            u"0,,0",
            u"0,,0,",
            u"#,##0E0"};

    for (auto pattern : invalidPatterns) {
        UErrorCode status = U_ZERO_ERROR;
        ParsedPatternInfo patternInfo;
        PatternParser::parseToPatternInfo(pattern, patternInfo, status);
        assertTrue(pattern, U_FAILURE(status));
    }
}

void testBug13117() {
    UErrorCode status = U_ZERO_ERROR;
    DecimalFormatProperties expected = PatternParser::parseToProperties(
            u"0",
            PatternParser::IGNORE_ROUNDING_NEVER,
            status);
    DecimalFormatProperties actual = PatternParser::parseToProperties(
            u"0;",
            PatternParser::IGNORE_ROUNDING_NEVER,
            status);
    assertSuccess("Spot 1", status);
    assertTrue("Should not consume negative subpattern", expected == actual);
}
