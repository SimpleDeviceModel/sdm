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
 * This module provides an implementation of the PlotterScrollArea
 * class members.
 */

#include "plotterscrollarea.h"

#include "plottercursorwidget.h"
#include "plotteraddcursordialog.h"

#include <QPainter>
#include <QScrollBar>
#include <QMouseEvent>
#include <QCursor>
#include <QTextStream>
#include <QScreen>
#include <QSvgGenerator>
#include <QMessageBox>
#include <QImageWriter>
#include <QPdfWriter>
#include <QApplication>
#include <QDesktopWidget>
#include <QLayout>
#include <QLocale>
#include <QTimer>
#include <QRubberBand>

#include <QtMath>

#include <cmath>
#include <limits>
#include <algorithm>

/* 
 * Transform members
 */

PlotterScrollArea::Transform::Transform(qreal sx,qreal sy,qreal dxx,qreal dyy):
	_scaleX(sx),_scaleY(sy),_dx(dxx),_dy(dyy) {}

PlotterScrollArea::Transform::Transform():
	_scaleX(1),_scaleY(-1),_dx(0),_dy(0){}

qreal PlotterScrollArea::Transform::scaleX() const {
	return _scaleX;
}

void PlotterScrollArea::Transform::setScaleX(qreal d) {
	_scaleX=d;
}

qreal PlotterScrollArea::Transform::scaleY() const {
	return _scaleY;
}

void PlotterScrollArea::Transform::setScaleY(qreal d) {
	_scaleY=d;
}

qreal PlotterScrollArea::Transform::dx() const {
	return _dx;
}

void PlotterScrollArea::Transform::setDx(qreal d) {
	_dx=d;
}

qreal PlotterScrollArea::Transform::dy() const {
	return _dy;
}

void PlotterScrollArea::Transform::setDy(qreal d) {
	_dy=d;
}

bool PlotterScrollArea::Transform::operator==(const Transform &other) const {
	if(_scaleX!=other._scaleX) return false;
	if(_scaleY!=other._scaleY) return false;
	if(_dx!=other._dx) return false;
	if(_dy!=other._dy) return false;
	return true;
}

bool PlotterScrollArea::Transform::operator!=(const Transform &other) const {
	return !operator==(other);
}

PlotterScrollArea::Transform::operator bool() const {
	return !operator==(Transform());
}

void PlotterScrollArea::Transform::reset() {
	operator=(Transform());
}

QPointF PlotterScrollArea::Transform::map(const QPointF &from) const {
	return QPointF(_scaleX*from.x()+_dx,_scaleY*from.y()+_dy);
}

QSizeF PlotterScrollArea::Transform::map(const QSizeF &from) const {
	return QSizeF(_scaleX*from.width(),_scaleY*from.height());
}

QRectF PlotterScrollArea::Transform::map(const QRectF &from) const {
	return QRectF(map(from.topLeft()),map(from.bottomRight()));
}

PlotterScrollArea::Transform PlotterScrollArea::Transform::inverted() const {
	return Transform(1/_scaleX,1/_scaleY,-_dx/_scaleX,-_dy/_scaleY);
}

PlotterScrollArea::Transform PlotterScrollArea::Transform::scaled(qreal x,qreal y,qreal x0,qreal y0) const {
	return Transform(_scaleX*x,_scaleY*y,_dx+x0*(_scaleX*(1-x)),_dy+y0*(_scaleY*(1-y)));
}

QTransform PlotterScrollArea::Transform::toQTransform() const {
	return QTransform(_scaleX,0,0,_scaleY,_dx,_dy);
}

// Note: rectToRect() is a static member of the PlotterScrollArea::Transform class

PlotterScrollArea::Transform PlotterScrollArea::Transform::rectToRect(const QRectF &src,const QRectF &dest) {
	Transform res;
	res._scaleX=dest.width()/src.width();
	res._scaleY=dest.height()/src.height();
	res._dx=dest.left()-res._scaleX*src.left();
	res._dy=dest.top()-res._scaleY*src.top();
	return res;
}

/*
 * ScrollHelper member
 */

