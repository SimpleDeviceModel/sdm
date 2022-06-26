/*
 * Copyright (c) 2015-2022 Simple Device Model contributors
 * 
 * This file is part of the Simple Device Model (SDM) framework SDK.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * This module provides an implementation of the VideoFrame class
 * used to generate test video.
 */

#include "videoframe.h"

#include <cstdlib>
#include <cmath>

VideoFrame::VideoFrame(int w,int h) {
	_width=w;
	_height=h;
	_x0=w/2;
	_y0=h/2;
}

void VideoFrame::next() {
	double x0_offset=(static_cast<double>(std::rand())/RAND_MAX-0.5)*2*_velocity;
	double y0_offset=(static_cast<double>(std::rand())/RAND_MAX-0.5)*2*_velocity;
	
	x0_offset-=(_x0-_width/2)*_gravity;
	y0_offset-=(_y0-_height/2)*_gravity;
	
	_x0+=x0_offset;
	_y0+=y0_offset;
	
	if(_x0<_margin) _x0=_margin;
	else if(_x0>_width-_margin) _x0=_width-_margin;
	if(_y0<_margin) _y0=_margin;
	else if(_y0>_height-_margin) _y0=_height-_margin;
}

double VideoFrame::sample(int x,int y) const {
	double x0=_x0+_offsetX;
	double y0=_y0+_offsetY;
	double d=std::sqrt((x-x0)*(x-x0)+(y-y0)*(y-y0));
	double pixel=0;
	
	if(d<_radius-_blur/2) pixel=_white;
	else if(d<_radius+_blur/2) pixel=_white-(_white-_black)*(d-(_radius-_blur/2))/_blur;
	else pixel=_black;
	
	double n=(static_cast<double>(std::rand())/RAND_MAX-0.5)*2*_noise;
	pixel+=n;
	
	if(pixel<0) pixel=0;
	if(pixel>16383) pixel=16383;
	
	return std::floor(pixel);
}

int VideoFrame::width() const {
	return _width;
}

void VideoFrame::setWidth(int i) {
	_width=i;
}

int VideoFrame::height() const {
	return _height;
}

void VideoFrame::setHeight(int i) {
	_height=i;
}

double VideoFrame::radius() const {
	return _radius;
}

void VideoFrame::setRadius(double d) {
	_radius=d;
}

double VideoFrame::blur() const {
	return _blur;
}

void VideoFrame::setBlur(double d) {
	_blur=d;
}

double VideoFrame::margin() const {
	return _margin;
}

void VideoFrame::setMargin(double d) {
	_margin=d;
}

double VideoFrame::black() const {
	return _black;
}

void VideoFrame::setBlack(double d) {
	_black=d;
}

double VideoFrame::white() const {
	return _white;
}

void VideoFrame::setWhite(double d) {
	_white=d;
}

double VideoFrame::noise() const {
	return _noise;
}

void VideoFrame::setNoise(double d) {
	_noise=d;
}

double VideoFrame::velocity() const {
	return _velocity;
}

void VideoFrame::setVelocity(double d) {
	_velocity=d;
}

double VideoFrame::gravity() const {
	return _gravity;
}

void VideoFrame::setGravity(double d) {
	_gravity=d;
}

double VideoFrame::offsetX() const {
	return _offsetX;
}

void VideoFrame::setOffsetX(double d) {
	_offsetX=d;
}

double VideoFrame::offsetY() const {
	return _offsetY;
}

void VideoFrame::setOffsetY(double d) {
	_offsetY=d;
}
