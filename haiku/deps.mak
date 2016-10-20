PlatHaiku.o: PlatHaiku.cxx ../include/Platform.h ../include/Scintilla.h \
 ../include/Sci_Position.h ../src/UniConversion.h ../src/XPM.h
ScintillaHaiku.o: ScintillaHaiku.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/Sci_Position.h ScintillaView.h \
 ../include/ILexer.h ../include/SciLexer.h ../lexlib/PropSetSimple.h \
 ../lexlib/LexAccessor.h ../lexlib/Accessor.h ../lexlib/LexerModule.h \
 ../src/ExternalLexer.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/ContractionState.h ../src/CellBuffer.h \
 ../src/CallTip.h ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h \
 ../src/LineMarker.h ../src/Style.h ../src/AutoComplete.h \
 ../src/ViewStyle.h ../src/CharClassify.h ../src/CaseFolder.h \
 ../src/Decoration.h ../src/Document.h ../src/Selection.h \
 ../src/PositionCache.h ../src/EditModel.h ../src/MarginView.h \
 ../src/EditView.h ../src/Editor.h ../src/ScintillaBase.h
AutoComplete.o: ../src/AutoComplete.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/Sci_Position.h \
 ../lexlib/CharacterSet.h ../src/Position.h ../src/AutoComplete.h
CallTip.o: ../src/CallTip.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/Sci_Position.h ../lexlib/StringCopy.h \
 ../src/Position.h ../src/CallTip.h
CaseConvert.o: ../src/CaseConvert.cxx ../lexlib/StringCopy.h \
 ../src/CaseConvert.h ../src/UniConversion.h ../src/UnicodeFromUTF8.h
CaseFolder.o: ../src/CaseFolder.cxx ../src/CaseFolder.h \
 ../src/CaseConvert.h ../src/UniConversion.h
Catalogue.o: ../src/Catalogue.cxx ../include/ILexer.h \
 ../include/Sci_Position.h ../include/Scintilla.h ../include/SciLexer.h \
 ../lexlib/LexerModule.h ../src/Catalogue.h
CellBuffer.o: ../src/CellBuffer.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/Sci_Position.h ../src/Position.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/CellBuffer.h \
 ../src/UniConversion.h
CharClassify.o: ../src/CharClassify.cxx ../src/CharClassify.h
ContractionState.o: ../src/ContractionState.cxx ../include/Platform.h \
 ../src/Position.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/ContractionState.h
Decoration.o: ../src/Decoration.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/Sci_Position.h ../src/Position.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/Decoration.h
Document.o: ../src/Document.cxx ../include/Platform.h ../include/ILexer.h \
 ../include/Sci_Position.h ../include/Scintilla.h \
 ../lexlib/CharacterSet.h ../src/Position.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/CellBuffer.h \
 ../src/PerLine.h ../src/CharClassify.h ../src/Decoration.h \
 ../src/CaseFolder.h ../src/Document.h ../src/RESearch.h \
 ../src/UniConversion.h ../src/UnicodeFromUTF8.h
EditModel.o: ../src/EditModel.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Sci_Position.h ../include/Scintilla.h \
 ../lexlib/StringCopy.h ../src/Position.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/ContractionState.h \
 ../src/CellBuffer.h ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h \
 ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/UniConversion.h ../src/Selection.h \
 ../src/PositionCache.h ../src/EditModel.h
Editor.o: ../src/Editor.cxx ../include/Platform.h ../include/ILexer.h \
 ../include/Sci_Position.h ../include/Scintilla.h ../lexlib/StringCopy.h \
 ../src/Position.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/ContractionState.h ../src/CellBuffer.h \
 ../src/PerLine.h ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h \
 ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/UniConversion.h ../src/Selection.h \
 ../src/PositionCache.h ../src/EditModel.h ../src/MarginView.h \
 ../src/EditView.h ../src/Editor.h