PlotterScrollArea::ScrollHelper::ScrollHelper(const QRectF &sceneRect,const QRectF &viewRect,const Transform &transform):
		_sceneRect(sceneRect),_viewRect(viewRect),_transform(transform) {
// Calculate scene rectangle in view coordinate system
	QRectF fullRect=_transform.map(_sceneRect);
// Calculate valid dx range
	if(qAbs(fullRect.width())>qAbs(_viewRect.width())) {
		if(_transform.scaleX()>0) {
			_dxRange.first=_viewRect.left()-_sceneRect.left()*_transform.scaleX();
			_dxRange.second=_viewRect.right()-_sceneRect.right()*_transform.scaleX();
		}
		else {
			_dxRange.first=_viewRect.left()-_sceneRect.right()*_transform.scaleX();
			_dxRange.second=_viewRect.right()-_sceneRect.left()*_transform.scaleX();
		}
	}
	else _dxRange=qMakePair(_transform.dx(),_transform.dx());
	
	if(qAbs(fullRect.height())>qAbs(_viewRect.height())) {
		if(_transform.scaleY()>0) {
			_dyRange.first=_viewRect.top()-_sceneRect.top()*_transform.scaleY();
			_dyRange.second=_viewRect.bottom()-_sceneRect.bottom()*_transform.scaleY();
		}
		else {
			_dyRange.first=_viewRect.top()-_sceneRect.bottom()*_transform.scaleY();
			_dyRange.second=_viewRect.bottom()-_sceneRect.top()*_transform.scaleY();
		}
	}
	else _dyRange=qMakePair(_transform.dy(),_transform.dy());
}

int PlotterScrollArea::ScrollHelper::xScrollRange() const {
	return static_cast<int>(qAbs(_dxRange.second-_dxRange.first));
}

int PlotterScrollArea::ScrollHelper::yScrollRange() const {
	return static_cast<int>(qAbs(_dyRange.second-_dyRange.first));
}

qreal PlotterScrollArea::ScrollHelper::scrollToDx(int scrollx) const {
	if(_dxRange.second>_dxRange.first) return _dxRange.first+scrollx;
	else return _dxRange.first-scrollx;
}

qreal PlotterScrollArea::ScrollHelper::scrollToDy(int scrolly) const {
	if(_dyRange.second>_dyRange.first) return _dyRange.first+scrolly;
	else return _dyRange.first-scrolly;
}

int PlotterScrollArea::ScrollHelper::dxToScroll(qreal dx) const {
	if(_dxRange.second>_dxRange.first) return static_cast<int>(dx-_dxRange.first);
	else return static_cast<int>(_dxRange.first-dx);
}

int PlotterScrollArea::ScrollHelper::dyToScroll(qreal dy) const {
	if(_dyRange.second>_dyRange.first) return static_cast<int>(dy-_dyRange.first);
	else return static_cast<int>(_dyRange.first-dy);
}

/*
 * PlotterScrollArea members
 */

static const int StableCounterMax=5;

PlotterScrollArea::PlotterScrollArea(QWidget *parent):
	QAbstractScrollArea(parent)
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setupViewport(viewport());
	
	_rubberBand=new QRubberBand(QRubberBand::Rectangle,this);
	
	auto timer=new QTimer(this);
	QObject::connect(timer,&QTimer::timeout,this,&PlotterScrollArea::calculateFps);
	timer->start(1000);
	_time.start();
}

void PlotterScrollArea::setupViewport(QWidget *viewport) {
	viewport->setMouseTracking(true);
	restoreCursor();
}

void PlotterScrollArea::setScene(PlotterAbstractScene *s) {
	_scene=s;
	QObject::disconnect(this,SLOT(sceneChanged()));
	QObject::connect(_scene,&PlotterAbstractScene::changed,this,&PlotterScrollArea::sceneChanged);
	_transform=Transform();
	for(auto &c: _cursors) if(!c.isNull()) c->setScene(s);
	updateCursors();
	viewport()->update();
	_alwaysFit=true;
	_fullSceneWidth=-1;
	_prevSceneWidth=-1;
	_stableCounter=0;
}

QSize PlotterScrollArea::sizeHint() const {
// Request a reasonably large area
	auto geometry=QApplication::desktop()->availableGeometry(this);
	return QSize(geometry.width()/2,geometry.height()*2/5);
}

void PlotterScrollArea::scrollContentsBy(int dx,int dy) {
	if(horizontalScrollBar()->maximum()>0) _transform.setDx(_scrollHelper.scrollToDx(horizontalScrollBar()->value()));
	if(verticalScrollBar()->maximum()>0) _transform.setDy(_scrollHelper.scrollToDy(verticalScrollBar()->value()));
	updateCursors();
	viewport()->update();
}

