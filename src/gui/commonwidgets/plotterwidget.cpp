/*
 * Copyright (c) 2015-2022 Simple Device Model contributors
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
 * This module provides an implementation of the PlotterWidget
 * class members.
 */

#include "plotterwidget.h"

#include "plotterscrollarea.h"
#include "plotterbarscene.h"
#include "plotterbitmapscene.h"
#include "plottervideoscene.h"
#include "plotterbinaryscene.h"
#include "plotterlayersdialog.h"
#include "filedialogex.h"

#include <QToolButton>
#include <QStatusBar>
#include <QLabel>
#include <QMouseEvent>
#include <QPen>
#include <QBrush>
#include <QInputDialog>
#include <QImageWriter>
#include <QCheckBox>
#include <QApplication>
#include <QStyle>
#include <QSettings>
#include <QMessageBox>
#include <QToolBar>
#include <QMenu>
#include <QTextStream>

#include <utility>

/*
 * Public members
 */

PlotterWidget::PlotterWidget(PlotMode m,QWidget *parent):
	QMainWindow(parent)
{
	Q_INIT_RESOURCE(commonwidgets);
	
	setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
	
	_scrollArea=new PlotterScrollArea;
	setCentralWidget(_scrollArea);
	setMode(m);
	
// Set up the toolbar
	_toolBar=new QToolBar(tr("Plotter toolbar"));
	addToolBar(_toolBar);
	const int iconWidth=QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
	_toolBar->setIconSize(QSize(iconWidth,iconWidth));

	auto modeMenu=new QMenu(_toolBar);
	QObject::connect(modeMenu->addAction(tr("Bars")),&QAction::triggered,[this]{selectMode(Bars);});
	QObject::connect(modeMenu->addAction(tr("Plot")),&QAction::triggered,[this]{selectMode(Plot);});
	QObject::connect(modeMenu->addAction(tr("Bitmap")),&QAction::triggered,[this]{selectMode(Bitmap);});
	QObject::connect(modeMenu->addAction(tr("Video")),&QAction::triggered,[this]{selectMode(Video);});
	QObject::connect(modeMenu->addAction(tr("Binary")),&QAction::triggered,[this]{selectMode(Binary);});
	auto modeToolButton=new QToolButton();
	modeToolButton->setText(tr("Mode"));
	modeToolButton->setMenu(modeMenu);
	modeToolButton->setPopupMode(QToolButton::InstantPopup);
	_toolBar->addWidget(modeToolButton);
	
	QAction *a=_toolBar->addAction(QIcon(":/commonwidgets/icons/hzoomin.svg"),tr("Horizontal zoom in"));
	QObject::connect(a,&QAction::triggered,[this]{zoom(1.2,1);});
	a=_toolBar->addAction(QIcon(":/commonwidgets/icons/hzoomout.svg"),tr("Horizontal zoom out"));
	QObject::connect(a,&QAction::triggered,[this]{zoom(1/1.2,1);});
	a=_toolBar->addAction(QIcon(":/commonwidgets/icons/vzoomin.svg"),tr("Vertical zoom in"));
	QObject::connect(a,&QAction::triggered,[this]{zoom(1,1.2);});
	a=_toolBar->addAction(QIcon(":/commonwidgets/icons/vzoomout.svg"),tr("Vertical zoom out"));
	QObject::connect(a,&QAction::triggered,[this]{zoom(1,1/1.2);});
	a=_toolBar->addAction(QIcon(":/commonwidgets/icons/zoomfit.svg"),tr("Fit"));
	QObject::connect(a,&QAction::triggered,this,&PlotterWidget::zoomFit);
	
	_toolBar->addSeparator();
	
	_dragToScrollAction=_toolBar->addAction(QIcon(":/commonwidgets/icons/dragtoscroll.svg"),tr("Drag to scroll"));
	_dragToScrollAction->setCheckable(true);
	_dragToScrollAction->setChecked(true);
	QObject::connect(_dragToScrollAction,&QAction::toggled,this,&PlotterWidget::dragToScroll);
	
	_dragToZoomAction=_toolBar->addAction(QIcon(":/commonwidgets/icons/dragtozoom.svg"),tr("Drag to zoom"));
	_dragToZoomAction->setCheckable(true);
	QObject::connect(_dragToZoomAction,&QAction::toggled,this,&PlotterWidget::dragToZoom);
	
	_toolBar->addSeparator();
	
	_freezeAction=_toolBar->addAction(QIcon(":/commonwidgets/icons/freeze.svg"),tr("Freeze"));
	_freezeAction->setCheckable(true);
	QObject::connect(_freezeAction,&QAction::toggled,this,&PlotterWidget::setFreezed);
	
	a=_toolBar->addAction(QIcon(":/commonwidgets/icons/saveimage.svg"),tr("Save image")+"...");
	QObject::connect(a,&QAction::triggered,this,&PlotterWidget::saveImage);
	a=_toolBar->addAction(QIcon(":/commonwidgets/icons/exportdata.svg"),tr("Export data")+"...");
	QObject::connect(a,&QAction::triggered,this,&PlotterWidget::exportData);
	a=_toolBar->addAction(QIcon(":/commonwidgets/icons/addcursor.svg"),tr("Add cursor")+"...");
	QObject::connect(a,&QAction::triggered,this,static_cast<void(PlotterWidget::*)()>(&PlotterWidget::addCursor));
	a=_toolBar->addAction(QIcon(":/commonwidgets/icons/layers.svg"),tr("Layers")+"...");
	QObject::connect(a,&QAction::triggered,this,&PlotterWidget::configureLayers);
	
// Status bar
	auto coords=new QLabel;
	statusBar()->addWidget(coords);
	QObject::connect(_scrollArea,&PlotterScrollArea::coordStatus,coords,&QLabel::setText);
	
	_fpsLabel=new QLabel;
	_fpsLabel->setAlignment(Qt::AlignRight);
	_fpsLabel->setIndent(fontInfo().pixelSize());
	statusBar()->addPermanentWidget(_fpsLabel);
	QObject::connect(_scrollArea,&PlotterScrollArea::fpsCalculated,this,&PlotterWidget::updateFps);
}

