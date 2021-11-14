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
 * This module provides an implementation of the StatusProgressWidget
 * class members.
 */

#include "statusprogresswidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

StatusProgressWidget::StatusProgressWidget(const QString &text,int max,QWidget *parent): QWidget(parent) {
	auto layout=new QHBoxLayout;
	layout->setContentsMargins(0,0,0,0);
	layout->addWidget(new QLabel(text));
	
	_progressBar=new QProgressBar;
	_progressBar->setMaximum(max);
	_progressBar->setValue(0);
	layout->addWidget(_progressBar);
	
	_detailsLabel=new QLabel("0/"+QString::number(max));
	layout->addWidget(_detailsLabel);
	
	auto cancelButton=new QPushButton(tr("Abort"));
	QObject::connect(cancelButton,&QAbstractButton::clicked,[this]{_canceled=true;});
	layout->addWidget(cancelButton);
	
	setLayout(layout);
}

int StatusProgressWidget::value() const {
	return _progressBar->value();
}

void StatusProgressWidget::setValue(int i) {
	_progressBar->setValue(i);
	_detailsLabel->setText(QString::number(i)+"/"+QString::number(_progressBar->maximum()));
}

bool StatusProgressWidget::canceled() const {
	return _canceled;
}