void PlotterScrollArea::resizeEvent(QResizeEvent *) {
	if(_alwaysFit) zoomFit();
	else {
		updateScrollBars();
		horizontalScrollBar()->setPageStep(viewport()->rect().width());
		horizontalScrollBar()->setSingleStep(std::max(viewport()->rect().width()/5,1));
		verticalScrollBar()->setPageStep(viewport()->rect().height());
		verticalScrollBar()->setSingleStep(std::max(viewport()->rect().height()/5,1));
	}
}

void PlotterScrollArea::paintEvent(QPaintEvent *) {
	if(_scene==nullptr) return;
	QPainter painter(viewport());
	painter.setTransform(_transform.toQTransform());
	painter.setRenderHint(QPainter::Antialiasing);
	_scene->paint(painter);
	paintCursors(painter);
	updateStatus();
	_frames++;
}

void PlotterScrollArea::mouseDoubleClickEvent(QMouseEvent *e) {
	if(e->buttons()&Qt::LeftButton) zoomFit();
	else QAbstractScrollArea::mouseDoubleClickEvent(e);
}

void PlotterScrollArea::mousePressEvent(QMouseEvent *e) {
	if(e->buttons()&Qt::LeftButton) {
		_mouseDragPos=e->pos();
		if(_drag==DragToScroll) viewport()->setCursor(Qt::ClosedHandCursor);
		else { // DragToZoom
			_rubberBand->setGeometry(QRect(_mouseDragPos,_mouseDragPos));
			_rubberBand->show();
		}
	}
	else QAbstractScrollArea::mousePressEvent(e);
}

void PlotterScrollArea::mouseReleaseEvent(QMouseEvent *e) {
	restoreCursor();
	if(_rubberBand->isVisible()) {
		_rubberBand->hide();
		const QRectF selected=QRectF(_mouseDragPos,e->pos()).normalized();
		if(selected.width()>0&&selected.height()>0) {
			const QRectF selectedSrc=_transform.inverted().map(selected);
			_transform=Transform::rectToRect(selectedSrc,viewport()->rect());
			updateScrollBars();
			updateCursors();
			viewport()->update();
			_alwaysFit=false;
		}
		_mouseDragPos=QPoint();
	}
	else QAbstractScrollArea::mouseReleaseEvent(e);
}

void PlotterScrollArea::mouseMoveEvent(QMouseEvent *e) {
	if(e->buttons()&Qt::LeftButton) {
		if(_drag==DragToScroll) {
// Wrap mouse cursor position around screen
			const int delta=logicalDpiX()/5;
			QPoint globalPos=e->globalPos();
			QRect screenRect=QApplication::desktop()->screenGeometry(this);
			
			if(qAbs(globalPos.x()-screenRect.left())<delta) {
				globalPos=QPoint(screenRect.right()-2*delta,globalPos.y());
				QCursor::setPos(globalPos);
				_mouseDragPos=viewport()->mapFromGlobal(globalPos);
			}
			else if(qAbs(globalPos.x()-screenRect.right())<delta) {
				globalPos=QPoint(screenRect.left()+2*delta,globalPos.y());
				QCursor::setPos(globalPos);
				_mouseDragPos=viewport()->mapFromGlobal(globalPos);
			}
			
			if(qAbs(globalPos.y()-screenRect.top())<delta) {
				globalPos=QPoint(globalPos.x(),screenRect.bottom()-2*delta);
				QCursor::setPos(globalPos);
				_mouseDragPos=viewport()->mapFromGlobal(globalPos);
			}
			else if(qAbs(globalPos.y()-screenRect.bottom())<delta) {
				globalPos=QPoint(globalPos.x(),screenRect.top()+2*delta);
				QCursor::setPos(globalPos);
				_mouseDragPos=viewport()->mapFromGlobal(globalPos);
			}
			
			QPoint localPos=viewport()->mapFromGlobal(globalPos);
			QPoint diff=localPos-_mouseDragPos;
			_mouseDragPos=localPos;
			
			_transform.setDx(_transform.dx()+diff.x());
			_transform.setDy(_transform.dy()+diff.y());
			updateScrollBars();
			updateCursors();
			viewport()->update();
		}
		else {
			_rubberBand->setGeometry(QRect(_mouseDragPos,e->pos()).normalized());
		}
	}
	
	updateStatus();
}