/*
 * Slots
 */

void PlotterWidget::setMode(PlotMode m) {
	if(m==Preferred) { // use either Bars or Plot, depending on settings
		QSettings s;
		auto pref=s.value("Plotter/PreferredMode").toString();
		if(pref=="Plot") m=Plot;
		else m=Bars;
	}
	
	if(m==Bars||m==Plot) { // implemented by PlotterBarScene
		const PlotterBarScene::Mode sm=(m==Bars)?(PlotterBarScene::Bars):(PlotterBarScene::Plot);
		auto barScene=dynamic_cast<PlotterBarScene*>(_scene.get());
		if(barScene) barScene->setMode(sm); // switch PlotterBarScene mode
		else { // Otherwise, create a new scene
			_scene.reset(new PlotterBarScene(sm));
			_scrollArea->setScene(_scene.get());
		}
		
		_scene->setBackgroundBrush(Qt::white);
		setupFgBrushes();
		_scene->setGridPen(QPen(QColor(64,64,64),0));
	}
	else {
		if(m==Bitmap) _scene.reset(new PlotterBitmapScene);
		else if(m==Video) _scene.reset(new PlotterVideoScene); 
		else _scene.reset(new PlotterBinaryScene);
		_scrollArea->setScene(_scene.get());
		
		_scene->setBackgroundBrush(QColor(64,64,64));
		_scene->setGridPen(QPen(QColor(224,224,224),0));
	}
	
	QToolBar *tb=_scene->toolBar();
	if(tb) addToolBar(Qt::BottomToolBarArea,tb);
}

void PlotterWidget::zoom(qreal x,qreal y) {
	_scrollArea->zoom(x,y);
}

void PlotterWidget::zoomFit() {
	_scrollArea->zoomFit();
}

void PlotterWidget::dragToScroll(bool checked) {
	if(checked) {
		_scrollArea->setDragMode(PlotterScrollArea::DragToScroll);
		_dragToZoomAction->setChecked(false);
	}
}