EditView.o: ../src/EditView.cxx ../include/Platform.h ../include/ILexer.h \
 ../include/Sci_Position.h ../include/Scintilla.h ../lexlib/StringCopy.h \
 ../src/Position.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/ContractionState.h ../src/CellBuffer.h \
 ../src/PerLine.h ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h \
 ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/UniConversion.h ../src/Selection.h \
 ../src/PositionCache.h ../src/EditModel.h ../src/MarginView.h \
 ../src/EditView.h
ExternalLexer.o: ../src/ExternalLexer.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Sci_Position.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/LexerModule.h ../src/Catalogue.h \
 ../src/ExternalLexer.h
Indicator.o: ../src/Indicator.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/Sci_Position.h ../src/Indicator.h \
 ../src/XPM.h
KeyMap.o: ../src/KeyMap.cxx ../include/Platform.h ../include/Scintilla.h \
 ../include/Sci_Position.h ../src/KeyMap.h
LineMarker.o: ../src/LineMarker.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/Sci_Position.h ../lexlib/StringCopy.h \
 ../src/XPM.h ../src/LineMarker.h
MarginView.o: ../src/MarginView.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Sci_Position.h ../include/Scintilla.h \
 ../lexlib/StringCopy.h ../src/Position.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/ContractionState.h \
 ../src/CellBuffer.h ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h \
 ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/UniConversion.h ../src/Selection.h \
 ../src/PositionCache.h ../src/EditModel.h ../src/MarginView.h \
 ../src/EditView.h
PerLine.o: ../src/PerLine.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/Sci_Position.h ../src/Position.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/CellBuffer.h \
 ../src/PerLine.h
PositionCache.o: ../src/PositionCache.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Sci_Position.h ../include/Scintilla.h \
 ../src/Position.h ../src/SplitVector.h ../src/Partitioning.h \
 ../src/RunStyles.h ../src/ContractionState.h ../src/CellBuffer.h \
 ../src/KeyMap.h ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h \
 ../src/Style.h ../src/ViewStyle.h ../src/CharClassify.h \
 ../src/Decoration.h ../src/CaseFolder.h ../src/Document.h \
 ../src/UniConversion.h ../src/Selection.h ../src/PositionCache.h
RESearch.o: ../src/RESearch.cxx ../src/Position.h ../src/CharClassify.h \
 ../src/RESearch.h
RunStyles.o: ../src/RunStyles.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/Sci_Position.h ../src/Position.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h
ScintillaBase.o: ../src/ScintillaBase.cxx ../include/Platform.h \
 ../include/ILexer.h ../include/Sci_Position.h ../include/Scintilla.h \
 ../include/SciLexer.h ../lexlib/PropSetSimple.h ../lexlib/LexerModule.h \
 ../src/Catalogue.h ../src/Position.h ../src/SplitVector.h \
 ../src/Partitioning.h ../src/RunStyles.h ../src/ContractionState.h \
 ../src/CellBuffer.h ../src/CallTip.h ../src/KeyMap.h ../src/Indicator.h \
 ../src/XPM.h ../src/LineMarker.h ../src/Style.h ../src/ViewStyle.h \
 ../src/CharClassify.h ../src/Decoration.h ../src/CaseFolder.h \
 ../src/Document.h ../src/Selection.h ../src/PositionCache.h \
 ../src/EditModel.h ../src/MarginView.h ../src/EditView.h ../src/Editor.h \
 ../src/AutoComplete.h ../src/ScintillaBase.h
Selection.o: ../src/Selection.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/Sci_Position.h ../src/Position.h \
 ../src/Selection.h
Style.o: ../src/Style.cxx ../include/Platform.h ../include/Scintilla.h \
 ../include/Sci_Position.h ../src/Style.h
UniConversion.o: ../src/UniConversion.cxx ../src/UniConversion.h
ViewStyle.o: ../src/ViewStyle.cxx ../include/Platform.h \
 ../include/Scintilla.h ../include/Sci_Position.h ../src/Position.h \
 ../src/SplitVector.h ../src/Partitioning.h ../src/RunStyles.h \
 ../src/Indicator.h ../src/XPM.h ../src/LineMarker.h ../src/Style.h \
 ../src/ViewStyle.h
XPM.o: ../src/XPM.cxx ../include/Platform.h ../src/XPM.h