void PlotterScrollArea::wheelEvent(QWheelEvent *e) {
	QPointF invariantDest=e->posF();
	QPointF invariantSrc=_transform.inverted().map(invariantDest);

// Decode keyboard flags
	bool zoom=false;
	bool invert=false;
	if(e->modifiers()&Qt::ControlModifier) zoom=true;
	if(e->modifiers()&Qt::ShiftModifier) invert=true;
	
	QPoint delta=e->angleDelta();
	if(invert) delta=QPoint(delta.y(),delta.x());
	_wheelPos+=delta;
	
	if(!zoom) { // scroll
		int hscroll=horizontalScrollBar()->singleStep();
		int vscroll=verticalScrollBar()->singleStep();
		
		if(_wheelPos.x()>=120) {
			horizontalScrollBar()->setValue(horizontalScrollBar()->value()-hscroll);
			_wheelPos.setX(0);
		}
		else if(_wheelPos.x()<=-120) {
			horizontalScrollBar()->setValue(horizontalScrollBar()->value()+hscroll);
			_wheelPos.setX(0);
		}
		if(_wheelPos.y()>=120) {
			verticalScrollBar()->setValue(verticalScrollBar()->value()-vscroll);
			_wheelPos.setY(0);
		}
		else if(_wheelPos.y()<=-120) {
			verticalScrollBar()->setValue(verticalScrollBar()->value()+vscroll);
			_wheelPos.setY(0);
		}
	}
	else { // zoom
		if(_wheelPos.x()>=120) {
			_transform=_transform.scaled(1.2,1,invariantSrc.x(),invariantSrc.y());
			_wheelPos.setX(0);
		}
		else if(_wheelPos.x()<=-120) {
			_transform=_transform.scaled(1/1.2,1,invariantSrc.x(),invariantSrc.y());
			_wheelPos.setX(0);
		}
		if(_wheelPos.y()>=120) {
			_transform=_transform.scaled(1,1.2,invariantSrc.x(),invariantSrc.y());
			_wheelPos.setY(0);
		}
		else if(_wheelPos.y()<=-120) {
			_transform=_transform.scaled(1,1/1.2,invariantSrc.x(),invariantSrc.y());
			_wheelPos.setY(0);
		}
		updateScrollBars();
		updateCursors();
		_alwaysFit=false;
	}
	
	viewport()->update();
}

void PlotterScrollArea::zoom(qreal x,qreal y) {
	if(_scene==nullptr) return;
	QRectF viewRect=viewport()->rect();
	QRectF validRect=_transform.map(_scene->rect()).normalized();
	QPointF invariantDest=QPointF(
		qMin(viewRect.width()/2,validRect.width()/2),
		qMin(viewRect.height()/2,validRect.height()/2)
	);
	QPointF invariantSrc=_transform.inverted().map(invariantDest);
	_transform=_transform.scaled(x,y,invariantSrc.x(),invariantSrc.y());
	updateScrollBars();
	updateCursors();
	viewport()->update();
	_alwaysFit=false;
}

void PlotterScrollArea::zoomFit() {
	if(_scene==nullptr) return;
	if(_scene->rect().width()==0||_scene->rect().height()==0) return;
	QRectF viewportRect;
	if(!_scene->zeroIsTop()) viewportRect=QRectF(0,maximumViewportSize().height(),maximumViewportSize().width(),-maximumViewportSize().height());
	else viewportRect=QRectF(0,0,maximumViewportSize().width(),maximumViewportSize().height());
	_transform=Transform::rectToRect(_scene->rect(),viewportRect);
	updateScrollBars();
	updateCursors();
	viewport()->update();
	_alwaysFit=true;
	_fullSceneWidth=_scene->rect().width();
}

void PlotterScrollArea::setDragMode(DragMode m) {
	_drag=m;
	restoreCursor();
}

void PlotterScrollArea::sceneChanged() {
	auto w=_scene->rect().width();
	
	if(_scene->rect().isValid()&&_alwaysFit&&w>_fullSceneWidth&&_stableCounter<StableCounterMax) zoomFit();
	
	if(_scene->rect().isValid()&&w==_prevSceneWidth) {
		if(_stableCounter<StableCounterMax) _stableCounter++;
	}
	else _stableCounter=0;
	_prevSceneWidth=w;
	
	updateScrollBars();
	viewport()->update();
}