void PlotterWidget::dragToZoom(bool checked) {
	if(checked) {
		_scrollArea->setDragMode(PlotterScrollArea::DragToZoom);
		_dragToScrollAction->setChecked(false);
	}
}

void PlotterWidget::setFreezed(bool b) {
	_freezed=b;
}

void PlotterWidget::saveImage() {
	FileDialogEx d(this);
	QSettings s;
	s.beginGroup("Plotter");
	
	auto const &formats=QImageWriter::supportedImageFormats().toSet();
	
	auto dir=s.value("SaveImageDirectory");
	if(dir.isValid()) d.setDirectory(dir.toString());
	
	d.setAcceptMode(QFileDialog::AcceptSave);
	d.setFileMode(QFileDialog::AnyFile);
	QStringList filters;
	if(formats.find("png")!=formats.cend()) filters<<tr("Portable Network Graphics (*.png)");
	if(formats.find("tif")!=formats.cend()) filters<<tr("Tagged Image File Format (*.tif)");
	if(formats.find("jpg")!=formats.cend()) filters<<tr("JPEG Image (*.jpg)");
	if(formats.find("bmp")!=formats.cend()) filters<<tr("Windows Bitmap (*.bmp)");
	filters<<tr("Scalable Vector Graphics (*.svg)");
	filters<<tr("Portable Document Format (*.pdf)");
	d.setNameFilters(filters);
	
	d.selectNameFilter(filters.front());
	d.selectFile(tr("Untitled"));
	
	bool tempFreeze=true;
	std::swap(_freezed,tempFreeze);
	
	if(d.exec()) {
		s.setValue("SaveImageDirectory",d.directory().absolutePath());
		
		auto const &format=d.filteredExtension();
		
		int quality=-1;
		if(format=="jpg") {
			QInputDialog id(this);
			id.setWindowFlags(id.windowFlags()&~Qt::WindowContextHelpButtonHint);
			id.setLabelText(tr("JPEG compression quality (0-100):"));
			id.setInputMode(QInputDialog::IntInput);
			id.setIntRange(0,100);
			id.setIntValue(80);
			if(!id.exec()) {
				std::swap(_freezed,tempFreeze);
				return;
			}
			quality=id.intValue();
		}
		
		auto const &filename=d.fileName();
		if(!filename.isEmpty()) _scrollArea->saveImage(filename,format,quality);
	}
	
	std::swap(_freezed,tempFreeze);
}

void PlotterWidget::exportData() {
	if(!_scene) return;
	
	FileDialogEx d(this);
	QSettings s;
	s.beginGroup("Plotter");
	
	auto dir=s.value("SaveImageDirectory");
	if(dir.isValid()) d.setDirectory(dir.toString());
	
	d.setAcceptMode(QFileDialog::AcceptSave);
	d.setFileMode(QFileDialog::AnyFile);
	d.setNameFilter(tr("CSV files (*.csv)"));
	d.selectFile(tr("Untitled"));
	
	bool tempFreeze=true;
	std::swap(_freezed,tempFreeze);
	
	if(d.exec()) {
		s.setValue("SaveImageDirectory",d.directory().absolutePath());
		auto const &filename=d.fileName();
		if(!filename.isEmpty()) {
			QFile f(filename);
			if(!f.open(QIODevice::WriteOnly)) {
				QMessageBox::critical(nullptr,QObject::tr("Error"),tr("Cannot open file"),QMessageBox::Ok);
				std::swap(_freezed,tempFreeze);
				return;
			}
			QTextStream ts(&f);
			_scene->exportData(ts);
		}
	}
	
	std::swap(_freezed,tempFreeze);
}

QString PlotterWidget::layerName(int layer) const {
	auto it=_layerNames.find(layer);
	if(it==_layerNames.end()||it.value().isEmpty()) return tr("Layer")+" "+QString::number(layer);
	return it.value();
}

void PlotterWidget::setLayerName(int layer,const QString &name) {
	_layerNames[layer]=name;
}

void PlotterWidget::addCursor() {
	_scrollArea->addCursor();
}

void PlotterWidget::addCursor(const QString &name,int pos) {
	_scrollArea->addCursor(name,pos);
}

