/*
 * Current issues with this test:
 *
 *  - Support for num-rows-repeated.
 *  - Fix empty block at end of table.
 */

include("common.qs");

var firstTableFormat = QTextTableFormat.clone(defaultTableFormat);
firstTableFormat.setBackground(new QBrush(new QColor(0xff, 0x33, 0x66)));

var secondTableFormat = QTextTableFormat.clone(defaultTableFormat);
secondTableFormat.setBackground(new QBrush(new QColor(0, 0, 0), Qt.NoBrush));

// first table
cursor.insertText("this is an example of table with a page break after the table.", defaultTextFormat);
cursor.insertTable(1, 3, firstTableFormat);
cursor.movePosition(QTextCursor.End);
cursor.insertBlock(defaultBlockFormat);

// second table
cursor.movePosition(QTextCursor.End);
cursor.insertText("this is an example of table with a page break before the table.", defaultTextFormat);
cursor.insertTable(1, 3, secondTableFormat);
cursor.movePosition(QTextCursor.End);
cursor.insertBlock(defaultBlockFormat);

cursor.insertBlock(defaultBlockFormat);
document;