void PlotterScrollArea::saveImage(const QString &filename,const QString &format,int quality) {
	if(format=="svg") {
		QSvgGenerator svg;
		svg.setFileName(filename);
		svg.setTitle("sdmconsole drawing");
		svg.setSize(viewport()->rect().size());
		svg.setViewBox(viewport()->rect());
		QPainter painter;
		painter.begin(&svg);
		painter.setTransform(_transform.toQTransform());
		_scene->paint(painter);
		painter.end();
	}
	else if(format=="pdf") {
		QPdfWriter pdf(filename);
		pdf.setTitle("sdmconsole drawing");
 // Note: 25.4 is a number of millimeters per inch
		QSizeF pageSize(25.4*viewport()->rect().width()/logicalDpiX(),
			25.4*viewport()->rect().height()/logicalDpiY());
		QSizeF margins=QSizeF(pdf.margins().right+pdf.margins().left,
			pdf.margins().bottom+pdf.margins().top);
		pageSize+=margins;
		pdf.setPageSizeMM(pageSize);
		QPainter painter(&pdf);
		const QRectF visibleArea=_transform.inverted().map(viewport()->rect());
		const QRectF targetRect=QRectF(0,0,pdf.width(),pdf.height());
		Transform pdftransform=Transform::rectToRect(visibleArea,targetRect);
		painter.setTransform(pdftransform.toQTransform());
		_scene->paint(painter);
	}
	else { // raster image
		QImage img(viewport()->rect().size(),QImage::Format_RGB32);
		QPainter painter(&img);
		painter.setTransform(_transform.toQTransform());
		painter.setRenderHint(QPainter::Antialiasing);
		_scene->paint(painter);

		QImageWriter writer(filename);
		writer.setFormat(format.toLatin1());
		writer.setCompression(1);
		if(format=="jpg") writer.setQuality(quality);
		bool b=writer.write(img);
		if(!b) QMessageBox::critical(this,tr("Image generation failed"),writer.errorString());
	}
}

void PlotterScrollArea::addCursor() {
	int pos=qCeil(_transform.inverted().map(QPointF(0,0)).x());
	PlotterAddCursorDialog d(this);
	d.setName(tr("Cursor %1").arg(_nextCursorNumber));
	d.setPosition(pos);
	if(d.exec()) {
		addCursor(d.name(),d.position());
		updateCursors();
		_nextCursorNumber++;
	}
}

void PlotterScrollArea::addCursor(const QString &name,int pos) {
	auto cursorWidget=new PlotterCursorWidget(pos,viewport());
	cursorWidget->setScene(_scene);
	QObject::connect(cursorWidget,&PlotterCursorWidget::positionChanged,this,&PlotterScrollArea::updateCursors);
	QObject::connect(cursorWidget,&PlotterCursorWidget::positionChanged,viewport(),
		static_cast<void(QWidget::*)()>(&QWidget::update));
	QObject::connect(cursorWidget,&PlotterCursorWidget::closeButtonClicked,viewport(),
		static_cast<void(QWidget::*)()>(&QWidget::update));
	QObject::connect(cursorWidget,&PlotterCursorWidget::drag,this,&PlotterScrollArea::dragCursor);
	cursorWidget->setWindowTitle(name);
	_cursors.emplace(cursorWidget);
	viewport()->update();
}

void PlotterScrollArea::dragCursor(const QPoint &pos) {
	auto cursorWidget=dynamic_cast<PlotterCursorWidget*>(sender());
	if(!cursorWidget) return;

	QPointF scenePos=_transform.inverted().map(QPointF(pos));
	QRectF sceneRect=_transform.inverted().map(viewport()->rect());
	int low=qCeil(sceneRect.left());
	int high=qFloor(sceneRect.right());
	int x=qRound(scenePos.x());
	if(x<low) x=low;
	else if(x>high) x=high;
	
	cursorWidget->setPos(x);
	updateCursors();
}