void PlotterWidget::configureLayers() {
	if(!_scene) return;
	
	auto layers=_scene->layers();
	if(layers.empty()) {
		QMessageBox::critical(this,QObject::tr("Error"),tr("Layer set is empty"),QMessageBox::Ok);
		return;
	}
	
	auto barScene=dynamic_cast<PlotterBarScene*>(_scene.get());
	
	PlotterLayersDialog::Flags f=PlotterLayersDialog::Default;
	if(barScene) f|=(PlotterLayersDialog::UseScale|PlotterLayersDialog::UseColors);
	
	PlotterLayersDialog d(f,this);
	
	if(barScene) d.setSelectionMode(PlotterLayersDialog::MultipleSelection);
	else d.setSelectionMode(PlotterLayersDialog::SingleSelection);
	
	for(auto const l: layers) {
		d.addLayer(l,layerName(l));
		d.setLayerEnabled(l,_scene->layerEnabled(l));
		if(barScene) {
			d.setLayerScale(l,barScene->layerScale(l));
			d.setLayerInputOffset(l,barScene->layerInputOffset(l));
			d.setLayerOutputOffset(l,barScene->layerOutputOffset(l));
			d.setLayerColor(l,barScene->foregroundBrush(l).color());
		}
	}
	
	if(!d.exec()) return;
	
	for(auto const l: layers) _scene->setLayerEnabled(l,d.layerEnabled(l));
	
	if(barScene) {
		QSettings s;
		s.beginGroup("Plotter");
		s.beginGroup("LayerColors");
		for(auto const l: layers) {
			QColor c=d.layerColor(l);
			s.setValue(QString::number(l),c.name());
			
			barScene->setForegroundBrush(l,c);
			barScene->setLayerScale(l,d.layerScale(l));
			barScene->setLayerInputOffset(l,d.layerInputOffset(l));
			barScene->setLayerOutputOffset(l,d.layerOutputOffset(l));
		}
	}
}

void PlotterWidget::showFps(bool b) {
	_fpsLabel->setVisible(b);
}

void PlotterWidget::addData(int layer,const QVector<qreal> &data) {
	if(!_freezed) _scene->addData(layer,data);
}

void PlotterWidget::removeData(int layer,int n) {
	if(!_freezed) _scene->removeData(layer,n);
}

/*
 * Protected members
 */

void PlotterWidget::changeEvent(QEvent *e) {
	if(e->type()==QEvent::WindowStateChange)
		QMetaObject::invokeMethod(_scrollArea,"zoomFit",Qt::QueuedConnection);
	QMainWindow::changeEvent(e);
}

void PlotterWidget::mousePressEvent(QMouseEvent *e) {
	if(e->buttons()&Qt::RightButton) {
		if(!_dragToScrollAction->isChecked()) _dragToScrollAction->toggle();
		else _dragToZoomAction->toggle();
		e->accept();
	}
}

void PlotterWidget::keyPressEvent(QKeyEvent *e) {
	if(e->key()==Qt::Key_Space) _freezeAction->toggle();
	else QMainWindow::keyPressEvent(e);
}

/*
 * Private members
 */

void PlotterWidget::setupFgBrushes() {
	QSettings s;
	s.beginGroup("Plotter");
	s.beginGroup("LayerColors");
	
	for(int i=0;;i++) {
		auto val=s.value(QString::number(i));
		if(val.isNull()) break;
		_scene->setForegroundBrush(i,QColor(val.toString()));
	}
}

/*
 * Note: PlotterWidget::selectMode() is called only when the user
 * selects the mode using the toolbar, not when it is changed
 * programmatically.
 */

void PlotterWidget::selectMode(PlotMode m) {
	setMode(m);
	if(m==Bars||m==Plot) {
		QSettings s;
		s.setValue("Plotter/PreferredMode",(m==Plot)?"Plot":"Bars");
	}
}

void PlotterWidget::updateFps(double d) {
	QString str;
	if(d>=5.0&&!_freezed) str=QLocale().toString(d,'f',2)+" fps";
	_fpsLabel->setText(str);
}
