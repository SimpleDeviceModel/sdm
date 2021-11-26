/*
 * Copyright (c) 2015-2021 by Microproject LLC
 * 
 * This file is part of the Simple Device Model (SDM) framework.
 * 
 * SDM framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SDM framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SDM framework.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This module implements the TextViewer class members.
 */

#include "textviewer.h"

#include "fruntime_error.h"
#include "filedialogex.h"
#include "fontutils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QPushButton>
#include <QCheckBox>
#include <QMessageBox>

#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QTextBlock>
#include <QTextDocumentWriter>
#include <QTextStream>
#include <QTextCodec>

#include <QApplication>
#include <QStyle>
#include <QDesktopWidget>
#include <QSettings>
#include <QClipboard>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QFontDialog>
#include <QContextMenuEvent>
#include <QMenu>

#include <algorithm>
#include <memory>

TextViewer::TextViewer(QWidget *parent): QDialog(parent) {
	auto flags=windowFlags();
	flags|=Qt::CustomizeWindowHint|Qt::WindowMaximizeButtonHint;
	flags&=~Qt::WindowContextHelpButtonHint;
	setWindowFlags(flags);
	
	setSizeGripEnabled(true);
	
	auto layout=new QVBoxLayout;
	layout->setContentsMargins(0,0,0,0);
	
	_edit=new HintedWidget<QPlainTextEdit>(this);
	_edit->setReadOnly(true);
	_edit->setContextMenuPolicy(Qt::NoContextMenu); // defer context menu handling to this widget
	layout->addWidget(_edit);
	
	QSettings s;
	s.beginGroup("TextViewer");
	
	auto const savedFont=s.value("Font");
	if(savedFont.isValid()) {
		QFont f;
		f.fromString(savedFont.toString());
		applyFont(f);
	}
	else applyFont(FontUtils::defaultFixedFont());
	
	auto hbox=new QHBoxLayout;
	hbox->addStretch();
	
	_wrapCheckBox=new QCheckBox(tr("Word wrap"));
	wrap(s.value("WordWrap",true).toBool());
	QObject::connect(_wrapCheckBox,&QCheckBox::clicked,this,&TextViewer::wrapCheckBoxClicked);
	hbox->addWidget(_wrapCheckBox);
	
	auto copyButton=new QPushButton(tr("Copy"));
	QObject::connect(copyButton,&QPushButton::clicked,this,&TextViewer::copy);
	hbox->addWidget(copyButton);
	
	auto saveButton=new QPushButton(tr("Save as..."));
	QObject::connect(saveButton,&QPushButton::clicked,this,&TextViewer::saveAs);
	hbox->addWidget(saveButton);
	
	auto closeButton=new QPushButton(tr("Close"));
	closeButton->setDefault(true);
	QObject::connect(closeButton,&QPushButton::clicked,this,&TextViewer::reject);
	hbox->addWidget(closeButton);
	
	auto marginLeft=QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
	auto marginRight=QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
	auto marginBottom=QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
	hbox->setContentsMargins(marginLeft,0,marginRight,marginBottom);
	
	layout->addLayout(hbox);
	
	setLayout(layout);
}

void TextViewer::chooseFont() {
	const QFont oldFont=_edit->font();
	QFontDialog d(oldFont,this);
	QObject::connect(&d,&QFontDialog::currentFontChanged,this,&TextViewer::applyFont);
	int r=d.exec();
	if(!r) {
		applyFont(oldFont);
		return;
	}
	const QFont &newFont=d.selectedFont();
	applyFont(newFont);
	QSettings s;
	s.setValue("TextViewer/Font",newFont.toString());
}

void TextViewer::applyFont(const QFont &f) {
	_edit->setFont(f);
	_edit->setTabStopWidth(QFontMetrics(f).averageCharWidth()*8);
}

void TextViewer::clear() {
	_edit->document()->clear();
}

void TextViewer::loadFile(const QString &filename,const char *encoding) {
	QFile f(filename);
	if(!f.open(QIODevice::ReadOnly))
		throw fruntime_error(tr("Cannot open file: \"")+filename+"\"");
	
	QTextStream ts(&f);
	auto codec=QTextCodec::codecForName(encoding);
	ts.setCodec(codec);
	clear();
	loadStream(ts);
	_edit->moveCursor(QTextCursor::Start);
	setWindowFilePath(filename);
}

void TextViewer::loadString(const QString &str) {
	auto s=str;
	QTextStream ts(&s);
	clear();
	loadStream(ts);
	_edit->moveCursor(QTextCursor::Start);
}

