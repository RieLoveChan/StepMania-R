// SYNTHETIC .sma fixture -- hand-authored for tests/test_NotesLoaderSMA.cpp.
//
// No real .sma simfile could be found anywhere (the format is an extinct
// 2009-2011 SMA-editor variant by Aldo Fregoso / Jason Felds). Per the
// maintainer's 2026-09-10 decision, the CURRENT SMALoader read behavior is
// taken as correct, and this file exists only to pin it against regression.
// It deliberately exercises the SMA-distinctive tags that SMLoader does not
// have: #SMAVERSION, #ROWSPERBEAT, #BEATSPERMEASURE, #SPEED (with the "s"
// seconds suffix), #MULTIPLIER. Everything here is invented; there is no
// upstream source to diff against.
#SMAVERSION:1;
#TITLE:SMA Fixture (Synthetic);
#SUBTITLE:pinned;
#ARTIST:Test Fixture;
#TITLETRANSLIT:sma fixture;
#GENRE:test;
#CREDIT:StepMania-R tests;
#MUSIC:fixture.ogg;
#OFFSET:-0.007;
#SAMPLESTART:12.500;
#SAMPLELENGTH:8.000;
#SELECTABLE:YES;
#ROWSPERBEAT:0=4;
#BPMS:0=120.000,8r=150.000;
#STOPS:16r=0.500;
#DELAYS:24=0.250;
#BEATSPERMEASURE:4=3,8=5;
#SPEED:0=1=0,4=2=1,8=0.5=32s;
#MULTIPLIER:0=2,4=3=7;
#TICKCOUNT:2;
#DISPLAYBPM:120:150;
#NOTES:dance-single:Synthetic:Challenge:9:0,0,0,0,0:
0000
0000
0000
0000
,
1000
0000
0100
0000
0010
0000
0001
0000
;