void PlotterScrollArea::updateScrollBars() {
	if(_scene==nullptr) return;
	
	if(_scene->rect().isEmpty()) return;
	
// Save transform to avoid Transform::dx and Transform::dy being changed by scrollContentsBy()
	Transform t=_transform;

// Take a union of current view and new scene
	const QRectF &extendedSceneRect=_scene->rect().united(_transform.inverted().map(viewport()->rect()));
	_scrollHelper=ScrollHelper(extendedSceneRect,viewport()->rect(),_transform);
	
	const QRectF &mapped=t.map(extendedSceneRect).normalized();
	if(mapped.width()<=maximumViewportSize().width()&&mapped.height()<=maximumViewportSize().height()) {
		horizontalScrollBar()->setRange(0,0);
		verticalScrollBar()->setRange(0,0);
	}
	else {
/* To reduce scrollbar slider flicker when the scene is constantly changing,
 * we update scrollbar ranges when at least one of the following conditions
 * is true:
 * 	1. The range needs to be increased
 * 	2. The range needs to be significantly decreased
 */
		if(_scrollHelper.xScrollRange()>horizontalScrollBar()->maximum()||
				(horizontalScrollBar()->maximum()-_scrollHelper.xScrollRange()>
				horizontalScrollBar()->maximum()/100)) {
			horizontalScrollBar()->setRange(0,_scrollHelper.xScrollRange());
			horizontalScrollBar()->setValue(_scrollHelper.dxToScroll(t.dx()));
			horizontalScrollBar()->setPageStep(viewport()->rect().width());
			horizontalScrollBar()->setSingleStep(std::max(viewport()->rect().width()/5,1));
		}
		if(_scrollHelper.yScrollRange()>verticalScrollBar()->maximum()||
				(verticalScrollBar()->maximum()-_scrollHelper.yScrollRange()>
				verticalScrollBar()->maximum()/100)) {
			verticalScrollBar()->setRange(0,_scrollHelper.yScrollRange());
			verticalScrollBar()->setValue(_scrollHelper.dyToScroll(t.dy()));
			verticalScrollBar()->setPageStep(viewport()->rect().height());
			verticalScrollBar()->setSingleStep(std::max(viewport()->rect().height()/5,1));
		}
	}

	_transform=t; // restore transform
}

void PlotterScrollArea::restoreCursor() {
	if(_drag==DragToScroll) viewport()->setCursor(Qt::OpenHandCursor);
	else viewport()->setCursor(Qt::CrossCursor);
}

void PlotterScrollArea::updateStatus() {
// If mouse cursor is outside the viewport, return immediately
	if(!viewport()->rect().contains(viewport()->mapFromGlobal(QCursor::pos()))) return;
	QString str;
	QTextStream ts(&str);

	if(_rubberBand->isVisible()) {
		const QPointF orig=_transform.inverted().map(_mouseDragPos);
		const QPointF current=_transform.inverted().map(viewport()->mapFromGlobal(QCursor::pos()));
		ts<<tr("Selected: ")<<"x="<<qRound(orig.x());
		ts<<", y="<<qRound(orig.y());
		ts<<", "<<tr("width=")<<qRound(current.x()-orig.x());
		ts<<", "<<tr("height=")<<qRound(orig.y()-current.y());
	}
	else {
		const QPointF current=_transform.inverted().map(viewport()->mapFromGlobal(QCursor::pos()));
		ts<<"x="<<qRound(current.x())<<", y="<<qRound(current.y());
		const QVector<int> &layers=_scene->layers();
		QLocale locale;
		for(auto it=layers.cbegin();it!=layers.cend();it++) {
			if(!_scene->layerEnabled(*it)) continue;
			const qreal sample=_scene->sample(*it,qRound(current.x()),qRound(current.y()));
			if(!std::isnan(sample)) ts<<", V"<<(*it)<<"="<<locale.toString(sample);
		}
	}
	emit coordStatus(str);
}

void PlotterScrollArea::updateCursors() {
	if(!_scene) return;
	
	for(auto it=_cursors.cbegin();it!=_cursors.cend();) {
		if(it->isNull()) it=_cursors.erase(it);
		else {
			int x=qFloor(_transform.map(QPointF((*it)->pos(),0)).x());
			if(x>=0&&x<viewport()->width()) {
				(*it)->moveOrigin(x,(*it)->origin().y());
				(*it)->show();
			}
			else {
				(*it)->hide();
			}
			it++;
		}
	}
}

void PlotterScrollArea::paintCursors(QPainter &painter) {
	if(!_scene) return;
	painter.save();
	painter.resetTransform();
	QPen pen=_scene->gridPen();
	pen.setWidth(pen.width()>0?2*pen.width():2);
	painter.setPen(pen);
	
	for(auto it=_cursors.cbegin();it!=_cursors.cend();) {
		if(it->isNull()) it=_cursors.erase(it);
		else {
			int x=qFloor(_transform.map(QPointF((*it)->pos(),0)).x());
			if(x>=0&&x<viewport()->width())
				painter.drawLine(QPointF(x,0),QPointF(x,viewport()->height()));
			it++;
		}
	}
	
	painter.restore();
}

void PlotterScrollArea::calculateFps() {
	auto dt=_time.restart();
	if(dt==0) return;
	emit fpsCalculated(static_cast<double>(_frames)*1000/dt);
	_frames=0;
}
