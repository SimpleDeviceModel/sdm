/*
 * Copyright (c) 2015-2021 by Microproject LLC
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
 * This header files defines a class for the test video frame.
 */

#ifndef VIDEOFRAME_H_INCLUDED
#define VIDEOFRAME_H_INCLUDED

class VideoFrame {
	int _width;
	int _height;
	double _radius=20;
	double _blur=20;
	double _margin=64;
	double _black=1024;
	double _white=15360;
	double _noise=256;
	double _velocity=2;
	double _gravity=0.01;
	double _offsetX=0;
	double _offsetY=0;
	
	double _x0;
	double _y0;
public:
	VideoFrame(int w=256,int h=256);
	void next();
	double sample(int x,int y) const;
	
	int width() const;
	void setWidth(int i);
	int height() const;
	void setHeight(int i);
	double radius() const;
	void setRadius(double d);
	double blur() const;
	void setBlur(double d);
	double margin() const;
	void setMargin(double d);
	double black() const;
	void setBlack(double d);
	double white() const;
	void setWhite(double d);
	double noise() const;
	void setNoise(double d);
	double velocity() const;
	void setVelocity(double d);
	double gravity() const;
	void setGravity(double d);
	double offsetX() const;
	void setOffsetX(double d);
	double offsetY() const;
	void setOffsetY(double d);
};

#endif