void TextViewer::appendString(const QString &str) {
	auto s=str;
	QTextStream ts(&s);
	loadStream(ts);
	_edit->moveCursor(QTextCursor::End);
}

void TextViewer::loadStream(QTextStream &s) {
	setWindowFilePath("");
	if(s.atEnd()) return;
	
	auto const &fm=_edit->fontMetrics();
	
	auto cur=_edit->textCursor();
	auto doc=_edit->document();
	auto layout=doc->documentLayout();
	
	cur.movePosition(QTextCursor::End);
	
	while(!s.atEnd()) cur.insertText(s.read(4096));
	
// Force document width calculation
	for(auto tb=doc->begin();tb!=doc->end();tb=tb.next()) {
		(void)layout->blockBoundingRect(tb);
		if(tb.blockNumber()>10000) break; // limit number of blocks in calculation
	}
	
	auto docWidth=static_cast<int>(layout->documentSize().width());
	
// Find a visible parent (if any)
	const QWidget *vp;
	for(vp=this;vp&&!vp->isVisible();vp=vp->parentWidget());
	if(!vp) vp=this;
	
// Does the text look formatted?
	auto geometry=QApplication::desktop()->availableGeometry(vp);
	bool formatted=false;
	if(docWidth<geometry.width()*3/5) formatted=true;
	
	int w,h;
	if(formatted&&docWidth>0&&_edit->blockCount()>0) {
		auto scrollBarWidth=QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
		w=docWidth+scrollBarWidth+fm.height();
		h=fm.height()*_edit->blockCount()+fm.lineSpacing();
		h+=_edit->contentsMargins().top()+_edit->contentsMargins().bottom();
	}
	else {
		w=geometry.width()*2/5;
		h=geometry.height()/2;
	}
	_edit->overrideWidth(w);
	_edit->overrideHeight(h);
}

void TextViewer::copy() {
	auto cur=_edit->textCursor();
	if(cur.selectionEnd()>cur.selectionStart()) _edit->copy();
	else QApplication::clipboard()->setText(_edit->toPlainText());
}

void TextViewer::saveAs() try {
	QSettings s;
	s.beginGroup("TextViewer");
	
	FileDialogEx d(this);
	auto dir=s.value("SaveDirectory");
	if(dir.isValid()) d.setDirectory(dir.toString());
	d.setAcceptMode(QFileDialog::AcceptSave);
	d.setFileMode(QFileDialog::AnyFile);
	d.setNameFilters({tr("Text files (*.txt)"),tr("All files (*)")});
	
	if(!windowFilePath().isEmpty())
		d.selectFile(QFileInfo(windowFilePath()).baseName()+".txt");
	
	if(!d.exec()) return;
	
	auto const filename=d.fileName();
	
	QFile f(filename);
	if(!f.open(QIODevice::WriteOnly|QIODevice::Text))
		throw fruntime_error(tr("Cannot open file: \"")+filename+"\"");
	
	QTextDocumentWriter writer(&f,"plaintext");
	if(!writer.write(_edit->document()))
		throw fruntime_error(tr("Cannot save document"));
	
	s.setValue("SaveDirectory",d.directory().absolutePath());
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void TextViewer::wrap(bool b) {
	_edit->setWordWrapMode(b?QTextOption::WrapAtWordBoundaryOrAnywhere:QTextOption::NoWrap);
	if(_wrapCheckBox->isChecked()!=b) _wrapCheckBox->setChecked(b);
}

void TextViewer::contextMenuEvent(QContextMenuEvent *e) {
	if(!_edit->geometry().contains(e->pos())) return; // not in edit area
// Obtain standard context menu from QPlainTextEdit
	auto menu=std::unique_ptr<QMenu>(_edit->createStandardContextMenu());
	
// Add more menu items
	menu->addSeparator();
	
	auto a=menu->addAction(QIcon(":/commonwidgets/icons/font.svg"),tr("Choose font..."));
	QObject::connect(a,&QAction::triggered,this,&TextViewer::chooseFont);
	
	menu->exec(e->globalPos());
}

/*
 * Note: the difference between wrap() and wrapCheckBoxClicked() is
 * that the latter is called only when the user clicks on the checkbox,
 * but not when wrap mode is changed programmatically.
 */

void TextViewer::wrapCheckBoxClicked(bool b) {
	wrap(b);
	QSettings s;
	s.setValue("TextViewer/WordWrap",b);
}
